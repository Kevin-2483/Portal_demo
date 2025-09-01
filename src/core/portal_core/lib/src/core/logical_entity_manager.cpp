#include "../../include/core/logical_entity_manager.h"
#include <iostream>
#include <algorithm>
#include <cmath>

namespace Portal
{

LogicalEntityManager::LogicalEntityManager(IPhysicsDataProvider* physics_data,
                                          IPhysicsManipulator* physics_manipulator,
                                          IPortalEventHandler* event_handler)
    : physics_data_(physics_data)
    , physics_manipulator_(physics_manipulator)
    , event_handler_(event_handler)
    , next_logical_id_(1)
    , update_frequency_(60.0f)
    , last_update_time_(0.0f)
{
    if (!physics_data_ || !physics_manipulator_) {
        throw std::invalid_argument("Physics data provider and manipulator are required");
    }
    std::cout << "LogicalEntityManager created" << std::endl;
}

LogicalEntityId LogicalEntityManager::create_logical_entity(EntityId main_entity_id, EntityId ghost_entity_id,
                                                           PhysicsStateMergeStrategy strategy)
{
    LogicalEntityId logical_id = allocate_logical_id();
    
    // 创建逻辑实体状态
    LogicalEntityState logical_state;
    logical_state.logical_id = logical_id;
    logical_state.main_entity_id = main_entity_id;
    logical_state.ghost_entity_id = ghost_entity_id;
    logical_state.merge_strategy = strategy;
    logical_state.physics_unified_mode = true;
    logical_state.ignore_engine_physics = true; // 默认由逻辑实体完全控制
    
    // 根据策略设置权重和配置
    switch (strategy) {
        case PhysicsStateMergeStrategy::MAIN_PRIORITY:
            logical_state.main_weight = 1.0f;
            logical_state.ghost_weight = 0.0f;
            break;
        case PhysicsStateMergeStrategy::GHOST_PRIORITY:
            logical_state.main_weight = 0.0f;
            logical_state.ghost_weight = 1.0f;
            break;
        case PhysicsStateMergeStrategy::WEIGHTED_AVERAGE:
            logical_state.main_weight = 0.5f;
            logical_state.ghost_weight = 0.5f;
            break;
        case PhysicsStateMergeStrategy::FORCE_SUMMATION:
            logical_state.main_weight = 1.0f;
            logical_state.ghost_weight = 1.0f;
            logical_state.use_physics_simulation = true;
            break;
        case PhysicsStateMergeStrategy::PHYSICS_SIMULATION:
            logical_state.main_weight = 0.5f;
            logical_state.ghost_weight = 0.5f;
            logical_state.use_physics_simulation = true;
            break;
        case PhysicsStateMergeStrategy::MOST_RESTRICTIVE:
        default:
            logical_state.main_weight = 0.5f;
            logical_state.ghost_weight = 0.5f;
            break;
    }
    
    // 收集初始物理状态
    if (!merge_physics_states(logical_id)) {
        std::cerr << "LogicalEntityManager: Failed to merge initial physics states for logical entity " 
                  << logical_id << std::endl;
        return INVALID_LOGICAL_ENTITY_ID;
    }
    
    // 存储逻辑实体
    logical_entities_[logical_id] = logical_state;
    
    // 建立映射
    entity_to_logical_mapping_[main_entity_id] = logical_id;
    if (ghost_entity_id != INVALID_ENTITY_ID) {
        entity_to_logical_mapping_[ghost_entity_id] = logical_id;
    }
    
    // 禁用物理引擎对相关实体的直接控制
    physics_manipulator_->set_entity_physics_engine_controlled(main_entity_id, false);
    if (ghost_entity_id != INVALID_ENTITY_ID) {
        physics_manipulator_->set_entity_physics_engine_controlled(ghost_entity_id, false);
    }
    
    // 通知事件处理器
    notify_event_handler([logical_id, main_entity_id, ghost_entity_id](IPortalEventHandler* handler) {
        handler->on_logical_entity_created(logical_id, main_entity_id, ghost_entity_id);
    });
    
    std::cout << "LogicalEntityManager: Created logical entity " << logical_id 
              << " for main=" << main_entity_id << ", ghost=" << ghost_entity_id << std::endl;
    
    return logical_id;
}

void LogicalEntityManager::destroy_logical_entity(LogicalEntityId logical_id)
{
    auto it = logical_entities_.find(logical_id);
    if (it == logical_entities_.end()) {
        return;
    }
    
    const LogicalEntityState& state = it->second;
    EntityId main_entity = state.main_entity_id;
    EntityId ghost_entity = state.ghost_entity_id;
    
    // 恢复物理引擎控制
    physics_manipulator_->set_entity_physics_engine_controlled(main_entity, true);
    if (ghost_entity != INVALID_ENTITY_ID) {
        physics_manipulator_->set_entity_physics_engine_controlled(ghost_entity, true);
    }
    
    // 清理映射
    entity_to_logical_mapping_.erase(main_entity);
    if (ghost_entity != INVALID_ENTITY_ID) {
        entity_to_logical_mapping_.erase(ghost_entity);
    }
    
    // 通知事件处理器
    notify_event_handler([logical_id, main_entity, ghost_entity](IPortalEventHandler* handler) {
        handler->on_logical_entity_destroyed(logical_id, main_entity, ghost_entity);
    });
    
    // 移除逻辑实体
    logical_entities_.erase(it);
    
    std::cout << "LogicalEntityManager: Destroyed logical entity " << logical_id << std::endl;
}

void LogicalEntityManager::update(float delta_time)
{
    last_update_time_ += delta_time;
    
    // 基于频率控制更新
    float update_interval = 1.0f / update_frequency_;
    if (last_update_time_ < update_interval) {
        return;
    }
    
    // 更新所有逻辑实体
    for (auto& [logical_id, state] : logical_entities_) {
        // 根據實體數量選擇合成策略
        if (state.controlled_entities.size() > 2) {
            // 多實體鏈系統：使用高級物理狀態合成
            merge_multi_entity_physics_states(logical_id);
            
            // 分佈約束檢測和協調
            distribute_constraints_across_chain(state);
        } else {
            // 傳統雙實體系統：使用原有合成方法
            merge_physics_states(logical_id);
            
            // 檢測傳統約束
            detect_physics_constraints(logical_id);
        }
        
        // 2. 合成复杂物理属性（力、力矩等）
        if (state.merge_strategy == PhysicsStateMergeStrategy::FORCE_SUMMATION ||
            state.merge_strategy == PhysicsStateMergeStrategy::PHYSICS_SIMULATION) {
            merge_complex_physics_properties(logical_id);
        }
        
        // 3. 物理引擎模拟（如果启用）
        if (state.use_physics_simulation) {
            create_or_update_physics_proxy(logical_id);
            apply_merged_forces_to_proxy(logical_id);
        }
        
        // 4. 应用约束
        apply_physics_constraints(logical_id);
        
        // 5. 从物理代理获取模拟结果（如果使用了代理）
        if (state.has_simulation_proxy) {
            get_simulation_result_from_proxy(logical_id);
        }
        
        // 6. 同步到物理實體
        if (state.controlled_entities.size() > 2) {
            // 多實體鏈同步
            sync_logical_to_chain_entities(logical_id);
        } else {
            // 傳統雙實體同步
            sync_logical_to_entities(logical_id);
        }
    }
    
    last_update_time_ = 0.0f;
    
    std::cout << "LogicalEntityManager: Updated " << logical_entities_.size() 
              << " logical entities" << std::endl;
}

bool LogicalEntityManager::merge_physics_states(LogicalEntityId logical_id)
{
    auto it = logical_entities_.find(logical_id);
    if (it == logical_entities_.end()) {
        return false;
    }
    
    LogicalEntityState& state = it->second;
    
    // 收集主体和幽灵的物理状态
    Transform main_transform, ghost_transform;
    PhysicsState main_physics, ghost_physics;
    
    bool has_main = collect_entity_physics_state(state.main_entity_id, main_transform, main_physics);
    bool has_ghost = (state.ghost_entity_id != INVALID_ENTITY_ID) ? 
                     collect_entity_physics_state(state.ghost_entity_id, ghost_transform, ghost_physics) : false;
    
    if (!has_main && !has_ghost) {
        return false;
    }
    
    // 合成变换
    if (has_main && has_ghost) {
        state.unified_transform = merge_transforms(main_transform, ghost_transform, 
                                                  state.merge_strategy, 
                                                  state.main_weight, state.ghost_weight);
        state.unified_physics = merge_physics_states(main_physics, ghost_physics,
                                                    state.merge_strategy,
                                                    state.main_weight, state.ghost_weight);
    } else if (has_main) {
        state.unified_transform = main_transform;
        state.unified_physics = main_physics;
    } else if (has_ghost) {
        state.unified_transform = ghost_transform;
        state.unified_physics = ghost_physics;
    }
    
    // 通知合成完成
    notify_event_handler([logical_id, state](IPortalEventHandler* handler) {
        handler->on_logical_entity_state_merged(logical_id, state.merge_strategy);
    });
    
    return true;
}

bool LogicalEntityManager::detect_physics_constraints(LogicalEntityId logical_id)
{
    auto it = logical_entities_.find(logical_id);
    if (it == logical_entities_.end()) {
        return false;
    }
    
    LogicalEntityState& state = it->second;
    PhysicsConstraintState main_constraint, ghost_constraint;
    
    // 检测主体约束
    bool main_constrained = physics_manipulator_->detect_entity_collision_constraints(
        state.main_entity_id, main_constraint);
    
    // 检测幽灵约束
    bool ghost_constrained = false;
    if (state.ghost_entity_id != INVALID_ENTITY_ID) {
        ghost_constrained = physics_manipulator_->detect_entity_collision_constraints(
            state.ghost_entity_id, ghost_constraint);
    }
    
    // 合成约束状态
    bool was_constrained = state.constraint_state.is_blocked;
    state.constraint_state.is_blocked = false;
    
    if (main_constrained || ghost_constrained) {
        state.constraint_state.is_blocked = true;
        
        // 根据策略选择约束来源
        if (state.merge_strategy == PhysicsStateMergeStrategy::MOST_RESTRICTIVE) {
            // 使用最严格的约束
            if (main_constrained && ghost_constrained) {
                // 选择约束最严格的一个（简化为选择主体）
                state.constraint_state = main_constraint;
            } else if (main_constrained) {
                state.constraint_state = main_constraint;
            } else {
                state.constraint_state = ghost_constraint;
            }
        } else if (state.merge_strategy == PhysicsStateMergeStrategy::MAIN_PRIORITY && main_constrained) {
            state.constraint_state = main_constraint;
        } else if (state.merge_strategy == PhysicsStateMergeStrategy::GHOST_PRIORITY && ghost_constrained) {
            state.constraint_state = ghost_constraint;
        } else if (main_constrained) {
            state.constraint_state = main_constraint;
        } else {
            state.constraint_state = ghost_constraint;
        }
        
        // 如果刚被约束，发送事件
        if (!was_constrained) {
            notify_event_handler([logical_id, state](IPortalEventHandler* handler) {
                handler->on_logical_entity_constrained(logical_id, state.constraint_state);
            });
        }
    } else if (was_constrained) {
        // 约束被解除
        notify_event_handler([logical_id](IPortalEventHandler* handler) {
            handler->on_logical_entity_constraint_released(logical_id);
        });
    }
    
    return state.constraint_state.is_blocked;
}

void LogicalEntityManager::apply_physics_constraints(LogicalEntityId logical_id)
{
    auto it = logical_entities_.find(logical_id);
    if (it == logical_entities_.end()) {
        return;
    }
    
    LogicalEntityState& state = it->second;
    
    if (!state.constraint_state.is_blocked) {
        return;
    }
    
    // 应用约束到物理状态
    apply_constraint_to_physics(state.unified_physics, state.constraint_state);
}

bool LogicalEntityManager::sync_logical_to_entities(LogicalEntityId logical_id)
{
    auto it = logical_entities_.find(logical_id);
    if (it == logical_entities_.end()) {
        return false;
    }
    
    const LogicalEntityState& state = it->second;
    
    // 同步到主体实体
    physics_manipulator_->force_set_entity_physics_state(
        state.main_entity_id, state.unified_transform, state.unified_physics);
    
    // 同步到幽灵实体
    if (state.ghost_entity_id != INVALID_ENTITY_ID) {
        physics_manipulator_->force_set_entity_physics_state(
            state.ghost_entity_id, state.unified_transform, state.unified_physics);
    }
    
    return true;
}

const LogicalEntityState* LogicalEntityManager::get_logical_entity_state(LogicalEntityId logical_id) const
{
    auto it = logical_entities_.find(logical_id);
    return (it != logical_entities_.end()) ? &it->second : nullptr;
}

LogicalEntityId LogicalEntityManager::get_logical_entity_by_physical_entity(EntityId entity_id) const
{
    auto it = entity_to_logical_mapping_.find(entity_id);
    return (it != entity_to_logical_mapping_.end()) ? it->second : INVALID_LOGICAL_ENTITY_ID;
}

