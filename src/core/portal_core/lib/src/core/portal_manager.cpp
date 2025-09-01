#include "../../include/core/portal_manager.h"
#include "../../include/math/portal_math.h"
#include <iostream>
#include <algorithm>

namespace Portal
{

  PortalManager::PortalManager(const PortalInterfaces &interfaces)
      : interfaces_(interfaces), next_portal_id_(1), is_initialized_(false), default_max_recursion_depth_(3)
  {
    if (!interfaces_.is_valid())
    {
      throw std::invalid_argument("Invalid interfaces: physics_data and physics_manipulator are required");
    }

    // 創建子管理器
    teleport_manager_ = std::make_unique<TeleportManager>(
        interfaces_.physics_data,
        interfaces_.physics_manipulator,
        interfaces_.event_handler);

    // 創建質心管理器（暫時不傳遞 provider，讓外部通過 set_provider 設置）
    center_of_mass_manager_ = std::make_unique<CenterOfMassManager>(nullptr);

    std::cout << "PortalManager created with event-driven architecture" << std::endl;
  }

  PortalManager::~PortalManager()
  {
    shutdown();
  }

  bool PortalManager::initialize()
  {
    if (is_initialized_)
    {
      return true;
    }

    if (!interfaces_.is_valid())
    {
      std::cerr << "PortalManager: Cannot initialize - invalid interfaces" << std::endl;
      return false;
    }

    // 設置 TeleportManager 的 Portal 獲取回調
    if (teleport_manager_)
    {
      teleport_manager_->set_portal_getter([this](PortalId portal_id) -> Portal*
      {
        return this->get_portal(portal_id);
      });
      std::cout << "PortalManager: Set portal getter callback for TeleportManager" << std::endl;
    }

    is_initialized_ = true;
    std::cout << "PortalManager: Initialized successfully with event-driven architecture" << std::endl;
    return true;
  }

  void PortalManager::shutdown()
  {
    if (!is_initialized_)
    {
      return;
    }

    // 清理所有傳送門
    portals_.clear();
    registered_entities_.clear();

    // 子管理器會自動清理
    teleport_manager_.reset();
    center_of_mass_manager_.reset();

    is_initialized_ = false;
    std::cout << "PortalManager: Shutdown completed" << std::endl;
  }

  void PortalManager::update(float delta_time)
  {
    if (!is_initialized_)
    {
      return;
    }

    // 更新傳送門遞歸狀態（用於渲染）
    update_portal_recursive_states();

    // 更新傳送狀態管理器
    if (teleport_manager_)
    {
      teleport_manager_->update(delta_time);
    }

    // 更新質心管理器
    if (center_of_mass_manager_)
    {
      center_of_mass_manager_->update_auto_update_entities(delta_time);
    }

    // 注意：新架構中不再有主動檢測循環！
  }

  // === 傳送門管理 ===

  PortalId PortalManager::create_portal(const PortalPlane &plane)
  {
    PortalId id = generate_portal_id();
    auto portal = std::make_unique<Portal>(id);
    portal->set_plane(plane);

    portals_[id] = std::move(portal);

    std::cout << "PortalManager: Created portal " << id << std::endl;
    return id;
  }

  void PortalManager::destroy_portal(PortalId portal_id)
  {
    auto it = portals_.find(portal_id);
    if (it == portals_.end())
    {
      return;
    }

    // 取消鏈接
    unlink_portal(portal_id);

    // 清理與此傳送門相關的所有傳送狀態
    // 這裡需要通知 teleport_manager_ 清理相關狀態

    portals_.erase(it);
    std::cout << "PortalManager: Destroyed portal " << portal_id << std::endl;
  }

