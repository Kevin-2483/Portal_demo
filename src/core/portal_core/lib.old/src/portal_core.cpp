#include "../include/portal_core.h"
#include <algorithm>
#include <cmath>
#include <iostream>

namespace Portal
{

  // Portal class implementation
  Portal::Portal(PortalId id)
      : id_(id), linked_portal_id_(INVALID_PORTAL_ID), is_active_(true), is_recursive_(false), max_recursion_depth_(3)
  {
  }

  // PortalManager class implementation - 舊版構造函數（向後相容）
  PortalManager::PortalManager(const HostInterfaces &interfaces)
      : interfaces_(interfaces),              // 第1个成员
        detection_manager_(nullptr),          // 第2个成员
        physics_manipulator_(nullptr),        // 第3个成员（舊版不設置新版介面）
        render_query_(nullptr),               // 第4个成员（舊版不設置新版介面）
        render_manipulator_(nullptr),         // 第5个成員（舊版不設置新版介面）
        event_handler_(nullptr),              // 第6个成员
        ghost_sync_timer_(0.0f),              // 第12个成员
        seamless_teleport_enabled_(true),     // 第17个成员
        center_crossing_check_interval_(1.0f / 60.0f),  // 第18个成员
        next_portal_id_(1),                   // 第19个成员
        is_initialized_(false),               // 第20个成员
        teleport_transition_duration_(0.1f),  // 第21个成员
        portal_detection_distance_(0.5f),     // 第22个成员
        default_max_recursion_depth_(3),       // 第23个成员
        timestamp_provider_(nullptr)          // 第24个成員：時間戳提供者
  {
  }

  // 新版混合架構構造函數
  PortalManager::PortalManager(const PortalCore::PortalInterfaces &physics_interfaces)
      : interfaces_{},                        // 清空舊版介面
        physics_manipulator_(physics_interfaces.physics_manipulator),
        render_query_(physics_interfaces.render_query),
        render_manipulator_(physics_interfaces.render_manipulator),
        event_handler_(physics_interfaces.event_handler),
        center_of_mass_manager_(nullptr),     // 將在初始化時創建
        ghost_sync_timer_(0.0f),
        seamless_teleport_enabled_(true),
        center_crossing_check_interval_(1.0f / 60.0f),
        next_portal_id_(1),
        is_initialized_(false),
        teleport_transition_duration_(0.1f),
        portal_detection_distance_(0.5f),
        default_max_recursion_depth_(3),
        timestamp_provider_(nullptr)
  {
    // 創建檢測管理器
    if (physics_interfaces.physics_data && physics_interfaces.is_valid()) {
      detection_manager_ = std::make_unique<PortalCore::PortalDetectionManager>(
          physics_interfaces.physics_data,
          physics_interfaces.detection_override);
      
      // 創建質心管理器
      center_of_mass_manager_ = std::make_unique<CenterOfMassManager>(
          physics_interfaces.physics_data);
    }
  }

  // 完全自定義構造函數
  PortalManager::PortalManager(PortalCore::IPhysicsDataProvider* data_provider,
                              PortalCore::IPhysicsManipulator* physics_manipulator,
                              PortalCore::IRenderQuery* render_query,
                              PortalCore::IRenderManipulator* render_manipulator,
                              PortalCore::IPortalEventHandler* event_handler,
                              PortalCore::IPortalDetectionOverride* detection_override)
      : interfaces_{},                        // 清空舊版介面
        physics_manipulator_(physics_manipulator),
        render_query_(render_query),
        render_manipulator_(render_manipulator),
        event_handler_(event_handler),
        center_of_mass_manager_(nullptr),     // 將在初始化時創建
        ghost_sync_timer_(0.0f),
        seamless_teleport_enabled_(true),
        center_crossing_check_interval_(1.0f / 60.0f),
        next_portal_id_(1),
        is_initialized_(false),
        teleport_transition_duration_(0.1f),
        portal_detection_distance_(0.5f),
        default_max_recursion_depth_(3),
        timestamp_provider_(nullptr)
  {
    // 創建檢測管理器
    if (data_provider) {
      detection_manager_ = std::make_unique<PortalCore::PortalDetectionManager>(
          data_provider, detection_override);
      
      // 創建質心管理器
      center_of_mass_manager_ = std::make_unique<CenterOfMassManager>(data_provider);
    }
  }

  bool PortalManager::initialize()
  {
    if (is_initialized_)
    {
      return true;
    }

    // 檢查舊版介面或新版介面是否有效
    bool has_valid_interfaces = false;
    
    // 檢查舊版介面
    if (interfaces_.is_valid()) {
      has_valid_interfaces = true;
    }
    
    // 檢查新版架構
    if (detection_manager_ && detection_manager_->is_initialized() && 
        physics_manipulator_ && render_query_ && render_manipulator_) {
      has_valid_interfaces = true;
    }
    
    if (!has_valid_interfaces) {
      std::cerr << "Portal Manager: No valid interfaces provided" << std::endl;
      return false;
    }

    is_initialized_ = true;
    std::cout << "Portal Manager: Initialized successfully" << std::endl;
    return true;
  }

  void PortalManager::shutdown()
  {
    if (!is_initialized_)
    {
      return;
    }

    // 清理所有传送门
    portals_.clear();
    registered_entities_.clear();
    active_teleports_.clear();

    // 清理幽灵同步数据
    ghost_sync_configs_.clear();
    ghost_snapshots_.clear();

    is_initialized_ = false;
  }

  void PortalManager::update(float delta_time)
  {
    if (!is_initialized_)
    {
      return;
    }

    // 更新传送门递归状态
    update_portal_recursive_states();

    // 检查实体与传送门的交互
    check_entity_portal_intersections();

    // 新增：无缝传送 - 检测质心穿越
    if (seamless_teleport_enabled_)
    {
      std::cout << "PortalManager::update - 開始檢查無縫傳送，註冊實體數量: " << registered_entities_.size() << std::endl;
      for (const auto &entity_id : registered_entities_)
      {
        std::cout << "PortalManager::update - 檢查實體 ID: " << entity_id << std::endl;
        detect_and_handle_center_crossing(entity_id, delta_time);
      }
    }
    else
    {
      std::cout << "PortalManager::update - 無縫傳送未啟用" << std::endl;
    }

    // 更新正在进行的传送
    update_entity_teleportation(delta_time);

    // 更新質心管理器（處理自動更新）
    if (center_of_mass_manager_) {
      center_of_mass_manager_->update_auto_update_entities(delta_time);
    }

    // 同步所有幽灵实体状态
    sync_all_ghost_entities(delta_time, false);

    // 清理已完成的传送
    cleanup_completed_teleports();
  }

  PortalId PortalManager::create_portal(const PortalPlane &plane)
  {
    PortalId id = generate_portal_id();
    auto portal = std::make_unique<Portal>(id);
    portal->set_plane(plane);

    portals_[id] = std::move(portal);

    return id;
  }

  void PortalManager::destroy_portal(PortalId portal_id)
  {
    auto it = portals_.find(portal_id);
    if (it == portals_.end())
    {
      return;
    }

    // 取消链接
    unlink_portal(portal_id);

    // 取消所有与此传送门相关的传送
    for (auto &[entity_id, teleport_state] : active_teleports_)
    {
      if (teleport_state.source_portal == portal_id || teleport_state.target_portal == portal_id)
      {
        cancel_entity_teleport(entity_id);
      }
    }

    portals_.erase(it);
  }

