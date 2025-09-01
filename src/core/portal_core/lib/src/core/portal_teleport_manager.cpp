#include "../../include/core/portal_teleport_manager.h"
#include "../../include/core/portal.h"
#include "../../include/math/portal_math.h"
#include "../../include/rendering/multi_segment_clipping.h"
#include <iostream>
#include <algorithm>
#include <chrono>

namespace Portal
{

  TeleportManager::TeleportManager(IPhysicsDataProvider *physics_data,
                                   IPhysicsManipulator *physics_manipulator,
                                   IPortalEventHandler *event_handler)
      : physics_data_(physics_data), physics_manipulator_(physics_manipulator), event_handler_(event_handler),
        portal_getter_(nullptr), ghost_sync_timer_(0.0f), sync_frequency_(60.0f),
        use_logical_entity_control_(false) // 預設關閉邏輯實體控制
  {
    if (!physics_data_ || !physics_manipulator_)
    {
      throw std::invalid_argument("Physics data provider and manipulator are required");
    }

    // 創建邏輯實體管理器
    logical_entity_manager_ = std::make_unique<LogicalEntityManager>(
        physics_data_, physics_manipulator_, event_handler_);

    // **新增：創建多段裁切管理器**
    multi_segment_clipping_manager_ = std::make_unique<MultiSegmentClippingManager>();

    // 設置多段裁切管理器的回調
    multi_segment_clipping_manager_->set_apply_clipping_callback(
        [this](EntityId entity_id, const MultiSegmentClippingDescriptor& descriptor) {
            apply_multi_segment_clipping_to_entity(entity_id, descriptor);
        });

    multi_segment_clipping_manager_->set_clear_clipping_callback(
        [this](EntityId entity_id) {
            clear_entity_multi_segment_clipping(entity_id);
        });

    std::cout << "TeleportManager created with logical entity support and multi-segment clipping" << std::endl;
  }

  TeleportManager::~TeleportManager()
  {
    // 析构函数在这里定义，确保unique_ptr可以正确析构
  }

  void TeleportManager::update(float delta_time)
  {
    std::cout << "DEBUG: TeleportManager::update() start" << std::endl;
    ghost_sync_timer_ += delta_time;

    // **修復：嚴格分離新舊系統，確保不會同時運行**
    if (use_logical_entity_control_ && logical_entity_manager_)
    {
      std::cout << "DEBUG: Using NEW logical entity system (old ghost sync DISABLED)" << std::endl;
      logical_entity_manager_->update(delta_time);
      update_logical_entity_teleport_states(delta_time);
      std::cout << "DEBUG: Logical entity system update completed" << std::endl;
      
      // **關鍵：完全跳過舊系統，避免雙重同步**
      // 注意：不再調用 sync_all_ghost_entities
    }
    else
    {
      std::cout << "DEBUG: Using LEGACY ghost entity system (logical entity DISABLED)" << std::endl;
      sync_all_ghost_entities(delta_time);
      std::cout << "DEBUG: Legacy ghost entity sync completed" << std::endl;
      
      // **注意：邏輯實體系統完全不被調用**
    }

    // 清理已完成的傳送
    std::cout << "DEBUG: Starting cleanup_completed_teleports" << std::endl;
    cleanup_completed_teleports();
    std::cout << "DEBUG: cleanup_completed_teleports completed" << std::endl;

    std::cout << "TeleportManager: Updated " << active_teleports_.size()
              << " active teleports (logical entity mode: "
              << (use_logical_entity_control_ ? "enabled" : "disabled") << ")" << std::endl;
    
    std::cout << "DEBUG: TeleportManager::update() end" << std::endl;
  }

  void TeleportManager::handle_entity_intersect_start(EntityId entity_id, PortalId portal_id, Portal *portal,
                                                      PortalId target_portal_id, Portal *target_portal)
  {
    if (!portal || !target_portal || !portal->is_active() || !target_portal->is_active())
    {
      std::cout << "TeleportManager: Invalid portals for entity " << entity_id << std::endl;
      return;
    }

    std::cout << "TeleportManager: Entity " << entity_id
              << " started intersecting portal " << portal_id
              << " (target: " << target_portal_id << ") - delegating to chain system" << std::endl;

    // ** 新架構：直接委託給鏈式系統處理 **
    // 這樣做的好處：
    // 1. 保持向後兼容 - 舊的二元傳送會自動變成長度為2的鏈
    // 2. 統一處理邏輯 - 所有傳送都通過鏈式系統管理
    // 3. 支持擴展 - 如果實體進入更多傳送門，鏈會自動擴展

    handle_chain_node_intersect_portal(entity_id, portal_id, portal, target_portal_id, target_portal);

    // 向後兼容：維護傳統的 TeleportState（從鏈狀態中同步）
    const EntityChainState *chain_state = get_entity_chain_state(entity_id);
    if (chain_state && !chain_state->chain.empty())
    {
      TeleportState *state = get_or_create_teleport_state(entity_id, portal_id);
      state->source_portal = portal_id;
      state->target_portal = target_portal_id;
      state->crossing_state = PortalCrossingState::CROSSING;
      state->is_teleporting = true;
      state->seamless_mode = true;

      // 設置A/B面配置（默認A->B）
      state->source_face = PortalFace::A;
      state->target_face = PortalFace::B;
      state->active_source_face = PortalFace::A;
      state->active_target_face = PortalFace::B;

      // 如果鏈長度大於1，則找到幽靈實體ID
      if (chain_state->chain.size() > 1)
      {
        // 找到非主體位置的第一個實體作為ghost_entity_id（向後兼容）
        for (size_t i = 0; i < chain_state->chain.size(); ++i)
        {
          if (static_cast<int>(i) != chain_state->main_position)
          {
            state->ghost_entity_id = chain_state->chain[i].entity_id;
            break;
          }
        }
      }
    }

    // 通知事件處理器
    notify_event_handler([entity_id, portal_id, target_portal_id](IPortalEventHandler *handler)
                         { handler->on_entity_teleport_begin(entity_id, portal_id, target_portal_id); });
  }

  void TeleportManager::handle_entity_center_crossed(EntityId entity_id, PortalId portal_id,
                                                     PortalFace crossed_face, Portal *portal,
                                                     PortalId target_portal_id, PortalFace target_face, Portal *target_portal)
  {
    if (!portal || !target_portal)
      return;

    std::cout << "TeleportManager: Entity " << entity_id
              << " center crossed portal " << portal_id
              << " on face " << static_cast<int>(crossed_face)
              << " (target: " << target_portal_id << ", target_face: " << static_cast<int>(target_face)
              << ") - delegating to chain system" << std::endl;

    // ** 新架構：委託給鏈式系統處理質心穿越 **
    // 鏈式系統會自動處理主體位置遷移
    handle_chain_node_center_crossed(entity_id, portal_id, crossed_face, portal, target_portal_id, target_face, target_portal);

    // 向後兼容：更新傳統的 TeleportState
    auto it = active_teleports_.find(entity_id);
    if (it != active_teleports_.end())
    {
      TeleportState &state = it->second;

      // 更新面配置
      state.active_source_face = crossed_face;
      state.active_target_face = target_face;
      state.center_has_crossed = true;

      // 計算穿越點
      if (physics_data_)
      {
        [[maybe_unused]] Transform entity_transform = physics_data_->get_entity_transform(entity_id);
        Vector3 center_of_mass = physics_data_->calculate_entity_center_of_mass(entity_id);
        state.crossing_point = center_of_mass;
      }

      // ** 重要：鏈式系統已經處理了主體位置遷移，這裡只需要同步狀態 **
      const EntityChainState *chain_state = get_entity_chain_state(entity_id);
      if (chain_state)
      {
        // 檢查鏈中的主體是否已經發生變化
        EntityId current_main_entity = get_chain_main_entity(entity_id);
        if (current_main_entity != INVALID_ENTITY_ID && current_main_entity != entity_id)
        {
          // 主體已經遷移到鏈的另一個位置
          std::cout << "TeleportManager: Main entity migrated from " << entity_id
                    << " to " << current_main_entity << " in chain" << std::endl;

          // 標記角色互換已完成
          state.role_swapped = true;
          state.crossing_state = PortalCrossingState::TELEPORTED;

          // ** 修復: 不再重複觸發角色交換事件 **
          // 事件已經由 shift_main_entity_position 觸發了
        }
        // 如果主體沒有變化，說明這是鏈中非主體節點的質心穿越，不需要特殊處理
      }
      else
      {
        // 沒有鏈狀態，可能是傳統的二元傳送，執行舊的邏輯
        // 檢查是否需要執行角色互換
        if (state.ghost_entity_id != INVALID_ENTITY_ID && !state.role_swapped)
        {
          std::cout << "TeleportManager: Executing legacy entity role swap for entity " << entity_id << std::endl;

          if (execute_entity_role_swap(entity_id, state.ghost_entity_id, crossed_face, target_face))
          {
            state.role_swapped = true;
            state.crossing_state = PortalCrossingState::TELEPORTED;

            // 通知事件處理器
            Transform main_transform = physics_data_->get_entity_transform(entity_id);
            Transform ghost_transform = physics_data_->get_entity_transform(state.ghost_entity_id);

            notify_event_handler([entity_id, portal_id, state, main_transform, ghost_transform](IPortalEventHandler *handler)
                                 { handler->on_entity_roles_swapped(
                                       entity_id, state.ghost_entity_id, // old main, old ghost
                                       state.ghost_entity_id, entity_id, // new main, new ghost
                                       portal_id,
                                       ghost_transform, main_transform); });
          }
        }
      }
    }
  }

  void TeleportManager::handle_entity_fully_passed(EntityId entity_id, PortalId portal_id, Portal *portal,
                                                   PortalId target_portal_id, Portal *target_portal)
  {
    std::cout << "TeleportManager: Entity " << entity_id
              << " fully passed through portal " << portal_id
              << " to target " << target_portal_id << " - delegating to chain system" << std::endl;

    // ** 新架構：委託給鏈式系統處理 **
    handle_chain_node_fully_passed(entity_id, portal_id);

    // 向後兼容：更新傳統的 TeleportState
    auto it = active_teleports_.find(entity_id);
    if (it != active_teleports_.end())
    {
      TeleportState &state = it->second;
      state.crossing_state = PortalCrossingState::TELEPORTED;
      state.is_teleporting = false;
      state.transition_progress = 1.0f;

      // 注意：不再立即銷毀幽靈實體，因為鏈式系統會管理所有節點
      // 只有當整個鏈都不再需要某個節點時，鏈式系統才會銷毀它

      // 通知完成
      notify_event_handler([entity_id, portal_id, target_portal_id](IPortalEventHandler *handler)
                           { handler->on_entity_teleport_complete(entity_id, portal_id, target_portal_id); });
    }
  }

