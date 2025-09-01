# 多段裁切系统：原理与算法详解

## 1. 问题背景

### 传统单一传送门的裁切

在传统的传送门系统中，当一个物体（比如玩家）穿越一个传送门时，需要进行简单的二分裁切：

```
传送门前的部分 | 传送门平面 | 传送门后的部分
     A段       |    裁切面   |      B段
```

- **A段**：显示在原位置
- **B段**：显示在目标传送门位置

这种情况下，只需要**一个裁切平面**就能完成渲染。

### 多传送门链式穿越的挑战

当物体连续穿越多个传送门时，情况变得复杂：

```
玩家穿越路径：
位置1 → [传送门P1-P2] → 位置2 → [传送门P3-P4] → 位置3 → [传送门P5-P6] → 位置4

物体被分割成多段：
[段1@位置1] - [段2@位置2] - [段3@位置3] - [段4@位置4]
```

现在需要**多个裁切平面**来正确渲染每一段。

## 2. 核心算法原理

### 2.1 链式节点数据结构

每个穿越传送门的实体都维护一个**链状态**：

```cpp
struct EntityChainState {
    EntityId original_entity_id;         // 原始实体ID
    std::vector<EntityChainNode> chain;  // 链节点列表
    int main_position;                   // 主体位置（质心所在）
    // ... 其他状态
};

struct EntityChainNode {
    EntityId entity_id;        // 节点实体ID
    EntityType entity_type;    // MAIN 或 GHOST
    PortalId entry_portal;     // 进入的传送门
    PortalId exit_portal;      // 退出的传送门
    Transform transform;       // 位置变换
    
    // 裁切相关
    bool requires_clipping;
    ClippingPlane clipping_plane;
    float clipping_ratio;
    // ...
};
```

### 2.2 链扩展算法

当实体进入新的传送门时，链会自动扩展：

```cpp
bool extend_entity_chain(EntityId original_entity_id, 
                        EntityId extending_node_id,
                        PortalId entry_portal, 
                        PortalId exit_portal) {
    
    // 1. 获取链状态
    EntityChainState* chain_state = get_or_create_chain_state(original_entity_id);
    
    // 2. 检查是否已有相同出口的节点（避免重复）
    bool already_has_exit_node = check_existing_exit(chain_state, exit_portal);
    if (already_has_exit_node) return false;
    
    // 3. 创建新节点的变换状态
    Transform node_transform;
    PhysicsState node_physics;
    calculate_chain_node_state(*chain_state, entry_portal, exit_portal, 
                              node_transform, node_physics);
    
    // 4. 创建物理实体
    EntityId new_node_id = physics_manipulator_->create_chain_node_entity({
        .source_entity_id = extending_node_id,
        .target_transform = node_transform,
        .target_physics = node_physics,
        .through_portal = entry_portal,
        .full_functionality = true
    });
    
    // 5. 添加到链中
    EntityChainNode new_node = {
        .entity_id = new_node_id,
        .entity_type = EntityType::GHOST,
        .entry_portal = entry_portal,
        .exit_portal = exit_portal,
        .chain_position = chain_state->chain.size(),
        .transform = node_transform
    };
    
    chain_state->chain.push_back(new_node);
    
    return true;
}
```

### 2.3 主体位置迁移算法

当实体的质心穿越传送门时，主体位置需要迁移到链的下一个节点：

```cpp
bool shift_main_entity_position(EntityId original_entity_id, 
                               int new_main_position) {
    
    EntityChainState& chain_state = entity_chains_[original_entity_id];
    
    // 1. 验证新位置的有效性
    if (new_main_position < 0 || 
        new_main_position >= chain_state.chain.size()) {
        return false;
    }
    
    // 2. 更新实体类型标记
    int old_main_position = chain_state.main_position;
    
    // 旧主体变为幽灵
    if (old_main_position >= 0) {
        chain_state.chain[old_main_position].entity_type = EntityType::GHOST;
    }
    
    // 新主体激活
    chain_state.chain[new_main_position].entity_type = EntityType::MAIN;
    chain_state.main_position = new_main_position;
    
    // 3. 物理引擎角色交换
    EntityId old_main_entity = chain_state.chain[old_main_position].entity_id;
    EntityId new_main_entity = chain_state.chain[new_main_position].entity_id;
    
    physics_manipulator_->swap_entity_roles_with_faces(
        old_main_entity, new_main_entity, 
        PortalFace::A, PortalFace::B);
    
    // 4. 触发事件通知
    trigger_role_swap_event(old_main_entity, new_main_entity);
    
    return true;
}
```

