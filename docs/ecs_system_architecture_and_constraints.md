# ECS 系統架構和組件限制文檔

## 📋 目錄

1. [ECS 系統架構概述](#ecs-系統架構概述)
2. [核心組件介紹](#核心組件介紹)
3. [系統管理器架構](#系統管理器架構)
4. [組件屬性限制](#組件屬性限制)
5. [使用指南](#使用指南)
6. [最佳實踐](#最佳實踐)

---

## 🏗️ ECS 系統架構概述

### 架構組成

```
Portal Demo ECS 架構
├── 實體 (Entities)                 - entt::entity
├── 組件 (Components)               - 數據容器
│   ├── TransformComponent         - 變換數據
│   ├── PhysicsBodyComponent      - 物理體數據
│   ├── PhysicsSyncComponent      - 物理同步配置
│   └── PhysicsCommandComponent   - 物理命令數據
└── 系統 (Systems)                 - 邏輯處理器
    ├── SystemManager             - 系統管理和調度
    ├── PhysicsSystem             - 物理體生命週期
    ├── PhysicsCommandSystem      - 物理命令執行
    └── PhysicsQuerySystem        - 物理查詢服務
```

### 依賴關係

```
物理引擎集成: EnTT ECS ←→ Jolt Physics
系統調度: SystemManager → entt::organizer
並行執行: 分層執行 + 依賴管理
```

---

## 🧩 核心組件介紹

### 1. TransformComponent

**用途**: 實體的空間變換信息

```cpp
struct TransformComponent {
    Vector3 position;      // 世界坐標位置
    Quaternion rotation;   // 旋轉四元數 (w,x,y,z)
    Vector3 scale;         // 縮放比例
};
```

**限制**:

- ✅ 無特殊限制
- ℹ️ 所有物理實體都必須具備此組件

### 2. PhysicsBodyComponent

**用途**: 物理體的屬性和配置

```cpp
struct PhysicsBodyComponent {
    PhysicsBodyType body_type;     // 物理體類型
    PhysicsShapeDesc shape;        // 形狀描述
    PhysicsMaterial material;      // 材質屬性
    // ... 其他屬性
};
```

### 3. PhysicsSyncComponent

**用途**: 控制物理和變換組件間的同步

```cpp
struct PhysicsSyncComponent {
    bool sync_position;           // 是否同步位置
    bool sync_rotation;           // 是否同步旋轉
    bool sync_velocity;           // 是否同步速度
    float position_threshold;     // 位置同步閾值
    // ... 同步配置
};
```

### 4. PhysicsCommandComponent

**用途**: 存儲待執行的物理命令

```cpp
struct PhysicsCommandComponent {
    std::vector<PhysicsCommand> commands;  // 命令隊列
    // 支持多種命令類型: 施力、設置速度、移動等
};
```

---

## 🔧 系統管理器架構

### SystemManager 特性

- **自動註冊**: 從 SystemRegistry 自動載入所有系統
- **依賴管理**: 基於聲明的依賴關係構建執行圖
- **分層執行**: 將系統分組到執行層，支持並行處理
- **生命週期管理**: 統一的初始化、更新、清理流程

### 執行順序示例

```
Layer 0 (並行): ZRotationSystem, PhysicsCommandSystem, YRotationSystem, XRotationSystem
Layer 1:        PhysicsSystem
Layer 2:        PhysicsQuerySystem
```

### 系統註冊方式

```cpp
// 在 portal_game_world.cpp 中註冊
SystemRegistry::register_system("PhysicsSystem",
    []() -> std::unique_ptr<SystemBase> {
        return std::make_unique<PhysicsSystem>();
    },
    {"PhysicsCommandSystem"},  // 依賴項
    {},                        // 衝突項
    100                        // 優先級
);
```

---

## ⚠️ 組件屬性限制

### PhysicsBodyComponent 限制

#### 1. 物理體類型限制

```cpp
enum class PhysicsBodyType {
    STATIC,      // 靜態物體 - 不移動，無質量
    DYNAMIC,     // 動態物體 - 受力移動，需要質量
    KINEMATIC,   // 運動學物體 - 腳本控制移動，需要質量
    TRIGGER      // 觸發器 - 不產生碰撞響應，無質量
};
```

#### 2. 材質屬性限制

```cpp
struct PhysicsMaterial {
    float friction;      // 摩擦係數: >= 0.0
    float restitution;   // 彈性係數: 0.0 - 1.0
    float density;       // 密度: > 0.0 (動態/運動學物體)
};
```

**明確限制**:

- ❌ `friction < 0.0` - 摩擦係數不能為負
- ❌ `restitution < 0.0 || restitution > 1.0` - 彈性係數超出範圍
- ❌ `density <= 0.0` (動態/運動學物體) - 會導致質量為 0，觸發 Jolt 斷言

#### 3. 形狀尺寸限制

```cpp
// 盒子形狀
PhysicsShapeType::BOX:
- ❌ size.x <= 0 || size.y <= 0 || size.z <= 0

// 球體形狀
PhysicsShapeType::SPHERE:
- ❌ radius <= 0

// 膠囊形狀
PhysicsShapeType::CAPSULE:
- ❌ radius <= 0 || height <= 0
```

#### 4. 質量相關限制

```cpp
// 動態物體質量限制
if (body_type == DYNAMIC) {
    // ❌ mass <= 0.0 - 動態物體必須有正質量
}

// 密度與質量的關係
mass = density * volume
// 因此 density <= 0 會導致 mass <= 0
```

### 組件依賴限制

#### PhysicsSystem 必需組件

```cpp
// 創建物理體時必需的組件
required_components = {
    PhysicsBodyComponent,  // ✅ 必須存在
    TransformComponent     // ✅ 必須存在
};

optional_components = {
    PhysicsSyncComponent,     // 影響同步行為
    PhysicsCommandComponent   // 允許命令控制
};
```

#### PhysicsCommandSystem 必需組件

```cpp
// 執行物理命令時檢查的組件
has_physics_body = registry.try_get<PhysicsBodyComponent>(entity);
has_transform = registry.try_get<TransformComponent>(entity);
// 如果缺失任一組件，命令將被忽略
```

---

## 📖 使用指南

### 1. 創建基本物理實體

```cpp
// 1. 創建實體
auto entity = registry.create();

// 2. 添加變換組件 (必需)
auto& transform = registry.emplace<TransformComponent>(entity);
transform.position = Vector3(0, 5, 0);
transform.rotation = Quaternion(1, 0, 0, 0);

// 3. 添加物理體組件 (必需)
auto& physics_body = registry.emplace<PhysicsBodyComponent>(entity);
physics_body.body_type = PhysicsBodyType::DYNAMIC;
physics_body.set_box_shape(Vector3(1, 1, 1));           // ✅ 尺寸 > 0
physics_body.set_material(0.5f, 0.3f, 1000.0f);        // ✅ 有效材質

// 4. 添加物理同步組件 (可選)
auto& sync = registry.emplace<PhysicsSyncComponent>(entity);
sync.sync_position = true;
sync.sync_rotation = true;
```

### 2. 創建靜態地面

```cpp
auto ground = registry.create();

auto& transform = registry.emplace<TransformComponent>(ground);
transform.position = Vector3(0, -1, 0);

auto& physics_body = registry.emplace<PhysicsBodyComponent>(ground);
physics_body.body_type = PhysicsBodyType::STATIC;       // 靜態物體
physics_body.set_box_shape(Vector3(10, 0.5f, 10));
physics_body.set_material(0.8f, 0.1f, 1000.0f);        // 密度會被忽略，但建議設置
```

### 3. 創建運動學物體

```cpp
auto platform = registry.create();

auto& transform = registry.emplace<TransformComponent>(platform);
transform.position = Vector3(0, 2, 0);

auto& physics_body = registry.emplace<PhysicsBodyComponent>(platform);
physics_body.body_type = PhysicsBodyType::KINEMATIC;    // 運動學物體
physics_body.set_box_shape(Vector3(2, 0.2f, 2));
physics_body.set_material(0.3f, 0.0f, 1000.0f);        // ✅ 密度 > 0 必需

// 添加命令組件以控制移動
auto& commands = registry.emplace<PhysicsCommandComponent>(platform);
```

### 4. 系統初始化

```cpp
// 註冊所有物理系統
PortalGameWorld::register_physics_systems();

// 創建並初始化系統管理器
SystemManager system_manager;
system_manager.initialize();

// 更新循環
while (running) {
    float delta_time = calculate_delta_time();
    system_manager.update(registry, delta_time);
}

// 清理
system_manager.cleanup();
```

---

## 🎯 最佳實踐

### 1. 組件設置順序

```cpp
// ✅ 推薦順序
1. 設置 body_type
2. 設置 shape (set_box_shape, set_sphere_shape 等)
3. 設置 material (確保參數有效)
4. 設置其他可選屬性
```

### 2. 參數驗證

```cpp
// ✅ 在設置前進行驗證
float density = 1000.0f;
if (body_type == PhysicsBodyType::DYNAMIC || body_type == PhysicsBodyType::KINEMATIC) {
    if (density <= 0.0f) {
        density = 1000.0f;  // 使用默認值
        std::cerr << "Warning: Invalid density, using default 1000.0f" << std::endl;
    }
}
physics_body.set_material(friction, restitution, density);
```

### 3. 錯誤處理

```cpp
// ✅ 檢查組件是否成功創建
if (auto* physics_body = registry.try_get<PhysicsBodyComponent>(entity)) {
    // 組件存在，可以安全使用
} else {
    std::cerr << "Entity missing PhysicsBodyComponent" << std::endl;
}
```

### 4. 性能優化

```cpp
// ✅ 使用組件群組進行批量處理
auto view = registry.view<PhysicsBodyComponent, TransformComponent>();
for (auto entity : view) {
    auto& [physics, transform] = view.get(entity);
    // 批量處理
}
```

---

## 🚨 常見錯誤和解決方案

### 1. "Invalid mass" 斷言錯誤

**原因**: 動態或運動學物體的密度 ≤ 0

```cpp
// ❌ 錯誤
physics_body.set_material(0.5f, 0.3f, 0.0f);  // 密度為 0

// ✅ 正確
physics_body.set_material(0.5f, 0.3f, 1000.0f);  // 密度 > 0
```

### 2. 物理體創建失敗

**原因**: 缺少必需組件或參數無效

```cpp
// ✅ 確保必需組件存在
if (!registry.try_get<PhysicsBodyComponent>(entity) ||
    !registry.try_get<TransformComponent>(entity)) {
    std::cerr << "Missing required components" << std::endl;
    return;
}
```

### 3. 系統執行順序問題

**原因**: 系統依賴關係配置錯誤

```cpp
// ✅ 正確配置依賴關係
SystemRegistry::register_system("PhysicsQuerySystem",
    factory,
    {"PhysicsSystem"},  // 必須在 PhysicsSystem 之後執行
    {},
    50
);
```

---

## 📊 限制總結表

| 組件                 | 屬性                 | 限制條件            | 錯誤後果      |
| -------------------- | -------------------- | ------------------- | ------------- |
| PhysicsBodyComponent | material.friction    | ≥ 0.0               | 驗證失敗      |
| PhysicsBodyComponent | material.restitution | 0.0 - 1.0           | 驗證失敗      |
| PhysicsBodyComponent | material.density     | > 0.0 (動態/運動學) | Jolt 斷言失敗 |
| PhysicsBodyComponent | shape.radius         | > 0.0               | 驗證失敗      |
| PhysicsBodyComponent | shape.size           | 所有維度 > 0.0      | 驗證失敗      |
| PhysicsBodyComponent | mass                 | > 0.0 (動態物體)    | 驗證失敗      |
| 系統依賴             | 必需組件             | 必須存在            | 創建/更新失敗 |

---

_本文檔基於實際測試和代碼分析編寫，涵蓋了所有已確認的限制條件。_
