# Portal Core Library - 可移植傳送門系統

這是一個完全引擎無關的傳送門核心模組，專為需要傳送門功能的遊戲或應用程序設計。

## 特性

### 🚀 核心功能

- **引擎無關**: 純 C++ 實現，不依賴任何特定遊戲引擎
- **接口驅動**: 通過抽象接口與宿主應用程序交互
- **數學精確**: 完整的傳送門數學變換計算
- **高性能**: 優化的變換計算和狀態管理

### 🌀 傳送門特性

- **雙面有效**: 傳送門雙向工作
- **保持物理**: 穿過傳送門的物體保持動量和角動量
- **比例縮放**: 支持不同大小傳送門間的縮放變換
- **遞歸渲染**: 傳送門穿過自己形成無限遞歸效果
- **速度感知**: 支持移動傳送門的速度影響

### 🎯 設計原則

- **職責分離**: 庫專注數學變換，引擎負責碰撞檢測
- **易於集成**: 清晰的接口定義，易於適配到任何引擎
- **可擴展**: 模塊化設計，支持自定義擴展
- **類型安全**: 強類型系統，減少運行時錯誤

## 架構設計原則

### 🏗️ 職責分離

- **引擎負責**: 碰撞檢測、觸發器管理、物理模擬
- **傳送門庫負責**: 數學變換、狀態管理、傳送邏輯
- **清晰界限**: 庫不做物理檢測，引擎不做變換計算

### 📋 集成流程

1. **碰撞檢測 (引擎)**: 在遊戲引擎中為每個傳送門設置觸發器（Trigger Volume）
2. **穿越判斷 (引擎)**: 引擎判斷物體是否真的"穿過"傳送門平面
3. **觸發傳送 (引擎->庫)**: 引擎調用 `PortalManager::teleport_entity()`
4. **數學變換 (庫)**: 庫計算新的物理參數（位置、速度等）
5. **執行傳送 (庫->引擎)**: 庫通過接口將新參數設置回引擎
6. **完成渲染 (引擎)**: 下一幀，實體出現在新位置

## 快速開始

### 1. 編譯

```bash
mkdir build && cd build
cmake .. -DBUILD_PORTAL_EXAMPLES=ON
make
```

### 2. 基本使用

```cpp
#include "portal_core.h"

// 實現必需的接口
class MyPhysicsQuery : public Portal::IPhysicsQuery {
    // 實現接口方法...
};

// 創建傳送門系統
Portal::HostInterfaces interfaces;
interfaces.physics_query = new MyPhysicsQuery();
// ... 設置其他接口

Portal::PortalManager manager(interfaces);
manager.initialize();

// 創建傳送門
Portal::PortalPlane plane1;
plane1.center = Portal::Vector3(-5, 0, 0);
plane1.normal = Portal::Vector3(1, 0, 0);
// ... 設置其他屬性

Portal::PortalId portal1 = manager.create_portal(plane1);
Portal::PortalId portal2 = manager.create_portal(plane2);

// 鏈接傳送門
manager.link_portals(portal1, portal2);

// 每幀更新
manager.update(delta_time);
```

### 3. 運行測試控制台

```bash
./portal_console
```

## 接口實現指南

### 必需接口

#### IPhysicsQuery

獲取物理世界的狀態信息：

```cpp
class MyPhysicsQuery : public Portal::IPhysicsQuery {
public:
    Portal::Transform get_entity_transform(Portal::EntityId entity_id) const override {
        // 返回實體的位置、旋轉、縮放
    }

    Portal::PhysicsState get_entity_physics_state(Portal::EntityId entity_id) const override {
        // 返回實體的速度、角速度、質量
    }

    // ... 實現其他方法
};
```

#### IPhysicsManipulator

修改物理世界的狀態：

```cpp
class MyPhysicsManipulator : public Portal::IPhysicsManipulator {
public:
    void set_entity_transform(Portal::EntityId entity_id, const Portal::Transform& transform) override {
        // 設置實體的新位置和旋轉
    }

    void set_entity_physics_state(Portal::EntityId entity_id, const Portal::PhysicsState& state) override {
        // 設置實體的新速度
    }

    // ... 實現其他方法
};
```

#### IRenderQuery & IRenderManipulator

處理渲染相關的查詢和操作。

### 可選接口

#### IPortalEventHandler

接收傳送門系統事件通知：

```cpp
class MyEventHandler : public Portal::IPortalEventHandler {
public:
    void on_entity_teleport_start(Portal::EntityId entity_id,
                                Portal::PortalId source_portal,
                                Portal::PortalId target_portal) override {
        // 實體開始傳送時觸發
    }

    // ... 實現其他事件處理
};
```

## 核心 API

### 傳送門管理

```cpp
// 創建傳送門
PortalId create_portal(const PortalPlane& plane);

// 連接傳送門
bool link_portals(PortalId portal1, PortalId portal2);

// 斷開連接
void unlink_portal(PortalId portal_id);

// 銷毀傳送門
void destroy_portal(PortalId portal_id);
```