  bool PortalManager::link_portals(PortalId portal1, PortalId portal2)
  {
    Portal *p1 = get_portal(portal1);
    Portal *p2 = get_portal(portal2);

    if (!p1 || !p2 || portal1 == portal2)
    {
      return false;
    }

    // 取消现有链接
    unlink_portal(portal1);
    unlink_portal(portal2);

    // 建立新链接
    p1->set_linked_portal(portal2);
    p2->set_linked_portal(portal1);

    // 通知事件处理器
    notify_event_handler_if_available([portal1, portal2](IPortalEventHandler *handler)
                                      { handler->on_portals_linked(portal1, portal2); });

    return true;
  }

  void PortalManager::unlink_portal(PortalId portal_id)
  {
    Portal *portal = get_portal(portal_id);
    if (!portal || !portal->is_linked())
    {
      return;
    }

    PortalId linked_portal_id = portal->get_linked_portal();
    Portal *linked_portal = get_portal(linked_portal_id);

    // 断开双向链接
    portal->set_linked_portal(INVALID_PORTAL_ID);
    if (linked_portal)
    {
      linked_portal->set_linked_portal(INVALID_PORTAL_ID);
    }

    // 通知事件处理器
    notify_event_handler_if_available([portal_id, linked_portal_id](IPortalEventHandler *handler)
                                      { handler->on_portals_unlinked(portal_id, linked_portal_id); });
  }

  Portal *PortalManager::get_portal(PortalId portal_id)
  {
    auto it = portals_.find(portal_id);
    return (it != portals_.end()) ? it->second.get() : nullptr;
  }

  const Portal *PortalManager::get_portal(PortalId portal_id) const
  {
    auto it = portals_.find(portal_id);
    return (it != portals_.end()) ? it->second.get() : nullptr;
  }

  void PortalManager::update_portal_plane(PortalId portal_id, const PortalPlane &plane)
  {
    Portal *portal = get_portal(portal_id);
    if (portal)
    {
      portal->set_plane(plane);
    }
  }

  void PortalManager::register_entity(EntityId entity_id)
  {
    if (interfaces_.physics_query->is_entity_valid(entity_id))
    {
      registered_entities_.insert(entity_id);
    }
  }

  void PortalManager::unregister_entity(EntityId entity_id)
  {
    registered_entities_.erase(entity_id);

    // 取消正在进行的传送
    auto teleport_it = active_teleports_.find(entity_id);
    if (teleport_it != active_teleports_.end())
    {
      cancel_entity_teleport(entity_id);
    }
  }

  TeleportResult PortalManager::teleport_entity(EntityId entity_id, PortalId source_portal, PortalId target_portal)
  {
    if (!can_entity_teleport(entity_id, source_portal))
    {
      return TeleportResult::FAILED_INVALID_PORTAL;
    }

    const Portal *source = get_portal(source_portal);
    const Portal *target = get_portal(target_portal);

    if (!source || !target)
    {
      return TeleportResult::FAILED_INVALID_PORTAL;
    }

    // 获取实体当前状态
    Transform entity_transform = interfaces_.physics_query->get_entity_transform(entity_id);
    PhysicsState entity_physics = interfaces_.physics_query->get_entity_physics_state(entity_id);

    // 计算传送后的状态
    Transform new_transform = Math::PortalMath::transform_through_portal(
        entity_transform, source->get_plane(), target->get_plane());

    PhysicsState new_physics = Math::PortalMath::transform_physics_state_through_portal(
        entity_physics, source->get_plane(), target->get_plane());

    // 检查目标位置是否被阻挡
    if (interfaces_.physics_query->raycast(target->get_plane().center, new_transform.position, entity_id))
    {
      return TeleportResult::FAILED_BLOCKED;
    }

    // 开始传送过程
    TeleportState teleport_state;
    teleport_state.entity_id = entity_id;
    teleport_state.source_portal = source_portal;
    teleport_state.target_portal = target_portal;
    teleport_state.transition_progress = 0.0f;
    teleport_state.is_teleporting = true;

    active_teleports_[entity_id] = teleport_state;

    // 通知开始传送
    notify_event_handler_if_available([entity_id, source_portal, target_portal](IPortalEventHandler *handler)
                                      { handler->on_entity_teleport_start(entity_id, source_portal, target_portal); });

    // 立即应用新的变换和物理状态
    interfaces_.physics_manipulator->set_entity_transform(entity_id, new_transform);
    interfaces_.physics_manipulator->set_entity_physics_state(entity_id, new_physics);

    return TeleportResult::SUCCESS;
  }

  TeleportResult PortalManager::teleport_entity_with_velocity(EntityId entity_id, PortalId source_portal, PortalId target_portal)
  {
    if (!can_entity_teleport(entity_id, source_portal))
    {
      return TeleportResult::FAILED_INVALID_PORTAL;
    }

    const Portal *source = get_portal(source_portal);
    const Portal *target = get_portal(target_portal);

    if (!source || !target)
    {
      return TeleportResult::FAILED_INVALID_PORTAL;
    }

    // 获取实体当前状态
    Transform entity_transform = interfaces_.physics_query->get_entity_transform(entity_id);
    PhysicsState entity_physics = interfaces_.physics_query->get_entity_physics_state(entity_id);

    // 计算传送后的状态，考虑传送门速度
    Transform new_transform = Math::PortalMath::transform_through_portal(
        entity_transform, source->get_plane(), target->get_plane());

    PhysicsState new_physics = Math::PortalMath::transform_physics_state_with_portal_velocity(
        entity_physics,
        source->get_physics_state(),
        target->get_physics_state(),
        source->get_plane(),
        target->get_plane());

    // 检查目标位置是否被阻挡
    if (interfaces_.physics_query->raycast(target->get_plane().center, new_transform.position, entity_id))
    {
      return TeleportResult::FAILED_BLOCKED;
    }

    // 开始传送过程
    TeleportState teleport_state;
    teleport_state.entity_id = entity_id;
    teleport_state.source_portal = source_portal;
    teleport_state.target_portal = target_portal;
    teleport_state.transition_progress = 0.0f;
    teleport_state.is_teleporting = true;

    active_teleports_[entity_id] = teleport_state;

    // 通知开始传送
    notify_event_handler_if_available([entity_id, source_portal, target_portal](IPortalEventHandler *handler)
                                      { handler->on_entity_teleport_start(entity_id, source_portal, target_portal); });

    // 立即应用新的变换和物理状态
    interfaces_.physics_manipulator->set_entity_transform(entity_id, new_transform);
    interfaces_.physics_manipulator->set_entity_physics_state(entity_id, new_physics);

    return TeleportResult::SUCCESS;
  }

  void PortalManager::update_portal_physics_state(PortalId portal_id, const PhysicsState &physics_state)
  {
    Portal *portal = get_portal(portal_id);
    if (portal)
    {
      portal->set_physics_state(physics_state);
    }
  }

  const TeleportState *PortalManager::get_entity_teleport_state(EntityId entity_id) const
  {
    auto it = active_teleports_.find(entity_id);
    return (it != active_teleports_.end()) ? &it->second : nullptr;
  }

