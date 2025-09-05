# 快速測試指南

## 測試場景說明

物理測試場景包含：
- 灰色地面（大平台）
- 紅色球體 x2（空中）
- 綠色盒子 x2（空中）
- 膠囊和圓柱（空中）
- 額外平台和斜坡

每個物體都有 ECSNode 子節點，用於添加物理組件。

## 快速測試步驟

### 1. 設置地面（靜態物理體）
選擇 `Ground/GroundECS`，添加 `PhysicsBodyComponentResource`：
- Body Type: Static (0)
- Shape Type: Box (0)
- Shape Size: (5, 0.2, 5)

### 2. 設置球體（動態物理體）
選擇 `TestObjects/Sphere1/SphereECS`，添加 `PhysicsBodyComponentResource`：
- Body Type: Dynamic (1)
- Shape Type: Sphere (1)
- Shape Size: (0.5, 0.5, 0.5)
- Mass: 1.0

### 3. 運行場景
保存並運行場景，球體應該會下落並與地面碰撞。

## 擴展測試

對其他物體重複步驟2，使用適當的形狀類型：
- 盒子：Box (0)，Shape Size: (0.4, 0.4, 0.4)
- 膠囊：Capsule (2)，Shape Size: (0.3, 1.5, 0.3)

可以添加 `PhysicsCommandComponentResource` 來測試初始力或衝量。