  void TeleportManager::handle_entity_exit_portal(EntityId entity_id, PortalId portal_id)
  {
    std::cout << "TeleportManager: Entity " << entity_id
              << " exited portal " << portal_id << " - delegating to chain system" << std::endl;

    // ** 新架構：委託給鏈式系統處理 **
    // 鏈式系統會決定是否需要收縮鏈（移除不再需要的節點）
    handle_chain_node_exit_portal(entity_id, portal_id);

    // 向後兼容：檢查是否需要清理傳統的 TeleportState
    // 只有當實體完全離開所有傳送門時才清理
    const EntityChainState *chain_state = get_entity_chain_state(entity_id);
    if (!chain_state || chain_state->chain.empty())
    {
      // 沒有鏈狀態或鏈已空，說明實體已完全脫離傳送門系統
      cleanup_entity(entity_id);
      std::cout << "TeleportManager: Completely cleaned up entity " << entity_id << std::endl;
    }
    else
    {
      std::cout << "TeleportManager: Entity " << entity_id << " still has chain state, not cleaning up" << std::endl;
    }
  }

  const TeleportState *TeleportManager::get_teleport_state(EntityId entity_id) const
  {
    auto it = active_teleports_.find(entity_id);
    return (it != active_teleports_.end()) ? &it->second : nullptr;
  }

  const GhostEntitySnapshot *TeleportManager::get_ghost_snapshot(EntityId entity_id) const
  {
    auto it = ghost_snapshots_.find(entity_id);
    return (it != ghost_snapshots_.end()) ? &it->second : nullptr;
  }

  bool TeleportManager::is_entity_teleporting(EntityId entity_id) const
  {
    auto it = active_teleports_.find(entity_id);
    return (it != active_teleports_.end()) && it->second.is_teleporting;
  }

  size_t TeleportManager::get_teleporting_entity_count() const
  {
    return std::count_if(active_teleports_.begin(), active_teleports_.end(),
                         [](const auto &pair)
                         { return pair.second.is_teleporting; });
  }

  void TeleportManager::cleanup_entity(EntityId entity_id)
  {
    // 清理邏輯實體
    if (use_logical_entity_control_)
    {
      destroy_logical_entity_for_teleport(entity_id);
    }

    // **新增：清理多段裁切**
    if (multi_segment_clipping_manager_)
    {
      multi_segment_clipping_manager_->cleanup_entity_clipping(entity_id);
    }

    // 銷毀幽靈實體
    destroy_ghost_entity(entity_id);

    // 清理狀態
    active_teleports_.erase(entity_id);
    ghost_snapshots_.erase(entity_id);

    // 清理映射
    auto ghost_it = main_to_ghost_mapping_.find(entity_id);
    if (ghost_it != main_to_ghost_mapping_.end())
    {
      EntityId ghost_id = ghost_it->second;
      ghost_to_main_mapping_.erase(ghost_id);
      main_to_ghost_mapping_.erase(entity_id);
    }

    std::cout << "TeleportManager: Cleaned up entity " << entity_id << std::endl;
  }

  void TeleportManager::cleanup_completed_teleports()
  {
    auto it = active_teleports_.begin();
    while (it != active_teleports_.end())
    {
      if (!it->second.is_teleporting &&
          it->second.crossing_state == PortalCrossingState::TELEPORTED)
      {

        EntityId entity_id = it->first;
        it = active_teleports_.erase(it);

        // 也清理對應的快照
        ghost_snapshots_.erase(entity_id);

        std::cout << "TeleportManager: Cleaned up completed teleport for entity " << entity_id << std::endl;
      }
      else
      {
        ++it;
      }
    }
  }

  // === 私有方法實現 ===

  bool TeleportManager::create_ghost_entity(EntityId entity_id, PortalId portal_id,
                                            const Portal *source_portal, const Portal *target_portal,
                                            PortalFace source_face, PortalFace target_face)
  {
    // **修復：邏輯實體模式下禁止創建舊式幽靈實體**
    if (use_logical_entity_control_)
    {
      std::cout << "TeleportManager: SKIP legacy ghost creation - using logical entity system instead" << std::endl;
      return false; // 返回 false 表示使用邏輯實體系統
    }
    
    if (!source_portal || !target_portal || !physics_data_ || !physics_manipulator_)
    {
      return false;
    }

    // 計算幽靈實體狀態
    Transform ghost_transform;
    PhysicsState ghost_physics;

    if (!calculate_ghost_state(entity_id, source_portal, target_portal, source_face, target_face,
                               ghost_transform, ghost_physics))
    {
      return false;
    }

    // 獲取實體描述用於完整功能支持
    EntityDescription entity_desc = physics_data_->get_entity_description(entity_id);

    // 創建完整功能幽靈實體（支持A/B面）
    EntityId ghost_id = physics_manipulator_->create_full_functional_ghost(
        entity_desc, ghost_transform, ghost_physics, source_face, target_face);

    if (ghost_id == INVALID_ENTITY_ID)
    {
      std::cerr << "TeleportManager: Failed to create ghost entity for " << entity_id << std::endl;
      return false;
    }

    // 建立映射
    main_to_ghost_mapping_[entity_id] = ghost_id;
    ghost_to_main_mapping_[ghost_id] = entity_id;

    // 更新傳送狀態
    auto it = active_teleports_.find(entity_id);
    if (it != active_teleports_.end())
    {
      it->second.ghost_entity_id = ghost_id;

      // 自动启用批量同步（可以根据策略调整）
      it->second.enable_batch_sync = true;
      it->second.sync_group_id = portal_id; // 使用传送门ID作为同步组
    }

    // 創建完整快照（包含A/B面信息）
    GhostEntitySnapshot snapshot;
    snapshot.main_entity_id = entity_id;
    snapshot.ghost_entity_id = ghost_id;
    snapshot.main_transform = physics_data_->get_entity_transform(entity_id);
    snapshot.ghost_transform = ghost_transform;
    snapshot.main_physics = physics_data_->get_entity_physics_state(entity_id);
    snapshot.ghost_physics = ghost_physics;
    snapshot.source_face = source_face;
    snapshot.target_face = target_face;
    snapshot.has_full_functionality = true;
    snapshot.timestamp = static_cast<uint64_t>(ghost_sync_timer_ * 1000.0f);

    // 設置包圍盒
    physics_data_->get_entity_bounds(entity_id, snapshot.main_bounds_min, snapshot.main_bounds_max);
    snapshot.ghost_bounds_min = snapshot.main_bounds_min;
    snapshot.ghost_bounds_max = snapshot.main_bounds_max;

    ghost_snapshots_[entity_id] = snapshot;

    // 通知事件處理器
    notify_event_handler([entity_id, ghost_id, portal_id](IPortalEventHandler *handler)
                         { handler->on_ghost_entity_created(entity_id, ghost_id, portal_id); });

    std::cout << "TeleportManager: Created ghost entity " << ghost_id
              << " for main entity " << entity_id
              << " (A/B faces: " << static_cast<int>(source_face)
              << " -> " << static_cast<int>(target_face) << ")" << std::endl;

    return true;
  }

  void TeleportManager::update_ghost_entity(EntityId entity_id, const Portal *source_portal, const Portal *target_portal)
  {
    // **修復：邏輯實體模式下禁止舊式幽靈更新**
    if (use_logical_entity_control_)
    {
      std::cout << "TeleportManager: SKIP legacy ghost update - using logical entity system" << std::endl;
      return;
    }
    
    auto snapshot_it = ghost_snapshots_.find(entity_id);
    if (snapshot_it == ghost_snapshots_.end())
    {
      return;
    }

    auto teleport_it = active_teleports_.find(entity_id);
    if (teleport_it == active_teleports_.end())
    {
      return;
    }

    const TeleportState &state = teleport_it->second;

    // 重新計算幽靈狀態
    Transform new_ghost_transform;
    PhysicsState new_ghost_physics;

    if (calculate_ghost_state(entity_id, source_portal, target_portal,
                              state.active_source_face, state.active_target_face,
                              new_ghost_transform, new_ghost_physics))
    {

      GhostEntitySnapshot &snapshot = snapshot_it->second;
      snapshot.ghost_transform = new_ghost_transform;
      snapshot.ghost_physics = new_ghost_physics;
      snapshot.timestamp = static_cast<uint64_t>(ghost_sync_timer_ * 1000.0f);

      if (snapshot.ghost_entity_id != INVALID_ENTITY_ID)
      {
        physics_manipulator_->update_ghost_entity(
            snapshot.ghost_entity_id,
            new_ghost_transform,
            new_ghost_physics);
      }
    }
  }

  void TeleportManager::destroy_ghost_entity(EntityId entity_id)
  {
    auto ghost_it = main_to_ghost_mapping_.find(entity_id);
    if (ghost_it != main_to_ghost_mapping_.end())
    {
      EntityId ghost_id = ghost_it->second;

      // 銷毀物理實體
      physics_manipulator_->destroy_ghost_entity(ghost_id);

      // 通知事件處理器
      notify_event_handler([entity_id, ghost_id](IPortalEventHandler *handler)
                           { handler->on_ghost_entity_destroyed(entity_id, ghost_id, INVALID_PORTAL_ID); });

      // 清理映射
      ghost_to_main_mapping_.erase(ghost_id);
      main_to_ghost_mapping_.erase(entity_id);

      std::cout << "TeleportManager: Destroyed ghost entity " << ghost_id
                << " for main entity " << entity_id << std::endl;
    }

    // 清理快照
    ghost_snapshots_.erase(entity_id);
  }

