#include "../include/portal_core.h"
#include <algorithm>
#include <cmath>
#include <iostream>

namespace Portal {

    // Portal class implementation
    Portal::Portal(PortalId id)
        : id_(id)
        , linked_portal_id_(INVALID_PORTAL_ID)
        , is_active_(true)
        , is_recursive_(false)
        , max_recursion_depth_(3)
    {
    }

    // PortalManager class implementation
    PortalManager::PortalManager(const HostInterfaces& interfaces)
        : interfaces_(interfaces)
        , next_portal_id_(1)
        , is_initialized_(false)
        , teleport_transition_duration_(0.1f)
        , portal_detection_distance_(0.5f)
        , default_max_recursion_depth_(3)
    {
    }

    bool PortalManager::initialize() {
        if (is_initialized_) {
            return true;
        }

        if (!interfaces_.is_valid()) {
            return false;
        }

        is_initialized_ = true;
        return true;
    }

    void PortalManager::shutdown() {
        if (!is_initialized_) {
            return;
        }

        // 清理所有传送门
        portals_.clear();
        registered_entities_.clear();
        active_teleports_.clear();

        is_initialized_ = false;
    }

    void PortalManager::update(float delta_time) {
        if (!is_initialized_) {
            return;
        }

        // 更新传送门递归状态
        update_portal_recursive_states();

        // 检查实体与传送门的交互
        check_entity_portal_intersections();

        // 更新正在进行的传送
        update_entity_teleportation(delta_time);

        // 清理已完成的传送
        cleanup_completed_teleports();
    }

    PortalId PortalManager::create_portal(const PortalPlane& plane) {
        PortalId id = generate_portal_id();
        auto portal = std::make_unique<Portal>(id);
        portal->set_plane(plane);
        
        portals_[id] = std::move(portal);
        
        return id;
    }

    void PortalManager::destroy_portal(PortalId portal_id) {
        auto it = portals_.find(portal_id);
        if (it == portals_.end()) {
            return;
        }

        // 取消链接
        unlink_portal(portal_id);

        // 取消所有与此传送门相关的传送
        for (auto& [entity_id, teleport_state] : active_teleports_) {
            if (teleport_state.source_portal == portal_id || teleport_state.target_portal == portal_id) {
                cancel_entity_teleport(entity_id);
            }
        }

        portals_.erase(it);
    }

    bool PortalManager::link_portals(PortalId portal1, PortalId portal2) {
        Portal* p1 = get_portal(portal1);
        Portal* p2 = get_portal(portal2);

        if (!p1 || !p2 || portal1 == portal2) {
            return false;
        }

        // 取消现有链接
        unlink_portal(portal1);
        unlink_portal(portal2);

        // 建立新链接
        p1->set_linked_portal(portal2);
        p2->set_linked_portal(portal1);

        // 通知事件处理器
        notify_event_handler_if_available([portal1, portal2](IPortalEventHandler* handler) {
            handler->on_portals_linked(portal1, portal2);
        });

        return true;
    }

    void PortalManager::unlink_portal(PortalId portal_id) {
        Portal* portal = get_portal(portal_id);
        if (!portal || !portal->is_linked()) {
            return;
        }

        PortalId linked_portal_id = portal->get_linked_portal();
        Portal* linked_portal = get_portal(linked_portal_id);

        // 断开双向链接
        portal->set_linked_portal(INVALID_PORTAL_ID);
        if (linked_portal) {
            linked_portal->set_linked_portal(INVALID_PORTAL_ID);
        }

        // 通知事件处理器
        notify_event_handler_if_available([portal_id, linked_portal_id](IPortalEventHandler* handler) {
            handler->on_portals_unlinked(portal_id, linked_portal_id);
        });
    }

    Portal* PortalManager::get_portal(PortalId portal_id) {
        auto it = portals_.find(portal_id);
        return (it != portals_.end()) ? it->second.get() : nullptr;
    }

    const Portal* PortalManager::get_portal(PortalId portal_id) const {
        auto it = portals_.find(portal_id);
        return (it != portals_.end()) ? it->second.get() : nullptr;
    }