    // === 多實體物理狀態合成內部實現 ===

    bool LogicalEntityManager::merge_multi_entity_physics_states(LogicalEntityId logical_id)
    {
        auto it = logical_entities_.find(logical_id);
        if (it == logical_entities_.end()) {
            return false;
        }
        
        LogicalEntityState& state = it->second;
        
        if (state.controlled_entities.empty()) {
            return false; // 沒有受控實體
        }
        
        // 重新收集所有實體的當前物理狀態
        state.entity_transforms.clear();
        state.entity_physics.clear();
        state.segment_forces.clear();
        state.segment_torques.clear();
        
        state.entity_transforms.resize(state.controlled_entities.size());
        state.entity_physics.resize(state.controlled_entities.size());
        state.segment_forces.resize(state.controlled_entities.size());
        state.segment_torques.resize(state.controlled_entities.size());
        
        // 收集所有實體的物理狀態
        for (size_t i = 0; i < state.controlled_entities.size(); ++i) {
            EntityId entity_id = state.controlled_entities[i];
            
            if (!collect_entity_physics_state(entity_id, state.entity_transforms[i], state.entity_physics[i])) {
                std::cerr << "LogicalEntityManager: Failed to collect physics state for entity " 
                          << entity_id << " in chain " << logical_id << std::endl;
                continue;
            }
            
            // 收集當前作用於該實體的力和力矩
            Vector3 force(0, 0, 0), torque(0, 0, 0);
            if (physics_manipulator_->get_entity_applied_forces(entity_id, force, torque)) {
                state.segment_forces[i] = force;
                state.segment_torques[i] = torque;
            } else {
                state.segment_forces[i] = Vector3(0, 0, 0);
                state.segment_torques[i] = Vector3(0, 0, 0);
            }
        }
        
        // 計算鏈的質心和慣性屬性
        calculate_chain_mass_properties(state);
        
        // 根據策略合成物理狀態
        switch (state.merge_strategy) {
            case PhysicsStateMergeStrategy::FORCE_SUMMATION:
                merge_multi_entity_forces(state);
                break;
                
            case PhysicsStateMergeStrategy::WEIGHTED_AVERAGE:
                merge_multi_entity_weighted_average(state);
                break;
                
            case PhysicsStateMergeStrategy::PHYSICS_SIMULATION:
                merge_multi_entity_physics_simulation(state);
                break;
                
            case PhysicsStateMergeStrategy::MOST_RESTRICTIVE:
                merge_multi_entity_restrictive(state);
                break;
                
            default:
                // 默認使用力求和
                merge_multi_entity_forces(state);
                break;
        }
        
        std::cout << "LogicalEntityManager: Merged multi-entity physics states for logical entity " 
                  << logical_id << " with " << state.controlled_entities.size() << " entities" << std::endl;
        
        return true;
    }