  bool TeleportManager::execute_entity_role_swap(EntityId main_entity_id, EntityId ghost_entity_id,
                                                 PortalFace source_face, PortalFace target_face)
  {
    if (!physics_manipulator_)
    {
      return false;
    }

    // **修復：邏輯實體模式下使用新的角色交換機制**
    if (use_logical_entity_control_)
    {
      std::cout << "TeleportManager: Using logical entity role swap (NEW system)" << std::endl;
      
      // 在邏輯實體系統中，角色交換由邏輯實體管理器處理
      // 這裡只需要更新映射關係，不需要直接操作物理狀態
      LogicalEntityId logical_id = logical_entity_manager_->get_logical_entity_by_physical_entity(main_entity_id);
      if (logical_id != INVALID_LOGICAL_ENTITY_ID)
      {
        // 更新主控實體
        logical_entity_manager_->set_primary_controlled_entity(logical_id, ghost_entity_id);
        
        // 強制更新邏輯實體狀態
        logical_entity_manager_->force_update_logical_entity(logical_id);
        
        std::cout << "TeleportManager: Logical entity role swap completed" << std::endl;
        return true;
      }
      else
      {
        std::cout << "TeleportManager: Warning - No logical entity found for role swap" << std::endl;
        return false;
      }
    }

    // **以下是舊系統的無感角色互換邏輯（僅在非邏輯實體模式下使用）**
    std::cout << "TeleportManager: Using legacy seamless role swap (OLD system)" << std::endl;

    // === 無感角色互換的正確實現 ===

    // 1. 保存當前物理狀態（各自保持原狀態是無感的關鍵）
    Transform main_transform = physics_data_->get_entity_transform(main_entity_id);
    PhysicsState main_physics = physics_data_->get_entity_physics_state(main_entity_id);
    Transform ghost_transform = physics_data_->get_entity_transform(ghost_entity_id);
    PhysicsState ghost_physics = physics_data_->get_entity_physics_state(ghost_entity_id);

    std::cout << "TeleportManager: 保存狀態 - 主體位置: ("
              << main_transform.position.x << ", " << main_transform.position.y << ", " << main_transform.position.z
              << "), 幽靈位置: ("
              << ghost_transform.position.x << ", " << ghost_transform.position.y << ", " << ghost_transform.position.z << ")" << std::endl;

    // 2. 執行角色邏輯互換（不交換物理狀態！）
    bool success = physics_manipulator_->swap_entity_roles_with_faces(
        main_entity_id, ghost_entity_id, source_face, target_face);

    if (success)
    {
      // 3. 確保物理狀態保持不變（這是無感的核心）
      // 原主體保持原來的物理狀態
      physics_manipulator_->set_entity_transform(main_entity_id, main_transform);
      physics_manipulator_->force_set_entity_physics_state(main_entity_id, main_transform, main_physics);

      // 原幽靈保持原來的物理狀態
      physics_manipulator_->set_entity_transform(ghost_entity_id, ghost_transform);
      physics_manipulator_->force_set_entity_physics_state(ghost_entity_id, ghost_transform, ghost_physics);

      // 4. 更新映射關係（角色邏輯已互換）
      main_to_ghost_mapping_.erase(main_entity_id);
      ghost_to_main_mapping_.erase(ghost_entity_id);

      // 現在 ghost_entity_id 成為主控實體，main_entity_id 成為跟隨實體
      // 但各自保持原來的物理狀態，確保無感

      // 5. 發送角色互換事件給外部實現，並處理返回結果
      bool engine_processing_success = false;

      notify_event_handler([&engine_processing_success, main_entity_id, ghost_entity_id, source_face, target_face,
                            main_transform, ghost_transform](IPortalEventHandler *handler)
                           {
        
        // 發送角色互換事件，外部實現決定如何處理
        engine_processing_success = handler->on_entity_roles_swapped(
            main_entity_id,           // 原主體（現在是幽靈）
            ghost_entity_id,          // 原幽靈（現在是主體）
            ghost_entity_id,          // 新主體ID
            main_entity_id,           // 新幽靈ID
            INVALID_PORTAL_ID,        // 角色交換不特定於單個傳送門
            ghost_transform,          // 新主體的變換
            main_transform            // 新幽靈的變換
        ); });

      if (engine_processing_success)
      {
        std::cout << "TeleportManager: Seamless role swap successful - 外部實現已確認處理角色互換" << std::endl;
      }
      else
      {
        std::cout << "TeleportManager: Warning - 外部實現角色互換處理失敗，可能需要重試或錯誤處理" << std::endl;
        // 可以在這裡添加重試邏輯或錯誤處理
      }
    }
    else
    {
      std::cout << "TeleportManager: Role swap failed" << std::endl;
    }

    return success;
  }

  bool TeleportManager::calculate_ghost_state(EntityId main_entity_id,
                                              const Portal *source_portal,
                                              const Portal *target_portal,
                                              PortalFace source_face,
                                              PortalFace target_face,
                                              Transform &ghost_transform,
                                              PhysicsState &ghost_physics)
  {
    if (!physics_data_ || !source_portal || !target_portal)
    {
      return false;
    }

    // 獲取主體實體狀態
    Transform main_transform = physics_data_->get_entity_transform(main_entity_id);
    PhysicsState main_physics = physics_data_->get_entity_physics_state(main_entity_id);

    // 使用數學庫計算變換，考慮A/B面
    // 根據source_face和target_face調整傳送門平面
    PortalPlane adjusted_source_plane = source_portal->get_plane();
    PortalPlane adjusted_target_plane = target_portal->get_plane();

    // 根據面向調整法向量
    if (source_face == PortalFace::B)
    {
      adjusted_source_plane.normal = adjusted_source_plane.normal * -1.0f;
    }
    if (target_face == PortalFace::A)
    {
      adjusted_target_plane.normal = adjusted_target_plane.normal * -1.0f;
    }

    // 使用數學庫計算變換
    ghost_transform = Math::PortalMath::transform_through_portal(
        main_transform,
        adjusted_source_plane,
        adjusted_target_plane);

    ghost_physics = Math::PortalMath::transform_physics_state_through_portal(
        main_physics,
        adjusted_source_plane,
        adjusted_target_plane);

    return true;
  }

  void TeleportManager::sync_all_ghost_entities(float delta_time)
  {
    // 收集需要同步的实体
    std::vector<GhostEntitySnapshot> snapshots_to_sync;
    std::vector<EntityId> individual_sync_entities;

    for (auto &[entity_id, teleport_state] : active_teleports_)
    {
      if (teleport_state.ghost_entity_id != INVALID_ENTITY_ID &&
          should_sync_ghost_entity(entity_id, delta_time))
      {

        // 检查是否启用批量同步
        if (teleport_state.enable_batch_sync)
        {
          auto snapshot_it = ghost_snapshots_.find(entity_id);
          if (snapshot_it != ghost_snapshots_.end())
          {
            // 更新时间戳
            snapshot_it->second.timestamp = static_cast<uint64_t>(ghost_sync_timer_ * 1000.0f);
            snapshots_to_sync.push_back(snapshot_it->second);
          }
        }
        else
        {
          individual_sync_entities.push_back(entity_id);
        }
      }
    }

    // 执行批量同步
    if (!snapshots_to_sync.empty())
    {
      std::cout << "TeleportManager: Batch syncing " << snapshots_to_sync.size() << " ghost entities" << std::endl;
      physics_manipulator_->sync_ghost_entities(snapshots_to_sync);
    }

    // 执行个别同步（对于没有启用批量同步的实体）
    for (EntityId entity_id : individual_sync_entities)
    {
      // 获取传送门信息用于更新
      auto state_it = active_teleports_.find(entity_id);
      if (state_it != active_teleports_.end())
      {
        // 使用回調機制獲取 Portal 指針
        if (portal_getter_)
        {
          const Portal *source_portal = portal_getter_(state_it->second.source_portal);
          const Portal *target_portal = portal_getter_(state_it->second.target_portal);

          if (source_portal && target_portal)
          {
            update_ghost_entity(entity_id, source_portal, target_portal);
            std::cout << "TeleportManager: Individual sync for entity " << entity_id << std::endl;
          }
          else
          {
            std::cout << "TeleportManager: Warning - Could not get portal pointers for entity " << entity_id << std::endl;
          }
        }
        else
        {
          std::cout << "TeleportManager: Warning - Portal getter callback not set, skipping individual sync for entity " << entity_id << std::endl;
        }
      }
    }

    std::cout << "TeleportManager: Synced " << (snapshots_to_sync.size() + individual_sync_entities.size())
              << " ghost entities (" << snapshots_to_sync.size() << " batch, "
              << individual_sync_entities.size() << " individual)" << std::endl;
  }

  bool TeleportManager::should_sync_ghost_entity(EntityId entity_id, float delta_time)
  {
    auto state_it = active_teleports_.find(entity_id);
    if (state_it == active_teleports_.end())
    {
      return false;
    }

    const TeleportState &state = state_it->second;

    // 高优先级实体总是需要同步
    if (state.is_high_priority || state.requires_full_sync)
    {
      return true;
    }

    // 批量同步的实体使用组同步频率
    if (state.enable_batch_sync)
    {
      // 批量同步频率较低，减少个别检查
      float batch_sync_interval = 1.0f / (sync_frequency_ * 0.5f); // 批量同步频率是个别同步的一半
      return ghost_sync_timer_ >= batch_sync_interval;
    }

    // 个别同步使用标准频率控制
    float sync_interval = 1.0f / sync_frequency_;
    return ghost_sync_timer_ >= sync_interval;
  }

  void TeleportManager::notify_event_handler(const std::function<void(IPortalEventHandler *)> &callback)
  {
    if (event_handler_)
    {
      callback(event_handler_);
    }
  }

  TeleportState *TeleportManager::get_or_create_teleport_state(EntityId entity_id, PortalId portal_id)
  {
    auto it = active_teleports_.find(entity_id);
    if (it != active_teleports_.end())
    {
      return &it->second;
    }

    // 創建新的傳送狀態
    TeleportState new_state;
    new_state.entity_id = entity_id;
    new_state.source_portal = portal_id;
    new_state.target_portal = INVALID_PORTAL_ID; // 由外部設置
    new_state.crossing_state = PortalCrossingState::NOT_TOUCHING;
    new_state.is_teleporting = false;

    // V2 新增字段初始化
    new_state.seamless_mode = true;
    new_state.enable_realtime_sync = true;
    new_state.auto_triggered = true;
    new_state.original_entity_type = EntityType::MAIN;

    // A/B面默認配置
    new_state.source_face = PortalFace::A;
    new_state.target_face = PortalFace::B;
    new_state.active_source_face = PortalFace::A;
    new_state.active_target_face = PortalFace::B;

    // V3 邏輯實體支持初始化
    new_state.logical_entity_id = INVALID_LOGICAL_ENTITY_ID;
    new_state.use_logical_entity_physics = use_logical_entity_control_;
    new_state.merge_strategy = PhysicsStateMergeStrategy::MOST_RESTRICTIVE;

    active_teleports_[entity_id] = new_state;
    return &active_teleports_[entity_id];
  }

  // === 批量操作实现 ===

