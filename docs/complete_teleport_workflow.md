# 完整的傳送流程：從碰撞到脫離

## 📋 傳送流程總覽

```
剛體運動 → 碰撞檢測 → 創建幽靈 → 質心穿越 → 角色互換 → 完全脫離 → 清理資源
```

## 🔍 詳細流程分析

### 階段 1：初始接觸 (Initial Contact)

**觸發條件：** 物理引擎檢測到剛體的包圍盒與傳送門平面相交

#### 物理引擎側：
```cpp
// 物理引擎內部邏輯（偽代碼）
void PhysicsEngine::update_collision_detection() {
    for (auto& rigidBody : rigidBodies) {
        for (auto& portal : portals) {
            if (rigidBody.bounds.intersects(portal.plane)) {
                // 發送相交事件到傳送門系統
                portalSystem->on_entity_intersect_portal_start(
                    rigidBody.id, portal.id);
            }
        }
    }
}
```

#### 傳送門系統接收：
```cpp
TeleportManager::handle_entity_intersect_start(
    EntityId entity_id,     // 剛體ID: 1001
    PortalId portal_id,     // 傳送門ID: 201  
    Portal* portal,         // 入口傳送門
    PortalId target_portal_id, // 目標傳送門ID: 202
    Portal* target_portal   // 出口傳送門
)
```

#### 關鍵操作：
1. **創建傳送狀態**
```cpp
TeleportState* state = get_or_create_teleport_state(entity_id, portal_id);
state->crossing_state = PortalCrossingState::CROSSING;
state->is_teleporting = true;
state->seamless_mode = true;
```

2. **創建幽靈實體**
```cpp
EntityId ghost_id = create_ghost_entity(entity_id, portal_id, portal, target_portal);
// 物理引擎創建新的剛體: ghost_id = 1002
```

3. **建立映射關係**
```cpp
main_to_ghost_mapping_[1001] = 1002;  // 主體 → 幽靈
ghost_to_main_mapping_[1002] = 1001;  // 幽靈 → 主體
```

4. **創建邏輯實體（如果啟用）**
```cpp
LogicalEntityId logical_id = logical_entity_manager_->create_logical_entity(
    1001, 1002, PhysicsStateMergeStrategy::MOST_RESTRICTIVE);
// logical_id = 5001
```

#### 實體狀態：
- **主體 (ID: 1001)**: 在入口側，正常物理行為
- **幽靈 (ID: 1002)**: 在出口側，同步主體狀態（變換後）
- **邏輯實體 (ID: 5001)**: 統一控制兩者

---

### 階段 2：持續穿越 (Ongoing Crossing)

**狀態：** 剛體部分穿過傳送門，同時存在於兩側

#### 持續同步過程：
```cpp
// 每幀更新 (60Hz)
void TeleportManager::update(float delta_time) {
    if (use_logical_entity_control_) {
        // 邏輯實體模式：統一物理控制
        logical_entity_manager_->update(delta_time);
        /*
        1. 收集主體和幽靈的物理狀態
        2. 合成統一的邏輯狀態
        3. 檢測約束（如碰牆）
        4. 同步回主體和幽靈
        */
    } else {
        // 傳統模式：雙向同步
        sync_all_ghost_entities(delta_time);
        /*
        1. 獲取主體當前狀態
        2. 通過傳送門變換計算幽靈狀態
        3. 更新幽靈實體
        */
    }
}
```

#### 實體狀態變化：
- **主體**: 繼續在入口側運動
- **幽靈**: 在出口側鏡像主體的運動
- **物理引擎**: 對兩個剛體分別進行物理模擬

#### 可能的約束處理：
```cpp
// 如果幽靈碰到障礙物
if (ghost_hits_obstacle) {
    // 邏輯實體模式：約束自動傳播
    LogicalEntityState& state = logical_entities_[logical_id];
    state.constraint_state.is_blocked = true;
    // 主體也會被約束，停止運動
}
```

---

### 階段 3：質心穿越 (Center Crossing) - 關鍵轉折點

**觸發條件：** 物理引擎檢測到實體質心跨過傳送門平面

#### 物理引擎檢測：
```cpp
void PhysicsEngine::check_center_crossing() {
    Vector3 center = calculate_center_of_mass(rigidBody);
    if (center_crossed_portal_plane(center, portal)) {
        portalSystem->on_entity_center_crossed_portal(
            rigidBody.id,      // 1001
            portal.id,         // 201
            PortalFace::A      // 穿越的面
        );
    }
}
```

