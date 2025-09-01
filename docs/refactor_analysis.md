# Portal 庫重構分析：完全外部檢測架構

## 重構目標

將傳送門庫從主動檢測模式改為純事件響應模式，所有物理檢測由外部引擎負責。

## 需要移除的組件

### 1. 檢測相關介面和類別

- `IPortalDetectionOverride` - 不再需要重載，直接移除
- `PortalDetectionManager` - 檢測管理器，完全移除
- `PortalManager::check_entity_portal_intersections()` - 主動檢測方法
- 所有內建的碰撞檢測邏輯

### 2. 舊版相容介面中的檢測部分

- `IPhysicsQuery::raycast()` - 射線檢測
- `IPhysicsQuery::check_center_crossing()` - 質心穿越檢測
- `IPhysicsQuery::calculate_center_crossing_progress()` - 穿越進度計算

### 3. 主動更新邏輯

- `PortalManager::update()` 中的檢測循環
- `detect_and_handle_center_crossing()` 的主動調用

## 需要保留和改進的組件

### 1. 核心功能（保留）

- ✅ 無縫傳送（幽靈實體機制）
- ✅ 自定義質心系統
- ✅ 模型裁切和 A/B 面支援
- ✅ 實體角色互換
- ✅ 渲染通道計算
- ✅ 傳送門鏈接管理

### 2. 新增事件驅動介面

- `IPortalPhysicsEventHandler` - 接收物理引擎事件
- 事件類型：
  - 實體開始與傳送門相交
  - 實體質心穿越傳送門
  - 實體完全穿過傳送門
  - 實體離開傳送門

## 新架構設計

### 事件驅動流程

1. **外部物理引擎**檢測實體與傳送門的相交
2. **外部引擎**調用庫的事件介面通知相交狀態
3. **傳送門庫**根據事件觸發對應的邏輯處理
4. **庫**請求外部引擎執行物理操作（創建幽靈、移動實體等）

### 介面簡化

- `IPhysicsDataProvider` → `IPhysicsStateProvider`（只提供基礎狀態查詢）
- `IPhysicsManipulator` → 保持不變（執行物理操作）
- 移除所有檢測相關介面

### 使用流程

```cpp
// 外部引擎檢測到相交時：
portal_manager->on_entity_intersect_portal_start(entity_id, portal_id);

// 外部引擎檢測到質心穿越時：
portal_manager->on_entity_center_crossed_portal(entity_id, portal_id, crossed_face);

// 外部引擎檢測到穿越完成時：
portal_manager->on_entity_teleport_complete(entity_id, portal_id);
```

## 修改優勢

1. **職責清晰**：庫專注邏輯處理，引擎專注物理檢測
2. **性能優化**：避免重複檢測，充分利用引擎優化
3. **引擎無關**：更容易集成到不同物理引擎
4. **彈性配置**：外部引擎可自定義檢測精度和策略