  void TeleportManager::set_entity_batch_sync(EntityId entity_id, bool enable_batch, uint32_t sync_group_id)
  {
    auto it = active_teleports_.find(entity_id);
    if (it != active_teleports_.end())
    {
      it->second.enable_batch_sync = enable_batch;
      it->second.sync_group_id = sync_group_id;

      std::cout << "TeleportManager: Set batch sync for entity " << entity_id
                << " to " << (enable_batch ? "enabled" : "disabled")
                << " (group: " << sync_group_id << ")" << std::endl;
    }
  }

  void TeleportManager::force_batch_sync_group(uint32_t sync_group_id)
  {
    std::vector<GhostEntitySnapshot> group_snapshots;

    // 收集指定组的所有快照
    for (const auto &[entity_id, teleport_state] : active_teleports_)
    {
      if (teleport_state.enable_batch_sync &&
          teleport_state.sync_group_id == sync_group_id &&
          teleport_state.ghost_entity_id != INVALID_ENTITY_ID)
      {

        auto snapshot_it = ghost_snapshots_.find(entity_id);
        if (snapshot_it != ghost_snapshots_.end())
        {
          // 标记为需要立即同步
          auto &snapshot = snapshot_it->second;
          snapshot.requires_immediate_sync = true;
          snapshot.timestamp = static_cast<uint64_t>(ghost_sync_timer_ * 1000.0f);
          group_snapshots.push_back(snapshot);
        }
      }
    }

    if (!group_snapshots.empty())
    {
      std::cout << "TeleportManager: Force batch sync group " << sync_group_id
                << " with " << group_snapshots.size() << " entities" << std::endl;
      physics_manipulator_->sync_ghost_entities(group_snapshots);
    }
  }

  TeleportManager::BatchSyncStats TeleportManager::get_batch_sync_stats() const
  {
    BatchSyncStats stats;
    stats.total_entities = active_teleports_.size();
    stats.batch_enabled_entities = 0;
    stats.pending_sync_count = 0;
    stats.last_batch_sync_time = ghost_sync_timer_;

    for (const auto &[entity_id, teleport_state] : active_teleports_)
    {
      if (teleport_state.enable_batch_sync)
      {
        stats.batch_enabled_entities++;
      }

      if (teleport_state.ghost_entity_id != INVALID_ENTITY_ID &&
          teleport_state.requires_full_sync)
      {
        stats.pending_sync_count++;
      }
    }

    return stats;
  }

  // === 邏輯實體控制方法實現 ===

  void TeleportManager::set_logical_entity_control_mode(bool enabled)
  {
    use_logical_entity_control_ = enabled;
    std::cout << "TeleportManager: Logical entity control mode "
              << (enabled ? "enabled" : "disabled") << std::endl;
  }

  void TeleportManager::set_logical_entity_merge_strategy(EntityId entity_id, PhysicsStateMergeStrategy strategy)
  {
    if (!use_logical_entity_control_ || !logical_entity_manager_)
    {
      std::cout << "TeleportManager: Logical entity control not enabled" << std::endl;
      return;
    }

    LogicalEntityId logical_id = logical_entity_manager_->get_logical_entity_by_physical_entity(entity_id);
    if (logical_id != INVALID_LOGICAL_ENTITY_ID)
    {
      logical_entity_manager_->set_merge_strategy(logical_id, strategy);
      std::cout << "TeleportManager: Set merge strategy for entity " << entity_id << std::endl;
    }
  }

  bool TeleportManager::is_logical_entity_constrained(EntityId entity_id) const
  {
    if (!use_logical_entity_control_ || !logical_entity_manager_)
    {
      return false;
    }

    LogicalEntityId logical_id = logical_entity_manager_->get_logical_entity_by_physical_entity(entity_id);
    return logical_entity_manager_->is_logical_entity_constrained(logical_id);
  }

  const PhysicsConstraintState *TeleportManager::get_logical_entity_constraint(EntityId entity_id) const
  {
    if (!use_logical_entity_control_ || !logical_entity_manager_)
    {
      return nullptr;
    }

    LogicalEntityId logical_id = logical_entity_manager_->get_logical_entity_by_physical_entity(entity_id);
    return logical_entity_manager_->get_constraint_state(logical_id);
  }

  void TeleportManager::force_update_logical_entity(EntityId entity_id)
  {
    if (!use_logical_entity_control_ || !logical_entity_manager_)
    {
      return;
    }

    LogicalEntityId logical_id = logical_entity_manager_->get_logical_entity_by_physical_entity(entity_id);
    if (logical_id != INVALID_LOGICAL_ENTITY_ID)
    {
      logical_entity_manager_->merge_physics_states(logical_id);
      logical_entity_manager_->sync_logical_to_entities(logical_id);
      std::cout << "TeleportManager: Force updated logical entity for entity " << entity_id << std::endl;
    }
  }

  // === 邏輯實體管理私有方法實現 ===

  bool TeleportManager::create_logical_entity_for_teleport(EntityId main_entity_id, EntityId ghost_entity_id)
  {
    if (!use_logical_entity_control_ || !logical_entity_manager_)
    {
      return false;
    }

    // 檢查是否已經為此實體創建了邏輯實體
    LogicalEntityId existing_logical_id = logical_entity_manager_->get_logical_entity_by_physical_entity(main_entity_id);
    if (existing_logical_id != INVALID_LOGICAL_ENTITY_ID)
    {
      std::cout << "TeleportManager: Logical entity already exists for entity " << main_entity_id << std::endl;
      return true;
    }

    // 根據傳送狀態決定合成策略
    PhysicsStateMergeStrategy strategy = PhysicsStateMergeStrategy::MOST_RESTRICTIVE;
    auto teleport_it = active_teleports_.find(main_entity_id);
    if (teleport_it != active_teleports_.end() && teleport_it->second.use_logical_entity_physics)
    {
      strategy = teleport_it->second.merge_strategy;
    }

    LogicalEntityId logical_id = logical_entity_manager_->create_logical_entity(
        main_entity_id, ghost_entity_id, strategy);

    if (logical_id != INVALID_LOGICAL_ENTITY_ID)
    {
      // 更新傳送狀態
      if (teleport_it != active_teleports_.end())
      {
        teleport_it->second.logical_entity_id = logical_id;
      }

      std::cout << "TeleportManager: Created logical entity " << logical_id
                << " for teleport (main: " << main_entity_id << ", ghost: " << ghost_entity_id << ")" << std::endl;
      return true;
    }

    return false;
  }

  void TeleportManager::destroy_logical_entity_for_teleport(EntityId main_entity_id)
  {
    if (!use_logical_entity_control_ || !logical_entity_manager_)
    {
      return;
    }

    LogicalEntityId logical_id = logical_entity_manager_->get_logical_entity_by_physical_entity(main_entity_id);
    if (logical_id != INVALID_LOGICAL_ENTITY_ID)
    {
      logical_entity_manager_->destroy_logical_entity(logical_id);

      // 清理傳送狀態中的邏輯實體引用
      auto teleport_it = active_teleports_.find(main_entity_id);
      if (teleport_it != active_teleports_.end())
      {
        teleport_it->second.logical_entity_id = INVALID_LOGICAL_ENTITY_ID;
      }

      std::cout << "TeleportManager: Destroyed logical entity for teleport (main: " << main_entity_id << ")" << std::endl;
    }
  }

  void TeleportManager::update_logical_entity_teleport_states(float delta_time)
  {
    if (!logical_entity_manager_)
    {
      return;
    }

    // 更新所有使用邏輯實體的傳送狀態
    for (auto &[entity_id, teleport_state] : active_teleports_)
    {
      if (teleport_state.use_logical_entity_physics &&
          teleport_state.logical_entity_id != INVALID_LOGICAL_ENTITY_ID)
      {

        // 檢查邏輯實體約束狀態
        bool is_constrained = logical_entity_manager_->is_logical_entity_constrained(teleport_state.logical_entity_id);

        // 根據約束狀態調整傳送行為
        if (is_constrained)
        {
          const PhysicsConstraintState *constraint = logical_entity_manager_->get_constraint_state(teleport_state.logical_entity_id);
          if (constraint)
          {
            handle_logical_entity_constraint(teleport_state.logical_entity_id, *constraint);
          }
        }
      }
    }
  }

  void TeleportManager::handle_logical_entity_constraint(LogicalEntityId logical_id, const PhysicsConstraintState &constraint)
  {
    // 找到對應的傳送狀態
    EntityId constrained_entity = INVALID_ENTITY_ID;
    for (const auto &[entity_id, teleport_state] : active_teleports_)
    {
      if (teleport_state.logical_entity_id == logical_id)
      {
        constrained_entity = entity_id;
        break;
      }
    }

    if (constrained_entity != INVALID_ENTITY_ID)
    {
      std::cout << "TeleportManager: Handling logical entity constraint for entity " << constrained_entity
                << " - blocking entity: " << constraint.blocking_entity
                << ", blocked: " << (constraint.is_blocked ? "yes" : "no") << std::endl;

      // 通知事件處理器
      notify_event_handler([logical_id, constraint](IPortalEventHandler *handler)
                           { handler->on_logical_entity_constrained(logical_id, constraint); });
    }
  }

  // === 實體鏈管理實現 ===