  std::vector<CameraParams> PortalManager::get_portal_render_cameras(
      PortalId portal_id,
      const CameraParams &base_camera,
      int max_depth) const
  {
    std::vector<CameraParams> cameras;
    const Portal *portal = get_portal(portal_id);

    if (!portal || !portal->is_linked())
    {
      return cameras;
    }

    const Portal *linked_portal = get_portal(portal->get_linked_portal());
    if (!linked_portal)
    {
      return cameras;
    }

    // 递归生成相机
    CameraParams current_camera = base_camera;

    for (int depth = 0; depth < max_depth; ++depth)
    {
      // 计算通过传送门的虚拟相机
      CameraParams portal_camera = Math::PortalMath::calculate_portal_camera(
          current_camera, portal->get_plane(), linked_portal->get_plane());

      cameras.push_back(portal_camera);

      // 检查是否会导致递归
      if (Math::PortalMath::is_portal_recursive(
              portal->get_plane(), linked_portal->get_plane(), portal_camera))
      {
        break;
      }

      current_camera = portal_camera;
    }

    return cameras;
  }

  bool PortalManager::is_portal_visible(PortalId portal_id, const CameraParams &camera) const
  {
    const Portal *portal = get_portal(portal_id);
    if (!portal)
    {
      return false;
    }

    return interfaces_.render_query->is_point_in_view_frustum(
        portal->get_plane().center, camera);
  }

  size_t PortalManager::get_teleporting_entity_count() const
  {
    size_t count = 0;
    for (const auto &[entity_id, teleport_state] : active_teleports_)
    {
      if (teleport_state.is_teleporting)
      {
        ++count;
      }
    }
    return count;
  }

  // Private methods implementation

  void PortalManager::update_entity_teleportation(float delta_time)
  {
    for (auto &[entity_id, teleport_state] : active_teleports_)
    {
      if (!teleport_state.is_teleporting)
      {
        continue;
      }

      teleport_state.transition_progress += delta_time / teleport_transition_duration_;

      if (teleport_state.transition_progress >= 1.0f)
      {
        complete_entity_teleport(entity_id);
      }
    }
  }

  void PortalManager::check_entity_portal_intersections()
  {
    for (EntityId entity_id : registered_entities_)
    {
      if (!interfaces_.physics_query->is_entity_valid(entity_id))
      {
        continue;
      }

      Transform entity_transform = interfaces_.physics_query->get_entity_transform(entity_id);
      Vector3 bounds_min, bounds_max;
      interfaces_.physics_query->get_entity_bounds(entity_id, bounds_min, bounds_max);

      // 检查与所有传送门的交互
      for (const auto &[portal_id, portal] : portals_)
      {
        if (!portal->is_active() || !portal->is_linked())
        {
          continue;
        }

        // === 第一步：粗筛 (引擎) ===
        bool is_intersecting = Math::PortalMath::does_entity_intersect_portal(
            bounds_min, bounds_max, entity_transform, portal->get_plane());

        if (!is_intersecting)
        {
          // 实体不与传送门相交，清理可能存在的状态
          cleanup_entity_portal_state(entity_id, portal_id);
          continue;
        }

        // === 第二步：精确状态判断 (库) ===

        // 分析包围盒分布
        BoundingBoxAnalysis bbox_analysis = Math::PortalMath::analyze_entity_bounding_box(
            bounds_min, bounds_max, entity_transform, portal->get_plane());

        // 获取当前状态
        TeleportState *teleport_state = get_or_create_teleport_state(entity_id, portal_id);
        PortalCrossingState previous_state = teleport_state->crossing_state;

        // 确定新的穿越状态
        PortalCrossingState new_state = Math::PortalMath::determine_crossing_state(
            bbox_analysis, previous_state);

        // 更新状态
        teleport_state->previous_state = previous_state;
        teleport_state->crossing_state = new_state;
        teleport_state->bbox_analysis = bbox_analysis;

        // === 第三步：根据状态变化执行动作 ===
        handle_crossing_state_change(entity_id, portal_id, previous_state, new_state);
      }
    }
  }

  void PortalManager::update_portal_recursive_states()
  {
    if (!interfaces_.render_query)
    {
      return;
    }

    CameraParams main_camera = interfaces_.render_query->get_main_camera();

    for (auto &portal_pair : portals_)
    {
      PortalId portal_id = portal_pair.first;
      auto &portal = portal_pair.second;
      if (!portal->is_linked())
      {
        portal->set_recursive(false);
        continue;
      }

      const Portal *linked_portal = get_portal(portal->get_linked_portal());
      if (!linked_portal)
      {
        continue;
      }

      bool was_recursive = portal->is_recursive();
      bool is_recursive = Math::PortalMath::is_portal_recursive(
          portal->get_plane(), linked_portal->get_plane(), main_camera);

      portal->set_recursive(is_recursive);

      // 通知状态变化
      if (was_recursive != is_recursive)
      {
        notify_event_handler_if_available([portal_id, is_recursive](IPortalEventHandler *handler)
                                          { handler->on_portal_recursive_state(portal_id, is_recursive); });
      }
    }
  }

  void PortalManager::cleanup_completed_teleports()
  {
    auto it = active_teleports_.begin();
    while (it != active_teleports_.end())
    {
      if (!it->second.is_teleporting)
      {
        it = active_teleports_.erase(it);
      }
      else
      {
        ++it;
      }
    }
  }

  bool PortalManager::can_entity_teleport(EntityId entity_id, PortalId portal_id) const
  {
    if (!interfaces_.physics_query->is_entity_valid(entity_id))
    {
      return false;
    }

    const Portal *portal = get_portal(portal_id);
    if (!portal || !portal->is_active() || !portal->is_linked())
    {
      return false;
    }

    // 检查实体是否已在传送中
    return active_teleports_.find(entity_id) == active_teleports_.end();
  }

  void PortalManager::start_entity_teleport(EntityId entity_id, PortalId source_portal)
  {
    const Portal *source = get_portal(source_portal);
    if (!source || !source->is_linked())
    {
      return;
    }

    PortalId target_portal = source->get_linked_portal();
    teleport_entity(entity_id, source_portal, target_portal);
  }

  void PortalManager::complete_entity_teleport(EntityId entity_id)
  {
    auto it = active_teleports_.find(entity_id);
    if (it == active_teleports_.end())
    {
      return;
    }

    TeleportState &teleport_state = it->second;
    teleport_state.is_teleporting = false;
    teleport_state.transition_progress = 1.0f;

    // 通知传送完成
    notify_event_handler_if_available([entity_id, &teleport_state](IPortalEventHandler *handler)
                                      { handler->on_entity_teleport_complete(entity_id, teleport_state.source_portal, teleport_state.target_portal); });
  }

  void PortalManager::cancel_entity_teleport(EntityId entity_id)
  {
    auto it = active_teleports_.find(entity_id);
    if (it != active_teleports_.end())
    {
      it->second.is_teleporting = false;
    }
  }

  PortalId PortalManager::generate_portal_id()
  {
    return next_portal_id_++;
  }

  bool PortalManager::is_valid_portal_id(PortalId portal_id) const
  {
    return portal_id != INVALID_PORTAL_ID && portals_.find(portal_id) != portals_.end();
  }

  void PortalManager::notify_event_handler_if_available(
      const std::function<void(IPortalEventHandler *)> &callback) const
  {
    if (interfaces_.event_handler)
    {
      callback(interfaces_.event_handler);
    }
  }

  // 新增：計算渲染通道描述符
  std::vector<RenderPassDescriptor> PortalManager::calculate_render_passes(
      const CameraParams &main_camera,
      int max_recursion_depth) const
  {
    std::vector<RenderPassDescriptor> render_passes;

    // 获取所有可见的传送门
    std::vector<PortalId> visible_portals;
    for (const auto &[portal_id, portal] : portals_)
    {
      if (portal->is_linked() && is_portal_visible(portal_id, main_camera))
      {
        visible_portals.push_back(portal_id);
      }
    }

    // 为每个可见传送门递归生成渲染通道
    for (PortalId portal_id : visible_portals)
    {
      calculate_recursive_render_passes(
          portal_id, main_camera, 0, max_recursion_depth, render_passes);
    }

    return render_passes;
  }