    void LogicalEntityManager::calculate_chain_mass_properties(LogicalEntityState& state)
    {
        state.total_chain_mass = 0.0f;
        Vector3 weighted_position(0, 0, 0);
        
        // 計算總質量和加權位置
        for (size_t i = 0; i < state.controlled_entities.size(); ++i) {
            float mass = state.entity_physics[i].mass;
            float weight = (i < state.entity_weights.size()) ? state.entity_weights[i] : 1.0f;
            
            // 有效質量 = 實際質量 × 權重
            float effective_mass = mass * weight;
            state.total_chain_mass += effective_mass;
            
            weighted_position += state.entity_transforms[i].position * effective_mass;
        }
        
        // 計算質心位置
        if (state.total_chain_mass > 0.001f) {
            state.chain_center_of_mass = weighted_position / state.total_chain_mass;
        } else {
            // 如果總質量為零，使用幾何中心
            Vector3 geometric_center(0, 0, 0);
            for (size_t i = 0; i < state.entity_transforms.size(); ++i) {
                geometric_center += state.entity_transforms[i].position;
            }
            state.chain_center_of_mass = geometric_center / float(state.entity_transforms.size());
            state.total_chain_mass = 1.0f; // 設置最小質量
        }
        
        // 更新統一變換的位置為質心
        state.unified_transform.position = state.chain_center_of_mass;
        
        // 計算慣性張量（簡化計算）
        Vector3 total_inertia(0, 0, 0);
        for (size_t i = 0; i < state.controlled_entities.size(); ++i) {
            float mass = state.entity_physics[i].mass;
            float weight = (i < state.entity_weights.size()) ? state.entity_weights[i] : 1.0f;
            float effective_mass = mass * weight;
            
            Vector3 inertia = state.entity_physics[i].inertia_tensor_diagonal;
            Vector3 offset = state.entity_transforms[i].position - state.chain_center_of_mass;
            
            // 平行軸定理：I_total = I_local + m * r²
            total_inertia += inertia * effective_mass;
            float offset_squared = offset.length_squared();
            total_inertia += Vector3(offset_squared, offset_squared, offset_squared) * effective_mass;
        }
        
        state.unified_physics.inertia_tensor_diagonal = total_inertia;
        state.unified_physics.mass = state.total_chain_mass;
        
        std::cout << "LogicalEntityManager: Chain mass properties - Total mass: " << state.total_chain_mass
                  << ", Center of mass: (" << state.chain_center_of_mass.x << ", " 
                  << state.chain_center_of_mass.y << ", " << state.chain_center_of_mass.z << ")" << std::endl;
    }

    void LogicalEntityManager::merge_multi_entity_forces(LogicalEntityState& state)
    {
        state.total_applied_force = Vector3(0, 0, 0);
        state.total_applied_torque = Vector3(0, 0, 0);
        Vector3 total_linear_velocity(0, 0, 0);
        Vector3 total_angular_velocity(0, 0, 0);
        
        // 力和力矩的簡單求和
        for (size_t i = 0; i < state.segment_forces.size(); ++i) {
            float weight = (i < state.entity_weights.size()) ? state.entity_weights[i] : 1.0f;
            
            state.total_applied_force += state.segment_forces[i] * weight;
            state.total_applied_torque += state.segment_torques[i] * weight;
            
            // 速度的加權平均
            total_linear_velocity += state.entity_physics[i].linear_velocity * weight;
            total_angular_velocity += state.entity_physics[i].angular_velocity * weight;
        }
        
        // 歸一化速度
        float total_weight = 0.0f;
        for (float w : state.entity_weights) {
            total_weight += w;
        }
        
        if (total_weight > 0.001f) {
            state.unified_physics.linear_velocity = total_linear_velocity / total_weight;
            state.unified_physics.angular_velocity = total_angular_velocity / total_weight;
        }
        
        // 計算由於位置差異產生的額外力矩
        for (size_t i = 0; i < state.controlled_entities.size(); ++i) {
            Vector3 position_offset = state.entity_transforms[i].position - state.chain_center_of_mass;
            Vector3 lever_torque = position_offset.cross(state.segment_forces[i]);
            state.total_applied_torque += lever_torque;
        }
        
        std::cout << "LogicalEntityManager: Force summation result - Force: (" 
                  << state.total_applied_force.x << ", " << state.total_applied_force.y 
                  << ", " << state.total_applied_force.z << "), Torque magnitude: " 
                  << state.total_applied_torque.length() << std::endl;
    }

    void LogicalEntityManager::merge_multi_entity_weighted_average(LogicalEntityState& state)
    {
        state.unified_physics.linear_velocity = Vector3(0, 0, 0);
        state.unified_physics.angular_velocity = Vector3(0, 0, 0);
        state.total_applied_force = Vector3(0, 0, 0);
        state.total_applied_torque = Vector3(0, 0, 0);
        
        float total_weight = 0.0f;
        
        // 加權平均所有物理屬性
        for (size_t i = 0; i < state.controlled_entities.size(); ++i) {
            float weight = (i < state.entity_weights.size()) ? state.entity_weights[i] : 1.0f;
            total_weight += weight;
            
            state.unified_physics.linear_velocity += state.entity_physics[i].linear_velocity * weight;
            state.unified_physics.angular_velocity += state.entity_physics[i].angular_velocity * weight;
            state.total_applied_force += state.segment_forces[i] * weight;
            state.total_applied_torque += state.segment_torques[i] * weight;
        }
        
        if (total_weight > 0.001f) {
            state.unified_physics.linear_velocity /= total_weight;
            state.unified_physics.angular_velocity /= total_weight;
            state.total_applied_force /= total_weight;
            state.total_applied_torque /= total_weight;
        }
        
        std::cout << "LogicalEntityManager: Weighted average result - Velocity: (" 
                  << state.unified_physics.linear_velocity.x << ", " 
                  << state.unified_physics.linear_velocity.y << ", " 
                  << state.unified_physics.linear_velocity.z << ")" << std::endl;
    }