### 實體傳送

```cpp
// 基本傳送
TeleportResult teleport_entity(EntityId entity_id, PortalId source_portal, PortalId target_portal);

// 速度感知傳送（考慮傳送門速度）
TeleportResult teleport_entity_with_velocity(EntityId entity_id, PortalId source_portal, PortalId target_portal);

// 註冊需要傳送檢測的實體
void register_entity(EntityId entity_id);

// 取消註冊實體
void unregister_entity(EntityId entity_id);
```

### 物理狀態管理

```cpp
// 更新傳送門物理狀態（位置、速度等）
void update_portal_physics_state(PortalId portal_id, const PhysicsState& physics_state);

// 更新傳送門平面
void update_portal_plane(PortalId portal_id, const PortalPlane& plane);
```

## 數學變換

傳送門系統包含完整的數學變換計算：

### 點變換

```cpp
Portal::Vector3 new_point = Portal::Math::PortalMath::transform_point_through_portal(
    original_point, source_plane, target_plane
);
```

### 物理狀態變換

```cpp
Portal::PhysicsState new_state = Portal::Math::PortalMath::transform_physics_state_through_portal(
    original_state, source_plane, target_plane
);
```

### 速度感知變換

```cpp
Portal::PhysicsState new_state = Portal::Math::PortalMath::transform_physics_state_with_portal_velocity(
    entity_physics_state, source_portal_physics, target_portal_physics,
    source_plane, target_plane
);
```

### 相機變換（用於渲染）

```cpp
Portal::CameraParams portal_camera = Portal::Math::PortalMath::calculate_portal_camera(
    original_camera, source_plane, target_plane
);
```

## 遞歸渲染支持

系統支持傳送門的遞歸渲染效果：

```cpp
// 獲取遞歸渲染的相機列表
std::vector<Portal::CameraParams> cameras = manager.get_portal_render_cameras(
    portal_id, base_camera, max_recursion_depth
);

// 按順序渲染每個相機視角
for (const auto& camera : cameras) {
    render_scene_with_camera(camera);
}
```

## 引擎集成示例

### Godot 引擎集成

```cpp
// 在 Godot GDExtension 中實現接口
class GodotPhysicsQuery : public Portal::IPhysicsQuery {
    Portal::Transform get_entity_transform(Portal::EntityId entity_id) const override {
        Node3D* node = get_node_by_id(entity_id);
        godot::Transform3D godot_transform = node->get_global_transform();
        return convert_to_portal_transform(godot_transform);
    }
    // ...
};

// 在 Godot 中設置碰撞檢測
void _on_portal_trigger_body_entered(Node3D* body) {
    // 判斷是否穿越傳送門平面
    if (check_crossing(body)) {
        // 調用傳送門庫
        portal_manager->teleport_entity(entity_id, source_portal, target_portal);
    }
}
```

### Unity 引擎集成

```cpp
// 在 Unity Native Plugin 中實現接口
class UnityPhysicsQuery : public Portal::IPhysicsQuery {
    Portal::Transform get_entity_transform(Portal::EntityId entity_id) const override {
        // 調用 Unity 的 C# 接口獲取 Transform
        UnityTransform unity_transform = GetEntityTransform(entity_id);
        return convert_to_portal_transform(unity_transform);
    }
    // ...
};
```

```csharp
// Unity C# 腳本
public class PortalTrigger : MonoBehaviour {
    private void OnTriggerEnter(Collider other) {
        // 檢查實體是否真正穿越傳送門
        if (CheckCrossing(other.transform)) {
            // 調用 Native Plugin
            PortalNative.TeleportEntity(entityId, sourcePortalId, targetPortalId);
        }
    }
}
```

## 性能考慮

### 優化建議

1. **實體註冊**: 只註冊需要傳送檢測的實體
2. **視錐體裁剪**: 在渲染前檢查傳送門可見性
3. **遞歸深度**: 限制遞歸渲染深度避免性能問題
4. **碰撞檢測**: 在引擎層面使用高效的觸發器系統

### 內存管理

- 系統使用 RAII 和智能指針管理內存
- 自動清理已完成的傳送狀態
- 支持動態創建和銷毀傳送門

## 調試功能

系統提供多種調試信息：

```cpp
// 獲取系統狀態
size_t portal_count = manager.get_portal_count();
size_t entity_count = manager.get_registered_entity_count();
size_t teleporting_count = manager.get_teleporting_entity_count();

// 檢查傳送門狀態
const Portal::Portal* portal = manager.get_portal(portal_id);
bool is_recursive = portal->is_recursive();
bool is_linked = portal->is_linked();
```

## 許可證

MIT License - 查看 LICENSE 文件了解詳情

## 貢獻

歡迎提交 Issue 和 Pull Request！

---

**下一步**: 查看 `../CONSOLE_TESTING_GUIDE.md` 了解如何使用控制台程序測試傳送門功能。