  bool PortalManager::link_portals(PortalId portal1, PortalId portal2)
  {
    Portal *p1 = get_portal(portal1);
    Portal *p2 = get_portal(portal2);

    if (!p1 || !p2 || portal1 == portal2)
    {
      return false;
    }

    // 取消現有鏈接
    unlink_portal(portal1);
    unlink_portal(portal2);

    // 建立新鏈接
    p1->set_linked_portal(portal2);
    p2->set_linked_portal(portal1);

    // 通知事件處理器
    notify_event_handler([portal1, portal2](IPortalEventHandler *handler)
                         {
                           // 這裡需要有相應的介面方法
                           // handler->on_portals_linked(portal1, portal2);
                         });

    std::cout << "PortalManager: Linked portals " << portal1 << " and " << portal2 << std::endl;
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

    // 斷開雙向鏈接
    portal->set_linked_portal(INVALID_PORTAL_ID);
    if (linked_portal)
    {
      linked_portal->set_linked_portal(INVALID_PORTAL_ID);
    }

    std::cout << "PortalManager: Unlinked portal " << portal_id << " from " << linked_portal_id << std::endl;
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
      std::cout << "PortalManager: Updated portal " << portal_id << " plane" << std::endl;
    }
  }

  void PortalManager::update_portal_physics_state(PortalId portal_id, const PhysicsState &physics_state)
  {
    Portal *portal = get_portal(portal_id);
    if (portal)
    {
      portal->set_physics_state(physics_state);
    }
  }

  // === 實體管理 ===

  void PortalManager::register_entity(EntityId entity_id)
  {
    if (interfaces_.physics_data->is_entity_valid(entity_id))
    {
      registered_entities_.insert(entity_id);
      std::cout << "PortalManager: Registered entity " << entity_id << std::endl;
    }
  }

  void PortalManager::unregister_entity(EntityId entity_id)
  {
    registered_entities_.erase(entity_id);

    // 清理相關的傳送狀態
    if (teleport_manager_)
    {
      teleport_manager_->cleanup_entity(entity_id);
    }

    std::cout << "PortalManager: Unregistered entity " << entity_id << std::endl;
  }

  // === IPortalPhysicsEventReceiver 介面實現 ===

  void PortalManager::on_entity_intersect_portal_start(EntityId entity_id, PortalId portal_id)
  {
    std::cout << "PortalManager: [EVENT] Entity " << entity_id
              << " intersect portal " << portal_id << " START" << std::endl;

    Portal *portal = get_portal(portal_id);
    if (!portal || !portal->is_active() || !portal->is_linked())
    {
      std::cout << "PortalManager: Invalid portal or not linked: " << portal_id << std::endl;
      return;
    }

    // 获取目标传送门
    PortalId target_portal_id = portal->get_linked_portal();
    Portal *target_portal = get_portal(target_portal_id);
    if (!target_portal || !target_portal->is_active())
    {
      std::cout << "PortalManager: Invalid target portal: " << target_portal_id << std::endl;
      return;
    }

    if (teleport_manager_)
    {
      teleport_manager_->handle_entity_intersect_start(entity_id, portal_id, portal, target_portal_id, target_portal);
    }
  }

  void PortalManager::on_entity_center_crossed_portal(EntityId entity_id, PortalId portal_id, PortalFace crossed_face)
  {
    std::cout << "PortalManager: [EVENT] Entity " << entity_id
              << " center crossed portal " << portal_id
              << " on face " << static_cast<int>(crossed_face) << std::endl;

    Portal *portal = get_portal(portal_id);
    if (!portal || !portal->is_linked())
    {
      return;
    }

    // 获取目标传送门
    PortalId target_portal_id = portal->get_linked_portal();
    Portal *target_portal = get_portal(target_portal_id);
    if (!target_portal)
    {
      return;
    }

    // 确定目标面（A<->B互换）
    PortalFace target_face = (crossed_face == PortalFace::A) ? PortalFace::B : PortalFace::A;

    if (teleport_manager_)
    {
      teleport_manager_->handle_entity_center_crossed(entity_id, portal_id, crossed_face, portal, target_portal_id, target_face, target_portal);
    }
  }

  void PortalManager::on_entity_fully_passed_portal(EntityId entity_id, PortalId portal_id)
  {
    std::cout << "PortalManager: [EVENT] Entity " << entity_id
              << " fully passed portal " << portal_id << std::endl;

    Portal *portal = get_portal(portal_id);
    if (!portal || !portal->is_linked())
    {
      return;
    }

    // 获取目标传送门
    PortalId target_portal_id = portal->get_linked_portal();
    Portal *target_portal = get_portal(target_portal_id);

    if (teleport_manager_)
    {
      teleport_manager_->handle_entity_fully_passed(entity_id, portal_id, portal, target_portal_id, target_portal);
    }
  }

  void PortalManager::on_entity_exit_portal(EntityId entity_id, PortalId portal_id)
  {
    std::cout << "PortalManager: [EVENT] Entity " << entity_id
              << " exit portal " << portal_id << std::endl;

    if (teleport_manager_)
    {
      teleport_manager_->handle_entity_exit_portal(entity_id, portal_id);
    }
  }

  // === 質心管理 ===

  void PortalManager::set_entity_center_of_mass_config(EntityId entity_id, const CenterOfMassConfig &config)
  {
    if (center_of_mass_manager_)
    {
      center_of_mass_manager_->set_entity_center_of_mass_config(entity_id, config);
      std::cout << "PortalManager: Set center of mass config for entity " << entity_id << std::endl;
    }
  }

  const CenterOfMassConfig *PortalManager::get_entity_center_of_mass_config(EntityId entity_id) const
  {
    if (center_of_mass_manager_)
    {
      return center_of_mass_manager_->get_entity_center_of_mass_config(entity_id);
    }
    return nullptr;
  }

  // === 渲染支援 ===

  std::vector<RenderPassDescriptor> PortalManager::calculate_render_passes(
      const CameraParams &main_camera, int max_recursion_depth) const
  {

    std::vector<RenderPassDescriptor> render_passes;

    if (!interfaces_.supports_rendering())
    {
      return render_passes; // 沒有渲染支援
    }

    // 獲取所有可見的傳送門
    std::vector<PortalId> visible_portals;
    for (const auto &[portal_id, portal] : portals_)
    {
      if (portal->is_linked() && is_portal_visible(portal_id, main_camera))
      {
        visible_portals.push_back(portal_id);
      }
    }

    // 為每個可見傳送門遞歸生成渲染通道
    for (PortalId portal_id : visible_portals)
    {
      calculate_recursive_render_passes(
          portal_id, main_camera, 0, max_recursion_depth, render_passes);
    }

    return render_passes;
  }

  bool PortalManager::get_entity_clipping_plane(EntityId entity_id, ClippingPlane &clipping_plane) const
  {
    if (!teleport_manager_)
    {
      return false;
    }

    const TeleportState *state = teleport_manager_->get_teleport_state(entity_id);
    if (!state || !state->is_teleporting)
    {
      return false;
    }

    const Portal *source_portal = get_portal(state->source_portal);
    if (!source_portal)
    {
      return false;
    }

    // 根据活跃的源面计算正确的裁剪平面
    const PortalPlane &portal_plane = source_portal->get_plane();
    Vector3 active_normal = portal_plane.get_face_normal(state->active_source_face);

    // 使用活跃面的法向量创建裁剪平面
    clipping_plane = ClippingPlane::from_point_and_normal(
        portal_plane.center,
        active_normal);

    std::cout << "PortalManager: Generated clipping plane for entity " << entity_id
              << " using face " << static_cast<int>(state->active_source_face) << std::endl;

    return true;
  }

  bool PortalManager::is_portal_visible(PortalId portal_id, const CameraParams &camera) const
  {
    const Portal *portal = get_portal(portal_id);
    if (!portal || !interfaces_.render_query)
    {
      return false;
    }

    return interfaces_.render_query->is_point_in_view_frustum(
        portal->get_plane().center, camera);
  }

  size_t PortalManager::get_teleporting_entity_count() const
  {
    return teleport_manager_ ? teleport_manager_->get_teleporting_entity_count() : 0;
  }

  // === 批量操作实现 ===

  void PortalManager::set_entity_batch_sync(EntityId entity_id, bool enable_batch, uint32_t sync_group_id)
  {
    if (teleport_manager_)
    {
      teleport_manager_->set_entity_batch_sync(entity_id, enable_batch, sync_group_id);
    }
  }

  void PortalManager::force_sync_portal_ghosts(PortalId portal_id)
  {
    if (teleport_manager_)
    {
      // 使用传送门ID作为同步组ID强制同步
      teleport_manager_->force_batch_sync_group(portal_id);
      std::cout << "PortalManager: Forced sync for all ghosts of portal " << portal_id << std::endl;
    }
  }

  PortalManager::BatchSyncStats PortalManager::get_batch_sync_stats() const
  {
    if (teleport_manager_)
    {
      auto teleport_stats = teleport_manager_->get_batch_sync_stats();
      return {teleport_stats.total_entities,
              teleport_stats.batch_enabled_entities,
              teleport_stats.pending_sync_count,
              teleport_stats.last_batch_sync_time};
    }
    return {0, 0, 0, 0.0f};
  }

  // === 手動傳送介面（向後相容） ===

  TeleportResult PortalManager::teleport_entity(EntityId entity_id, PortalId source_portal, PortalId target_portal)
  {
    const Portal *source = get_portal(source_portal);
    const Portal *target = get_portal(target_portal);

    if (!source || !target)
    {
      return TeleportResult::FAILED_INVALID_PORTAL;
    }

    if (!source->is_active() || !target->is_active())
    {
      return TeleportResult::FAILED_INVALID_PORTAL;
    }

    if (!interfaces_.physics_data->is_entity_valid(entity_id))
    {
      return TeleportResult::FAILED_INVALID_PORTAL;
    }

    // 獲取實體當前狀態
    Transform entity_transform = interfaces_.physics_data->get_entity_transform(entity_id);
    PhysicsState entity_physics = interfaces_.physics_data->get_entity_physics_state(entity_id);

    // 計算傳送後的狀態
    Transform new_transform = Math::PortalMath::transform_through_portal(
        entity_transform, source->get_plane(), target->get_plane());

    PhysicsState new_physics = Math::PortalMath::transform_physics_state_through_portal(
        entity_physics, source->get_plane(), target->get_plane());

    // 直接應用變換（立即傳送，不經過複雜的幽靈實體流程）
    interfaces_.physics_manipulator->set_entity_transform(entity_id, new_transform);
    interfaces_.physics_manipulator->set_entity_physics_state(entity_id, new_physics);

    // 通知事件處理器
    notify_event_handler([entity_id, source_portal, target_portal](IPortalEventHandler *handler)
                         {
        handler->on_entity_teleport_begin(entity_id, source_portal, target_portal);
        handler->on_entity_teleport_complete(entity_id, source_portal, target_portal); });

    std::cout << "PortalManager: Manual teleport completed for entity " << entity_id
              << " from portal " << source_portal << " to " << target_portal << std::endl;

    return TeleportResult::SUCCESS;
  }

  // === 私有方法實現 ===

  PortalId PortalManager::generate_portal_id()
  {
    return next_portal_id_++;
  }

  void PortalManager::update_portal_recursive_states()
  {
    if (!interfaces_.render_query)
    {
      return; // 沒有渲染支援
    }

    CameraParams main_camera = interfaces_.render_query->get_main_camera();

    for (auto &portal_pair : portals_)
    {
      Portal *portal = portal_pair.second.get();
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

      // 通知狀態變化（如果有相應的事件處理器方法）
      if (was_recursive != is_recursive)
      {
        // notify_event_handler([portal_id, is_recursive](IPortalEventHandler* handler) {
        //     handler->on_portal_recursive_state(portal_id, is_recursive);
        // });
      }
    }
  }

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

    // 确定A/B面映射（默认A→B，B→A）
    PortalFace source_face = PortalFace::A;
    PortalFace target_face = PortalFace::B;

    // TODO: 在实际应用中，应该从当前的传送状态或配置中获取面信息
    // 这里使用默认映射，可以根据需要扩展为参数

    // 創建渲染通道描述符
    RenderPassDescriptor descriptor;
    descriptor.source_portal_id = portal_id;
    descriptor.recursion_depth = current_depth;

    // 計算支持A/B面的虛擬相機
    descriptor.virtual_camera = Math::PortalMath::calculate_portal_camera(
        current_camera, portal->get_plane(), linked_portal->get_plane(), source_face, target_face);

    // 設定裁剪平面（使用目标面的法向量）
    descriptor.should_clip = true;
    Vector3 target_normal = linked_portal->get_plane().get_face_normal(target_face);
    descriptor.clipping_plane = ClippingPlane::from_point_and_normal(
        linked_portal->get_plane().center,
        target_normal);

    // 配置模板緩衝
    descriptor.use_stencil_buffer = true;
    descriptor.stencil_ref_value = current_depth + 1;

    render_passes.push_back(descriptor);

    std::cout << "PortalManager: Created render pass for portal " << portal_id
              << " at depth " << current_depth
              << " (faces: " << static_cast<int>(source_face)
              << " -> " << static_cast<int>(target_face) << ")" << std::endl;

    // 檢查遞歸傳送門（使用原始平面即可，因为递归检查主要看几何关系）
    if (!Math::PortalMath::is_portal_recursive(
            portal->get_plane(), linked_portal->get_plane(), descriptor.virtual_camera))
    {

      // 繼續遞歸處理其他可見的傳送門
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

  void PortalManager::notify_event_handler(const std::function<void(IPortalEventHandler *)> &callback)
  {
    if (interfaces_.event_handler)
    {
      callback(interfaces_.event_handler);
    }
  }

  bool PortalManager::is_valid_portal_id(PortalId portal_id) const
  {
    return portal_id != INVALID_PORTAL_ID && portals_.find(portal_id) != portals_.end();
  }

  // === 多段裁切系统方法实现 ===

  PortalManager::MultiSegmentClippingStats PortalManager::get_multi_segment_clipping_stats() const
  {
    MultiSegmentClippingStats stats{};
    
    if (teleport_manager_)
    {
      // 从 TeleportManager 获取多段裁切统计信息
      auto teleport_stats = teleport_manager_->get_multi_segment_clipping_stats();
      
      stats.active_multi_segment_entities = teleport_stats.active_multi_segment_entities;
      stats.total_clipping_planes = teleport_stats.total_clipping_planes;
      stats.total_visible_segments = teleport_stats.total_visible_segments;
      stats.average_segments_per_entity = teleport_stats.average_segments_per_entity;
      stats.frame_setup_time_ms = teleport_stats.frame_setup_time_ms;
    }
    
    return stats;
  }

  void PortalManager::set_entity_clipping_quality(EntityId entity_id, int quality_level)
  {
    if (teleport_manager_)
    {
      teleport_manager_->set_entity_clipping_quality(entity_id, quality_level);
      std::cout << "PortalManager: Set clipping quality " << quality_level 
                << " for entity " << entity_id << std::endl;
    }
  }

  void PortalManager::set_multi_segment_smooth_transitions(EntityId entity_id, bool enable, float blend_distance)
  {
    if (teleport_manager_)
    {
      teleport_manager_->set_multi_segment_smooth_transitions(entity_id, enable, blend_distance);
      std::cout << "PortalManager: Set smooth transitions " 
                << (enable ? "enabled" : "disabled") 
                << " for entity " << entity_id << std::endl;
    }
  }

  int PortalManager::get_entity_visible_segment_count(EntityId entity_id, const Vector3& camera_position) const
  {
    if (teleport_manager_)
    {
      return teleport_manager_->get_entity_visible_segment_count(entity_id, camera_position);
    }
    return 0;
  }

  void PortalManager::set_multi_segment_clipping_debug_mode(bool enable)
  {
    if (teleport_manager_)
    {
      teleport_manager_->set_multi_segment_clipping_debug_mode(enable);
      std::cout << "PortalManager: Multi-segment clipping debug mode " 
                << (enable ? "enabled" : "disabled") << std::endl;
    }
  }

} // namespace Portal