  // 新增：獲取實體裁切平面
  bool PortalManager::get_entity_clipping_plane(EntityId entity_id, ClippingPlane &clipping_plane) const
  {
    auto it = active_teleports_.find(entity_id);
    if (it == active_teleports_.end() || !it->second.is_teleporting)
    {
      return false;
    }

    const TeleportState &teleport_state = it->second;
    const Portal *source_portal = get_portal(teleport_state.source_portal);

    if (!source_portal)
    {
      return false;
    }

    // 计算传送门平面作为裁剪平面
    const PortalPlane &portal_plane = source_portal->get_plane();
    clipping_plane = ClippingPlane::from_point_and_normal(
        portal_plane.center,
        portal_plane.normal);

    return true;
  }

  // 輔助方法：遞歸計算渲染通道
  void PortalManager::calculate_recursive_render_passes(
      PortalId portal_id,
      const CameraParams &current_camera,
      int current_depth,
      int max_depth,
      std::vector<RenderPassDescriptor> &render_passes) const
  {
    if (current_depth >= max_depth)
    {
      return;
    }

    const Portal *portal = get_portal(portal_id);
    if (!portal || !portal->is_linked())
    {
      return;
    }

    const Portal *linked_portal = get_portal(portal->get_linked_portal());
    if (!linked_portal)
    {
      return;
    }

    // 创建渲染通道描述符
    RenderPassDescriptor descriptor;
    descriptor.source_portal_id = portal_id;
    descriptor.recursion_depth = current_depth;

    // 计算虚拟相机
    descriptor.virtual_camera = Math::PortalMath::calculate_portal_camera(
        current_camera, portal->get_plane(), linked_portal->get_plane());

    // 设置裁剪平面（防止看到传送门背后）
    descriptor.should_clip = true;
    descriptor.clipping_plane = ClippingPlane::from_point_and_normal(
        linked_portal->get_plane().center,
        linked_portal->get_plane().normal);

    // 配置模板缓冲
    descriptor.use_stencil_buffer = true;
    descriptor.stencil_ref_value = current_depth + 1;

    render_passes.push_back(descriptor);

    // 检查递归传送门
    if (Math::PortalMath::is_portal_recursive(
            portal->get_plane(), linked_portal->get_plane(), descriptor.virtual_camera))
    {
      // 检查递归传送门
      for (const auto &[next_portal_id, next_portal] : portals_)
      {
        if (next_portal_id != portal_id && next_portal->is_linked() &&
            is_portal_visible(next_portal_id, descriptor.virtual_camera))
        {
          calculate_recursive_render_passes(
              next_portal_id, descriptor.virtual_camera,
              current_depth + 1, max_depth, render_passes);
        }
      }
    }
  }

  // === 新增：三状态机辅助方法实现 ===

  TeleportState *PortalManager::get_or_create_teleport_state(EntityId entity_id, PortalId portal_id)
  {
    auto it = active_teleports_.find(entity_id);
    if (it != active_teleports_.end())
    {
      return &it->second;
    }

    // 创建新的传送状态
    TeleportState new_state;
    new_state.entity_id = entity_id;
    new_state.source_portal = portal_id;
    new_state.target_portal = get_portal(portal_id)->get_linked_portal();
    new_state.crossing_state = PortalCrossingState::NOT_TOUCHING;
    new_state.previous_state = PortalCrossingState::NOT_TOUCHING;

    active_teleports_[entity_id] = new_state;
    return &active_teleports_[entity_id];
  }

  void PortalManager::cleanup_entity_portal_state(EntityId entity_id, PortalId portal_id)
  {
    auto it = active_teleports_.find(entity_id);
    if (it != active_teleports_.end() && it->second.source_portal == portal_id)
    {
      // 清理幽灵碰撞体
      destroy_ghost_collider_if_exists(entity_id);

      // 重置状态
      it->second.crossing_state = PortalCrossingState::NOT_TOUCHING;
      it->second.has_ghost_collider = false;
    }
  }

  void PortalManager::handle_crossing_state_change(
      EntityId entity_id,
      PortalId portal_id,
      PortalCrossingState previous_state,
      PortalCrossingState new_state)
  {
    if (previous_state == new_state)
    {
      return; // 没有状态变化
    }

    TeleportState *state = get_or_create_teleport_state(entity_id, portal_id);

    switch (new_state)
    {
      case PortalCrossingState::CROSSING:
        if (previous_state == PortalCrossingState::NOT_TOUCHING)
        {
          // 开始穿越：启用模型裁切，创建幽灵实体
          std::cout << "Entity " << entity_id << " started crossing portal " << portal_id << std::endl;
          create_ghost_entity_with_faces(entity_id, portal_id, state->source_face, state->target_face);
          state->is_teleporting = true;
        }
        // 更新幽灵实体位置
        update_ghost_entity_with_faces(entity_id, portal_id, state->source_face, state->target_face);
        break;
        
    case PortalCrossingState::TELEPORTED:
      if (previous_state == PortalCrossingState::CROSSING)
      {
        // 完成传送：应用最终变换，销毁幽灵实体
        std::cout << "Entity " << entity_id << " completed teleportation through portal " << portal_id << std::endl;
        complete_entity_teleport(entity_id);
      }
      break;

    case PortalCrossingState::NOT_TOUCHING:
      if (previous_state == PortalCrossingState::CROSSING)
      {
        // 取消传送：实体退回到原侧
        std::cout << "Entity " << entity_id << " cancelled teleportation" << std::endl;
        cancel_entity_teleport(entity_id);
      }
      break;
    }
  }

  void PortalManager::create_ghost_collider_if_needed(EntityId entity_id, PortalId portal_id)
  {
    TeleportState *state = get_or_create_teleport_state(entity_id, portal_id);
    if (state->has_ghost_collider)
    {
      return; // 已经存在
    }

    // 计算幽灵碰撞体的变换
    Transform entity_transform = interfaces_.physics_query->get_entity_transform(entity_id);
    const Portal *source_portal = get_portal(portal_id);
    const Portal *target_portal = get_portal(source_portal->get_linked_portal());

    Transform ghost_transform = Math::PortalMath::calculate_ghost_transform(
        entity_transform,
        source_portal->get_plane(),
        target_portal->get_plane(),
        state->bbox_analysis.crossing_ratio);

    // 在引擎中创建幽灵碰撞体
    bool success = interfaces_.physics_manipulator->create_ghost_collider(entity_id, ghost_transform);
    if (success)
    {
      state->has_ghost_collider = true;
      std::cout << "Created ghost collider for entity " << entity_id << std::endl;
    }
  }

  void PortalManager::update_ghost_collider_position(EntityId entity_id, PortalId portal_id)
  {
    TeleportState *state = get_or_create_teleport_state(entity_id, portal_id);
    if (!state->has_ghost_collider)
    {
      return;
    }

    // 重新计算幽灵变换
    Transform entity_transform = interfaces_.physics_query->get_entity_transform(entity_id);
    PhysicsState entity_physics = interfaces_.physics_query->get_entity_physics_state(entity_id);

    const Portal *source_portal = get_portal(portal_id);
    const Portal *target_portal = get_portal(source_portal->get_linked_portal());

    Transform ghost_transform = Math::PortalMath::calculate_ghost_transform(
        entity_transform,
        source_portal->get_plane(),
        target_portal->get_plane(),
        state->bbox_analysis.crossing_ratio);

    PhysicsState ghost_physics = Math::PortalMath::transform_physics_state_through_portal(
        entity_physics, source_portal->get_plane(), target_portal->get_plane());

    // 更新幽灵碰撞体
    interfaces_.physics_manipulator->update_ghost_collider(entity_id, ghost_transform, ghost_physics);
  }