    void PortalManager::update_portal_plane(PortalId portal_id, const PortalPlane& plane) {
        Portal* portal = get_portal(portal_id);
        if (portal) {
            portal->set_plane(plane);
        }
    }

    void PortalManager::register_entity(EntityId entity_id) {
        if (interfaces_.physics_query->is_entity_valid(entity_id)) {
            registered_entities_.insert(entity_id);
        }
    }

    void PortalManager::unregister_entity(EntityId entity_id) {
        registered_entities_.erase(entity_id);
        
        // 取消正在进行的传送
        auto teleport_it = active_teleports_.find(entity_id);
        if (teleport_it != active_teleports_.end()) {
            cancel_entity_teleport(entity_id);
        }
    }

    TeleportResult PortalManager::teleport_entity(EntityId entity_id, PortalId source_portal, PortalId target_portal) {
        if (!can_entity_teleport(entity_id, source_portal)) {
            return TeleportResult::FAILED_INVALID_PORTAL;
        }

        const Portal* source = get_portal(source_portal);
        const Portal* target = get_portal(target_portal);

        if (!source || !target) {
            return TeleportResult::FAILED_INVALID_PORTAL;
        }

        // 获取实体当前状态
        Transform entity_transform = interfaces_.physics_query->get_entity_transform(entity_id);
        PhysicsState entity_physics = interfaces_.physics_query->get_entity_physics_state(entity_id);

        // 计算传送后的状态
        Transform new_transform = Math::PortalMath::transform_through_portal(
            entity_transform, source->get_plane(), target->get_plane()
        );

        PhysicsState new_physics = Math::PortalMath::transform_physics_state_through_portal(
            entity_physics, source->get_plane(), target->get_plane()
        );

        // 检查目标位置是否被阻挡
        if (interfaces_.physics_query->raycast(target->get_plane().center, new_transform.position, entity_id)) {
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
        notify_event_handler_if_available([entity_id, source_portal, target_portal](IPortalEventHandler* handler) {
            handler->on_entity_teleport_start(entity_id, source_portal, target_portal);
        });

        // 立即应用新的变换和物理状态
        interfaces_.physics_manipulator->set_entity_transform(entity_id, new_transform);
        interfaces_.physics_manipulator->set_entity_physics_state(entity_id, new_physics);

        return TeleportResult::SUCCESS;
    }

    TeleportResult PortalManager::teleport_entity_with_velocity(EntityId entity_id, PortalId source_portal, PortalId target_portal) {
        if (!can_entity_teleport(entity_id, source_portal)) {
            return TeleportResult::FAILED_INVALID_PORTAL;
        }

        const Portal* source = get_portal(source_portal);
        const Portal* target = get_portal(target_portal);

        if (!source || !target) {
            return TeleportResult::FAILED_INVALID_PORTAL;
        }

        // 获取实体当前状态
        Transform entity_transform = interfaces_.physics_query->get_entity_transform(entity_id);
        PhysicsState entity_physics = interfaces_.physics_query->get_entity_physics_state(entity_id);

        // 计算传送后的状态，考虑传送门速度
        Transform new_transform = Math::PortalMath::transform_through_portal(
            entity_transform, source->get_plane(), target->get_plane()
        );

        PhysicsState new_physics = Math::PortalMath::transform_physics_state_with_portal_velocity(
            entity_physics, 
            source->get_physics_state(), 
            target->get_physics_state(),
            source->get_plane(), 
            target->get_plane()
        );

        // 检查目标位置是否被阻挡
        if (interfaces_.physics_query->raycast(target->get_plane().center, new_transform.position, entity_id)) {
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
        notify_event_handler_if_available([entity_id, source_portal, target_portal](IPortalEventHandler* handler) {
            handler->on_entity_teleport_start(entity_id, source_portal, target_portal);
        });

        // 立即应用新的变换和物理状态
        interfaces_.physics_manipulator->set_entity_transform(entity_id, new_transform);
        interfaces_.physics_manipulator->set_entity_physics_state(entity_id, new_physics);

        return TeleportResult::SUCCESS;
    }

    void PortalManager::update_portal_physics_state(PortalId portal_id, const PhysicsState& physics_state) {
        Portal* portal = get_portal(portal_id);
        if (portal) {
            portal->set_physics_state(physics_state);
        }
    }

    const TeleportState* PortalManager::get_entity_teleport_state(EntityId entity_id) const {
        auto it = active_teleports_.find(entity_id);
        return (it != active_teleports_.end()) ? &it->second : nullptr;
    }

    std::vector<CameraParams> PortalManager::get_portal_render_cameras(
        PortalId portal_id,
        const CameraParams& base_camera,
        int max_depth
    ) const {
        std::vector<CameraParams> cameras;
        const Portal* portal = get_portal(portal_id);
        
        if (!portal || !portal->is_linked()) {
            return cameras;
        }

        const Portal* linked_portal = get_portal(portal->get_linked_portal());
        if (!linked_portal) {
            return cameras;
        }

        // 递归生成相机
        CameraParams current_camera = base_camera;
        
        for (int depth = 0; depth < max_depth; ++depth) {
            // 计算通过传送门的虚拟相机
            CameraParams portal_camera = Math::PortalMath::calculate_portal_camera(
                current_camera, portal->get_plane(), linked_portal->get_plane()
            );
            
            cameras.push_back(portal_camera);
            
            // 检查是否会导致递归
            if (Math::PortalMath::is_portal_recursive(
                portal->get_plane(), linked_portal->get_plane(), portal_camera)) {
                break;
            }
            
            current_camera = portal_camera;
        }

        return cameras;
    }

    bool PortalManager::is_portal_visible(PortalId portal_id, const CameraParams& camera) const {
        const Portal* portal = get_portal(portal_id);
        if (!portal) {
            return false;
        }

        return interfaces_.render_query->is_point_in_view_frustum(
            portal->get_plane().center, camera
        );
    }

    size_t PortalManager::get_teleporting_entity_count() const {
        size_t count = 0;
        for (const auto& [entity_id, teleport_state] : active_teleports_) {
            if (teleport_state.is_teleporting) {
                ++count;
            }
        }
        return count;
    }

    // Private methods implementation

    void PortalManager::update_entity_teleportation(float delta_time) {
        for (auto& [entity_id, teleport_state] : active_teleports_) {
            if (!teleport_state.is_teleporting) {
                continue;
            }

            teleport_state.transition_progress += delta_time / teleport_transition_duration_;
            
            if (teleport_state.transition_progress >= 1.0f) {
                complete_entity_teleport(entity_id);
            }
        }
    }

    void PortalManager::check_entity_portal_intersections() {
        for (EntityId entity_id : registered_entities_) {
            if (!interfaces_.physics_query->is_entity_valid(entity_id)) {
                continue;
            }

            Transform entity_transform = interfaces_.physics_query->get_entity_transform(entity_id);
            Vector3 bounds_min, bounds_max;
            interfaces_.physics_query->get_entity_bounds(entity_id, bounds_min, bounds_max);

            // 检查与所有传送门的交互
            for (const auto& [portal_id, portal] : portals_) {
                if (!portal->is_active() || !portal->is_linked()) {
                    continue;
                }

                // === 第一步：粗筛 (引擎) ===
                bool is_intersecting = Math::PortalMath::does_entity_intersect_portal(
                    bounds_min, bounds_max, entity_transform, portal->get_plane()
                );

                if (!is_intersecting) {
                    // 实体不与传送门相交，清理可能存在的状态
                    cleanup_entity_portal_state(entity_id, portal_id);
                    continue;
                }

                // === 第二步：精确状态判断 (库) ===
                
                // 分析包围盒分布
                BoundingBoxAnalysis bbox_analysis = Math::PortalMath::analyze_entity_bounding_box(
                    bounds_min, bounds_max, entity_transform, portal->get_plane()
                );

                // 获取当前状态
                TeleportState* teleport_state = get_or_create_teleport_state(entity_id, portal_id);
                PortalCrossingState previous_state = teleport_state->crossing_state;

                // 确定新的穿越状态
                PortalCrossingState new_state = Math::PortalMath::determine_crossing_state(
                    bbox_analysis, previous_state
                );

                // 更新状态
                teleport_state->previous_state = previous_state;
                teleport_state->crossing_state = new_state;
                teleport_state->bbox_analysis = bbox_analysis;

                // === 第三步：根据状态变化执行动作 ===
                handle_crossing_state_change(entity_id, portal_id, previous_state, new_state);
            }
        }
    }

    void PortalManager::update_portal_recursive_states() {
        if (!interfaces_.render_query) {
            return;
        }

        CameraParams main_camera = interfaces_.render_query->get_main_camera();

        for (auto& portal_pair : portals_) {
            PortalId portal_id = portal_pair.first;
            auto& portal = portal_pair.second;
            if (!portal->is_linked()) {
                portal->set_recursive(false);
                continue;
            }

            const Portal* linked_portal = get_portal(portal->get_linked_portal());
            if (!linked_portal) {
                continue;
            }

            bool was_recursive = portal->is_recursive();
            bool is_recursive = Math::PortalMath::is_portal_recursive(
                portal->get_plane(), linked_portal->get_plane(), main_camera
            );

            portal->set_recursive(is_recursive);

            // 通知状态变化
            if (was_recursive != is_recursive) {
                notify_event_handler_if_available([portal_id, is_recursive](IPortalEventHandler* handler) {
                    handler->on_portal_recursive_state(portal_id, is_recursive);
                });
            }
        }
    }

    void PortalManager::cleanup_completed_teleports() {
        auto it = active_teleports_.begin();
        while (it != active_teleports_.end()) {
            if (!it->second.is_teleporting) {
                it = active_teleports_.erase(it);
            } else {
                ++it;
            }
        }
    }

    bool PortalManager::can_entity_teleport(EntityId entity_id, PortalId portal_id) const {
        if (!interfaces_.physics_query->is_entity_valid(entity_id)) {
            return false;
        }

        const Portal* portal = get_portal(portal_id);
        if (!portal || !portal->is_active() || !portal->is_linked()) {
            return false;
        }

        // 检查实体是否已在传送中
        return active_teleports_.find(entity_id) == active_teleports_.end();
    }

    void PortalManager::start_entity_teleport(EntityId entity_id, PortalId source_portal) {
        const Portal* source = get_portal(source_portal);
        if (!source || !source->is_linked()) {
            return;
        }

        PortalId target_portal = source->get_linked_portal();
        teleport_entity(entity_id, source_portal, target_portal);
    }

    void PortalManager::complete_entity_teleport(EntityId entity_id) {
        auto it = active_teleports_.find(entity_id);
        if (it == active_teleports_.end()) {
            return;
        }

        TeleportState& teleport_state = it->second;
        teleport_state.is_teleporting = false;
        teleport_state.transition_progress = 1.0f;

        // 通知传送完成
        notify_event_handler_if_available([entity_id, &teleport_state](IPortalEventHandler* handler) {
            handler->on_entity_teleport_complete(entity_id, teleport_state.source_portal, teleport_state.target_portal);
        });
    }

    void PortalManager::cancel_entity_teleport(EntityId entity_id) {
        auto it = active_teleports_.find(entity_id);
        if (it != active_teleports_.end()) {
            it->second.is_teleporting = false;
        }
    }

    PortalId PortalManager::generate_portal_id() {
        return next_portal_id_++;
    }

    bool PortalManager::is_valid_portal_id(PortalId portal_id) const {
        return portal_id != INVALID_PORTAL_ID && portals_.find(portal_id) != portals_.end();
    }

    void PortalManager::notify_event_handler_if_available(
        const std::function<void(IPortalEventHandler*)>& callback
    ) const {
        if (interfaces_.event_handler) {
            callback(interfaces_.event_handler);
        }
    }

    // 新增：計算渲染通道描述符
    std::vector<RenderPassDescriptor> PortalManager::calculate_render_passes(
        const CameraParams& main_camera,
        int max_recursion_depth
    ) const {
        std::vector<RenderPassDescriptor> render_passes;
        
        // 获取所有可见的传送门
        std::vector<PortalId> visible_portals;
        for (const auto& [portal_id, portal] : portals_) {
            if (portal->is_linked() && is_portal_visible(portal_id, main_camera)) {
                visible_portals.push_back(portal_id);
            }
        }
        
        // 为每个可见传送门递归生成渲染通道
        for (PortalId portal_id : visible_portals) {
            calculate_recursive_render_passes(
                portal_id, main_camera, 0, max_recursion_depth, render_passes
            );
        }
        
        return render_passes;
    }

    // 新增：獲取實體裁切平面
    bool PortalManager::get_entity_clipping_plane(EntityId entity_id, ClippingPlane& clipping_plane) const {
        auto it = active_teleports_.find(entity_id);
        if (it == active_teleports_.end() || !it->second.is_teleporting) {
            return false;
        }
        
        const TeleportState& teleport_state = it->second;
        const Portal* source_portal = get_portal(teleport_state.source_portal);
        
        if (!source_portal) {
            return false;
        }
        
        // 计算传送门平面作为裁剪平面
        const PortalPlane& portal_plane = source_portal->get_plane();
        clipping_plane = ClippingPlane::from_point_and_normal(
            portal_plane.center, 
            portal_plane.normal
        );
        
        return true;
    }

    // 輔助方法：遞歸計算渲染通道
    void PortalManager::calculate_recursive_render_passes(
        PortalId portal_id,
        const CameraParams& current_camera,
        int current_depth,
        int max_depth,
        std::vector<RenderPassDescriptor>& render_passes
    ) const {
        if (current_depth >= max_depth) {
            return;
        }
        
        const Portal* portal = get_portal(portal_id);
        if (!portal || !portal->is_linked()) {
            return;
        }
        
        const Portal* linked_portal = get_portal(portal->get_linked_portal());
        if (!linked_portal) {
            return;
        }
        
        // 创建渲染通道描述符
        RenderPassDescriptor descriptor;
        descriptor.source_portal_id = portal_id;
        descriptor.recursion_depth = current_depth;
        
        // 计算虚拟相机
        descriptor.virtual_camera = Math::PortalMath::calculate_portal_camera(
            current_camera, portal->get_plane(), linked_portal->get_plane()
        );
        
        // 设置裁剪平面（防止看到传送门背后）
        descriptor.should_clip = true;
        descriptor.clipping_plane = ClippingPlane::from_point_and_normal(
            linked_portal->get_plane().center,
            linked_portal->get_plane().normal
        );
        
        // 配置模板缓冲
        descriptor.use_stencil_buffer = true;
        descriptor.stencil_ref_value = current_depth + 1;
        
        render_passes.push_back(descriptor);
        
        // 检查递归传送门
        if (Math::PortalMath::is_portal_recursive(
            portal->get_plane(), linked_portal->get_plane(), descriptor.virtual_camera
        )) {
            // 检查递归传送门
            for (const auto& [next_portal_id, next_portal] : portals_) {
                if (next_portal_id != portal_id && next_portal->is_linked() && 
                    is_portal_visible(next_portal_id, descriptor.virtual_camera)) {
                    calculate_recursive_render_passes(
                        next_portal_id, descriptor.virtual_camera, 
                        current_depth + 1, max_depth, render_passes
                    );
                }
            }
        }
    }

    // === 新增：三状态机辅助方法实现 ===
    
    TeleportState* PortalManager::get_or_create_teleport_state(EntityId entity_id, PortalId portal_id) {
        auto it = active_teleports_.find(entity_id);
        if (it != active_teleports_.end()) {
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
    
    void PortalManager::cleanup_entity_portal_state(EntityId entity_id, PortalId portal_id) {
        auto it = active_teleports_.find(entity_id);
        if (it != active_teleports_.end() && it->second.source_portal == portal_id) {
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
        PortalCrossingState new_state
    ) {
        if (previous_state == new_state) {
            return; // 没有状态变化
        }
        
        TeleportState* state = get_or_create_teleport_state(entity_id, portal_id);
        
        switch (new_state) {
            case PortalCrossingState::CROSSING:
                if (previous_state == PortalCrossingState::NOT_TOUCHING) {
                    // 开始穿越：启用模型裁切，创建幽灵碰撞体
                    std::cout << "Entity " << entity_id << " started crossing portal " << portal_id << std::endl;
                    create_ghost_collider_if_needed(entity_id, portal_id);
                    state->is_teleporting = true;
                }
                // 更新幽灵碰撞体位置
                update_ghost_collider_position(entity_id, portal_id);
                break;
                
            case PortalCrossingState::TELEPORTED:
                if (previous_state == PortalCrossingState::CROSSING) {
                    // 完成传送：应用最终变换，销毁幽灵碰撞体
                    std::cout << "Entity " << entity_id << " completed teleportation through portal " << portal_id << std::endl;
                    complete_entity_teleport(entity_id);
                }
                break;
                
            case PortalCrossingState::NOT_TOUCHING:
                if (previous_state == PortalCrossingState::CROSSING) {
                    // 取消传送：实体退回到原侧
                    std::cout << "Entity " << entity_id << " cancelled teleportation" << std::endl;
                    cancel_entity_teleport(entity_id);
                }
                break;
        }
    }
    
    void PortalManager::create_ghost_collider_if_needed(EntityId entity_id, PortalId portal_id) {
        TeleportState* state = get_or_create_teleport_state(entity_id, portal_id);
        if (state->has_ghost_collider) {
            return; // 已经存在
        }
        
        // 计算幽灵碰撞体的变换
        Transform entity_transform = interfaces_.physics_query->get_entity_transform(entity_id);
        const Portal* source_portal = get_portal(portal_id);
        const Portal* target_portal = get_portal(source_portal->get_linked_portal());
        
        Transform ghost_transform = Math::PortalMath::calculate_ghost_transform(
            entity_transform,
            source_portal->get_plane(),
            target_portal->get_plane(),
            state->bbox_analysis.crossing_ratio
        );
        
        // 在引擎中创建幽灵碰撞体
        bool success = interfaces_.physics_manipulator->create_ghost_collider(entity_id, ghost_transform);
        if (success) {
            state->has_ghost_collider = true;
            std::cout << "Created ghost collider for entity " << entity_id << std::endl;
        }
    }
    
    void PortalManager::update_ghost_collider_position(EntityId entity_id, PortalId portal_id) {
        TeleportState* state = get_or_create_teleport_state(entity_id, portal_id);
        if (!state->has_ghost_collider) {
            return;
        }
        
        // 重新计算幽灵变换
        Transform entity_transform = interfaces_.physics_query->get_entity_transform(entity_id);
        PhysicsState entity_physics = interfaces_.physics_query->get_entity_physics_state(entity_id);
        
        const Portal* source_portal = get_portal(portal_id);
        const Portal* target_portal = get_portal(source_portal->get_linked_portal());
        
        Transform ghost_transform = Math::PortalMath::calculate_ghost_transform(
            entity_transform,
            source_portal->get_plane(),
            target_portal->get_plane(),
            state->bbox_analysis.crossing_ratio
        );
        
        PhysicsState ghost_physics = Math::PortalMath::transform_physics_state_through_portal(
            entity_physics, source_portal->get_plane(), target_portal->get_plane()
        );
        
        // 更新幽灵碰撞体
        interfaces_.physics_manipulator->update_ghost_collider(entity_id, ghost_transform, ghost_physics);
    }
    
    void PortalManager::destroy_ghost_collider_if_exists(EntityId entity_id) {
        auto it = active_teleports_.find(entity_id);
        if (it != active_teleports_.end() && it->second.has_ghost_collider) {
            interfaces_.physics_manipulator->destroy_ghost_collider(entity_id);
            it->second.has_ghost_collider = false;
            std::cout << "Destroyed ghost collider for entity " << entity_id << std::endl;
        }
    }

} // namespace Portal