    void LogicalEntityManager::merge_multi_entity_physics_simulation(LogicalEntityState& state)
    {
        // 首先使用力求和獲得基礎合成
        merge_multi_entity_forces(state);
        
        // 如果啟用了物理模擬，創建或更新模擬代理
        if (state.use_physics_simulation) {
            if (!create_or_update_physics_proxy(state.logical_id)) {
                std::cerr << "LogicalEntityManager: Failed to create physics proxy for multi-entity simulation" << std::endl;
                return;
            }
            
            // 將合成的力施加到代理實體
            apply_merged_forces_to_proxy(state.logical_id);
            
            std::cout << "LogicalEntityManager: Applied physics simulation for multi-entity logical control " 
                      << state.logical_id << std::endl;
        }
    }

    void LogicalEntityManager::merge_multi_entity_restrictive(LogicalEntityState& state)
    {
        // 對於最限制策略，選擇最小的速度和最受約束的狀態
        float min_speed = std::numeric_limits<float>::max();
        size_t most_restrictive_index = 0;
        
        for (size_t i = 0; i < state.entity_physics.size(); ++i) {
            float speed = state.entity_physics[i].linear_velocity.length();
            if (speed < min_speed) {
                min_speed = speed;
                most_restrictive_index = i;
            }
        }
        
        // 使用最受限制實體的物理狀態作為基礎
        state.unified_physics.linear_velocity = state.entity_physics[most_restrictive_index].linear_velocity;
        state.unified_physics.angular_velocity = state.entity_physics[most_restrictive_index].angular_velocity;
        
        // 但仍然合成所有的力
        state.total_applied_force = Vector3(0, 0, 0);
        state.total_applied_torque = Vector3(0, 0, 0);
        
        for (size_t i = 0; i < state.segment_forces.size(); ++i) {
            state.total_applied_force += state.segment_forces[i];
            state.total_applied_torque += state.segment_torques[i];
        }
        
        std::cout << "LogicalEntityManager: Most restrictive result - Selected entity index: " 
                  << most_restrictive_index << ", speed: " << min_speed << std::endl;
    }

    void LogicalEntityManager::distribute_constraints_across_chain(LogicalEntityState& state)
    {
        state.segment_constraints.clear();
        state.segment_constraints.resize(state.controlled_entities.size());
        state.has_distributed_constraints = false;
        
        // 檢測每個實體的約束
        for (size_t i = 0; i < state.controlled_entities.size(); ++i) {
            EntityId entity_id = state.controlled_entities[i];
            PhysicsConstraintState constraint;
            
            bool is_constrained = physics_manipulator_->detect_entity_collision_constraints(entity_id, constraint);
            
            if (is_constrained) {
                state.segment_constraints[i] = constraint;
                state.has_distributed_constraints = true;
                
                std::cout << "LogicalEntityManager: Detected constraint on entity " << entity_id 
                          << " in chain segment " << i << std::endl;
            }
        }
        
        // 如果有分佈式約束，需要協調所有實體的運動
        if (state.has_distributed_constraints) {
            coordinate_distributed_motion(state);
        }
    }

    void LogicalEntityManager::coordinate_distributed_motion(LogicalEntityState& state)
    {
        // 計算所有約束的聯合影響
        Vector3 combined_allowed_velocity(0, 0, 0);
        Vector3 combined_blocking_normal(0, 0, 0);
        int constraint_count = 0;
        
        for (size_t i = 0; i < state.segment_constraints.size(); ++i) {
            const PhysicsConstraintState& constraint = state.segment_constraints[i];
            
            if (constraint.is_blocked) {
                combined_allowed_velocity += constraint.allowed_velocity;
                combined_blocking_normal += constraint.blocking_normal;
                constraint_count++;
            }
        }
        
        if (constraint_count > 0) {
            // 平均化約束影響
            combined_allowed_velocity /= float(constraint_count);
            combined_blocking_normal = combined_blocking_normal.normalized();
            
            // 更新統一約束狀態
            state.constraint_state.is_blocked = true;
            state.constraint_state.allowed_velocity = combined_allowed_velocity;
            state.constraint_state.blocking_normal = combined_blocking_normal;
            
            // 修正統一的物理速度以滿足約束
            Vector3 original_velocity = state.unified_physics.linear_velocity;
            
            // 移除朝向阻擋方向的速度分量
            float blocking_dot = original_velocity.dot(combined_blocking_normal);
            if (blocking_dot < 0.0f) {
                state.unified_physics.linear_velocity = original_velocity - combined_blocking_normal * blocking_dot;
            }
            
            // 如果有允許的速度，疊加它
            if (combined_allowed_velocity.length() > 0.001f) {
                state.unified_physics.linear_velocity += combined_allowed_velocity;
            }
            
            std::cout << "LogicalEntityManager: Coordinated motion with " << constraint_count 
                      << " constraints - Final velocity: (" << state.unified_physics.linear_velocity.x 
                      << ", " << state.unified_physics.linear_velocity.y << ", " 
                      << state.unified_physics.linear_velocity.z << ")" << std::endl;
        }
    }

    bool LogicalEntityManager::sync_logical_to_chain_entities(LogicalEntityId logical_id)
    {
        auto it = logical_entities_.find(logical_id);
        if (it == logical_entities_.end()) {
            return false;
        }

        const LogicalEntityState& state = it->second;
        
        // 同步統一物理狀態到所有受控實體
        for (size_t i = 0; i < state.controlled_entities.size(); ++i) {
            EntityId entity_id = state.controlled_entities[i];
            
            // 計算該實體的目標狀態
            Transform target_transform = state.unified_transform;
            PhysicsState target_physics = state.unified_physics;
            
            // 根據實體在鏈中的相對位置調整變換
            if (i < state.entity_transforms.size()) {
                Vector3 offset = state.entity_transforms[i].position - state.chain_center_of_mass;
                target_transform.position = state.unified_transform.position + offset;
            }
            
            // 同步到物理引擎
            physics_manipulator_->force_set_entity_physics_state(entity_id, target_transform, target_physics);
            std::cout << "LogicalEntityManager: Synced physics state for entity " 
                      << entity_id << " in chain " << logical_id << std::endl;
        }
        
        std::cout << "LogicalEntityManager: Synced logical state to " << state.controlled_entities.size() 
                  << " chain entities for logical control " << logical_id << std::endl;
        
        return true;
    }