  void PortalManager::destroy_ghost_collider_if_exists(EntityId entity_id)
  {
    auto it = active_teleports_.find(entity_id);
    if (it != active_teleports_.end() && it->second.has_ghost_collider)
    {
      interfaces_.physics_manipulator->destroy_ghost_collider(entity_id);
      it->second.has_ghost_collider = false;
      std::cout << "Destroyed ghost collider for entity " << entity_id << std::endl;
    }
  }

  // === 新增：質心配置管理實現 ===

  void PortalManager::set_entity_center_of_mass_config(EntityId entity_id, const CenterOfMassConfig& config) {
    if (center_of_mass_manager_) {
      center_of_mass_manager_->set_entity_center_of_mass_config(entity_id, config);
      std::cout << "Set center of mass config for entity " << entity_id 
                << " to type " << static_cast<int>(config.type) << std::endl;
    } else {
      std::cerr << "Warning: Center of mass manager not available for entity " << entity_id << std::endl;
    }
  }

  const CenterOfMassConfig* PortalManager::get_entity_center_of_mass_config(EntityId entity_id) const {
    if (center_of_mass_manager_) {
      return center_of_mass_manager_->get_entity_center_of_mass_config(entity_id);
    }
    return nullptr;
  }

  // === 新增：幽灵状态同步管理实现 ===

  void PortalManager::set_ghost_sync_config(EntityId entity_id, const GhostSyncConfig &config)
  {
    ghost_sync_configs_[entity_id] = config;
  }

  const GhostSyncConfig *PortalManager::get_ghost_sync_config(EntityId entity_id) const
  {
    auto it = ghost_sync_configs_.find(entity_id);
    return (it != ghost_sync_configs_.end()) ? &it->second : nullptr;
  }

  bool PortalManager::force_sync_ghost_state(EntityId entity_id, PortalFace source_face, PortalFace target_face)
  {
    auto teleport_it = active_teleports_.find(entity_id);
    if (teleport_it == active_teleports_.end() || !teleport_it->second.is_teleporting)
    {
      return false; // 实体不在传送状态
    }

    TeleportState &state = teleport_it->second;
    
    // 更新面信息
    state.source_face = source_face;
    state.target_face = target_face;

    // 获取传送门
    const Portal *source_portal = get_portal(state.source_portal);
    const Portal *target_portal = get_portal(state.target_portal);
    
    if (!source_portal || !target_portal)
    {
      return false;
    }

    // 获取主体当前状态
    if (!interfaces_.physics_query->is_entity_valid(entity_id))
    {
      return false;
    }

    Transform main_transform = interfaces_.physics_query->get_entity_transform(entity_id);
    PhysicsState main_physics = interfaces_.physics_query->get_entity_physics_state(entity_id);
    Vector3 main_bounds_min, main_bounds_max;
    interfaces_.physics_query->get_entity_bounds(entity_id, main_bounds_min, main_bounds_max);

    // 计算幽灵状态
    Transform ghost_transform;
    PhysicsState ghost_physics;
    Vector3 ghost_bounds_min, ghost_bounds_max;

    bool success = calculate_ghost_state(
        main_transform, main_physics, main_bounds_min, main_bounds_max,
        state.source_portal, source_face, target_face,
        ghost_transform, ghost_physics, ghost_bounds_min, ghost_bounds_max);

    if (!success)
    {
      return false;
    }

    // 创建或更新幽灵实体
    auto ghost_it = ghost_snapshots_.find(entity_id);
    if (ghost_it == ghost_snapshots_.end())
    {
      // 创建新的幽灵实体
      if (!create_ghost_entity_with_faces(entity_id, state.source_portal, source_face, target_face))
      {
        return false;
      }
      ghost_it = ghost_snapshots_.find(entity_id);
    }

    if (ghost_it != ghost_snapshots_.end())
    {
      // 更新快照
      GhostEntitySnapshot &snapshot = ghost_it->second;
      snapshot.main_transform = main_transform;
      snapshot.main_physics = main_physics;
      snapshot.main_bounds_min = main_bounds_min;
      snapshot.main_bounds_max = main_bounds_max;
      snapshot.ghost_transform = ghost_transform;
      snapshot.ghost_physics = ghost_physics;
      snapshot.ghost_bounds_min = ghost_bounds_min;
      snapshot.ghost_bounds_max = ghost_bounds_max;
      snapshot.timestamp = get_current_timestamp();

      // 同步到引擎
      std::vector<GhostEntitySnapshot> snapshots = {snapshot};
      interfaces_.physics_manipulator->sync_ghost_entities(snapshots);

      return true;
    }

    return false;
  }

  void PortalManager::sync_all_ghost_entities(float delta_time, bool force_sync)
  {
    ghost_sync_timer_ += delta_time;

    std::vector<GhostEntitySnapshot> snapshots_to_sync;

    for (auto &[entity_id, teleport_state] : active_teleports_)
    {
      if (!teleport_state.is_teleporting || !teleport_state.enable_realtime_sync)
      {
        continue;
      }

      // 检查是否需要同步
      if (!force_sync && !should_sync_ghost_state(entity_id, delta_time))
      {
        continue;
      }

      // 强制同步幽灵状态
      if (force_sync_ghost_state(entity_id, teleport_state.source_face, teleport_state.target_face))
      {
        auto snapshot_it = ghost_snapshots_.find(entity_id);
        if (snapshot_it != ghost_snapshots_.end())
        {
          snapshots_to_sync.push_back(snapshot_it->second);
        }
      }
    }

    // 批量同步
    if (!snapshots_to_sync.empty())
    {
      interfaces_.physics_manipulator->sync_ghost_entities(snapshots_to_sync);
    }
  }

  const GhostEntitySnapshot *PortalManager::get_ghost_snapshot(EntityId entity_id) const
  {
    auto it = ghost_snapshots_.find(entity_id);
    return (it != ghost_snapshots_.end()) ? &it->second : nullptr;
  }

  bool PortalManager::calculate_ghost_state(
      const Transform &main_transform,
      const PhysicsState &main_physics,
      const Vector3 &main_bounds_min,
      const Vector3 &main_bounds_max,
      PortalId portal_id,
      PortalFace source_face,
      PortalFace target_face,
      Transform &ghost_transform,
      PhysicsState &ghost_physics,
      Vector3 &ghost_bounds_min,
      Vector3 &ghost_bounds_max) const
  {
    const Portal *source_portal = get_portal(portal_id);
    if (!source_portal || !source_portal->is_linked())
    {
      return false;
    }

    const Portal *target_portal = get_portal(source_portal->get_linked_portal());
    if (!target_portal)
    {
      return false;
    }

    // 变换位置和旋转
    ghost_transform = Math::PortalMath::transform_through_portal(
        main_transform, source_portal->get_plane(), target_portal->get_plane(), 
        source_face, target_face);

    // 变换物理状态
    ghost_physics = Math::PortalMath::transform_physics_state_through_portal(
        main_physics, source_portal->get_plane(), target_portal->get_plane(),
        source_face, target_face);

    // 变换包围盒
    Transform temp_transform;
    Math::PortalMath::transform_bounds_through_portal(
        main_bounds_min, main_bounds_max, main_transform,
        source_portal->get_plane(), target_portal->get_plane(),
        source_face, target_face,
        ghost_bounds_min, ghost_bounds_max, temp_transform);

    return true;
  }