## 3. 多段裁切算法

### 3.1 裁切平面计算

为链中相邻节点之间生成裁切平面：

```cpp
std::vector<ClippingPlane> calculate_inter_node_clipping_planes(
    const EntityChainState& chain_state) {
    
    std::vector<ClippingPlane> planes;
    
    // 为每对相邻节点生成分割平面
    for (size_t i = 0; i < chain_state.chain.size() - 1; ++i) {
        const EntityChainNode& current = chain_state.chain[i];
        const EntityChainNode& next = chain_state.chain[i + 1];
        
        // 计算中点和法向量
        Vector3 current_pos = current.transform.position;
        Vector3 next_pos = next.transform.position;
        Vector3 midpoint = (current_pos + next_pos) * 0.5f;
        Vector3 direction = (next_pos - current_pos).normalized();
        
        // 创建垂直分割平面
        ClippingPlane plane = ClippingPlane::from_point_and_normal(
            midpoint, direction);
        
        planes.push_back(plane);
    }
    
    return planes;
}
```

### 3.2 多段裁切描述符生成

为每个链节点生成对应的裁切描述符：

```cpp
void generate_multi_segment_descriptors(const EntityChainState& chain_state,
                                       const std::vector<ClippingPlane>& inter_planes) {
    
    for (size_t i = 0; i < chain_state.chain.size(); ++i) {
        const EntityChainNode& node = chain_state.chain[i];
        MultiSegmentClippingDescriptor descriptor;
        
        descriptor.entity_id = node.entity_id;
        
        // 规则：第i个节点受到第i-1和第i个裁切平面的影响
        
        // 前方裁切平面（裁掉前面的部分）
        if (i > 0 && i - 1 < inter_planes.size()) {
            descriptor.clipping_planes.push_back(inter_planes[i - 1]);
            descriptor.plane_enabled.push_back(true);
        }
        
        // 后方裁切平面（裁掉后面的部分）
        if (i < inter_planes.size()) {
            ClippingPlane back_plane = inter_planes[i];
            // 反向法线，裁掉后面
            back_plane.normal = back_plane.normal * -1.0f;
            back_plane.distance = -back_plane.distance;
            
            descriptor.clipping_planes.push_back(back_plane);
            descriptor.plane_enabled.push_back(true);
        }
        
        // 设置透明度和渲染属性
        setup_rendering_attributes(descriptor, i, chain_state);
        
        segment_descriptors.push_back(descriptor);
    }
}
```

### 3.3 裁切平面优化

移除冗余的近似平行平面：

```cpp
void optimize_clipping_planes(std::vector<ClippingPlane>& planes) {
    auto it = planes.begin();
    while (it != planes.end()) {
        bool should_remove = false;
        
        // 检查是否与其他平面近似平行
        for (auto other = planes.begin(); other != planes.end(); ++other) {
            if (it != other && are_planes_nearly_parallel(*it, *other)) {
                // 保留距离更保守的平面
                if (abs(it->distance) < abs(other->distance)) {
                    should_remove = true;
                    break;
                }
            }
        }
        
        it = should_remove ? planes.erase(it) : (it + 1);
    }
}

bool are_planes_nearly_parallel(const ClippingPlane& p1, 
                               const ClippingPlane& p2, 
                               float tolerance = 0.95f) {
    float dot_product = abs(p1.normal.dot(p2.normal));
    return dot_product >= tolerance;
}
```

## 4. 渲染集成算法

### 4.1 模板缓冲区技术

使用模板缓冲区确保每个段只渲染在正确的区域：

```cpp
void render_multi_segment_entity(const MultiSegmentClippingDescriptor& desc) {
    
    // 1. 清除模板缓冲区
    glClear(GL_STENCIL_BUFFER_BIT);
    
    // 2. 为每个裁切平面设置模板测试
    for (size_t i = 0; i < desc.clipping_planes.size(); ++i) {
        if (!desc.plane_enabled[i]) continue;
        
        const ClippingPlane& plane = desc.clipping_planes[i];
        int stencil_value = desc.segment_stencil_values[i];
        
        // 设置模板写入
        glStencilFunc(GL_ALWAYS, stencil_value, 0xFF);
        glStencilOp(GL_KEEP, GL_KEEP, GL_REPLACE);
        
        // 渲染裁切平面到模板缓冲区
        render_clipping_plane_to_stencil(plane);
    }
    
    // 3. 使用模板测试渲染实体
    glStencilFunc(GL_EQUAL, final_stencil_value, 0xFF);
    glStencilOp(GL_KEEP, GL_KEEP, GL_KEEP);
    
    // 4. 设置裁切平面到GPU
    for (size_t i = 0; i < desc.clipping_planes.size(); ++i) {
        set_gpu_clipping_plane(i, desc.clipping_planes[i]);
    }
    
    // 5. 渲染实体几何体
    render_entity_geometry(desc.entity_id, desc.segment_alpha[0]);
}
```