  void TeleportManager::handle_chain_node_intersect_portal(EntityId node_entity_id, PortalId portal_id,
                                                           Portal *portal, PortalId target_portal_id, Portal *target_portal)
  {
    if (!portal || !target_portal || !portal->is_active() || !target_portal->is_active())
    {
      std::cout << "TeleportManager: Invalid portals for chain node " << node_entity_id << std::endl;
      return;
    }

    // ** 關鍵修復1：正確確定這個節點屬於哪個原始實體鏈 **
    EntityId original_entity_id = INVALID_ENTITY_ID;
    auto node_mapping_it = chain_node_to_original_.find(node_entity_id);

    if (node_mapping_it != chain_node_to_original_.end())
    {
      // 這是一個已存在鏈的節點（幽靈實體或原始實體）
      original_entity_id = node_mapping_it->second;
      std::cout << "TeleportManager: Found existing chain node " << node_entity_id
                << " belonging to original entity " << original_entity_id << std::endl;
    }
    else
    {
      // 檢查是否存在以此實體ID為原始實體的鏈
      auto chain_it = entity_chains_.find(node_entity_id);
      if (chain_it != entity_chains_.end())
      {
        // 這是一個原始實體，已經有鏈狀態
        original_entity_id = node_entity_id;
        std::cout << "TeleportManager: Found original entity " << node_entity_id
                  << " with existing chain state" << std::endl;
      }
      else
      {
        // 這是全新的原始實體，第一次進入傳送門
        original_entity_id = node_entity_id;
        std::cout << "TeleportManager: New original entity " << node_entity_id
                  << " first time entering portal" << std::endl;
      }
    }

    std::cout << "TeleportManager: Chain node " << node_entity_id
              << " (original: " << original_entity_id << ") intersecting portal " << portal_id << std::endl;

    // 獲取或創建鏈狀態
    EntityChainState *chain_state = get_or_create_chain_state(original_entity_id);

    // 檢查是否需要擴展鏈（創建新的幽靈節點）
    PortalFace entry_face = PortalFace::A; // 默認，後續可根據物理檢測優化
    PortalFace exit_face = PortalFace::B;

    if (extend_entity_chain(original_entity_id, node_entity_id, portal_id, target_portal_id, entry_face, exit_face))
    {
      std::cout << "TeleportManager: Extended chain for entity " << original_entity_id
                << " through portal " << portal_id << std::endl;

      // 更新鏈的裁切狀態
      update_chain_clipping_states(*chain_state);

      // **新增：使用多段裁切系統**
      if (multi_segment_clipping_manager_)
      {
        // 使用估算的相機位置（可通過回調優化）
        Vector3 camera_position = estimate_camera_position(*chain_state);
        multi_segment_clipping_manager_->setup_chain_clipping(*chain_state, camera_position);
      }

      // 如果啟用邏輯實體控制，同步到邏輯實體
      if (use_logical_entity_control_)
      {
        sync_chain_to_logical_entity(*chain_state);
      }
    }
  }

  void TeleportManager::handle_chain_node_center_crossed(EntityId node_entity_id, PortalId portal_id,
                                                         PortalFace crossed_face, Portal *portal,
                                                         PortalId target_portal_id, PortalFace target_face, Portal *target_portal)
  {
    // ** 关键修复2：正确处理原始实体和幽灵节点的映射查找 **
    EntityId original_entity_id = INVALID_ENTITY_ID;

    // 首先检查是否在节点映射中（幽灵节点）
    auto node_mapping_it = chain_node_to_original_.find(node_entity_id);
    if (node_mapping_it != chain_node_to_original_.end())
    {
      // 这是一个幽灵节点
      original_entity_id = node_mapping_it->second;
      std::cout << "TeleportManager: Found ghost node " << node_entity_id
                << " maps to original " << original_entity_id << std::endl;
    }
    else
    {
      // 检查是否是原始实体本身
      auto chain_it = entity_chains_.find(node_entity_id);
      if (chain_it != entity_chains_.end())
      {
        // 这是原始实体
        original_entity_id = node_entity_id;
        std::cout << "TeleportManager: Node " << node_entity_id << " is original entity" << std::endl;
      }
      else
      {
        std::cout << "TeleportManager: Chain node " << node_entity_id
                  << " not found in mappings or chains" << std::endl;
        return;
      }
    }

    auto chain_it = entity_chains_.find(original_entity_id);
    if (chain_it == entity_chains_.end())
    {
      std::cout << "TeleportManager: Chain state not found for entity " << original_entity_id << std::endl;
      return;
    }

    EntityChainState &chain_state = chain_it->second;

    std::cout << "TeleportManager: Chain node " << node_entity_id
              << " center crossed portal " << portal_id
              << " (original entity: " << original_entity_id << ")" << std::endl;

    // ** 關鍵修復5：正確的主體位置遷移邏輯 **
    // 當實體的質心穿越傳送門時，主體位置應該向前移動到鏈中的下一個位置

    // 找到這個節點在鏈中的當前位置
    int node_position = -1;
    for (size_t i = 0; i < chain_state.chain.size(); ++i)
    {
      if (chain_state.chain[i].entity_id == node_entity_id)
      {
        node_position = static_cast<int>(i);
        break;
      }
    }

    if (node_position == -1)
    {
      std::cout << "TeleportManager: Node " << node_entity_id << " not found in chain" << std::endl;
      return;
    }

    // 如果這個節點是當前的主體位置，且還有下一個位置，則向前遷移
    if (node_position == chain_state.main_position &&
        node_position + 1 < static_cast<int>(chain_state.chain.size()))
    {

      int new_main_position = node_position + 1;

      std::cout << "TeleportManager: Migrating main position from " << chain_state.main_position
                << " to " << new_main_position << " for chain " << original_entity_id << std::endl;

      if (shift_main_entity_position(original_entity_id, new_main_position))
      {
        std::cout << "TeleportManager: Successfully shifted main entity position to "
                  << new_main_position << " for chain " << original_entity_id << std::endl;
      }
      else
      {
        std::cout << "TeleportManager: Failed to shift main entity position for chain "
                  << original_entity_id << std::endl;
      }
    }
    else
    {
      std::cout << "TeleportManager: No main position migration needed for node "
                << node_entity_id << " (position " << node_position
                << ", main position " << chain_state.main_position
                << ", chain length " << chain_state.chain.size() << ")" << std::endl;
    }
  }

  void TeleportManager::handle_chain_node_fully_passed(EntityId node_entity_id, PortalId portal_id)
  {
    std::cout << "TeleportManager: Chain node " << node_entity_id
              << " fully passed portal " << portal_id << std::endl;

    // 這裡可以添加完全穿越的特殊處理邏輯
    // 例如更新物理狀態、觸發特殊效果等
  }

  void TeleportManager::handle_chain_node_exit_portal(EntityId node_entity_id, PortalId portal_id)
  {
    // ** 關鍵修復4：正確處理鏈收縮 **
    EntityId original_entity_id = INVALID_ENTITY_ID;

    // 查找節點對應的原始實體
    auto node_mapping_it = chain_node_to_original_.find(node_entity_id);
    if (node_mapping_it != chain_node_to_original_.end())
    {
      original_entity_id = node_mapping_it->second;
    }
    else
    {
      // 如果不在映射中，檢查是否是原始實體本身
      auto chain_it = entity_chains_.find(node_entity_id);
      if (chain_it != entity_chains_.end())
      {
        original_entity_id = node_entity_id;
      }
      else
      {
        std::cout << "TeleportManager: Chain node " << node_entity_id
                  << " not found in any chain mapping" << std::endl;
        return;
      }
    }

    auto chain_it = entity_chains_.find(original_entity_id);
    if (chain_it == entity_chains_.end())
    {
      std::cout << "TeleportManager: Chain state not found for original entity "
                << original_entity_id << std::endl;
      return;
    }

    EntityChainState &chain_state = chain_it->second;

    std::cout << "TeleportManager: Chain node " << node_entity_id
              << " exited portal " << portal_id << std::endl;

    // ** 核心修復：正確的鏈收縮邏輯 **
    // 當實體完全退出傳送門時，移除鏈的第一個節點（最早的位置）
    // 這是因為：實體已經完全穿過該傳送門，最早的位置不再需要

    if (chain_state.chain.size() > 1)
    {
      // 總是移除鏈的第一個節點（鏈[0]）
      EntityChainNode &first_node = chain_state.chain[0];
      EntityId node_to_remove = first_node.entity_id;

      std::cout << "TeleportManager: Node " << node_entity_id
                << " exited portal " << portal_id
                << ", removing first chain node " << node_to_remove
                << " (chain length: " << chain_state.chain.size() << " -> "
                << (chain_state.chain.size() - 1) << ")" << std::endl;

      // 處理第一個節點的清理
      if (first_node.entity_type == EntityType::GHOST)
      {
        // 保存節點信息用於事件觸發
        PortalId node_entry_portal = first_node.entry_portal;
        
        // 標準幽靈實體清理
        destroy_chain_node_entity(node_to_remove);

        // **修復: 觸發幽靈實體銷毀事件**
        notify_event_handler([original_entity_id, node_to_remove, node_entry_portal](IPortalEventHandler *handler)
                             { handler->on_ghost_entity_destroyed(original_entity_id, node_to_remove, node_entry_portal); });

        std::cout << "TeleportManager: Destroyed ghost node entity " << node_to_remove
                  << " for original entity " << original_entity_id << std::endl;

        // 從映射中移除
        auto mapping_it = chain_node_to_original_.find(node_to_remove);
        if (mapping_it != chain_node_to_original_.end())
        {
          chain_node_to_original_.erase(mapping_it);
        }
      }
      else if (node_to_remove == original_entity_id)
      {
        // ** 修復：原始實體的特殊處理 **
        // 當原始實體被移出鏈時，它不是幽靈但也需要清理
        // 因為它已經不再是活躍的主實體
        std::cout << "TeleportManager: Original entity " << node_to_remove
                  << " removed from chain, scheduling cleanup" << std::endl;

        // 從映射中移除
        auto mapping_it = chain_node_to_original_.find(node_to_remove);
        if (mapping_it != chain_node_to_original_.end())
        {
          chain_node_to_original_.erase(mapping_it);
        }

        // 注意：我們不立即銷毀原始實體，而是在後續的清理過程中處理
      }

      // 從鏈中移除第一個節點
      chain_state.chain.erase(chain_state.chain.begin());

      // 調整主體位置（向前移動，因為所有索引都減1）
      chain_state.main_position = std::max(0, chain_state.main_position - 1);

      std::cout << "TeleportManager: Adjusted main position to " << chain_state.main_position
                << " after removing first node" << std::endl;

      chain_state.chain_version++;

      // 更新所有節點的chain_position（重新編號）
      for (size_t i = 0; i < chain_state.chain.size(); ++i)
      {
        chain_state.chain[i].chain_position = static_cast<int>(i);
      }

      // 更新裁切狀態
      update_chain_clipping_states(chain_state);

      // **新增：更新多段裁切**
      if (multi_segment_clipping_manager_)
      {
        multi_segment_clipping_manager_->update_chain_clipping(chain_state);
      }

      std::cout << "TeleportManager: Chain " << original_entity_id
                << " shrunk to length " << chain_state.chain.size()
                << ", main position: " << chain_state.main_position << std::endl;

      // ** 修復：檢查被移除的節點是否需要完全清理 **
      if (node_to_remove == original_entity_id)
      {
        // 原始實體已從鏈中移除，現在可以安全地清理它
        cleanup_entity(node_to_remove);
        std::cout << "TeleportManager: Completely cleaned up entity " << node_to_remove << std::endl;
      }
      else if (first_node.entity_type == EntityType::GHOST)
      {
        // 幽靈實體的物理清理在之前的 destroy_chain_node_entity 中已經完成
        // 這裡只需要執行通用清理
        cleanup_entity(node_to_remove);
        std::cout << "TeleportManager: Completely cleaned up entity " << node_to_remove << std::endl;
      }

      // ** 關鍵修復：當鏈收縮到長度1時，完成傳送流程 **
      if (chain_state.chain.size() == 1)
      {
        EntityId final_entity = chain_state.chain[0].entity_id;

        std::cout << "TeleportManager: Chain teleportation completed. Final entity: "
                  << final_entity << std::endl;

        // 調試：顯示最終實體的類型
        std::cout << "TeleportManager: Final entity type: "
                  << (chain_state.chain[0].entity_type == EntityType::MAIN ? "MAIN" : "GHOST") << std::endl;

        // 如果最終實體是幽靈，需要將其從幽靈列表中移除並轉正
        if (chain_state.chain[0].entity_type == EntityType::GHOST)
        {
          chain_state.chain[0].entity_type = EntityType::MAIN;

          // 通知物理引擎實體角色轉正（從幽靈變為正常實體）
          if (physics_manipulator_)
          {
            physics_manipulator_->set_entity_functional_state(final_entity, true);
            std::cout << "TeleportManager: Converted final ghost entity " << final_entity
                      << " to main entity" << std::endl;
          }
        }

        // 清理傳送狀態
        chain_state.is_actively_teleporting = false;

        // 觸發傳送完成事件
        if (event_handler_)
        {
          // 這裡需要找到最初和最終的傳送門組合
          // 簡化處理：使用鏈中的資訊
          PortalId initial_portal = INVALID_PORTAL_ID;
          PortalId final_portal = INVALID_PORTAL_ID;

          // 找到鏈中記錄的傳送門信息
          for (const auto &node : chain_state.chain)
          {
            if (node.entry_portal != INVALID_PORTAL_ID)
            {
              if (initial_portal == INVALID_PORTAL_ID)
              {
                initial_portal = node.entry_portal;
              }
              final_portal = node.exit_portal;
            }
          }

          event_handler_->on_entity_teleport_complete(original_entity_id, initial_portal, final_portal);
        }
      }
    }
    else
    {
      std::cout << "TeleportManager: Chain " << original_entity_id
                << " has only one node, not shrinking" << std::endl;
    }
  }