  // === 新增：幽灵状态同步辅助方法实现 ===

  bool PortalManager::create_ghost_entity_with_faces(EntityId entity_id, PortalId portal_id,
                                                     PortalFace source_face, PortalFace target_face)
  {
    // 获取主体状态
    if (!interfaces_.physics_query->is_entity_valid(entity_id))
    {
      return false;
    }

    Transform main_transform = interfaces_.physics_query->get_entity_transform(entity_id);
    PhysicsState main_physics = interfaces_.physics_query->get_entity_physics_state(entity_id);
    Vector3 main_bounds_min, main_bounds_max;
    interfaces_.physics_query->get_entity_bounds(entity_id, main_bounds_min, main_bounds_max);

    // 计算幽灵状态
    Transform ghost_transform;
    PhysicsState ghost_physics;
    Vector3 ghost_bounds_min, ghost_bounds_max;

    if (!calculate_ghost_state(main_transform, main_physics, main_bounds_min, main_bounds_max,
                              portal_id, source_face, target_face,
                              ghost_transform, ghost_physics, ghost_bounds_min, ghost_bounds_max))
    {
      return false;
    }

    // 在引擎中创建幽灵实体
    EntityId ghost_entity_id = interfaces_.physics_manipulator->create_ghost_entity(
        entity_id, ghost_transform, ghost_physics);

    if (ghost_entity_id == INVALID_ENTITY_ID)
    {
      return false;
    }

    // 设置幽灵实体包围盒
    interfaces_.physics_manipulator->set_ghost_entity_bounds(
        ghost_entity_id, ghost_bounds_min, ghost_bounds_max);

    // 创建快照
    GhostEntitySnapshot snapshot;
    snapshot.main_entity_id = entity_id;
    snapshot.ghost_entity_id = ghost_entity_id;
    snapshot.main_transform = main_transform;
    snapshot.main_physics = main_physics;
    snapshot.main_bounds_min = main_bounds_min;
    snapshot.main_bounds_max = main_bounds_max;
    snapshot.ghost_transform = ghost_transform;
    snapshot.ghost_physics = ghost_physics;
    snapshot.ghost_bounds_min = ghost_bounds_min;
    snapshot.ghost_bounds_max = ghost_bounds_max;
    snapshot.timestamp = get_current_timestamp();

    ghost_snapshots_[entity_id] = snapshot;

    // 建立映射關係
    ghost_to_main_mapping_[ghost_entity_id] = entity_id;
    main_to_ghost_mapping_[entity_id] = ghost_entity_id;

    // 重要：更新對應的傳送狀態中的幽靈實體ID
    auto teleport_it = active_teleports_.find(entity_id);
    if (teleport_it != active_teleports_.end()) {
        teleport_it->second.ghost_entity_id = ghost_entity_id;
        std::cout << "Updated teleport state: ghost_entity_id = " << ghost_entity_id << std::endl;
    }

    std::cout << "Created ghost entity " << ghost_entity_id 
              << " for main entity " << entity_id 
              << " with faces " << static_cast<int>(source_face) 
              << "->" << static_cast<int>(target_face) << std::endl;

    return true;
  }

  void PortalManager::update_ghost_entity_with_faces(EntityId entity_id, PortalId portal_id,
                                                     PortalFace source_face, PortalFace target_face)
  {
    auto snapshot_it = ghost_snapshots_.find(entity_id);
    if (snapshot_it == ghost_snapshots_.end())
    {
      // 如果幽灵实体不存在，创建它
      create_ghost_entity_with_faces(entity_id, portal_id, source_face, target_face);
      return;
    }

    // 强制同步状态
    force_sync_ghost_state(entity_id, source_face, target_face);
  }

  void PortalManager::destroy_ghost_entity_if_exists(EntityId entity_id)
  {
    auto snapshot_it = ghost_snapshots_.find(entity_id);
    if (snapshot_it != ghost_snapshots_.end())
    {
      EntityId ghost_entity_id = snapshot_it->second.ghost_entity_id;
      if (ghost_entity_id != INVALID_ENTITY_ID)
      {
        interfaces_.physics_manipulator->destroy_ghost_entity(ghost_entity_id);
        std::cout << "Destroyed ghost entity " << ghost_entity_id 
                  << " for main entity " << entity_id << std::endl;
      }
      ghost_snapshots_.erase(snapshot_it);
    }
  }

  bool PortalManager::should_sync_ghost_state(EntityId entity_id, float delta_time) const
  {
    const GhostSyncConfig *config = get_ghost_sync_config(entity_id);
    if (!config)
    {
      // 使用默认配置
      static const GhostSyncConfig default_config;
      config = &default_config;
    }

    // 检查频率限制
    float sync_interval = 1.0f / config->sync_frequency;
    auto snapshot_it = ghost_snapshots_.find(entity_id);
    if (snapshot_it != ghost_snapshots_.end())
    {
      uint64_t current_time = get_current_timestamp();
      uint64_t elapsed = current_time - snapshot_it->second.timestamp;
      if (elapsed < static_cast<uint64_t>(sync_interval * 1000.0f))
      {
        return false; // 还没到同步时间
      }
    }

    // 检查变化程度
    if (snapshot_it != ghost_snapshots_.end() && interfaces_.physics_query->is_entity_valid(entity_id))
    {
      Transform current_transform = interfaces_.physics_query->get_entity_transform(entity_id);
      PhysicsState current_physics = interfaces_.physics_query->get_entity_physics_state(entity_id);

      float transform_diff = Math::PortalMath::calculate_transform_distance(
          current_transform, snapshot_it->second.main_transform);
      float physics_diff = Math::PortalMath::calculate_physics_distance(
          current_physics, snapshot_it->second.main_physics);

      if (transform_diff < config->transform_threshold && physics_diff < config->velocity_threshold)
      {
        return false; // 变化太小，不需要同步
      }
    }

    return true;
  }

  float PortalManager::calculate_transform_difference(const Transform &t1, const Transform &t2) const
  {
    return Math::PortalMath::calculate_transform_distance(t1, t2);
  }

  float PortalManager::calculate_physics_difference(const PhysicsState &p1, const PhysicsState &p2) const
  {
    return Math::PortalMath::calculate_physics_distance(p1, p2);
  }

  uint64_t PortalManager::get_current_timestamp() const
  {
    // 使用可配置的時間戳提供者，如果沒有設置則使用預設的計數器
    if (timestamp_provider_) {
      return timestamp_provider_();
    } else {
      // 預設實現：使用簡單的計數器（用於測試）
      static uint64_t counter = 0;
      return ++counter;
    }
  }

  void PortalManager::set_timestamp_provider(std::function<uint64_t()> timestamp_provider)
  {
    timestamp_provider_ = std::move(timestamp_provider);
    std::cout << "PortalManager: 已設置自定義時間戳提供者" << std::endl;
  }

  void PortalManager::reset_timestamp_provider()
  {
    timestamp_provider_ = nullptr;
    std::cout << "PortalManager: 已重置為預設時間戳提供者（計數器模式）" << std::endl;
  }

  // === 无缝传送核心实现 ===

