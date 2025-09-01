# Portal 庫重構實施步驟

## 實施順序（建議按此順序進行）

### 第 1 步：新增事件介面（風險最小）

**新建文件：** `portal_event_interfaces.h`

```cpp
#pragma once
#include "portal_types.h"

namespace Portal {

/**
 * 物理引擎事件接收介面
 * 外部物理引擎通過此介面通知傳送門庫發生的碰撞事件
 */
class IPortalPhysicsEventReceiver {
public:
    virtual ~IPortalPhysicsEventReceiver() = default;

    // 實體開始與傳送門相交（包圍盒相交）
    virtual void on_entity_intersect_portal_start(EntityId entity_id, PortalId portal_id) = 0;

    // 實體質心穿越傳送門平面
    virtual void on_entity_center_crossed_portal(EntityId entity_id, PortalId portal_id, PortalFace crossed_face) = 0;

    // 實體完全穿過傳送門
    virtual void on_entity_fully_passed_portal(EntityId entity_id, PortalId portal_id) = 0;

    // 實體與傳送門分離
    virtual void on_entity_exit_portal(EntityId entity_id, PortalId portal_id) = 0;
};

} // namespace Portal
```

### 第 2 步：修改 PortalManager 添加事件響應

**修改文件：** `portal_core.h`

在 `PortalManager` 類中新增事件處理方法：

```cpp
public:
    // === 新增：外部物理引擎事件響應介面 ===

    /**
     * 處理實體開始與傳送門相交事件
     * 由外部物理引擎調用
     */
    void on_entity_intersect_portal_start(EntityId entity_id, PortalId portal_id);

    /**
     * 處理實體質心穿越傳送門事件
     * 由外部物理引擎調用
     */
    void on_entity_center_crossed_portal(EntityId entity_id, PortalId portal_id, PortalFace crossed_face);

    /**
     * 處理實體完全穿過傳送門事件
     * 由外部物理引擎調用
     */
    void on_entity_fully_passed_portal(EntityId entity_id, PortalId portal_id);

    /**
     * 處理實體離開傳送門事件
     * 由外部物理引擎調用
     */
    void on_entity_exit_portal(EntityId entity_id, PortalId portal_id);
```

### 第 3 步：實現事件處理邏輯

**修改文件：** `portal_core.cpp`

將現有的檢測處理邏輯改為事件響應：

```cpp
void PortalManager::on_entity_intersect_portal_start(EntityId entity_id, PortalId portal_id) {
    std::cout << "Entity " << entity_id << " started intersecting portal " << portal_id << std::endl;

    // 重用現有的邏輯：創建或更新傳送狀態
    TeleportState* state = get_or_create_teleport_state(entity_id, portal_id);
    state->crossing_state = PortalCrossingState::CROSSING;

    // 重用現有的邏輯：創建幽靈實體
    create_ghost_entity_with_faces(entity_id, portal_id, state->source_face, state->target_face);
}

void PortalManager::on_entity_center_crossed_portal(EntityId entity_id, PortalId portal_id, PortalFace crossed_face) {
    std::cout << "Entity " << entity_id << " center crossed portal " << portal_id << std::endl;

    // 重用現有的邏輯：處理質心穿越事件
    handle_center_crossing_event(entity_id, portal_id, crossed_face);
}
```

### 第 4 步：簡化更新循環

**修改文件：** `portal_core.cpp` 中的 `update()` 方法

```cpp
void PortalManager::update(float delta_time) {
    if (!is_initialized_) return;

    // 保留：更新传送门递归状态
    update_portal_recursive_states();

    // 移除：check_entity_portal_intersections(); // ← 刪除這行
    // 移除：detect_and_handle_center_crossing();  // ← 刪除相關循環

    // 保留：更新正在进行的传送
    update_entity_teleportation(delta_time);

    // 保留：质心管理器更新
    if (center_of_mass_manager_) {
        center_of_mass_manager_->update_auto_update_entities(delta_time);
    }

    // 保留：同步幽灵实体状态
    sync_all_ghost_entities(delta_time, false);

    // 保留：清理已完成的传送
    cleanup_completed_teleports();
}
```

### 第 5 步：清理構造函數

**修改文件：** `portal_core.h` 和 `portal_core.cpp`

移除與檢測管理器相關的構造函數：

```cpp
// 保留舊版構造函數（向後相容）
PortalManager(const HostInterfaces &interfaces);

// 移除這些構造函數：
// PortalManager(const PortalCore::PortalInterfaces &physics_interfaces);
// PortalManager(PortalCore::IPhysicsDataProvider* data_provider, ...);

// 或者保留但簡化，移除 detection_manager_ 的創建
```

### 第 6 步：清理成員變數

**修改文件：** `portal_core.h`

```cpp
private:
    HostInterfaces interfaces_;

    // 移除：std::unique_ptr<PortalCore::PortalDetectionManager> detection_manager_;

    // 保留其他所有成員變數
    PortalCore::IPhysicsManipulator* physics_manipulator_;
    // ... 其他成員
```

### 第 7 步：移除檢測相關文件

**刪除文件：**

- `portal_detection_manager.h`
- `portal_detection_manager.cpp`

### 第 8 步：清理介面定義

**修改文件：** `portal_physics_interfaces.h`

移除 `IPortalDetectionOverride` 類：

```cpp
// 完全刪除這個類
class IPortalDetectionOverride { ... }

// 修改 PortalInterfaces 結構
struct PortalInterfaces {
    IPhysicsDataProvider* physics_data;
    IPhysicsManipulator* physics_manipulator;
    IRenderQuery* render_query;
    IRenderManipulator* render_manipulator;
    IPortalEventHandler* event_handler;
    // 移除：IPortalDetectionOverride* detection_override;
};
```

### 第 9 步：清理舊版介面（可選）

**修改文件：** `portal_interfaces.h`

從 `IPhysicsQuery` 移除檢測方法：

```cpp
class IPhysicsQuery {
    // 保留基礎查詢方法
    virtual Transform get_entity_transform(EntityId entity_id) const = 0;
    virtual PhysicsState get_entity_physics_state(EntityId entity_id) const = 0;
    // ...

    // 移除檢測方法
    // virtual bool raycast(...) const = 0;
    // virtual CenterOfMassCrossing check_center_crossing(...) const = 0;
    // virtual float calculate_center_crossing_progress(...) const = 0;
};
```

## 測試驗證步驟

### 階段性測試

1. **第 1-3 步後**：測試事件響應是否正常工作
2. **第 4-6 步後**：測試更新循環是否正確移除了主動檢測
3. **第 7-9 步後**：測試完整的事件驅動架構

### 驗證要點

- ✅ 幽靈實體創建和同步功能正常
- ✅ 質心穿越檢測響應正確
- ✅ 實體角色互換功能正常
- ✅ 渲染裁剪和遞歸渲染正常
- ✅ 不再有主動的碰撞檢測循環

## 風險評估

**低風險修改**（先做）：

- 新增事件介面
- 添加事件響應方法
- 簡化更新循環

**中等風險修改**（後做）：

- 移除檢測管理器
- 清理構造函數

**注意事項**：

- 保持現有的所有數學計算不變
- 保持質心管理系統不變
- 保持渲染系統不變
- 確保無縫傳送功能完整保留
