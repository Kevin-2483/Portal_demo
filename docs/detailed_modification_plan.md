# Portal 庫重構具體修改計劃

## 1. 需要大幅修改的文件

### 1.1 `portal_physics_interfaces.h` - 簡化物理介面

**修改範圍：重構檢測相關介面**

需要移除：

```cpp
// 移除整個 IPortalDetectionOverride 類
class IPortalDetectionOverride { ... }

// 移除 PortalInterfaces 中的 detection_override 成員
struct PortalInterfaces {
    // ... 其他成員保持不變
    IPortalDetectionOverride* detection_override; // ← 移除這行
};
```

需要修改：

- `IPhysicsDataProvider` → 移除檢測相關方法，保留基礎數據獲取
- 保留質心管理系統集成（`ICenterOfMassProvider`）

### 1.2 `portal_detection_manager.h` 和 `.cpp` - 完全移除

**修改範圍：整個文件刪除**

這兩個文件可以完全刪除，因為：

- 檢測邏輯將完全由外部引擎實現
- 不再需要混合檢測架構

### 1.3 `portal_core.h` - 重構 PortalManager

**修改範圍：移除檢測相關成員和方法**

需要移除的構造函數：

```cpp
// 移除混合架構構造函數（因為不再需要detection_manager）
PortalManager(const PortalCore::PortalInterfaces &physics_interfaces);
PortalManager(PortalCore::IPhysicsDataProvider* data_provider, ...);
```

需要移除的成員變數：

```cpp
private:
    std::unique_ptr<PortalCore::PortalDetectionManager> detection_manager_; // ← 移除
    // 其他成員保持不變
```

需要移除的方法：

```cpp
// 移除主動檢測方法
void check_entity_portal_intersections();
bool detect_and_handle_center_crossing(EntityId entity_id, float delta_time);
```

需要新增的事件介面方法：

```cpp
// 新增事件響應方法
void on_entity_intersect_portal_start(EntityId entity_id, PortalId portal_id);
void on_entity_center_crossed_portal(EntityId entity_id, PortalId portal_id, PortalFace crossed_face);
void on_entity_teleport_complete_external(EntityId entity_id, PortalId portal_id);
```

### 1.4 `portal_core.cpp` - 重構實現

**修改範圍：移除檢測邏輯，保留處理邏輯**

需要移除的方法實現：

- `check_entity_portal_intersections()` - 完整移除
- `detect_and_handle_center_crossing()` - 完整移除
- 構造函數中創建 `detection_manager_` 的代碼

需要修改的 `update()` 方法：

```cpp
void PortalManager::update(float delta_time) {
    if (!is_initialized_) return;

    // 保留：更新传送门递归状态
    update_portal_recursive_states();

    // 移除：check_entity_portal_intersections();
    // 移除：无缝传送的主动检测循环

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

## 2. 需要少量修改的文件

### 2.1 `portal_interfaces.h` - 清理舊版介面

**修改範圍：移除檢測相關方法**

需要從 `IPhysicsQuery` 移除：

```cpp
// 移除這些檢測方法
virtual bool raycast(...) const = 0;
virtual CenterOfMassCrossing check_center_crossing(...) const = 0;
virtual float calculate_center_crossing_progress(...) const = 0;
```

保留其他所有方法（變換查詢、包圍盒獲取等）。

### 2.2 `portal_types.h` - 移除檢測相關類型

**修改範圍：移除部分結構體**

可以移除：

```cpp
// 這個結構體主要用於內建檢測，可以移除
struct CenterOfMassCrossing { ... }
```

保留所有其他類型定義。

## 3. 完全不需要修改的文件

### 3.1 `portal_math.h` 和 `portal_math.cpp` - 保持不變

**原因：純數學函數庫**

所有數學計算函數都可以重用：

- ✅ 傳送門變換計算
- ✅ 幽靈實體位置計算
- ✅ 包圍盒變換
- ✅ 相機變換計算
- ✅ 質心位置計算

### 3.2 `portal_center_of_mass.h` 和 `.cpp` - 完全保留

**原因：質心管理系統獨立於檢測**

整個質心管理系統可以完全重用：

- ✅ 自定義質心類型
- ✅ 骨骼附著
- ✅ 多點加權平均
- ✅ 動態計算

### 3.3 渲染相關代碼 - 完全保留

**原因：與檢測無關**

所有渲染功能保持不變：

- ✅ 遞歸渲染通道計算
- ✅ 相機變換
- ✅ 裁剪平面計算
- ✅ 模板緩衝區支援

## 4. 新增文件

### 4.1 `portal_event_interfaces.h` - 新增事件介面

**內容：定義外部引擎與庫的事件通信**

```cpp
namespace Portal {

/**
 * 物理引擎事件介面
 * 外部物理引擎通過此介面通知庫發生的碰撞事件
 */
class IPortalPhysicsEventReceiver {
public:
    virtual ~IPortalPhysicsEventReceiver() = default;

    // 實體開始與傳送門相交
    virtual void on_entity_intersect_portal_start(EntityId entity_id, PortalId portal_id) = 0;

    // 實體質心穿越傳送門
    virtual void on_entity_center_crossed_portal(EntityId entity_id, PortalId portal_id, PortalFace crossed_face) = 0;

    // 實體完全穿過傳送門
    virtual void on_entity_fully_passed_portal(EntityId entity_id, PortalId portal_id) = 0;

    // 實體離開傳送門範圍
    virtual void on_entity_exit_portal(EntityId entity_id, PortalId portal_id) = 0;
};

} // namespace Portal
```

## 5. 修改統計

### 代碼重用率分析：

- **數學庫**: 100% 重用（0 行修改）
- **質心系統**: 100% 重用（0 行修改）
- **渲染系統**: 100% 重用（0 行修改）
- **幽靈實體邏輯**: 95% 重用（僅觸發方式改變）
- **傳送門管理**: 90% 重用（移除檢測循環）
- **物理介面**: 60% 重用（移除檢測方法）

### 預估修改行數：

- **刪除**: ~500 行（檢測管理器 + 檢測方法）
- **修改**: ~200 行（構造函數 + update 方法）
- **新增**: ~100 行（事件介面）
- **總代碼**: ~4000 行

**結論：約 80%的代碼可以直接重用，主要修改集中在移除主動檢測邏輯。**