  bool PortalManager::detect_and_handle_center_crossing(EntityId entity_id, float delta_time)
  {
    // 檢查是否有可用的檢測系統
    bool has_detection = false;
    EntityDescription entity_desc;
    
    // 優先使用新版檢測管理器
    if (detection_manager_ && detection_manager_->is_initialized()) {
      has_detection = true;
      // 從新版系統獲取實體描述
      entity_desc.entity_id = entity_id;
      entity_desc.entity_type = EntityType::MAIN;
      
      // 從data_provider獲取完整的實體描述
      if (auto* data_provider = detection_manager_->get_data_provider()) {
        entity_desc.transform = data_provider->get_entity_transform(entity_id);
        
        // 獲取速度並設置到physics結構中
        Vector3 velocity = data_provider->get_entity_velocity(entity_id);
        entity_desc.physics.linear_velocity = velocity;
        entity_desc.physics.angular_velocity = Vector3(0, 0, 0);  // 預設無角速度
        
        // 獲取包圍盒
        auto bbox = data_provider->get_entity_bounding_box(entity_id);
        entity_desc.bounds_min = bbox.min;
        entity_desc.bounds_max = bbox.max;
        
        // 使用質心管理系統計算質心
        if (center_of_mass_manager_) {
          entity_desc.center_of_mass = center_of_mass_manager_->get_entity_center_of_mass_local(entity_id);
          std::cout << "從質心管理器獲取實體 " << entity_id << " 的本地質心: (" 
                    << entity_desc.center_of_mass.x << ", " << entity_desc.center_of_mass.y 
                    << ", " << entity_desc.center_of_mass.z << ")" << std::endl;
        } else {
          // 回退到幾何中心
          entity_desc.center_of_mass = (entity_desc.bounds_min + entity_desc.bounds_max) * 0.5f;
        }
        
        std::cout << "成功從data_provider獲取實體 " << entity_id << " 的完整描述" << std::endl;
      } else {
        std::cerr << "PortalManager: data_provider不可用，使用預設值" << std::endl;
        // 使用最基本的預設值
        entity_desc.transform = Transform();
        entity_desc.physics.linear_velocity = Vector3(0, 0, 0);
        entity_desc.physics.angular_velocity = Vector3(0, 0, 0);
        entity_desc.bounds_min = Vector3(-0.5f, -0.5f, -0.5f);
        entity_desc.bounds_max = Vector3(0.5f, 0.5f, 0.5f);
        entity_desc.center_of_mass = Vector3(0, 0, 0);
      }
    }
    // 否則使用舊版介面
    else if (interfaces_.physics_query) {
      has_detection = true;
      entity_desc = interfaces_.physics_query->get_entity_description(entity_id);
    }
    
    if (!has_detection) {
      std::cerr << "PortalManager: No physics query available for center crossing detection" << std::endl;
      return false;
    }

    // 計算當前質心世界位置
    Vector3 current_center;
    if (detection_manager_ && center_of_mass_manager_) {
      // 使用新的質心管理系統
      Transform entity_transform = entity_desc.transform;
      current_center = center_of_mass_manager_->get_entity_center_of_mass_world(
          entity_id, entity_transform);
      std::cout << "使用質心管理器獲取質心位置: (" 
                << current_center.x << ", " << current_center.y << ", " << current_center.z << ")" << std::endl;
    } else if (interfaces_.physics_query) {
      current_center = Math::PortalMath::calculate_center_of_mass_world_pos(
          entity_desc.transform, entity_desc.center_of_mass);
    } else {
      std::cerr << "PortalManager: No valid center of mass calculation method available" << std::endl;
      return false;
    }

    // 獲取或創建質心穿越狀態
    auto crossing_it = center_crossings_.find(entity_id);
    if (crossing_it == center_crossings_.end()) {
      center_crossings_[entity_id] = CenterOfMassCrossing();
      center_crossings_[entity_id].entity_id = entity_id;
      center_crossings_[entity_id].center_world_pos = current_center;
      return false;
    }

    CenterOfMassCrossing &crossing = crossing_it->second;
    Vector3 prev_center = crossing.center_world_pos;
    crossing.center_world_pos = current_center;

    // 檢查所有傳送門的穿越
    bool crossing_detected = false;
    for (const auto &portal_pair : portals_) {
      PortalId portal_id = portal_pair.first;
      const Portal &portal = *portal_pair.second;

      std::cout << "  檢查傳送門 ID: " << portal_id << "，實體 ID: " << entity_id << std::endl;

      // 使用新版檢測管理器或舊版介面
      bool center_crossed = false;
      
      if (detection_manager_) {
        center_crossed = detection_manager_->check_center_crossing(entity_id, portal);
      } else if (interfaces_.physics_query) {
        auto result_a = interfaces_.physics_query->check_center_crossing(entity_id, portal.get_plane(), PortalFace::A);
        center_crossed = result_a.just_started;
      }
      
      if (center_crossed) {
        std::cout << "    PortalManager: 質心穿越檢測到，準備開始傳送流程！" << std::endl;
        crossing_detected |= handle_center_crossing_event(entity_id, portal_id, PortalFace::A);
      }

      // 检查B面穿越
      auto result_b = interfaces_.physics_query->check_center_crossing(entity_id, portal.get_plane(), PortalFace::B);
      std::cout << "    B面檢查結果 - just_started: " << result_b.just_started 
                << ", crossing_progress: " << result_b.crossing_progress << std::endl;
      
      if (result_b.just_started)
      {
        std::cout << "    PortalManager: B面條件滿足，準備開始傳送流程！" << std::endl;
        crossing_detected |= handle_center_crossing_event(entity_id, portal_id, PortalFace::B);
      }

      // 检查穿越完成
      if (crossing.portal_id == portal_id)
      {
        float progress = interfaces_.physics_query->calculate_center_crossing_progress(
            entity_id, portal.get_plane());
        
        crossing.crossing_progress = progress;

        // 检查是否准备好交换实体角色
        if (progress >= 1.0f && !crossing.just_completed)
        {
          if (is_ready_for_entity_swap(entity_id))
          {
            std::cout << "  PortalManager: 實體 " << entity_id << " 穿越完成，準備執行角色互換！" << std::endl;
            execute_entity_role_swap(entity_id, crossing.entity_id);
            crossing.just_completed = true;
          }
        }
      }
    }

    return crossing_detected;
  }

  bool PortalManager::handle_center_crossing_event(EntityId entity_id, PortalId portal_id, PortalFace crossed_face)
  {
    std::cout << "    handle_center_crossing_event: 實體 " << entity_id << " 質心穿越傳送門 " << portal_id << std::endl;
    
    // 检查是否已经在传送中
    auto teleport_it = active_teleports_.find(entity_id);
    if (teleport_it != active_teleports_.end())
    {
      std::cout << "    實體已在傳送狀態中，檢查是否需要角色互換..." << std::endl;
      
      // 如果已經在傳送中，檢查是否需要執行角色互換
      TeleportState &teleport_state = teleport_it->second;
      
      std::cout << "    幽靈實體ID: " << teleport_state.ghost_entity_id << std::endl;
      std::cout << "    角色已互換: " << (teleport_state.role_swapped ? "是" : "否") << std::endl;
      
      if (teleport_state.ghost_entity_id != INVALID_ENTITY_ID && !teleport_state.role_swapped)
      {
        std::cout << "  PortalManager: 質心穿越檢測 - 執行幽靈與本體互換！實體 " << entity_id 
                  << " 與幽靈 " << teleport_state.ghost_entity_id << std::endl;
        
        if (execute_entity_role_swap(entity_id, teleport_state.ghost_entity_id))
        {
          teleport_state.role_swapped = true;
          return true;
        }
      }
      return false;
    }

    std::cout << "    實體不在傳送狀態中，創建無縫傳送..." << std::endl;
    // 创建无缝传送（如果還沒有傳送狀態）
    return create_seamless_teleport(entity_id, portal_id, crossed_face);
  }

