# Portal Demo 物理事件系统集成指南

## 概述

本文档描述如何将 Portal Demo 的统一事件管理系统与 Jolt Physics 引擎深度集成，实现懒加载、零性能开销的物理行为事件化。

## 目录

1. [系统架构](#系统架构)
2. [事件类型映射](#事件类型映射)
3. [集成策略](#集成策略)
4. [实现细节](#实现细节)
5. [性能优化](#性能优化)
6. [使用示例](#使用示例)

## 系统架构

### 核心组件关系

```
PhysicsWorldManager (Jolt封装)
        ↓
ContactListener & ActivationListener (Jolt回调)
        ↓
EventManager (统一事件入口)
        ↓
┌─────────────────┬─────────────────┬─────────────────┐
│   立即事件       │   队列事件       │   实体事件       │
│ (Dispatcher)    │ (Queued)       │ (Entity-based)  │
└─────────────────┴─────────────────┴─────────────────┘
        ↓
应用层事件处理器 (音效、粒子、逻辑等)
```

### 分层架构设计

```cpp
// 第1层: Jolt Physics 原生层
ContactListener::OnContactAdded/Removed/Persisted
BodyActivationListener::OnBodyActivated/Deactivated
NarrowPhaseQuery::CastRay/CollideShape/CollideSphere

// 第2层: 事件适配层 (新增)
PhysicsEventAdapter {
    - 将Jolt回调转换为统一事件
    - 实现懒加载机制
    - 管理事件过滤
}

// 第3层: 事件管理层 (现有)
EventManager {
    - 统一事件分发
    - 对象池管理
    - 并发处理
}

// 第4层: 应用逻辑层
Game Systems & Components
```

## 事件类型映射

### 🟢 立即事件 (零延迟响应)

| 物理行为 | Jolt原生支持 | 事件类型 | 使用场景 |
|---------|-------------|----------|----------|
| **碰撞开始** | `ContactListener::OnContactAdded` | `CollisionStartEvent` | 音效、粒子效果 |
| **碰撞结束** | `ContactListener::OnContactRemoved` | `CollisionEndEvent` | 停止音效、清理效果 |
| **触发器进入** | `ContactListener::OnContactAdded` (IsSensor) | `TriggerEnterEvent` | 区域检测、交互提示 |
| **触发器退出** | `ContactListener::OnContactRemoved` (IsSensor) | `TriggerExitEvent` | 离开区域、清理状态 |

### 🟡 队列事件 (批量处理)

| 物理行为 | Jolt原生支持 | 事件类型 | 使用场景 |
|---------|-------------|----------|----------|
| **射线检测** | `NarrowPhaseQuery::CastRay` | `RaycastResultEvent` | 瞄准、路径检测 |
| **形状查询** | `NarrowPhaseQuery::CollideShape` | `ShapeQueryEvent` | 复杂碰撞检测 |
| **物体激活** | `BodyActivationListener::OnBodyActivated` | `BodyActivationEvent` | LOD切换、优化控制 |
| **距离查询** | 基于Jolt查询 | `DistanceQueryEvent` | 最近敌人、交互范围 |

### 🔵 实体事件 (持续状态)

| 物理行为 | 实现方式 | 事件组件 | 使用场景 |
|---------|---------|----------|----------|
| **区域监控** | `BroadPhaseQuery::CollideSphere` | `AreaMonitorComponent` | 伤害区域、影响范围 |
| **包含检测** | `BroadPhaseQuery::CollideAABox` | `ContainmentComponent` | 房间检测、边界判断 |
| **相交** | 几何计算 + Jolt查询 | `PlaneIntersectionComponent` | 水面检测、平台判断 |
| **持续碰撞** | `ContactListener::OnContactPersisted` | `PersistentContactComponent` | 摩擦效果、持续伤害 |



## 集成策略

### 1. 懒加载集成原则

```cpp
// 原则1: 按需创建物理组件
class LazyPhysicsIntegration {
    // 只有需要事件时才添加PhysicsEventComponent
    void enable_collision_events_if_needed(entt::entity entity) {
        if (!registry.all_of<PhysicsEventComponent>(entity)) {
            auto& physics_events = registry.emplace<PhysicsEventComponent>(entity);
            physics_events.enable_collision_events = true;
        }
    }
    
    // 只有需要查询时才添加QueryComponent
    void enable_area_monitoring_if_needed(entt::entity entity, Vec3 center, float radius) {
        if (!registry.all_of<PhysicsQueryComponent>(entity)) {
            auto& query_comp = registry.emplace<PhysicsQueryComponent>(entity);
            query_comp.add_sphere_overlap(center, radius);
        }
    }
};
```

### 2. 零开销事件路由

```cpp
// 原则2: 事件路由不产生额外性能开销
class PhysicsEventRouter {
    // 直接路由，无中间转换
    void route_collision_event(BodyID body1, BodyID body2, const ContactPoint& contact) {
        auto entity1 = body_id_to_entity(body1);
        auto entity2 = body_id_to_entity(body2);
        
        // 检查是否为传感器 (Jolt原生判断，零开销)
        if (physics_world->get_body(body1).IsSensor() || 
            physics_world->get_body(body2).IsSensor()) {
            // 立即事件: 触发器
            event_manager.publish_immediate(TriggerEvent{entity1, entity2, contact.point});
        } else {
            // 立即事件: 普通碰撞
            event_manager.publish_immediate(CollisionEvent{entity1, entity2, contact});
        }
    }
};
```

### 3. 分层过滤集成

```cpp
// 原则3: 利用Jolt多层过滤避免无用计算
class LayeredPhysicsFiltering {
    // 设置专用触发器层，避免与无关物体的宽相位检测
    void setup_trigger_layers() {
        // 触发器只与DYNAMIC物体交互
        object_layer_pair_filter.enable_collision(TRIGGER_LAYER, DYNAMIC_LAYER);
        object_layer_pair_filter.disable_collision(TRIGGER_LAYER, STATIC_LAYER);
        object_layer_pair_filter.disable_collision(TRIGGER_LAYER, TRIGGER_LAYER);
    }
};
```

## 实现细节

### 1. 物理事件适配器

```cpp
class PhysicsEventAdapter {
private:
    EventManager& event_manager_;
    PhysicsWorldManager& physics_world_;
    entt::registry& registry_;
    
public:
    PhysicsEventAdapter(EventManager& em, PhysicsWorldManager& pw, entt::registry& reg)
        : event_manager_(em), physics_world_(pw), registry_(reg) {
        setup_jolt_callbacks();
    }
    
private:
    void setup_jolt_callbacks() {
        // 设置Jolt碰撞回调
        physics_world_.set_contact_added_callback([this](BodyID body1, BodyID body2) {
            handle_contact_added(body1, body2);
        });
        
        physics_world_.set_contact_removed_callback([this](BodyID body1, BodyID body2) {
            handle_contact_removed(body1, body2);
        });
        
        // 设置激活回调
        physics_world_.set_body_activated_callback([this](BodyID body_id, uint64 user_data) {
            handle_body_activated(body_id, user_data);
        });
    }
    
    void handle_contact_added(BodyID body1, BodyID body2) {
        auto entity1 = body_id_to_entity(body1);
        auto entity2 = body_id_to_entity(body2);
        
        if (entity1 == entt::null || entity2 == entt::null) return;
        
        // 获取接触信息
        auto contact_info = get_contact_info(body1, body2);
        
        // 判断是否为传感器事件
        bool is_sensor1 = physics_world_.get_body_interface().GetBodyCreationSettings(body1).mIsSensor;
        bool is_sensor2 = physics_world_.get_body_interface().GetBodyCreationSettings(body2).mIsSensor;
        
        if (is_sensor1 || is_sensor2) {
            // 触发器事件 - 立即处理
            auto trigger_event = TriggerEnterEvent{
                .sensor_entity = is_sensor1 ? entity1 : entity2,
                .other_entity = is_sensor1 ? entity2 : entity1,
                .contact_point = contact_info.point,
                .contact_normal = contact_info.normal
            };
            event_manager_.publish_immediate(trigger_event);
            
            // 同时处理持续监控 (实体事件)
            handle_area_monitoring(trigger_event.sensor_entity, trigger_event.other_entity, true);
        } else {
            // 普通碰撞事件 - 立即处理
            auto collision_event = CollisionStartEvent{
                .entity_a = entity1,
                .entity_b = entity2,
                .contact_point = contact_info.point,
                .contact_normal = contact_info.normal,
                .impact_force = contact_info.impulse_magnitude
            };
            event_manager_.publish_immediate(collision_event);
        }
    }
    
    void handle_area_monitoring(entt::entity sensor_entity, entt::entity other_entity, bool entering) {
        // 检查是否有区域监控组件
        if (registry_.all_of<AreaMonitorComponent>(sensor_entity)) {
            auto& monitor = registry_.get<AreaMonitorComponent>(sensor_entity);
            
            if (entering) {
                monitor.entities_in_area.insert(other_entity);
            } else {
                monitor.entities_in_area.erase(other_entity);
            }
            
            // 更新监控状态 (实体事件的优势 - 状态持久化)
            event_manager_.add_component_event(sensor_entity, 
                AreaStatusUpdateComponent{monitor.entities_in_area.size()});
        }
    }
    
    entt::entity body_id_to_entity(BodyID body_id) {
        // 从PhysicsBodyComponent中查找对应实体
        auto view = registry_.view<PhysicsBodyComponent>();
        for (auto entity : view) {
            auto& body_comp = view.get<PhysicsBodyComponent>(entity);
            if (body_comp.body_id == body_id) {
                return entity;
            }
        }
        return entt::null;
    }
};
```

### 2. 懒加载查询管理器

```cpp
class LazyPhysicsQueryManager {
private:
    EventManager& event_manager_;
    PhysicsWorldManager& physics_world_;
    entt::registry& registry_;
    
public:
    // 懒加载射线检测
    void request_raycast(entt::entity requester, Vec3 origin, Vec3 direction, float max_distance) {
        // 检查是否已有查询组件
        if (!registry_.all_of<PhysicsQueryComponent>(requester)) {
            registry_.emplace<PhysicsQueryComponent>(requester);
        }
        
        auto& query_comp = registry_.get<PhysicsQueryComponent>(requester);
        query_comp.add_raycast(origin, direction, max_distance);
        
        // 标记需要处理
        if (!registry_.all_of<PendingQueryTag>(requester)) {
            registry_.emplace<PendingQueryTag>(requester);
        }
    }
    
    // 懒加载区域监控
    void request_area_monitoring(entt::entity requester, Vec3 center, float radius, uint32_t layer_mask = 0xFFFFFFFF) {
        // 按需创建监控组件
        if (!registry_.all_of<AreaMonitorComponent>(requester)) {
            registry_.emplace<AreaMonitorComponent>(requester, center, radius);
        }
        
        // 按需创建查询组件
        if (!registry_.all_of<PhysicsQueryComponent>(requester)) {
            auto& query_comp = registry_.emplace<PhysicsQueryComponent>(requester);
            query_comp.add_sphere_overlap(center, radius, layer_mask);
        }
        
        // 实体事件 - 持续状态管理
        event_manager_.create_entity_event(
            AreaMonitoringActiveComponent{requester, center, radius},
            EventMetadata{.auto_cleanup = false}  // 持续到手动清理
        );
    }
    
    // 处理所有待处理的查询
    void process_pending_queries() {
        auto pending_view = registry_.view<PendingQueryTag, PhysicsQueryComponent>();
        
        for (auto entity : pending_view) {
            auto& query_comp = pending_view.get<PhysicsQueryComponent>(entity);
            
            // 批量处理射线查询
            for (auto& raycast : query_comp.raycast_queries) {
                auto result = physics_world_.raycast(raycast.origin, raycast.direction, raycast.max_distance);
                
                // 队列事件 - 批量处理结果
                event_manager_.enqueue(RaycastResultEvent{
                    .requester = entity,
                    .hit = result.hit,
                    .hit_point = result.hit_point,
                    .hit_normal = result.hit_normal,
                    .hit_distance = result.distance,
                    .hit_entity = body_id_to_entity(result.body_id)
                });
            }
            
            // 批量处理重叠查询
            for (auto& overlap : query_comp.overlap_queries) {
                auto bodies = physics_world_.overlap_sphere(overlap.center, overlap.size.x);
                
                std::vector<entt::entity> overlapping_entities;
                for (auto body_id : bodies) {
                    auto entity = body_id_to_entity(body_id);
                    if (entity != entt::null) {
                        overlapping_entities.push_back(entity);
                    }
                }
                
                // 队列事件 - 批量处理重叠结果
                event_manager_.enqueue(OverlapQueryResultEvent{
                    .requester = entity,
                    .overlapping_entities = std::move(overlapping_entities)
                });
            }
            
            // 清理待处理标记
            registry_.remove<PendingQueryTag>(entity);
        }
    }
};
```

### 3. 事件组件定义

```cpp
// 立即事件结构
struct CollisionStartEvent {
    entt::entity entity_a;
    entt::entity entity_b;
    Vec3 contact_point;
    Vec3 contact_normal;
    float impact_force;
    std::string collision_type = "collision";
};

struct TriggerEnterEvent {
    entt::entity sensor_entity;
    entt::entity other_entity;
    Vec3 contact_point;
    Vec3 contact_normal;
    std::string trigger_type = "trigger_enter";
};

// 队列事件结构
struct RaycastResultEvent {
    entt::entity requester;
    bool hit;
    Vec3 hit_point;
    Vec3 hit_normal;
    float hit_distance;
    entt::entity hit_entity;
};

// 实体事件组件
struct AreaMonitorComponent {
    using is_event_component = void;
    
    Vec3 center;
    float radius;
    std::unordered_set<entt::entity> entities_in_area;
    uint32_t layer_mask = 0xFFFFFFFF;
    bool active = true;
};

struct PlaneIntersectionComponent {
    using is_event_component = void;
    
    Vec3 plane_normal;
    float plane_distance;
    entt::entity monitored_entity;
    bool was_above = true;
    float last_check_time = 0.0f;
};

// 临时标记组件
struct CollisionMarkerComponent {
    using is_event_component = void;
    
    Vec3 collision_direction;
    float impact_magnitude;
    std::string effect_type;
};

struct InvulnerabilityMarker {
    using is_event_component = void;
    
    float remaining_time;
    std::unordered_set<std::string> immunity_types;
};
```

## 性能优化

### 1. Jolt层级优化

```cpp
// 优化1: 使用专用触发器层
void setup_optimized_layers() {
    // 触发器只与动态物体交互，避免无用的宽相位检测
    broad_phase_layer_interface->map_object_to_broad_phase(TRIGGER_LAYER, TRIGGER_BROAD_PHASE);
    
    // 对象层过滤 - 只允许必要的交互
    object_layer_pair_filter->enable_collision(TRIGGER_LAYER, DYNAMIC_LAYER);
    object_layer_pair_filter->disable_collision(TRIGGER_LAYER, STATIC_LAYER);
    object_layer_pair_filter->disable_collision(TRIGGER_LAYER, TRIGGER_LAYER);
}

// 优化2: 静态传感器最高性能
BodyCreationSettings create_optimized_trigger(Vec3 position, float radius) {
    BodyCreationSettings settings;
    settings.mPosition = position;
    settings.mMotionType = EMotionType::Static;  // 最高性能
    settings.mIsSensor = true;                   // 触发器
    settings.mObjectLayer = TRIGGER_LAYER;       // 专用层
    settings.mShape = new SphereShapeSettings(radius);
    return settings;
}
```

### 2. 事件池化优化

```cpp
// 优化3: 事件对象池
class PhysicsEventPoolManager {
    EventPool<CollisionStartEvent> collision_pool_{64};  // 预分配64个碰撞事件
    EventPool<TriggerEnterEvent> trigger_pool_{32};      // 预分配32个触发事件
    EventPool<RaycastResultEvent> raycast_pool_{16};     // 预分配16个射线事件
    
public:
    template<typename TEvent>
    auto acquire_event(auto&&... args) {
        if constexpr (std::is_same_v<TEvent, CollisionStartEvent>) {
            return collision_pool_.acquire(std::forward<decltype(args)>(args)...);
        } else if constexpr (std::is_same_v<TEvent, TriggerEnterEvent>) {
            return trigger_pool_.acquire(std::forward<decltype(args)>(args)...);
        } else if constexpr (std::is_same_v<TEvent, RaycastResultEvent>) {
            return raycast_pool_.acquire(std::forward<decltype(args)>(args)...);
        }
    }
};
```

### 3. 批量查询优化

```cpp
// 优化4: 分帧处理大量查询
class FramedQueryProcessor {
    static constexpr int MAX_QUERIES_PER_FRAME = 100;
    
    void process_queries_gradually() {
        auto pending_view = registry_.view<PendingQueryTag>();
        int processed = 0;
        
        for (auto entity : pending_view) {
            if (processed >= MAX_QUERIES_PER_FRAME) {
                break;  // 分帧处理，避免卡顿
            }
            
            process_entity_queries(entity);
            registry_.remove<PendingQueryTag>(entity);
            ++processed;
        }
    }
};
```

## 使用示例

### 1. 基础碰撞检测

```cpp
void setup_basic_collision_detection() {
    // 设置碰撞事件处理器
    event_manager.subscribe<CollisionStartEvent>().connect([](const CollisionStartEvent& event) {
        // 播放撞击音效
        AudioSystem::play_impact_sound(event.impact_force);
        
        // 创建粒子效果
        ParticleSystem::create_spark_effect(event.contact_point, event.contact_normal);
        
        // 添加碰撞标记 (临时标记，3帧后自动清理)
        event_manager.add_temporary_marker(event.entity_a,
            CollisionMarkerComponent{event.contact_normal, event.impact_force, "impact"}, 3);
    });
}
```

### 2. 传送门区域检测

```cpp
void setup_portal_area_detection(entt::entity portal_entity, Vec3 position, float radius) {
    // 懒加载: 只有需要时才创建传感器
    auto sensor_body = physics_world.create_body(
        PhysicsBodyDesc{
            .body_type = PhysicsBodyType::TRIGGER,  // 使用触发器类型
            .shape = PhysicsShapeDesc::sphere(radius),
            .position = position
        }
    );
    
    // 添加物理组件到实体
    registry.emplace<PhysicsBodyComponent>(portal_entity, sensor_body);
    
    // 设置触发器事件处理
    event_manager.subscribe<TriggerEnterEvent>().connect([portal_entity](const TriggerEnterEvent& event) {
        if (event.sensor_entity == portal_entity) {
            // 实体进入传送门
            handle_portal_entry(event.other_entity, portal_entity);
        }
    });
    
    // 可选: 添加区域监控 (实体事件 - 持续状态)
    lazy_query_manager.request_area_monitoring(portal_entity, position, radius, DYNAMIC_LAYER);
}

void handle_portal_entry(entt::entity entity, entt::entity portal) {
    // 添加传送准备状态 (实体事件)
    event_manager.create_entity_event(
        PortalTransitionComponent{entity, portal, 1.0f},  // 1秒传送时间
        EventMetadata{.auto_cleanup = true, .frame_lifetime = 60}  // 60帧后自动清理
    );
    
    // 添加传送特效标记 (临时标记)
    event_manager.add_temporary_marker(entity,
        TeleportEffectMarker{"swirl", 1.0f}, 60);  // 60帧传送特效
}
```

### 3. 伤害区域持续监控

```cpp
void setup_damage_area(entt::entity damage_source, Vec3 center, float radius, float damage_per_second) {
    // 使用实体事件进行持续状态管理
    auto area_entity = event_manager.create_entity_event(
        DamageAreaComponent{
            .center = center,
            .radius = radius,
            .damage_per_second = damage_per_second,
            .affected_entities = {}
        },
        EventMetadata{.auto_cleanup = false}  // 手动管理生命周期
    );
    
    // 懒加载区域监控
    lazy_query_manager.request_area_monitoring(area_entity, center, radius, DYNAMIC_LAYER);
    
    // 处理区域更新事件
    event_manager.subscribe<OverlapQueryResultEvent>().connect([damage_per_second](const OverlapQueryResultEvent& event) {
        if (registry.all_of<DamageAreaComponent>(event.requester)) {
            auto& damage_area = registry.get<DamageAreaComponent>(event.requester);
            
            // 对区域内的实体造成伤害
            for (auto entity : event.overlapping_entities) {
                if (registry.all_of<HealthComponent>(entity)) {
                    // 队列事件 - 批量处理伤害
                    event_manager.enqueue(DamageEvent{
                        .attacker = event.requester,
                        .target = entity,
                        .damage_amount = damage_per_second * delta_time,
                        .damage_type = "area_damage"
                    });
                }
            }
        }
    });
}
```

### 4. 智能射线检测

```cpp
void setup_smart_raycast_system() {
    // 处理射线检测请求
    event_manager.subscribe<RequestRaycastEvent>().connect([](const RequestRaycastEvent& event) {
        // 懒加载: 只在需要时添加查询组件
        lazy_query_manager.request_raycast(
            event.requester,
            event.origin,
            event.direction,
            event.max_distance
        );
    });
    
    // 处理射线检测结果
    event_manager.subscribe<RaycastResultEvent>().connect([](const RaycastResultEvent& event) {
        if (event.hit) {
            // 命中目标 - 添加瞄准标记
            event_manager.add_temporary_marker(event.hit_entity,
                AimTargetMarker{event.hit_point}, 30);  // 30帧瞄准标记
                
            // 可能的伤害处理
            if (registry.all_of<WeaponComponent>(event.requester)) {
                auto& weapon = registry.get<WeaponComponent>(event.requester);
                event_manager.enqueue(DamageEvent{
                    .attacker = event.requester,
                    .target = event.hit_entity,
                    .damage_amount = weapon.damage,
                    .hit_position = event.hit_point,
                    .damage_type = weapon.damage_type
                });
            }
        }
    });
}
```

## 总结

这个集成方案提供了：

1. **零性能开销**: 利用Jolt原生功能，避免额外计算
2. **完全懒加载**: 按需创建组件和查询，内存高效
3. **类型安全**: 强类型事件系统，编译时检查
4. **高度可扩展**: 基于组件的设计，易于添加新功能
5. **性能优化**: 分层过滤、对象池、批量处理
6. **统一接口**: 通过EventManager统一管理所有物理事件

通过这个设计，物理行为的事件化变得简单、高效且易于维护。