  const EntityChainState *TeleportManager::get_entity_chain_state(EntityId original_entity_id) const
  {
    auto it = entity_chains_.find(original_entity_id);
    return (it != entity_chains_.end()) ? &it->second : nullptr;
  }

  EntityId TeleportManager::get_chain_main_entity(EntityId original_entity_id) const
  {
    const EntityChainState *chain_state = get_entity_chain_state(original_entity_id);
    if (chain_state && chain_state->main_position >= 0 &&
        chain_state->main_position < static_cast<int>(chain_state->chain.size()))
    {
      return chain_state->chain[chain_state->main_position].entity_id;
    }
    return INVALID_ENTITY_ID;
  }

  size_t TeleportManager::get_chain_length(EntityId original_entity_id) const
  {
    const EntityChainState *chain_state = get_entity_chain_state(original_entity_id);
    return chain_state ? chain_state->chain.size() : 0;
  }

  // === 實體鏈核心邏輯實現 ===

  bool TeleportManager::extend_entity_chain(EntityId original_entity_id, EntityId extending_node_id,
                                            PortalId entry_portal, PortalId exit_portal,
                                            PortalFace entry_face, PortalFace exit_face)
  {
    EntityChainState *chain_state = get_or_create_chain_state(original_entity_id);

    std::cout << "TeleportManager: Extending chain for original " << original_entity_id
              << ", extending node " << extending_node_id
              << ", entry portal " << entry_portal
              << ", exit portal " << exit_portal << std::endl;

    // ** 关键修复1：正确处理链扩展逻辑 **
    // 无论extending_node_id是原始实体还是已存在的链节点，
    // 只要它进入新的传送门，都应该在出口创建新的幽灵实体

    // 检查是否已经有通过这个出口传送门的节点
    bool already_has_exit_node = false;
    for (const auto &node : chain_state->chain)
    {
      if (node.exit_portal == exit_portal)
      {
        already_has_exit_node = true;
        std::cout << "TeleportManager: Exit portal " << exit_portal << " already has a node" << std::endl;
        break;
      }
    }

    if (already_has_exit_node)
    {
      // 这个出口已经有节点了，不需要重复创建
      return false;
    }

    // 创建新的鏈节点描述符
    ChainNodeCreateDescriptor descriptor;
    descriptor.source_entity_id = extending_node_id; // 基于进入传送门的实体
    descriptor.through_portal = entry_portal;
    descriptor.entry_face = entry_face;
    descriptor.exit_face = exit_face;
    descriptor.full_functionality = true;

    // 计算新节点的变换和物理状态
    Transform node_transform;
    PhysicsState node_physics;
    std::cout << "DEBUG: Getting portal via portal_getter_..." << std::endl;
    Portal *through_portal = portal_getter_ ? portal_getter_(entry_portal) : nullptr;
    std::cout << "DEBUG: through_portal: " << (through_portal ? "valid" : "null") << std::endl;

    std::cout << "DEBUG: Calling calculate_chain_node_state..." << std::endl;
    if (!calculate_chain_node_state(*chain_state, static_cast<int>(chain_state->chain.size()),
                                    through_portal, entry_face, exit_face,
                                    node_transform, node_physics))
    {
      std::cerr << "TeleportManager: Failed to calculate chain node state" << std::endl;
      return false;
    }
    std::cout << "DEBUG: calculate_chain_node_state succeeded" << std::endl;

    descriptor.target_transform = node_transform;
    descriptor.target_physics = node_physics;

    // 創建新的鏈節點實體
    EntityId new_node_id = create_chain_node_entity(descriptor);
    if (new_node_id == INVALID_ENTITY_ID)
    {
      std::cerr << "TeleportManager: Failed to create chain node entity" << std::endl;
      return false;
    }

    // 添加到鏈中
    EntityChainNode new_node;
    new_node.entity_id = new_node_id;
    new_node.entity_type = EntityType::GHOST;
    new_node.entry_portal = entry_portal;
    new_node.exit_portal = exit_portal;
    new_node.chain_position = static_cast<int>(chain_state->chain.size());
    new_node.transform = node_transform;
    new_node.physics_state = node_physics;
    new_node.entry_face = entry_face;
    new_node.exit_face = exit_face;

    chain_state->chain.push_back(new_node);
    chain_state->chain_version++;
    chain_state->last_update_timestamp = static_cast<uint64_t>(ghost_sync_timer_ * 1000.0f);

    // 建立映射
    chain_node_to_original_[new_node_id] = original_entity_id;

    // **修復: 觸發幽靈實體創建事件**
    notify_event_handler([original_entity_id, new_node_id, entry_portal](IPortalEventHandler *handler)
                         { handler->on_ghost_entity_created(original_entity_id, new_node_id, entry_portal); });

    std::cout << "TeleportManager: Created chain node entity " << new_node_id
              << " for original entity " << original_entity_id
              << " through portal " << entry_portal << std::endl;

    return true;
  }

  void TeleportManager::shrink_entity_chain(EntityId original_entity_id, EntityId removing_node_id)
  {
    auto chain_it = entity_chains_.find(original_entity_id);
    if (chain_it == entity_chains_.end())
    {
      return;
    }

    EntityChainState &chain_state = chain_it->second;

    // 找到要移除的節點
    for (auto it = chain_state.chain.begin(); it != chain_state.chain.end(); ++it)
    {
      if (it->entity_id == removing_node_id)
      {
        // 保存節點信息用於事件觸發
        PortalId node_entry_portal = it->entry_portal;
        
        // 銷毀實體
        destroy_chain_node_entity(removing_node_id);

        // **修復: 觸發幽靈實體銷毀事件**
        notify_event_handler([original_entity_id, removing_node_id, node_entry_portal](IPortalEventHandler *handler)
                             { handler->on_ghost_entity_destroyed(original_entity_id, removing_node_id, node_entry_portal); });

        std::cout << "TeleportManager: Destroyed chain node entity " << removing_node_id
                  << " for original entity " << original_entity_id << std::endl;

        // 從鏈中移除
        chain_state.chain.erase(it);
        chain_state.chain_version++;

        // 更新鏈位置索引
        for (size_t i = 0; i < chain_state.chain.size(); ++i)
        {
          chain_state.chain[i].chain_position = static_cast<int>(i);
        }

        // 調整主體位置索引
        if (chain_state.main_position >= static_cast<int>(chain_state.chain.size()))
        {
          chain_state.main_position = std::max(0, static_cast<int>(chain_state.chain.size()) - 1);
        }

        // 清理映射
        chain_node_to_original_.erase(removing_node_id);

        break;
      }
    }

    // 如果鏈為空，清理整個鏈狀態
    if (chain_state.chain.empty())
    {
      entity_chains_.erase(chain_it);
    }
  }

  bool TeleportManager::shift_main_entity_position(EntityId original_entity_id, int new_main_position)
  {
    auto chain_it = entity_chains_.find(original_entity_id);
    if (chain_it == entity_chains_.end())
    {
      return false;
    }

    EntityChainState &chain_state = chain_it->second;

    if (new_main_position < 0 || new_main_position >= static_cast<int>(chain_state.chain.size()))
    {
      return false;
    }

    // 更新主體位置
    int old_main_position = chain_state.main_position;
    chain_state.main_position = new_main_position;

    // 更新實體類型
    if (old_main_position >= 0 && old_main_position < static_cast<int>(chain_state.chain.size()))
    {
      chain_state.chain[old_main_position].entity_type = EntityType::GHOST;
    }

    // 如果新主實體之前是幽靈，需要轉為正常實體
    bool was_ghost = chain_state.chain[new_main_position].entity_type == EntityType::GHOST;
    chain_state.chain[new_main_position].entity_type = EntityType::MAIN;

    // 通知物理引擎實體角色變更
    if (was_ghost && physics_manipulator_)
    {
      physics_manipulator_->set_entity_functional_state(
          chain_state.chain[new_main_position].entity_id, true);
      std::cout << "TeleportManager: Converted ghost entity "
                << chain_state.chain[new_main_position].entity_id
                << " to main entity during position shift" << std::endl;
    }

    // **物理引擎無關的角色交換邏輯**
    EntityId old_main_entity = (old_main_position >= 0 && old_main_position < static_cast<int>(chain_state.chain.size())) 
                               ? chain_state.chain[old_main_position].entity_id : INVALID_ENTITY_ID;
    EntityId new_main_entity = chain_state.chain[new_main_position].entity_id;
    
    if (old_main_entity != INVALID_ENTITY_ID && old_main_entity != new_main_entity)
    {
      // 通知物理引擎進行角色交換，具體實現由引擎決定
      if (physics_manipulator_)
      {
        physics_manipulator_->swap_entity_roles_with_faces(
            old_main_entity, new_main_entity, PortalFace::A, PortalFace::B);
      }
      
      // 觸發角色交換事件供外部實現處理
      Transform old_main_transform = physics_data_ ? physics_data_->get_entity_transform(old_main_entity) : Transform();
      Transform new_main_transform = physics_data_ ? physics_data_->get_entity_transform(new_main_entity) : Transform();
      
      notify_event_handler([old_main_entity, new_main_entity, old_main_transform, new_main_transform](IPortalEventHandler *handler)
                           { handler->on_entity_roles_swapped(
                                 old_main_entity, new_main_entity, // old main, old ghost
                                 new_main_entity, old_main_entity, // new main, new ghost
                                 INVALID_PORTAL_ID, // 鏈式交換不特定於某個傳送門
                                 new_main_transform, old_main_transform); });
      
      std::cout << "TeleportManager: Triggered role swap event - Old main: " << old_main_entity
                << " -> New main: " << new_main_entity << std::endl;
    }

    // 通知邏輯實體管理器主控實體變更
    if (use_logical_entity_control_ && logical_entity_manager_ &&
        chain_state.logical_entity_id != INVALID_LOGICAL_ENTITY_ID)
    {
      logical_entity_manager_->set_primary_controlled_entity(
          chain_state.logical_entity_id,
          chain_state.chain[new_main_position].entity_id);
    }

    chain_state.chain_version++;
    return true;
  }