    // === 私有方法实现 ===
    LogicalEntityId LogicalEntityManager::allocate_logical_id()
{
    return next_logical_id_++;
}

bool LogicalEntityManager::collect_entity_physics_state(EntityId entity_id, Transform& transform, PhysicsState& physics)
{
    if (!physics_data_->is_entity_valid(entity_id)) {
        return false;
    }
    
    transform = physics_data_->get_entity_transform(entity_id);
    physics = physics_data_->get_entity_physics_state(entity_id);
    return true;
}

Transform LogicalEntityManager::merge_transforms(const Transform& main_transform, const Transform& ghost_transform,
                                                PhysicsStateMergeStrategy strategy, float main_weight, float ghost_weight)
{
    switch (strategy) {
        case PhysicsStateMergeStrategy::MAIN_PRIORITY:
            return main_transform;
            
        case PhysicsStateMergeStrategy::GHOST_PRIORITY:
            return ghost_transform;
            
        case PhysicsStateMergeStrategy::WEIGHTED_AVERAGE: {
            Transform result;
            float total_weight = main_weight + ghost_weight;
            if (total_weight > 0.0f) {
                float norm_main = main_weight / total_weight;
                float norm_ghost = ghost_weight / total_weight;
                
                result.position = main_transform.position * norm_main + ghost_transform.position * norm_ghost;
                result.scale = main_transform.scale * norm_main + ghost_transform.scale * norm_ghost;
                // 旋转的插值需要特殊处理，这里简化
                result.rotation = main_transform.rotation; // 简化处理
            }
            return result;
        }
        
        case PhysicsStateMergeStrategy::MOST_RESTRICTIVE:
        default:
            // 对于最严格策略，选择速度较小的变换
            return main_transform; // 简化处理
    }
}

PhysicsState LogicalEntityManager::merge_physics_states(const PhysicsState& main_physics, const PhysicsState& ghost_physics,
                                                       PhysicsStateMergeStrategy strategy, float main_weight, float ghost_weight)
{
    switch (strategy) {
        case PhysicsStateMergeStrategy::MAIN_PRIORITY:
            return main_physics;
            
        case PhysicsStateMergeStrategy::GHOST_PRIORITY:
            return ghost_physics;
            
        case PhysicsStateMergeStrategy::WEIGHTED_AVERAGE: {
            PhysicsState result;
            float total_weight = main_weight + ghost_weight;
            if (total_weight > 0.0f) {
                float norm_main = main_weight / total_weight;
                float norm_ghost = ghost_weight / total_weight;
                
                result.linear_velocity = main_physics.linear_velocity * norm_main + ghost_physics.linear_velocity * norm_ghost;
                result.angular_velocity = main_physics.angular_velocity * norm_main + ghost_physics.angular_velocity * norm_ghost;
                result.mass = main_physics.mass * norm_main + ghost_physics.mass * norm_ghost;
            }
            return result;
        }
        
        case PhysicsStateMergeStrategy::MOST_RESTRICTIVE:
        default: {
            // 选择速度较小的物理状态
            float main_speed = main_physics.linear_velocity.length();
            float ghost_speed = ghost_physics.linear_velocity.length();
            return (main_speed <= ghost_speed) ? main_physics : ghost_physics;
        }
    }
}

void LogicalEntityManager::apply_constraint_to_physics(PhysicsState& physics, const PhysicsConstraintState& constraint)
{
    if (!constraint.is_blocked) {
        return;
    }
    
    // 将速度投影到允许的方向
    Vector3 velocity = physics.linear_velocity;
    
    // 如果有允许的速度向量，使用它
    if (constraint.allowed_velocity.length() > 0.001f) {
        physics.linear_velocity = constraint.allowed_velocity;
    } else {
        // 否则移除朝向阻挡面的速度分量
        Vector3 normal = constraint.blocking_normal.normalized();
        float dot_product = velocity.dot(normal);
        if (dot_product < 0.0f) { // 只处理朝向阻挡面的速度
            physics.linear_velocity = velocity - normal * dot_product;
        }
    }
    
    // 角速度也可能需要约束，这里简化处理
    // physics.angular_velocity = physics.angular_velocity * 0.9f; // 减缓旋转
}

void LogicalEntityManager::notify_event_handler(const std::function<void(IPortalEventHandler*)>& callback)
{
    if (event_handler_) {
        callback(event_handler_);
    }
}

LogicalEntityManager::LogicalEntityStats LogicalEntityManager::get_statistics() const
{
    LogicalEntityStats stats;
    stats.total_logical_entities = logical_entities_.size();
    stats.constrained_entities = 0;
    stats.unified_mode_entities = 0;
    stats.average_merge_time = 0.0f; // 需要实际测量
    stats.average_sync_time = 0.0f;  // 需要实际测量
    
    for (const auto& [id, state] : logical_entities_) {
        if (state.constraint_state.is_blocked) {
            stats.constrained_entities++;
        }
        if (state.physics_unified_mode) {
            stats.unified_mode_entities++;
        }
    }
    
    return stats;
}

// === 复杂物理属性合成实现 ===

bool LogicalEntityManager::merge_complex_physics_properties(LogicalEntityId logical_id)
{
    auto it = logical_entities_.find(logical_id);
    if (it == logical_entities_.end()) {
        return false;
    }
    
    LogicalEntityState& state = it->second;
    const ComplexPhysicsMergeConfig& config = state.complex_merge_config;
    
    // 获取主体和幽灵的作用力
    Vector3 main_force(0, 0, 0), main_torque(0, 0, 0);
    Vector3 ghost_force(0, 0, 0), ghost_torque(0, 0, 0);
    
    bool has_main_forces = physics_manipulator_->get_entity_applied_forces(
        state.main_entity_id, main_force, main_torque);
    bool has_ghost_forces = (state.ghost_entity_id != INVALID_ENTITY_ID) ?
        physics_manipulator_->get_entity_applied_forces(
            state.ghost_entity_id, ghost_force, ghost_torque) : false;
    
    // 重置合成的力和力矩
    state.total_applied_force = Vector3(0, 0, 0);
    state.total_applied_torque = Vector3(0, 0, 0);
    
    if (config.merge_forces && (has_main_forces || has_ghost_forces)) {
        // 基本力合成
        state.total_applied_force = main_force + ghost_force;
        
        if (config.merge_torques) {
            state.total_applied_torque = main_torque + ghost_torque;
            
            // 如果考虑杠杆效应，计算额外的力矩
            if (config.consider_leverage && has_main_forces && has_ghost_forces) {
                Vector3 main_pos = physics_data_->get_entity_transform(state.main_entity_id).position;
                Vector3 ghost_pos = (state.ghost_entity_id != INVALID_ENTITY_ID) ?
                    physics_data_->get_entity_transform(state.ghost_entity_id).position : main_pos;
                
                Vector3 leverage_torque = calculate_leverage_torque(
                    main_force, ghost_force, main_pos, ghost_pos, config.logical_pivot_point);
                    
                state.total_applied_torque = state.total_applied_torque + leverage_torque;
            }
        }
        
        std::cout << "LogicalEntityManager: Merged forces for logical entity " << logical_id 
                  << " - Force: (" << state.total_applied_force.x << ", " 
                  << state.total_applied_force.y << ", " << state.total_applied_force.z 
                  << ") - Torque: (" << state.total_applied_torque.x << ", " 
                  << state.total_applied_torque.y << ", " << state.total_applied_torque.z << ")" << std::endl;
    }
    
    return true;
}

Vector3 LogicalEntityManager::calculate_leverage_torque(const Vector3& main_force, const Vector3& ghost_force,
                                                       const Vector3& main_position, const Vector3& ghost_position,
                                                       const Vector3& pivot_point)
{
    // 计算主体和幽灵相对于支点的位置向量
    Vector3 main_arm = main_position - pivot_point;
    Vector3 ghost_arm = ghost_position - pivot_point;
    
    // 计算由于杠杆效应产生的力矩
    // τ = r × F
    Vector3 main_leverage_torque = main_arm.cross(main_force);
    Vector3 ghost_leverage_torque = ghost_arm.cross(ghost_force);
    
    // 合成杠杆力矩
    Vector3 total_leverage_torque = main_leverage_torque + ghost_leverage_torque;
    
    std::cout << "LogicalEntityManager: Calculated leverage torque - Main arm: (" 
              << main_arm.x << ", " << main_arm.y << ", " << main_arm.z 
              << "), Ghost arm: (" << ghost_arm.x << ", " << ghost_arm.y << ", " << ghost_arm.z
              << "), Leverage torque: (" << total_leverage_torque.x << ", " 
              << total_leverage_torque.y << ", " << total_leverage_torque.z << ")" << std::endl;
    
    return total_leverage_torque;
}

bool LogicalEntityManager::create_or_update_physics_proxy(LogicalEntityId logical_id)
{
    auto it = logical_entities_.find(logical_id);
    if (it == logical_entities_.end()) {
        return false;
    }
    
    LogicalEntityState& state = it->second;
    
    // 如果还没有代理实体，创建一个
    if (!state.has_simulation_proxy || state.simulation_proxy_entity == INVALID_ENTITY_ID) {
        // 使用主体实体作为模板
        EntityId template_entity = state.main_entity_id;
        
        state.simulation_proxy_entity = physics_manipulator_->create_physics_simulation_proxy(
            template_entity, state.unified_transform, state.unified_physics);
            
        if (state.simulation_proxy_entity != INVALID_ENTITY_ID) {
            state.has_simulation_proxy = true;
            std::cout << "LogicalEntityManager: Created physics simulation proxy " 
                      << state.simulation_proxy_entity << " for logical entity " << logical_id << std::endl;
        } else {
            std::cerr << "LogicalEntityManager: Failed to create physics simulation proxy for logical entity " 
                      << logical_id << std::endl;
            return false;
        }
    }
    
    // 更新代理实体的物理材质属性
    physics_manipulator_->set_proxy_physics_material(
        state.simulation_proxy_entity,
        state.unified_physics.friction_coefficient,
        state.unified_physics.restitution_coefficient,
        state.unified_physics.linear_damping,
        state.unified_physics.angular_damping);
    
    return true;
}

void LogicalEntityManager::apply_merged_forces_to_proxy(LogicalEntityId logical_id)
{
    auto it = logical_entities_.find(logical_id);
    if (it == logical_entities_.end() || !it->second.has_simulation_proxy) {
        return;
    }
    
    const LogicalEntityState& state = it->second;
    
    // 清除之前的作用力
    physics_manipulator_->clear_forces_on_proxy(state.simulation_proxy_entity);
    
    // 施加合成的力
    if (state.total_applied_force.length() > 0.001f) {
        physics_manipulator_->apply_force_to_proxy(
            state.simulation_proxy_entity, state.total_applied_force);
    }
    
    // 施加合成的力矩
    if (state.total_applied_torque.length() > 0.001f) {
        physics_manipulator_->apply_torque_to_proxy(
            state.simulation_proxy_entity, state.total_applied_torque);
    }
    
    std::cout << "LogicalEntityManager: Applied merged forces to proxy " 
              << state.simulation_proxy_entity << " - Force magnitude: " 
              << state.total_applied_force.length() << ", Torque magnitude: " 
              << state.total_applied_torque.length() << std::endl;
}

bool LogicalEntityManager::get_simulation_result_from_proxy(LogicalEntityId logical_id)
{
    auto it = logical_entities_.find(logical_id);
    if (it == logical_entities_.end() || !it->second.has_simulation_proxy) {
        return false;
    }
    
    LogicalEntityState& state = it->second;
    
    // 从代理实体获取物理引擎模拟后的状态
    Transform proxy_transform = physics_data_->get_entity_transform(state.simulation_proxy_entity);
    PhysicsState proxy_physics = physics_data_->get_entity_physics_state(state.simulation_proxy_entity);
    
    // 将模拟结果作为新的统一状态
    state.unified_transform = proxy_transform;
    state.unified_physics.linear_velocity = proxy_physics.linear_velocity;
    state.unified_physics.angular_velocity = proxy_physics.angular_velocity;
    
    // 保持其他物理属性不变（质量、惯性等）
    
    std::cout << "LogicalEntityManager: Got simulation result from proxy " 
              << state.simulation_proxy_entity << " for logical entity " << logical_id << std::endl;
    
    return true;
}

// === 配置方法实现 ===

void LogicalEntityManager::set_complex_physics_config(LogicalEntityId logical_id, const ComplexPhysicsMergeConfig& config)
{
    auto it = logical_entities_.find(logical_id);
    if (it != logical_entities_.end()) {
        it->second.complex_merge_config = config;
        std::cout << "LogicalEntityManager: Set complex physics config for logical entity " << logical_id << std::endl;
    }
}

void LogicalEntityManager::set_logical_pivot_point(LogicalEntityId logical_id, const Vector3& pivot_point)
{
    auto it = logical_entities_.find(logical_id);
    if (it != logical_entities_.end()) {
        it->second.complex_merge_config.logical_pivot_point = pivot_point;
        std::cout << "LogicalEntityManager: Set logical pivot point for entity " << logical_id 
                  << " to (" << pivot_point.x << ", " << pivot_point.y << ", " << pivot_point.z << ")" << std::endl;
    }
}

void LogicalEntityManager::set_physics_simulation_proxy_enabled(LogicalEntityId logical_id, bool enabled)
{
    auto it = logical_entities_.find(logical_id);
    if (it != logical_entities_.end()) {
        it->second.use_physics_simulation = enabled;
        
        // 如果禁用且有代理实体，销毁它
        if (!enabled && it->second.has_simulation_proxy) {
            physics_manipulator_->destroy_physics_simulation_proxy(it->second.simulation_proxy_entity);
            it->second.simulation_proxy_entity = INVALID_ENTITY_ID;
            it->second.has_simulation_proxy = false;
        }
        
        std::cout << "LogicalEntityManager: Set physics simulation proxy " 
                  << (enabled ? "enabled" : "disabled") << " for logical entity " << logical_id << std::endl;
    }
}

void LogicalEntityManager::set_leverage_arms(LogicalEntityId logical_id, float main_arm, float ghost_arm)
{
    auto it = logical_entities_.find(logical_id);
    if (it != logical_entities_.end()) {
        it->second.complex_merge_config.main_entity_leverage = main_arm;
        it->second.complex_merge_config.ghost_entity_leverage = ghost_arm;
        std::cout << "LogicalEntityManager: Set leverage arms for logical entity " << logical_id 
                  << " - Main: " << main_arm << ", Ghost: " << ghost_arm << std::endl;
    }
}

// === 其他缺失方法的實現 ===

bool LogicalEntityManager::add_entity_to_logical(LogicalEntityId logical_id, EntityId entity_id, bool is_main)
{
    auto it = logical_entities_.find(logical_id);
    if (it == logical_entities_.end()) {
        return false;
    }
    
    LogicalEntityState& state = it->second;
    
    if (is_main) {
        state.main_entity_id = entity_id;
    } else {
        state.ghost_entity_id = entity_id;
    }
    
    // 建立映射
    entity_to_logical_mapping_[entity_id] = logical_id;
    
    // 禁用物理引擎控制
    physics_manipulator_->set_entity_physics_engine_controlled(entity_id, false);
    
    std::cout << "LogicalEntityManager: Added entity " << entity_id 
              << " to logical entity " << logical_id << " as " 
              << (is_main ? "main" : "ghost") << std::endl;
    
    return true;
}

bool LogicalEntityManager::remove_entity_from_logical(LogicalEntityId logical_id, EntityId entity_id)
{
    auto it = logical_entities_.find(logical_id);
    if (it == logical_entities_.end()) {
        return false;
    }
    
    LogicalEntityState& state = it->second;
    
    // 移除實體引用
    if (state.main_entity_id == entity_id) {
        state.main_entity_id = INVALID_ENTITY_ID;
    } else if (state.ghost_entity_id == entity_id) {
        state.ghost_entity_id = INVALID_ENTITY_ID;
    } else {
        return false; // 實體不在此邏輯實體中
    }
    
    // 清理映射
    entity_to_logical_mapping_.erase(entity_id);
    
    // 恢復物理引擎控制
    physics_manipulator_->set_entity_physics_engine_controlled(entity_id, true);
    
    std::cout << "LogicalEntityManager: Removed entity " << entity_id 
              << " from logical entity " << logical_id << std::endl;
    
    return true;
}

void LogicalEntityManager::sync_all_logical_entities()
{
    for (const auto& [logical_id, state] : logical_entities_) {
        sync_logical_to_entities(logical_id);
    }
    std::cout << "LogicalEntityManager: Synced all " << logical_entities_.size() << " logical entities" << std::endl;
}

void LogicalEntityManager::set_entity_physics_engine_control(EntityId entity_id, bool engine_controlled)
{
    physics_manipulator_->set_entity_physics_engine_controlled(entity_id, engine_controlled);
    std::cout << "LogicalEntityManager: Set physics engine control for entity " << entity_id 
              << " to " << (engine_controlled ? "enabled" : "disabled") << std::endl;
}

bool LogicalEntityManager::is_logical_entity_constrained(LogicalEntityId logical_id) const
{
    auto it = logical_entities_.find(logical_id);
    return (it != logical_entities_.end()) ? it->second.constraint_state.is_blocked : false;
}

const PhysicsConstraintState* LogicalEntityManager::get_constraint_state(LogicalEntityId logical_id) const
{
    auto it = logical_entities_.find(logical_id);
    return (it != logical_entities_.end()) ? &it->second.constraint_state : nullptr;
}

void LogicalEntityManager::set_merge_strategy(LogicalEntityId logical_id, PhysicsStateMergeStrategy strategy)
{
    auto it = logical_entities_.find(logical_id);
    if (it != logical_entities_.end()) {
        it->second.merge_strategy = strategy;
        std::cout << "LogicalEntityManager: Set merge strategy for logical entity " << logical_id 
                  << " to " << static_cast<int>(strategy) << std::endl;
    }
}

void LogicalEntityManager::set_entity_weights(LogicalEntityId logical_id, float main_weight, float ghost_weight)
{
    auto it = logical_entities_.find(logical_id);
    if (it != logical_entities_.end()) {
        it->second.main_weight = main_weight;
        it->second.ghost_weight = ghost_weight;
        std::cout << "LogicalEntityManager: Set entity weights for logical entity " << logical_id 
                  << " - Main: " << main_weight << ", Ghost: " << ghost_weight << std::endl;
    }
}

void LogicalEntityManager::set_unified_physics_mode(LogicalEntityId logical_id, bool enabled)
{
    auto it = logical_entities_.find(logical_id);
    if (it != logical_entities_.end()) {
        it->second.physics_unified_mode = enabled;
        std::cout << "LogicalEntityManager: Set unified physics mode for logical entity " << logical_id 
                  << " to " << (enabled ? "enabled" : "disabled") << std::endl;
    }
}

void LogicalEntityManager::force_update_logical_entity(LogicalEntityId logical_id)
{
    auto it = logical_entities_.find(logical_id);
    if (it == logical_entities_.end()) {
        std::cout << "LogicalEntityManager: Cannot force update - logical entity " << logical_id << " not found" << std::endl;
        return;
    }

    LogicalEntityState& state = it->second;
    
    std::cout << "LogicalEntityManager: Force updating logical entity " << logical_id << std::endl;
    
    // 根據實體數量選擇更新策略
    if (state.controlled_entities.size() > 2) {
        std::cout << "LogicalEntityManager: Using multi-entity chain update for " << state.controlled_entities.size() << " entities" << std::endl;
        
        // 1. 多實體物理狀態合成
        if (!merge_multi_entity_physics_states(logical_id)) {
            std::cout << "LogicalEntityManager: Warning - Failed to merge multi-entity physics states for logical entity " << logical_id << std::endl;
        }
        
        // 2. 分佈約束檢測和協調
        distribute_constraints_across_chain(state);
        
        // 3. 複雜物理屬性合成（如果需要）
        if (state.merge_strategy == PhysicsStateMergeStrategy::FORCE_SUMMATION ||
            state.merge_strategy == PhysicsStateMergeStrategy::PHYSICS_SIMULATION) {
            merge_complex_physics_properties(logical_id);
        }
        
        // 4. 多實體鏈同步
        sync_logical_to_chain_entities(logical_id);
        
    } else {
        std::cout << "LogicalEntityManager: Using traditional dual-entity update" << std::endl;
        
        // 1. 傳統物理狀態合成
        if (!merge_physics_states(logical_id)) {
            std::cout << "LogicalEntityManager: Warning - Failed to merge physics states for logical entity " << logical_id << std::endl;
        }
        
        // 2. 複雜物理屬性合成（如果需要）
        if (state.merge_strategy == PhysicsStateMergeStrategy::FORCE_SUMMATION ||
            state.merge_strategy == PhysicsStateMergeStrategy::PHYSICS_SIMULATION) {
            merge_complex_physics_properties(logical_id);
        }
        
        // 3. 檢測和應用約束
        detect_physics_constraints(logical_id);
        apply_physics_constraints(logical_id);
        
        // 4. 傳統雙實體同步
        sync_logical_to_entities(logical_id);
    }
    
    std::cout << "LogicalEntityManager: Force update completed for logical entity " << logical_id << std::endl;
}

// === 多實體控制實現 ===

LogicalEntityId LogicalEntityManager::create_multi_entity_logical_control(
    const std::vector<EntityId>& entities,
    const std::vector<float>& weights)
{
    if (entities.empty()) {
        std::cout << "LogicalEntityManager: Cannot create multi-entity control with empty entity list" << std::endl;
        return INVALID_LOGICAL_ENTITY_ID;
    }

    LogicalEntityId logical_id = allocate_logical_id();
    
    // 创建逻辑实体状态
    LogicalEntityState logical_state;
    logical_state.logical_id = logical_id;
    logical_state.controlled_entities = entities;
    logical_state.primary_entity_id = entities[0]; // 默认第一个为主控
    
    // 设置权重
    if (weights.empty()) {
        // 默认权重：第一个实体权重为1.0，其他为0.5
        logical_state.entity_weights.resize(entities.size());
        for (size_t i = 0; i < entities.size(); ++i) {
            logical_state.entity_weights[i] = (i == 0) ? 1.0f : 0.5f;
        }
    } else {
        logical_state.entity_weights = weights;
        // 确保权重数量匹配实体数量
        logical_state.entity_weights.resize(entities.size(), 1.0f);
    }
    
    // 向后兼容：如果只有两个实体，设置为传统的主体/幽灵
    if (entities.size() == 2) {
        logical_state.main_entity_id = entities[0];
        logical_state.ghost_entity_id = entities[1];
        logical_state.main_weight = logical_state.entity_weights[0];
        logical_state.ghost_weight = logical_state.entity_weights[1];
    }
    
    // 默认合成策略
    logical_state.merge_strategy = PhysicsStateMergeStrategy::MOST_RESTRICTIVE;
    logical_state.physics_unified_mode = true;
    logical_state.ignore_engine_physics = true;
    
    // 收集初始物理状态
    logical_state.entity_transforms.resize(entities.size());
    logical_state.entity_physics.resize(entities.size());
    
    for (size_t i = 0; i < entities.size(); ++i) {
        if (physics_data_) {
            logical_state.entity_transforms[i] = physics_data_->get_entity_transform(entities[i]);
            logical_state.entity_physics[i] = physics_data_->get_entity_physics_state(entities[i]);
        }
        
        // 建立映射
        entity_to_logical_mapping_[entities[i]] = logical_id;
        
        // 禁用物理引擎直接控制
        if (physics_manipulator_) {
            physics_manipulator_->set_entity_physics_engine_controlled(entities[i], false);
        }
    }
    
    // 存储逻辑实体
    logical_entities_[logical_id] = logical_state;
    
    // 初始合成
    if (!merge_physics_states(logical_id)) {
        std::cerr << "LogicalEntityManager: Failed to merge initial physics states for multi-entity logical control " 
                  << logical_id << std::endl;
        // 清理并返回失败
        destroy_logical_entity(logical_id);
        return INVALID_LOGICAL_ENTITY_ID;
    }
    
    // 通知事件处理器
    notify_event_handler([logical_id, entities](IPortalEventHandler* handler) {
        if (entities.size() >= 2) {
            handler->on_logical_entity_created(logical_id, entities[0], entities[1]);
        } else {
            handler->on_logical_entity_created(logical_id, entities[0], INVALID_ENTITY_ID);
        }
    });
    
    std::cout << "LogicalEntityManager: Created multi-entity logical control " << logical_id 
              << " for " << entities.size() << " entities" << std::endl;
    
    return logical_id;
}

bool LogicalEntityManager::add_controlled_entity(LogicalEntityId logical_id, EntityId entity_id, float weight)
{
    auto it = logical_entities_.find(logical_id);
    if (it == logical_entities_.end()) {
        return false;
    }

    LogicalEntityState& state = it->second;
    
    // 检查是否已经存在
    for (EntityId existing_id : state.controlled_entities) {
        if (existing_id == entity_id) {
            return false; // 已存在
        }
    }
    
    // 添加新实体
    state.controlled_entities.push_back(entity_id);
    state.entity_weights.push_back(weight);
    
    // 收集新实体的物理状态
    if (physics_data_) {
        state.entity_transforms.push_back(physics_data_->get_entity_transform(entity_id));
        state.entity_physics.push_back(physics_data_->get_entity_physics_state(entity_id));
    } else {
        state.entity_transforms.push_back(Transform());
        state.entity_physics.push_back(PhysicsState());
    }
    
    // 建立映射
    entity_to_logical_mapping_[entity_id] = logical_id;
    
    // 禁用物理引擎直接控制
    if (physics_manipulator_) {
        physics_manipulator_->set_entity_physics_engine_controlled(entity_id, false);
    }
    
    std::cout << "LogicalEntityManager: Added entity " << entity_id << " to logical control " << logical_id << std::endl;
    return true;
}

bool LogicalEntityManager::remove_controlled_entity(LogicalEntityId logical_id, EntityId entity_id)
{
    auto it = logical_entities_.find(logical_id);
    if (it == logical_entities_.end()) {
        return false;
    }

    LogicalEntityState& state = it->second;
    
    // 找到并移除实体
    for (size_t i = 0; i < state.controlled_entities.size(); ++i) {
        if (state.controlled_entities[i] == entity_id) {
            state.controlled_entities.erase(state.controlled_entities.begin() + i);
            state.entity_weights.erase(state.entity_weights.begin() + i);
            state.entity_transforms.erase(state.entity_transforms.begin() + i);
            state.entity_physics.erase(state.entity_physics.begin() + i);
            
            // 恢复物理引擎控制
            if (physics_manipulator_) {
                physics_manipulator_->set_entity_physics_engine_controlled(entity_id, true);
            }
            
            // 清理映射
            entity_to_logical_mapping_.erase(entity_id);
            
            // 如果移除的是主控实体，重新设置
            if (state.primary_entity_id == entity_id && !state.controlled_entities.empty()) {
                state.primary_entity_id = state.controlled_entities[0];
            }
            
            std::cout << "LogicalEntityManager: Removed entity " << entity_id << " from logical control " << logical_id << std::endl;
            return true;
        }
    }
    
    return false;
}

void LogicalEntityManager::set_primary_controlled_entity(LogicalEntityId logical_id, EntityId primary_entity_id)
{
    auto it = logical_entities_.find(logical_id);
    if (it == logical_entities_.end()) {
        return;
    }

    LogicalEntityState& state = it->second;
    
    // 检查实体是否在受控列表中
    bool found = false;
    for (EntityId controlled_id : state.controlled_entities) {
        if (controlled_id == primary_entity_id) {
            found = true;
            break;
        }
    }
    
    if (found) {
        state.primary_entity_id = primary_entity_id;
        
        // 向后兼容：如果是双实体系统，更新main_entity_id
        if (state.controlled_entities.size() == 2) {
            state.main_entity_id = primary_entity_id;
            // 找到另一个实体作为ghost
            for (EntityId controlled_id : state.controlled_entities) {
                if (controlled_id != primary_entity_id) {
                    state.ghost_entity_id = controlled_id;
                    break;
                }
            }
        }
        
        std::cout << "LogicalEntityManager: Set primary entity " << primary_entity_id 
                  << " for logical control " << logical_id << std::endl;
    }
}

} // namespace Portal