### 4.2 LOD (Level of Detail) 算法

基于相机距离动态调整可见段数：

```cpp
int calculate_visible_segments(const EntityChainState& chain_state,
                              const Vector3& camera_position,
                              const ClippingConfig& config) {
    
    int visible_count = 0;
    
    for (const auto& node : chain_state.chain) {
        // 计算到相机的距离
        float distance = (node.transform.position - camera_position).length();
        
        // 距离衰减因子
        float distance_factor = std::max(0.1f, 1.0f - (distance * 0.01f));
        
        // 可见性阈值测试
        float visibility = node.base_alpha * distance_factor;
        
        if (visibility >= config.min_segment_visibility_threshold) {
            visible_count++;
        }
        
        // LOD限制
        if (visible_count >= config.max_visible_segments) {
            break;
        }
    }
    
    return visible_count;
}
```

## 5. 性能优化策略

### 5.1 批量渲染

将相似的段合并渲染：

```cpp
void batch_render_segments(const std::vector<MultiSegmentClippingDescriptor>& descriptors) {
    
    // 按材质和渲染状态分组
    std::map<RenderStateKey, std::vector<const MultiSegmentClippingDescriptor*>> batches;
    
    for (const auto& desc : descriptors) {
        RenderStateKey key = extract_render_state(desc);
        batches[key].push_back(&desc);
    }
    
    // 批量渲染每个组
    for (const auto& [state, batch] : batches) {
        setup_render_state(state);
        
        for (const auto* desc : batch) {
            quick_render_entity(desc->entity_id);
        }
    }
}
```

### 5.2 时间片段更新

避免在单帧内更新所有链：

```cpp
void update_chains_time_sliced(float delta_time) {
    static size_t update_index = 0;
    const size_t max_updates_per_frame = 3;
    
    size_t updates_this_frame = 0;
    size_t total_chains = entity_chains_.size();
    
    auto it = entity_chains_.begin();
    std::advance(it, update_index % total_chains);
    
    while (updates_this_frame < max_updates_per_frame && 
           updates_this_frame < total_chains) {
        
        update_single_chain(it->second, delta_time);
        
        ++it;
        ++updates_this_frame;
        if (it == entity_chains_.end()) {
            it = entity_chains_.begin();
        }
    }
    
    update_index = (update_index + updates_this_frame) % total_chains;
}
```

## 6. 实际应用示例

### 场景：玩家连续穿越3个传送门

```
初始状态: 玩家@位置A

步骤1: 玩家进入传送门1
链状态: [玩家@位置A, 幽灵1@位置B]
裁切: 1个平面，将玩家分成2段

步骤2: 玩家质心穿过传送门1  
链状态: [原体@位置A, 玩家@位置B] (角色交换)
裁切: 1个平面保持

步骤3: 玩家进入传送门2
链状态: [原体@位置A, 玩家@位置B, 幽灵2@位置C]
裁切: 2个平面，将玩家分成3段

步骤4: 玩家质心穿过传送门2
链状态: [原体@位置A, 幽灵@位置B, 玩家@位置C] (角色交换)
裁切: 2个平面保持

步骤5: 原体退出传送门1
链状态: [幽灵@位置B, 玩家@位置C] (收缩)
裁切: 1个平面，回到2段

步骤6: 玩家进入传送门3
链状态: [幽灵@位置B, 玩家@位置C, 幽灵3@位置D]
裁切: 2个平面，3段显示
```

每一步都有对应的裁切平面自动计算和GPU状态更新。

## 7. 总结

多段裁切系统的核心算法包括：

1. **链状态管理**：动态维护实体的多位置链条
2. **裁切平面计算**：自动生成节点间的分割平面
3. **主体迁移**：质心穿越时的无感角色交换
4. **渲染优化**：模板缓冲区、LOD、批量渲染
5. **性能控制**：时间片段、平面优化、可见性剔除

这个系统能够处理任意数量传送门的连续穿越，自动计算正确的裁切平面，并提供高性能的渲染支持。