  EntityChainState *TeleportManager::get_or_create_chain_state(EntityId original_entity_id)
  {
    auto it = entity_chains_.find(original_entity_id);
    if (it != entity_chains_.end())
    {
      return &it->second;
    }

    std::cout << "TeleportManager: Creating new chain state for original entity " << original_entity_id << std::endl;

    // 創建新的鏈狀態
    EntityChainState new_chain_state;
    new_chain_state.original_entity_id = original_entity_id;
    new_chain_state.main_position = 0;
    new_chain_state.is_actively_teleporting = true;

    // 添加原始實體作為鏈的第一個節點
    EntityChainNode original_node;
    original_node.entity_id = original_entity_id;
    original_node.entity_type = EntityType::MAIN;
    original_node.chain_position = 0;

    if (physics_data_)
    {
      original_node.transform = physics_data_->get_entity_transform(original_entity_id);
      original_node.physics_state = physics_data_->get_entity_physics_state(original_entity_id);
    }

    new_chain_state.chain.push_back(original_node);

    // ** 关键修复3：为原始实体建立自映射 **
    // 这样在处理质心穿越时可以正确找到原始实体对应的链
    chain_node_to_original_[original_entity_id] = original_entity_id;

    // 創建邏輯實體控制（如果啟用）
    if (use_logical_entity_control_ && logical_entity_manager_)
    {
      LogicalEntityId logical_id = logical_entity_manager_->create_multi_entity_logical_control(
          {original_entity_id});
      new_chain_state.logical_entity_id = logical_id;
    }

    entity_chains_[original_entity_id] = new_chain_state;
    std::cout << "TeleportManager: Created chain state with 1 node (original entity)" << std::endl;
    return &entity_chains_[original_entity_id];
  }

  bool TeleportManager::calculate_chain_node_state(const EntityChainState &chain_state, int node_position,
                                                   const Portal *through_portal, PortalFace entry_face, PortalFace exit_face,
                                                   Transform &node_transform, PhysicsState &node_physics)
  {
    std::cout << "DEBUG: calculate_chain_node_state called - through_portal: " 
              << (through_portal ? "valid" : "null") 
              << ", node_position: " << node_position << std::endl;
              
    if (!through_portal || node_position < 0)
    {
      std::cout << "DEBUG: Early return - invalid portal or position" << std::endl;
      return false;
    }

    std::cout << "DEBUG: chain_state.main_position: " << chain_state.main_position 
              << ", chain.size(): " << chain_state.chain.size() << std::endl;

    // 獲取參考節點（通常是主體節點）
    if (chain_state.main_position >= 0 &&
        chain_state.main_position < static_cast<int>(chain_state.chain.size()))
    {
      std::cout << "DEBUG: Getting reference node at position " << chain_state.main_position << std::endl;

      const EntityChainNode &reference_node = chain_state.chain[chain_state.main_position];
      std::cout << "DEBUG: Reference node obtained, entity_id: " << reference_node.entity_id << std::endl;

      // 安全檢查 portal_getter_ 回調
      if (!portal_getter_)
      {
        std::cout << "DEBUG: portal_getter_ is null!" << std::endl;
        return false;
      }

      std::cout << "DEBUG: Getting linked portal ID..." << std::endl;
      PortalId linked_portal_id = through_portal->get_linked_portal();
      std::cout << "DEBUG: Linked portal ID: " << linked_portal_id << std::endl;

      std::cout << "DEBUG: Calling portal_getter_..." << std::endl;
      Portal *target_portal = portal_getter_(linked_portal_id);
      std::cout << "DEBUG: target_portal: " << (target_portal ? "valid" : "null") << std::endl;
      
      if (target_portal)
      {
        std::cout << "DEBUG: Calculating transform through portal..." << std::endl;
        node_transform = Math::PortalMath::transform_through_portal(
            reference_node.transform,
            through_portal->get_plane(),
            target_portal->get_plane(),
            entry_face,
            exit_face);

        std::cout << "DEBUG: Calculating physics state through portal..." << std::endl;
        node_physics = Math::PortalMath::transform_physics_state_through_portal(
            reference_node.physics_state,
            through_portal->get_plane(),
            target_portal->get_plane(),
            entry_face,
            exit_face);

        std::cout << "DEBUG: calculate_chain_node_state completed successfully" << std::endl;
        return true;
      }
      else
      {
        std::cout << "DEBUG: target_portal is null after portal_getter_ call" << std::endl;
      }
    }
    else
    {
      std::cout << "DEBUG: Invalid main_position or empty chain" << std::endl;
    }

    std::cout << "DEBUG: calculate_chain_node_state returning false" << std::endl;
    return false;
  }

  void TeleportManager::update_chain_clipping_states(EntityChainState &chain_state)
  {
    // **修改：保持舊版本的簡單裁切作為備用**
    // 多段裁切系統已經在 setup_chain_clipping 中處理了更高級的裁切
    // 這裡保持簡單的單平面裁切作為向後兼容
    
    std::cout << "TeleportManager: Updating chain clipping states for " << chain_state.chain.size() << " nodes (legacy method)" << std::endl;

    // 為鏈中每個需要裁切的節點設置裁切平面
    for (auto &node : chain_state.chain)
    {
      if (node.entry_portal != INVALID_PORTAL_ID && portal_getter_)
      {
        Portal *entry_portal = portal_getter_(node.entry_portal);
        if (entry_portal)
        {
          // 計算裁切平面
          node.requires_clipping = true;
          node.clipping_plane = ClippingPlane::from_point_and_normal(
              entry_portal->get_plane().center,
              entry_portal->get_plane().get_face_normal(node.entry_face));

          // 僅在多段裁切不可用時才使用傳統方法
          if (!multi_segment_clipping_manager_ || !multi_segment_clipping_manager_->requires_multi_segment_clipping(chain_state.original_entity_id))
          {
            if (physics_manipulator_)
            {
              physics_manipulator_->set_entity_clipping_plane(node.entity_id, node.clipping_plane);
            }
          }
          else
          {
            std::cout << "TeleportManager: Skipping single-plane clipping (multi-segment clipping active)" << std::endl;
          }
        }
      }
      else
      {
        // 禁用裁切
        node.requires_clipping = false;
        
        // 僅在多段裁切不活跃時才禁用
        if (!multi_segment_clipping_manager_ || !multi_segment_clipping_manager_->requires_multi_segment_clipping(chain_state.original_entity_id))
        {
          if (physics_manipulator_)
          {
            physics_manipulator_->disable_entity_clipping(node.entity_id);
          }
        }
      }
    }
  }

  EntityId TeleportManager::create_chain_node_entity(const ChainNodeCreateDescriptor &descriptor)
  {
    if (!physics_manipulator_)
    {
      return INVALID_ENTITY_ID;
    }

    // 使用新的鏈節點創建接口
    return physics_manipulator_->create_chain_node_entity(descriptor);
  }

  void TeleportManager::destroy_chain_node_entity(EntityId node_entity_id)
  {
    if (physics_manipulator_)
    {
      physics_manipulator_->destroy_chain_node_entity(node_entity_id);
    }
  }

  bool TeleportManager::should_migrate_main_position(const EntityChainState &chain_state,
                                                     EntityId node_entity_id, PortalId crossed_portal)
  {
    // 檢查質心是否應該遷移到這個節點
    // 如果穿越傳送門的是當前主體節點，則質心應該遷移到對面的節點

    if (chain_state.main_position >= 0 &&
        chain_state.main_position < static_cast<int>(chain_state.chain.size()))
    {

      const EntityChainNode &current_main = chain_state.chain[chain_state.main_position];

      // 如果穿越的是當前主體，且有對應的出口節點
      if (current_main.entity_id == node_entity_id)
      {
        // 查找通過同一個傳送門的出口節點
        for (size_t i = 0; i < chain_state.chain.size(); ++i)
        {
          const EntityChainNode &node = chain_state.chain[i];
          if (node.entry_portal == crossed_portal &&
              static_cast<int>(i) != chain_state.main_position)
          {
            return true; // 應該遷移到位置 i
          }
        }
      }
    }

    return false;
  }