#### 傳送門系統處理：
```cpp
void TeleportManager::handle_entity_center_crossed(...) {
    TeleportState& state = active_teleports_[entity_id];
    state.center_has_crossed = true;
    state.crossing_point = center_position;
    
    // 執行角色互換
    if (!state.role_swapped) {
        execute_entity_role_swap(main_entity_id, ghost_entity_id);
        state.role_swapped = true;
        state.crossing_state = PortalCrossingState::TELEPORTED;
    }
}
```

#### 角色互換的實現：
```cpp
bool execute_entity_role_swap(EntityId main_id, EntityId ghost_id) {
    // 方式1：ID互換（推薦）
    swap_entity_roles_with_faces(main_id, ghost_id, source_face, target_face);
    
    // 實際效果：
    // - 原主體(1001)在出口側繼續，成為新主體
    // - 原幽靈(1002)在入口側成為幽靈，或被銷毀
    
    // 方式2：物理狀態互換
    // 互換兩個剛體的所有物理屬性
    
    return true;
}
```

#### 映射關係更新：
```cpp
// 互換後的映射（取決於實現方式）
main_to_ghost_mapping_[1001] = 1002;  // 可能保持不變或互換
```

---

### 階段 4：完全脫離 (Complete Exit)

**觸發條件：** 物理引擎檢測到實體完全離開傳送門區域

#### 物理引擎檢測：
```cpp
void PhysicsEngine::check_complete_exit() {
    if (!rigidBody.bounds.intersects(portal.plane)) {
        portalSystem->on_entity_fully_passed_portal(
            rigidBody.id, portal.id);
    }
}
```

#### 傳送門系統處理：
```cpp
void TeleportManager::handle_entity_fully_passed(...) {
    TeleportState& state = active_teleports_[entity_id];
    state.crossing_state = PortalCrossingState::TELEPORTED;
    state.is_teleporting = false;
    state.transition_progress = 1.0f;
    
    // 清理幽靈實體
    destroy_ghost_entity(entity_id);
    
    // 發送完成事件
    event_handler_->on_entity_teleport_complete(entity_id, portal_id, target_portal_id);
}
```

#### 資源清理：
```cpp
void destroy_ghost_entity(EntityId entity_id) {
    EntityId ghost_id = main_to_ghost_mapping_[entity_id];
    
    // 物理引擎銷毀幽靈剛體
    physics_manipulator_->destroy_ghost_entity(ghost_id);
    
    // 清理邏輯實體
    if (use_logical_entity_control_) {
        destroy_logical_entity_for_teleport(entity_id);
    }
    
    // 清理映射
    main_to_ghost_mapping_.erase(entity_id);
    ghost_to_main_mapping_.erase(ghost_id);
}
```

---

## 🎯 關鍵節點總結

### 1. 實體生命週期
```
原始剛體(1001) → 創建幽靈(1002) + 邏輯實體(5001) → 角色互換 → 銷毀幽靈(1002) + 邏輯實體(5001) → 原始剛體(1001)繼續
```

### 2. 數據同步點
- **每幀同步**: 主體 ↔ 幽靈 狀態同步（60Hz）
- **約束同步**: 任一實體受約束時立即同步
- **角色互換**: 質心穿越時的一次性操作

### 3. 物理引擎接口調用
- `create_ghost_entity()` - 創建幽靈剛體
- `update_ghost_entity()` - 每幀更新幽靈狀態  
- `swap_entity_roles()` - 角色互換
- `destroy_ghost_entity()` - 銷毀幽靈剛體

### 4. 事件序列
1. `on_entity_intersect_portal_start` - 開始接觸
2. 持續的狀態同步（60Hz）
3. `on_entity_center_crossed_portal` - 質心穿越
4. `on_entity_fully_passed_portal` - 完全脫離
5. `on_entity_teleport_complete` - 傳送完成

## 💡 特殊情況處理

### 約束傳播示例
```
幽靈(1002)碰牆 → 邏輯實體(5001)被約束 → 主體(1001)停止運動
```

### 複雜物理屬性
```
主體受力F₁ + 幽靈受力F₂ → 合成力F₁+F₂和力矩τ → 物理代理(1003)模擬 → 結果同步到主體和幽靈
```

這個完整流程確保了無感傳送的實現，讓玩家感受不到任何中斷或跳躍！
