# Portal Console - 測試指南

Portal Console 是一個交互式測試程序，用於驗證傳送門庫的功能。它模擬了遊戲引擎的角色，負責碰撞檢測和觸發器管理，然後調用傳送門庫進行數學變換。

## 啟動程序

```bash
cd portal_core/build
./portal_console
```

## 基本概念

### 工作流程
1. **控制台**（模擬引擎）：管理實體和傳送門，檢測碰撞
2. **傳送門庫**：接收傳送請求，執行數學變換
3. **控制台**：應用庫計算的新物理狀態

### 座標系統
- X: 左右（正值向右）
- Y: 上下（正值向上）  
- Z: 前後（正值向前）

## 指令參考

### 傳送門管理

```bash
# 創建傳送門
create_portal <name> <x> <y> <z> <nx> <ny> <nz> [width] [height]
cp <name> <x> <y> <z> <nx> <ny> <nz> [width] [height]

# 示例：在 X=-5 位置創建朝向右方的傳送門
cp left_portal -5 0 0 1 0 0 2 3

# 連接傳送門
link_portals <portal1> <portal2>
link <portal1> <portal2>

# 示例
link left_portal right_portal

# 列出所有傳送門
list_portals
lp

# 銷毀傳送門
destroy_portal <name>
dp <name>
```

### 實體管理

```bash
# 創建實體
create_entity <name> <x> <y> <z>
ce <name> <x> <y> <z>

# 示例：創建一個玩家實體
ce player 0 0 0

# 移動實體
move_entity <name> <x> <y> <z>
move <name> <x> <y> <z>

# 設置實體速度
set_velocity <name> <vx> <vy> <vz>
vel <name> <vx> <vy> <vz>

# 查看實體信息
info <name>

# 列出所有實體
list_entities
le
```

### 傳送門物理

```bash
# 設置傳送門速度（用於測試移動傳送門）
set_portal_velocity <portal> <vx> <vy> <vz> [avx] [avy] [avz]
pvel <portal> <vx> <vy> <vz> [avx] [avy] [avz]

# 示例：讓傳送門向右移動
pvel left_portal 5 0 0
```

### 傳送操作

```bash
# 手動傳送
teleport <entity> <source_portal> <target_portal>
tp <entity> <source_portal> <target_portal>

# 速度感知傳送（考慮傳送門速度）
teleport_with_velocity <entity> <source_portal> <target_portal>
tpv <entity> <source_portal> <target_portal>
```

### 測試和模擬

```bash
# 調試碰撞檢測
debug_collision <entity> <portal>
dbg <entity> <portal>

# 模擬引擎級碰撞檢測
simulate_collision <duration> [fps]
scol <duration> [fps]

# 基本物理模擬
simulate <duration> [fps]
sim <duration> [fps]

# 系統更新
update [count]
u [count]
```

### 系統信息

```bash
# 系統狀態
status

# 幫助信息
help
h

# 退出程序
exit
quit
```

## 測試場景

### 場景 1：基本傳送門功能

**目標**：驗證靜態傳送門基本工作

```bash
# 創建一對傳送門
cp left -10 0 0 1 0 0
cp right 10 0 0 -1 0 0
link left right

# 創建測試實體
ce player 0 0 0

# 手動測試傳送
tp player left right
info player

# 期望結果：實體出現在右側傳送門附近
```

### 場景 2：速度變換測試

**目標**：驗證速度正確變換

```bash
# 使用場景1的設置
vel player 5 0 0  # 向右移動
info player

# 使用速度感知傳送
tpv player left right
info player

# 期望結果：速度變為 (-5, 0, 0)，方向反轉
```

### 場景 3：移動傳送門測試

**目標**：測試移動傳送門的速度影響

```bash
# 創建新傳送門對
cp moving -5 0 5 1 0 0
cp static 5 0 5 -1 0 0
link moving static

# 設置傳送門速度
pvel moving 10 0 0

# 創建靜態實體
ce obstacle -3 0 5

# 使用引擎級碰撞檢測模擬
scol 2.0 60

# 期望結果：移動傳送門撞到實體，實體被傳送並獲得速度
```

### 場景 4：碰撞檢測調試

**目標**：分析碰撞檢測的詳細信息

```bash
# 創建測試環境
cp test_portal 0 0 0 0 0 1
ce test_entity 0 0 -2

# 調試碰撞信息
dbg test_entity test_portal

# 移動實體到不同位置並重複測試
move test_entity 0 0 2
dbg test_entity test_portal
```

### 場景 5：複雜速度場景

**目標**：測試複雜的速度組合

```bash
# 創建不同速度的傳送門
cp fast_source -8 2 0 1 0 0
cp slow_target 8 2 0 -1 0 0
link fast_source slow_target

# 設置不同速度
pvel fast_source 15 0 0
pvel slow_target -5 0 0

# 創建高速移動的實體
ce bullet 0 2 0
vel bullet 20 0 0

# 長時間模擬觀察行為
scol 3.0 120
```

## 測試技巧

### 1. 逐步調試
- 先用 `dbg` 命令分析幾何關係
- 再用 `scol` 模擬碰撞檢測
- 最後檢查結果狀態

### 2. 速度測試
- 使用 `tpv` 而不是 `tp` 來測試速度變換
- 比較傳送前後的速度變化
- 注意傳送門法向量對速度方向的影響

### 3. 移動傳送門
- 確保實體在傳送門路徑上但不跨越平面
- 使用較高的傳送門速度以確保檢測
- 用較小的時間步長提高精度

### 4. 性能測試
- 用 `sim` 命令測試長時間運行穩定性
- 觀察 `status` 中的實體計數變化
- 注意內存使用情況

## 常見問題排解

### 問題：實體沒有被傳送
**檢查**：
- 傳送門是否正確連接：`lp`
- 實體位置是否合理：`info <entity>`
- 使用 `dbg` 分析碰撞幾何

**解決**：
- 確保實體在傳送門觸發範圍內
- 檢查傳送門法向量設置是否正確
- 嘗試手動傳送：`tp <entity> <source> <target>`

### 問題：速度計算不正確
**檢查**：
- 使用 `tpv` 而不是 `tp`
- 檢查傳送門方向設置
- 比較傳送前後的速度值

### 問題：移動傳送門碰撞檢測失敗
**檢查**：
- 確保實體不是已經跨越傳送門平面
- 增加傳送門速度：`pvel <portal> <higher_speed> 0 0`
- 使用更高的 FPS：`scol <duration> 120`

### 問題：系統不穩定
**檢查**：
- 查看 `status` 中的實體數量
- 檢查是否有內存洩漏
- 減少同時進行的傳送數量

## 進階功能

### 自定義測試腳本
可以將常用的測試指令保存為腳本文件：

```bash
# test_basic.txt
cp left -5 0 0 1 0 0
cp right 5 0 0 -1 0 0  
link left right
ce player 0 0 0
vel player 3 0 0
tpv player left right
info player
```

### 性能基準測試
```bash
# 創建多個實體進行壓力測試
ce entity1 -1 0 0
ce entity2 -1 1 0
ce entity3 -1 2 0
# ... 更多實體

# 長時間高頻率模擬
scol 10.0 120
```

### 邊界條件測試
```bash
# 測試極小速度
vel player 0.001 0 0

# 測試極大速度  
vel player 1000 0 0

# 測試零大小傳送門
cp tiny 0 0 0 1 0 0 0.01 0.01
```

---

**提示**：使用 `help` 命令可以隨時查看所有可用指令的簡要說明。