  void TeleportManager::sync_chain_to_logical_entity(EntityChainState &chain_state)
  {
    if (!use_logical_entity_control_ || !logical_entity_manager_ ||
        chain_state.logical_entity_id == INVALID_LOGICAL_ENTITY_ID)
    {
      return;
    }

    std::cout << "TeleportManager: Syncing entity chain " << chain_state.original_entity_id
              << " to logical entity " << chain_state.logical_entity_id
              << " with " << chain_state.chain.size() << " nodes" << std::endl;

    // 獲取邏輯實體的當前狀態
    const LogicalEntityState *logical_state = logical_entity_manager_->get_logical_entity_state(chain_state.logical_entity_id);
    if (!logical_state)
    {
      std::cerr << "TeleportManager: Logical entity state not found for " << chain_state.logical_entity_id << std::endl;
      return;
    }

    // 收集當前鏈中的所有實體ID和權重
    std::vector<EntityId> current_chain_entities;
    std::vector<float> current_weights;

    for (const auto &node : chain_state.chain)
    {
      current_chain_entities.push_back(node.entity_id);

      // 根據節點在鏈中的位置和類型設定權重
      float weight = 1.0f;
      if (node.entity_type == EntityType::MAIN)
      {
        weight = 1.5f; // 主體節點權重最高
      }
      else if (node.chain_position == 0)
      {
        weight = 1.2f; // 質心位置權重較高
      }
      else
      {
        weight = 0.8f - (float(node.chain_position) * 0.1f); // 距離質心越遠權重越低
        weight = std::max(weight, 0.3f);                     // 最小權重
      }
      current_weights.push_back(weight);
    }

    // 檢查邏輯實體的受控實體列表是否需要更新
    bool need_update_entities = (logical_state->controlled_entities.size() != current_chain_entities.size());

    if (!need_update_entities)
    {
      // 檢查實體列表是否發生變化
      for (size_t i = 0; i < current_chain_entities.size(); ++i)
      {
        if (i >= logical_state->controlled_entities.size() ||
            logical_state->controlled_entities[i] != current_chain_entities[i])
        {
          need_update_entities = true;
          break;
        }
      }
    }

    if (need_update_entities)
    {
      std::cout << "TeleportManager: Updating logical entity controlled entities" << std::endl;

      // 移除不再在鏈中的實體
      for (EntityId logical_entity_id : logical_state->controlled_entities)
      {
        bool still_in_chain = false;
        for (EntityId chain_entity_id : current_chain_entities)
        {
          if (logical_entity_id == chain_entity_id)
          {
            still_in_chain = true;
            break;
          }
        }

        if (!still_in_chain)
        {
          logical_entity_manager_->remove_controlled_entity(chain_state.logical_entity_id, logical_entity_id);
          std::cout << "TeleportManager: Removed entity " << logical_entity_id << " from logical control" << std::endl;
        }
      }

      // 添加新的實體到邏輯控制
      for (size_t i = 0; i < current_chain_entities.size(); ++i)
      {
        EntityId entity_id = current_chain_entities[i];
        bool already_controlled = false;

        for (EntityId controlled_id : logical_state->controlled_entities)
        {
          if (controlled_id == entity_id)
          {
            already_controlled = true;
            break;
          }
        }

        if (!already_controlled)
        {
          logical_entity_manager_->add_controlled_entity(chain_state.logical_entity_id, entity_id, current_weights[i]);
          std::cout << "TeleportManager: Added entity " << entity_id << " to logical control with weight " << current_weights[i] << std::endl;
        }
      }
    }

    // 設定主控實體（質心位置對應的實體）
    if (chain_state.main_position >= 0 && chain_state.main_position < chain_state.chain.size())
    {
      EntityId primary_entity = chain_state.chain[chain_state.main_position].entity_id;
      logical_entity_manager_->set_primary_controlled_entity(chain_state.logical_entity_id, primary_entity);
    }

    // 根據鏈的狀態選擇物理合成策略
    PhysicsStateMergeStrategy strategy = PhysicsStateMergeStrategy::MOST_RESTRICTIVE;

    if (chain_state.is_actively_teleporting)
    {
      // 傳送過程中使用力求和策略，保證連續性
      strategy = PhysicsStateMergeStrategy::FORCE_SUMMATION;
    }
    else if (chain_state.chain.size() > 3)
    {
      // 長鏈使用物理模擬
      strategy = PhysicsStateMergeStrategy::PHYSICS_SIMULATION;
    }
    else
    {
      // 短鏈使用加權平均
      strategy = PhysicsStateMergeStrategy::WEIGHTED_AVERAGE;
    }

    logical_entity_manager_->set_merge_strategy(chain_state.logical_entity_id, strategy);

    // 強制更新邏輯實體的物理狀態合成
    logical_entity_manager_->force_update_logical_entity(chain_state.logical_entity_id);

    // 更新鏈狀態的時間戳
    chain_state.chain_version++;
    chain_state.last_update_timestamp = get_current_time_ms();

    std::cout << "TeleportManager: Successfully synced entity chain to logical entity. Strategy: "
              << static_cast<int>(strategy) << ", Chain version: " << chain_state.chain_version << std::endl;
  }

  uint64_t TeleportManager::get_current_time_ms() const
  {
    // 簡化實現：使用系統時間戳
    // 在實際應用中，應該使用高精度時鐘
    auto now = std::chrono::steady_clock::now();
    auto duration = now.time_since_epoch();
    return std::chrono::duration_cast<std::chrono::milliseconds>(duration).count();
  }

  // === 多段裁切方法实现（新增） ===

  void TeleportManager::set_entity_multi_segment_clipping(EntityId entity_id, bool enabled)
  {
    if (!multi_segment_clipping_manager_)
    {
      std::cout << "TeleportManager: Multi-segment clipping manager not available" << std::endl;
      return;
    }

    if (enabled)
    {
      // 查找实体的链状态
      auto chain_it = entity_chains_.find(entity_id);
      if (chain_it != entity_chains_.end())
      {
        Vector3 camera_position = estimate_camera_position(chain_it->second);
        multi_segment_clipping_manager_->setup_chain_clipping(chain_it->second, camera_position);
        std::cout << "TeleportManager: Enabled multi-segment clipping for entity " << entity_id << std::endl;
      }
      else
      {
        std::cout << "TeleportManager: No chain state found for entity " << entity_id << std::endl;
      }
    }
    else
    {
      multi_segment_clipping_manager_->cleanup_entity_clipping(entity_id);
      std::cout << "TeleportManager: Disabled multi-segment clipping for entity " << entity_id << std::endl;
    }
  }

  void TeleportManager::set_entity_clipping_quality(EntityId entity_id, int quality_level)
  {
    if (multi_segment_clipping_manager_)
    {
      multi_segment_clipping_manager_->set_entity_clipping_quality(entity_id, quality_level);
      std::cout << "TeleportManager: Set clipping quality level " << quality_level << " for entity " << entity_id << std::endl;
    }
  }

  void TeleportManager::set_multi_segment_smooth_transitions(EntityId entity_id, bool enabled, float blend_distance)
  {
    if (multi_segment_clipping_manager_)
    {
      multi_segment_clipping_manager_->set_smooth_transitions(entity_id, enabled, blend_distance);
      std::cout << "TeleportManager: Set smooth transitions " << (enabled ? "enabled" : "disabled") 
                << " for entity " << entity_id << std::endl;
    }
  }

  int TeleportManager::get_entity_visible_segment_count(EntityId entity_id, const Vector3& camera_position) const
  {
    if (multi_segment_clipping_manager_)
    {
      return multi_segment_clipping_manager_->get_visible_segment_count(entity_id, camera_position);
    }
    return 0;
  }

  void TeleportManager::set_multi_segment_clipping_debug_mode(bool enabled)
  {
    if (multi_segment_clipping_manager_)
    {
      multi_segment_clipping_manager_->set_debug_mode(enabled);
      std::cout << "TeleportManager: Multi-segment clipping debug mode " << (enabled ? "enabled" : "disabled") << std::endl;
    }
  }

  TeleportManager::MultiSegmentClippingStats TeleportManager::get_multi_segment_clipping_stats() const
  {
    MultiSegmentClippingStats stats;
    stats.active_multi_segment_entities = 0;
    stats.total_clipping_planes = 0;
    stats.total_visible_segments = 0;
    stats.average_segments_per_entity = 0.0f;
    stats.frame_setup_time_ms = 0.0f;

    if (multi_segment_clipping_manager_)
    {
      auto internal_stats = multi_segment_clipping_manager_->get_clipping_stats();
      stats.active_multi_segment_entities = internal_stats.active_entity_count;
      stats.total_clipping_planes = internal_stats.total_clipping_planes;
      stats.total_visible_segments = internal_stats.total_visible_segments;
      stats.average_segments_per_entity = internal_stats.average_segments_per_entity;
      stats.frame_setup_time_ms = internal_stats.frame_setup_time_ms;
    }

    return stats;
  }

  void TeleportManager::apply_multi_segment_clipping_to_entity(EntityId entity_id, const MultiSegmentClippingDescriptor& descriptor)
  {
    if (!physics_manipulator_)
    {
      return;
    }

    std::cout << "TeleportManager: Applying multi-segment clipping to entity " << entity_id 
              << " with " << descriptor.clipping_planes.size() << " clipping planes" << std::endl;

    // 使用批量设置方法（如果可用）
    if (!descriptor.clipping_planes.empty())
    {
      physics_manipulator_->set_entities_clipping_states(
        {entity_id}, 
        descriptor.clipping_planes, 
        descriptor.plane_enabled
      );
    }

    // 设置透明度（如果物理引擎支持）
    if (!descriptor.segment_alpha.empty())
    {
      // 这里可以通过扩展接口设置实体的透明度
      // physics_manipulator_->set_entity_alpha(entity_id, descriptor.segment_alpha[0]);
    }

    // 设置模板缓冲值（如果渲染引擎支持）
    if (!descriptor.segment_stencil_values.empty())
    {
      // 这里可以通过扩展接口设置模板值
      // render_manipulator_->set_entity_stencil_value(entity_id, descriptor.segment_stencil_values[0]);
    }
  }

  void TeleportManager::clear_entity_multi_segment_clipping(EntityId entity_id)
  {
    if (physics_manipulator_)
    {
      physics_manipulator_->disable_entity_clipping(entity_id);
      std::cout << "TeleportManager: Cleared multi-segment clipping for entity " << entity_id << std::endl;
    }
  }

  Vector3 TeleportManager::estimate_camera_position(const EntityChainState& chain_state) const
  {
    // 简化实现：使用主体节点的位置作为估算
    if (chain_state.main_position >= 0 && 
        chain_state.main_position < static_cast<int>(chain_state.chain.size()))
    {
      const EntityChainNode& main_node = chain_state.chain[chain_state.main_position];
      
      // 假设相机在主体位置后方一定距离
      Vector3 estimated_position = main_node.transform.position;
      
      // 使用主体的朝向来计算相机位置
      // 这里可以根据实际游戏的相机系统进行优化
      Vector3 back_direction = Vector3(0, 0, -1); // 默认后方
      estimated_position += back_direction * 5.0f; // 距离5个单位
      estimated_position.y += 2.0f; // 稍微抬高

      return estimated_position;
    }

    // 如果没有有效的主体节点，返回默认位置
    return Vector3(0, 2, -5);
  }

} // namespace Portal
