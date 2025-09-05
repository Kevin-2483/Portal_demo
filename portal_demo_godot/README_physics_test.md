# 物理組件測試場景使用指南

## 場景內容

測試場景 `physics_test_scene.tscn` 包含以下元素：

### 物體類型
1. **地面** - 大型平台，適合設置為靜態物理體
2. **球體** - 紅色球體 x2，適合測試滾動物理
3. **盒子** - 綠色立方體 x2，適合測試碰撞
4. **膠囊** - 適合測試複雜形狀物理
5. **圓柱** - 適合測試旋轉物理
6. **平台** - 額外的靜態平台
7. **斜坡** - 傾斜平台，測試重力和滑動

### ECS節點
每個物體都有對應的 `ECSNode` 子節點，用於添加物理組件。

## 測試步驟

### 1. 基本物理體設置

**地面設置（靜態物理體）：**
1. 選擇 `Ground/GroundECS` 節點
2. 在檢查器中點擊 `Components` 屬性旁的 `+` 按鈕
3. 選擇 `PhysicsBodyComponentResource`
4. 設置以下屬性：
   - `Body Type`: Static (0)
   - `Shape Type`: Box (0)
   - `Shape Size`: (5, 0.2, 5) - 匹配地面網格大小
   - `Friction`: 0.7
   - `Restitution`: 0.1

**動態物體設置（球體示例）：**
1. 選擇 `TestObjects/Sphere1/SphereECS` 節點
2. 添加 `PhysicsBodyComponentResource`
3. 設置屬性：
   - `Body Type`: Dynamic (1)
   - `Shape Type`: Sphere (1)
   - `Shape Size`: (0.5, 0.5, 0.5) - 球體半徑
   - `Mass`: 1.0
   - `Friction`: 0.5
   - `Restitution`: 0.3
   - `Gravity Scale`: 1.0

### 2. 運行測試
1. 保存場景
2. 編譯 GDExtension（如果需要）
3. 運行場景
4. 觀察物理模擬效果

### 3. 高級測試

**添加物理命令：**
1. 為任意動態物體添加 `PhysicsCommandComponentResource`
2. 設置初始力或衝量：
   - `Add Force`: (0, 100, 0) - 向上的力
   - `Add Impulse`: (50, 0, 0) - 側向衝量
3. 設置執行時機：
   - `Command Timing`: Before Physics (1)
   - `Execute Once`: true

**測試不同形狀：**
- 盒子：設置為 Box 形狀，調整 `Shape Size` 為 (0.4, 0.4, 0.4)
- 膠囊：設置為 Capsule 形狀，`Shape Size.x` = 半徑，`Shape Size.y` = 高度
- 圓柱：當前用盒子近似，可以調整比例

## 調試技巧

1. **檢查控制台輸出** - 組件應用時會有日誌輸出
2. **調整重力** - 可以通過 `Gravity Scale` 調整物體受重力影響程度
3. **調整阻尼** - `Linear Damping` 和 `Angular Damping` 控制物體減速
4. **碰撞過濾** - 使用 `Collision Layer` 和 `Collision Mask` 控制哪些物體會碰撞

## 預期效果

正確設置後，你應該看到：
- 動態物體會因重力下落
- 物體會與地面和平台發生碰撞
- 不同材質的物體會有不同的彈跳和滑動效果
- 物理命令會在指定時機執行

## 故障排除

如果物理模擬不工作：
1. 檢查 GameCoreManager 是否正確初始化
2. 確認物理組件資源已正確應用
3. 檢查物體的質量不為零（動態物體）
4. 確認形狀尺寸設置合理