  bool PortalManager::create_seamless_teleport(EntityId entity_id, PortalId portal_id, PortalFace crossed_face)
  {
    auto portal_it = portals_.find(portal_id);
    if (portal_it == portals_.end())
    {
      return false;
    }

    const Portal &portal = *portal_it->second;

    // 确定目标传送门
    PortalId target_portal_id = portal.get_linked_portal();
    if (target_portal_id == INVALID_PORTAL_ID)
    {
      return false; // 传送门没有链接
    }

    auto target_portal_it = portals_.find(target_portal_id);
    if (target_portal_it == portals_.end())
    {
      return false;
    }

    // 获取实体描述
    EntityDescription entity_desc;
    if (interfaces_.physics_query)
    {
      entity_desc = interfaces_.physics_query->get_entity_description(entity_id);
    }
    else
    {
      return false;
    }

    // 创建传送状态
    TeleportState teleport_state;
    teleport_state.entity_id = entity_id;
    teleport_state.source_portal = portal_id;
    teleport_state.target_portal = target_portal_id;
    teleport_state.source_face = crossed_face;
    teleport_state.target_face = crossed_face == PortalFace::A ? PortalFace::B : PortalFace::A;
    teleport_state.seamless_mode = true;
    teleport_state.auto_triggered = true;
    teleport_state.is_teleporting = true;
    teleport_state.crossing_state = PortalCrossingState::CROSSING;
    teleport_state.original_entity_type = entity_desc.entity_type;

    // 创建全功能幽灵实体
    Transform ghost_transform;
    PhysicsState ghost_physics;
    Vector3 ghost_bounds_min, ghost_bounds_max;

    if (calculate_ghost_state(
        entity_desc.transform, entity_desc.physics,
        entity_desc.bounds_min, entity_desc.bounds_max,
        portal_id, teleport_state.source_face, teleport_state.target_face,
        ghost_transform, ghost_physics, ghost_bounds_min, ghost_bounds_max))
    {
      // 创建全功能幽灵实体
      EntityId ghost_id = INVALID_ENTITY_ID;
      if (interfaces_.physics_manipulator)
      {
        ghost_id = interfaces_.physics_manipulator->create_full_functional_ghost(
            entity_desc, ghost_transform, ghost_physics);
      }

      if (ghost_id != INVALID_ENTITY_ID)
      {
        teleport_state.ghost_entity_id = ghost_id;
        
        // 建立映射关系
        ghost_to_main_mapping_[ghost_id] = entity_id;
        main_to_ghost_mapping_[entity_id] = ghost_id;

        // 更新质心穿越状态
        center_crossings_[entity_id].portal_id = portal_id;
        center_crossings_[entity_id].crossed_face = crossed_face;
        center_crossings_[entity_id].target_face = teleport_state.target_face;
        center_crossings_[entity_id].just_started = true;

        std::cout << "Created seamless teleport: Entity " << entity_id 
                  << " -> Ghost " << ghost_id 
                  << " through portal " << portal_id << std::endl;
        
        // 立即執行實體角色互換（質心穿越時的無縫傳送核心邏輯）
        std::cout << "Executing immediate entity role swap for seamless teleport..." << std::endl;
        if (execute_entity_role_swap(entity_id, ghost_id))
        {
          std::cout << "Successfully swapped entity roles: " << entity_id << " <-> " << ghost_id << std::endl;
          // 更新傳送狀態為已傳送
          teleport_state.crossing_state = PortalCrossingState::TELEPORTED;
        }
        else
        {
          std::cout << "Failed to swap entity roles!" << std::endl;
        }
      }
    }

    // 保存传送状态
    active_teleports_[entity_id] = teleport_state;

    // 通知事件处理器
    notify_event_handler_if_available([entity_id, portal_id, target_portal_id](IPortalEventHandler *handler)
                                      { handler->on_entity_teleport_start(entity_id, portal_id, target_portal_id); });

    return true;
  }

  bool PortalManager::promote_ghost_to_main(EntityId ghost_id, EntityId old_main_id)
  {
    if (!interfaces_.physics_manipulator)
    {
      return false;
    }

    return interfaces_.physics_manipulator->promote_ghost_to_main(ghost_id, old_main_id);
  }

  bool PortalManager::is_ready_for_entity_swap(EntityId entity_id) const
  {
    auto crossing_it = center_crossings_.find(entity_id);
    if (crossing_it == center_crossings_.end())
    {
      return false;
    }

    const CenterOfMassCrossing &crossing = crossing_it->second;
    
    // 检查穿越进度是否足够（质心已经完全通过传送门）
    return crossing.crossing_progress >= 0.9f; // 90% 穿越即可交换
  }

  bool PortalManager::execute_entity_role_swap(EntityId main_id, EntityId ghost_id)
  {
    std::cout << "  執行實體角色互換：主實體 " << main_id << " 與幽靈 " << ghost_id << std::endl;
    
    // 检查映射关系
    auto ghost_it = main_to_ghost_mapping_.find(main_id);
    if (ghost_it == main_to_ghost_mapping_.end())
    {
      std::cout << "  錯誤：找不到主實體 " << main_id << " 的幽靈映射關係" << std::endl;
      return false;
    }

    EntityId actual_ghost_id = ghost_it->second;

    if (!interfaces_.physics_manipulator)
    {
      std::cout << "  錯誤：physics_manipulator 接口為空" << std::endl;
      return false;
    }

    // 执行角色交换
    std::cout << "  呼叫 promote_ghost_to_main：幽靈 " << actual_ghost_id << " 替換主實體 " << main_id << std::endl;
    
    // 重要：獲取幽靈實體的位置信息並應用到主實體
    auto snapshot_it = ghost_snapshots_.find(main_id);
    if (snapshot_it != ghost_snapshots_.end()) {
      const GhostEntitySnapshot& ghost_snapshot = snapshot_it->second;
      std::cout << "  更新主實體位置：從 主實體位置 到 幽靈位置 ("
                << ghost_snapshot.ghost_transform.position.x << ", "
                << ghost_snapshot.ghost_transform.position.y << ", " 
                << ghost_snapshot.ghost_transform.position.z << ")" << std::endl;
                
      // 通過 physics_manipulator 更新實體位置
      if (interfaces_.physics_manipulator) {
        interfaces_.physics_manipulator->set_entity_transform(main_id, ghost_snapshot.ghost_transform);
        std::cout << "  ✅ 主實體位置已更新到幽靈位置！" << std::endl;
      }
    }
    
    bool success = interfaces_.physics_manipulator->promote_ghost_to_main(actual_ghost_id, main_id);

    if (success)
    {
      std::cout << "  ✅ 角色互換成功！" << std::endl;
      // 更新映射关系
      ghost_to_main_mapping_.erase(actual_ghost_id);
      main_to_ghost_mapping_.erase(main_id);

      // 更新传送状态
      auto teleport_it = active_teleports_.find(main_id);
      if (teleport_it != active_teleports_.end())
      {
        teleport_it->second.ready_for_swap = true;
        teleport_it->second.crossing_state = PortalCrossingState::TELEPORTED;
      }

      // 清理质心穿越状态
      center_crossings_.erase(main_id);

      std::cout << "Successfully swapped entity roles: " << main_id 
                << " (old main) <-> " << actual_ghost_id << " (new main)" << std::endl;
    }

    return success;
  }

} // namespace Portal
