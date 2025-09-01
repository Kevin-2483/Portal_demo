# 角色互換邏輯分析

## 🎯 無感角色互換的正確邏輯

### 當前問題分析
根據用戶反饋，角色互換應該遵循以下邏輯：

#### ✅ 正確的角色互換邏輯：
```
質心穿越前：
主體(1001) - 位置A，速度Va，狀態Sa
幽靈(1002) - 位置B，速度Vb，狀態Sb

質心穿越後（角色互換）：
原幽靈(1002) → 變成主體，但保持原位置B，速度Vb，狀態Sb
原主體(1001) → 變成幽靈，但保持原位置A，速度Va，狀態Sa
```

#### 🔑 關鍵要點：
1. **角色邏輯改變**：ID職責互換（誰是主控誰是同步）
2. **物理狀態保持**：各自保持自己的運動狀態
3. **無感體驗**：玩家感受不到任何跳躍或中斷

### 當前代碼檢查

#### 現有實現：
```cpp
bool success = physics_manipulator_->swap_entity_roles_with_faces(
    main_entity_id, ghost_entity_id, source_face, target_face);
```

#### 問題：
- 只調用了接口，但沒有明確的狀態繼承邏輯
- 缺少詳細的實現說明
- 不確定是否滿足"狀態保持，角色互換"的需求

### 正確實現應該是：

```cpp
bool execute_entity_role_swap(EntityId main_id, EntityId ghost_id) {
    // 1. 保存當前狀態
    Transform main_transform = get_entity_transform(main_id);
    PhysicsState main_physics = get_entity_physics_state(main_id);
    Transform ghost_transform = get_entity_transform(ghost_id);  
    PhysicsState ghost_physics = get_entity_physics_state(ghost_id);
    
    // 2. 角色邏輯互換（重要：ID職責互換，不是狀態互換）
    // - 原主體變成幽靈，但保持位置A和狀態Sa
    // - 原幽靈變成主體，但保持位置B和狀態Sb
    
    // 3. 更新系統映射（邏輯角色改變）
    update_entity_role_mapping(main_id, ghost_id);
    
    // 4. 確保物理狀態不變（這是無感的關鍵）
    // 不需要交換物理狀態，各自保持原狀態
    
    return true;
}
```

## 🚨 需要修正的地方

當前實現可能存在狀態交換，這會導致視覺跳躍。
正確的實現應該是：
- ✅ 角色邏輯互換
- ✅ 物理狀態保持
- ✅ 映射關係更新
- ❌ 避免狀態交換
