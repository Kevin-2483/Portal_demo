# 多段裁切系统使用指南

## 概述

多段裁切系统是对原有传送门系统的重要增强，专门用于处理实体连续穿越多个传送门时的复杂渲染需求。当一个实体穿过两对传送门时，会被分为三段，每段都需要正确的裁切以在不同位置显示。

## 系统架构

### 核心组件

1. **MultiSegmentClippingManager** - 多段裁切管理器
   - 负责计算和管理实体的多段裁切状态
   - 处理裁切平面的优化和LOD
   - 提供调试和性能监控功能

2. **MultiSegmentClippingDescriptor** - 多段裁切描述符
   - 描述单个段的裁切配置
   - 包含裁切平面、透明度、模板缓冲值等

3. **ChainClippingConfig** - 链式裁切配置
   - 针对实体链的完整裁切配置
   - 支持性能优化和质量控制

## 工作原理

### 传统裁切 vs 多段裁切

**传统裁切（单传送门）：**
```
[实体A] -----> [传送门] -----> [实体A']
         单一裁切平面
```

**多段裁切（多传送门）：**
```
[段1] -> [传送门1] -> [段2] -> [传送门2] -> [段3]
  ↑        裁切平面1    ↑       裁切平面2    ↑
原始位置                中间位置            目标位置
```

### 裁切计算

1. **节点间裁切平面计算**
   ```cpp
   // 为相邻链节点计算中点裁切平面
   Vector3 midpoint = (node1.position + node2.position) * 0.5f;
   Vector3 direction = (node2.position - node1.position).normalized();
   ClippingPlane plane = ClippingPlane::from_point_and_normal(midpoint, direction);
   ```

2. **段裁切规则**
   - 第i个段受到第i-1和第i个裁切平面影响
   - 前方平面裁掉前面部分，后方平面裁掉后面部分
   - 主体段完全不透明，其他段根据距离调整透明度

## 使用方法

### 基本集成

```cpp
#include "rendering/multi_segment_clipping.h"

// 1. 创建TeleportManager（自动集成多段裁切）
TeleportManager teleport_manager(physics_data, physics_manipulator, event_handler);

// 2. 实体穿越多个传送门时，系统自动启用多段裁切
// 无需额外代码，系统会在detect到长链时自动处理

// 3. 可选：手动配置裁切质量
teleport_manager.set_entity_clipping_quality(entity_id, 3); // 最高质量
teleport_manager.set_multi_segment_smooth_transitions(entity_id, true, 0.5f);
```

### 高级配置

```cpp
// 启用调试模式
teleport_manager.set_multi_segment_clipping_debug_mode(true);

// 获取统计信息
auto stats = teleport_manager.get_multi_segment_clipping_stats();
std::cout << "Active entities: " << stats.active_multi_segment_entities << std::endl;
std::cout << "Total clipping planes: " << stats.total_clipping_planes << std::endl;

// LOD控制
Vector3 camera_pos = get_camera_position();
int visible_segments = teleport_manager.get_entity_visible_segment_count(entity_id, camera_pos);
```

## 质量级别

| 级别 | 描述 | 特性 |
|------|------|------|
| 0 | 最低质量 | 批量渲染，无平滑过渡，最多2段 |
| 1 | 低质量 | 批量渲染，无平滑过渡，最多4段 |
| 2 | 中等质量 | 批量渲染，启用平滑过渡，最多6段 |
| 3 | 最高质量 | 个别渲染，完整平滑过渡，最多8段 |

## 性能优化

### 自动优化

1. **平面合并** - 移除近似平行的冗余裁切平面
2. **LOD系统** - 根据相机距离调整可见段数
3. **批量渲染** - 支持批量设置裁切状态
4. **智能更新** - 仅在链状态改变时重新计算

### 性能监控

```cpp
auto stats = manager.get_multi_segment_clipping_stats();
std::cout << "Frame setup time: " << stats.frame_setup_time_ms << "ms" << std::endl;
std::cout << "Average segments per entity: " << stats.average_segments_per_entity << std::endl;
```

## 调试功能

### 调试模式

启用调试模式后，系统会：
- 为不同段分配不同调试颜色
- 生成裁切平面可视化数据
- 输出详细的裁切计算信息

```cpp
manager.set_multi_segment_clipping_debug_mode(true);
```

### 调试输出示例

```
MultiSegmentClippingManager: Setting up multi-segment clipping for entity 2001 with 3 segments
MultiSegmentClippingManager: Generated 2 clipping planes for 3 chain segments
  EnhancedMockPhysics: Setting multi-segment clipping for 1 entities
    Entity 10000: clipping plane normal(1, 0, 0), distance: 15
    Entity 10000: clipping enabled
```

## 渲染引擎集成

### 物理引擎接口扩展

需要实现以下接口以支持多段裁切：

```cpp
// 批量设置裁切状态
virtual void set_entities_clipping_states(
    const std::vector<EntityId>& entity_ids,
    const std::vector<ClippingPlane>& clipping_planes,
    const std::vector<bool>& enable_clipping) = 0;

// 设置实体透明度（可选）
virtual void set_entity_alpha(EntityId entity_id, float alpha) = 0;

// 设置模板缓冲值（可选）
virtual void set_entity_stencil_value(EntityId entity_id, int stencil_value) = 0;
```

### Shader支持

渲染器需要支持：
1. **多重裁切平面** - 同时处理多个裁切平面
2. **模板缓冲** - 用于复杂的段分离
3. **透明度混合** - 支持段间的平滑过渡

## 常见问题

### Q: 什么时候启用多段裁切？
A: 当实体的链长度超过1时（即穿越多个传送门），系统自动启用多段裁切。

### Q: 如何优化性能？
A: 
- 使用较低的质量级别
- 启用批量渲染
- 适当设置最大可见段数
- 根据相机距离动态调整

### Q: 如何处理复杂的几何体？
A: 
- 确保实体有正确的包围盒设置
- 使用高质量级别获得更精确的裁切
- 启用平滑过渡减少视觉跳跃

## 扩展功能

### 未来增强

1. **动态LOD** - 基于GPU负载动态调整质量
2. **预计算优化** - 预计算常见传送门组合的裁切配置
3. **异步计算** - 在后台线程计算复杂的裁切配置
4. **AI辅助** - 使用机器学习优化裁切质量和性能平衡

### 自定义扩展点

```cpp
// 自定义相机位置获取
manager.set_camera_position_callback([](){ 
    return get_current_camera_position(); 
});

// 自定义质量评估
manager.set_quality_assessor([](EntityId id, const Vector3& camera_pos) {
    return calculate_optimal_quality_level(id, camera_pos);
});
```

## 总结

多段裁切系统为传送门效果提供了专业级的渲染支持，能够正确处理复杂的多传送门场景。通过合理的配置和优化，可以在保持高质量视觉效果的同时维持良好的性能表现。

系统的设计充分考虑了向后兼容性，现有的单传送门代码无需修改即可受益于新的多段裁切能力。
