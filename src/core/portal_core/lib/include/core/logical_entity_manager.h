#ifndef LOGICAL_ENTITY_MANAGER_H
#define LOGICAL_ENTITY_MANAGER_H

#include "../portal_types.h"
#include "../interfaces/portal_event_interfaces.h"
#include <unordered_map>
#include <memory>
#include <vector>
#include <functional>

namespace Portal
{

  /**
   * 逻辑实体管理器
   * 
   * 职责：
   * - 管理逻辑统一实体的生命周期
   * - 合成主体和幽灵的物理状态到逻辑实体
   * - 将逻辑实体状态同步到主体和幽灵
   * - 处理物理约束和阻挡情况
   * - 确保主体和幽灵在逻辑上表现为同一个物体
   * 
   * 核心概念：
   * - 逻辑实体是主体和幽灵的统一控制层
   * - 物理引擎不直接控制主体/幽灵，而是通过逻辑实体
   * - 当任一实体被阻挡时，整个逻辑实体都被约束
   */
  class LogicalEntityManager
  {
  private:
    // 接口引用
    IPhysicsDataProvider *physics_data_;
    IPhysicsManipulator *physics_manipulator_;
    IPortalEventHandler *event_handler_;

    // 逻辑实体存储
    std::unordered_map<LogicalEntityId, LogicalEntityState> logical_entities_;
    
    // 实体映射：物理实体 -> 逻辑实体
    std::unordered_map<EntityId, LogicalEntityId> entity_to_logical_mapping_;
    
    // 逻辑实体ID分配器
    LogicalEntityId next_logical_id_;
    
    // 更新控制
    float update_frequency_;
    float last_update_time_;

  public:
    LogicalEntityManager(IPhysicsDataProvider *physics_data,
                        IPhysicsManipulator *physics_manipulator,
                        IPortalEventHandler *event_handler = nullptr);
    
    ~LogicalEntityManager() = default;
    
    // 禁用拷贝
    LogicalEntityManager(const LogicalEntityManager&) = delete;
    LogicalEntityManager& operator=(const LogicalEntityManager&) = delete;
    
    // === 生命周期管理 ===
    
    /**
     * 创建逻辑实体
     * 将主体和幽灵实体绑定到一个逻辑实体
     */
    LogicalEntityId create_logical_entity(EntityId main_entity_id, EntityId ghost_entity_id,
                                         PhysicsStateMergeStrategy strategy = PhysicsStateMergeStrategy::MOST_RESTRICTIVE);
    
    /**
     * 销毁逻辑实体
     */
    void destroy_logical_entity(LogicalEntityId logical_id);
    
    /**
     * 添加实体到现有逻辑实体
     */
    bool add_entity_to_logical(LogicalEntityId logical_id, EntityId entity_id, bool is_main);
    
    /**
     * 从逻辑实体移除实体
     */
    bool remove_entity_from_logical(LogicalEntityId logical_id, EntityId entity_id);
    
    // === 主更新循环 ===
    
    /**
     * 更新所有逻辑实体
     * 1. 从物理引擎收集所有相关实体的状态
     * 2. 合成逻辑实体的统一状态
     * 3. 检测和处理物理约束
     * 4. 将统一状态同步回所有相关实体
     */
    void update(float delta_time);
    
    // === 多實體控制接口（實體鏈支持） ===
    
    /**
     * 創建多實體邏輯控制（用於實體鏈）
     */
    LogicalEntityId create_multi_entity_logical_control(
        const std::vector<EntityId>& entities,
        const std::vector<float>& weights = {});
    
    /**
     * 向邏輯實體添加受控實體（鏈擴展）
     */
    bool add_controlled_entity(LogicalEntityId logical_id, EntityId entity_id, float weight = 1.0f);
    
    /**
     * 從邏輯實體移除受控實體（鏈收縮）
     */
    bool remove_controlled_entity(LogicalEntityId logical_id, EntityId entity_id);
    
    /**
     * 更新實體在鏈中的主控地位
     */
    void set_primary_controlled_entity(LogicalEntityId logical_id, EntityId primary_entity_id);

    // === 物理状态合成 ===
    
    /**
     * 合成逻辑实体的物理状态
     * 根据策略将主体和幽灵的状态合成为统一状态
     */
    bool merge_physics_states(LogicalEntityId logical_id);
    
    /**
     * 检测物理约束
     * 检查主体或幽灵是否被阻挡，并更新约束状态
     */
    bool detect_physics_constraints(LogicalEntityId logical_id);
    
    /**
     * 应用物理约束
     * 根据约束状态调整逻辑实体的物理状态
     */
    void apply_physics_constraints(LogicalEntityId logical_id);

    // === 复杂物理属性合成 ===

    /**
     * 合成复杂物理属性（力和力矩）
     * 处理如杠杆效应、力矩合成等复杂情况
     */
    bool merge_complex_physics_properties(LogicalEntityId logical_id);

    /**
     * 计算杠杆效应
     * 当主体和幽灵在杠杆两端时，计算合成的力矩
     */
    Vector3 calculate_leverage_torque(const Vector3& main_force, const Vector3& ghost_force,
                                     const Vector3& main_position, const Vector3& ghost_position,
                                     const Vector3& pivot_point);

    /**
     * 创建或更新物理模拟代理
     * 为复杂物理模拟创建代理实体
     */
    bool create_or_update_physics_proxy(LogicalEntityId logical_id);

    /**
     * 从物理代理获取模拟结果
     * 获取物理引擎模拟后的状态
     */
    bool get_simulation_result_from_proxy(LogicalEntityId logical_id);

    /**
     * 对物理代理施加合成的力和力矩
     */
    void apply_merged_forces_to_proxy(LogicalEntityId logical_id);
    
    // === 同步控制 ===
    
    /**
     * 同步逻辑实体到所有关联的物理实体
     */
    bool sync_logical_to_entities(LogicalEntityId logical_id);
    
    /**
     * 批量同步所有逻辑实体
     */
    void sync_all_logical_entities();
    
    /**
     * 设置实体的物理引擎控制开关
     * 当启用逻辑实体控制时，应禁用物理引擎对相关实体的直接控制
     */
    void set_entity_physics_engine_control(EntityId entity_id, bool engine_controlled);
    
    // === 状态查询 ===
    
    /**
     * 获取逻辑实体状态
     */
    const LogicalEntityState* get_logical_entity_state(LogicalEntityId logical_id) const;
    
    /**
     * 通过物理实体ID获取逻辑实体
     */
    LogicalEntityId get_logical_entity_by_physical_entity(EntityId entity_id) const;
    
    /**
     * 检查逻辑实体是否被约束
     */
    bool is_logical_entity_constrained(LogicalEntityId logical_id) const;
    
    /**
     * 获取逻辑实体的约束信息
     */
    const PhysicsConstraintState* get_constraint_state(LogicalEntityId logical_id) const;
    
    /**
     * 强制更新逻辑实体状态
     * 立即触发物理状态合成和同步
     */
    void force_update_logical_entity(LogicalEntityId logical_id);
    
    // === 配置 ===
    
    /**
     * 设置逻辑实体的合成策略
     */
    void set_merge_strategy(LogicalEntityId logical_id, PhysicsStateMergeStrategy strategy);
    
    /**
     * 设置实体权重（用于加权平均策略）
     */
    void set_entity_weights(LogicalEntityId logical_id, float main_weight, float ghost_weight);
    
    /**
     * 启用/禁用统一物理模式
     */
    void set_unified_physics_mode(LogicalEntityId logical_id, bool enabled);
    
    /**
     * 设置更新频率
     */
    void set_update_frequency(float frequency) { update_frequency_ = frequency; }

    // === 复杂物理属性配置 ===

    /**
     * 设置复杂物理合成配置
     */
    void set_complex_physics_config(LogicalEntityId logical_id, const ComplexPhysicsMergeConfig& config);

    /**
     * 设置逻辑支点位置（用于杠杆计算）
     */
    void set_logical_pivot_point(LogicalEntityId logical_id, const Vector3& pivot_point);

    /**
     * 启用/禁用物理模拟代理
     */
    void set_physics_simulation_proxy_enabled(LogicalEntityId logical_id, bool enabled);

    /**
     * 设置杠杆臂长度
     */
    void set_leverage_arms(LogicalEntityId logical_id, float main_arm, float ghost_arm);
    
    // === 调试和统计 ===
    
    /**
     * 获取逻辑实体数量
     */
    size_t get_logical_entity_count() const { return logical_entities_.size(); }
    
    /**
     * 获取统计信息
     */
    struct LogicalEntityStats
    {
      size_t total_logical_entities;
      size_t constrained_entities;
      size_t unified_mode_entities;
      float average_merge_time;
      float average_sync_time;
    };
    LogicalEntityStats get_statistics() const;

  private:
    // === 多實體控制內部實現 ===
    // === 多實體控制內部實現 ===
    
    /**
     * 合成多實體物理狀態
     */
    bool merge_multi_entity_physics_states(LogicalEntityId logical_id);
    
    /**
     * 計算鏈的質心和慣性
     */
    void calculate_chain_mass_properties(LogicalEntityState& state);
    
    /**
     * 力求和策略的多實體合成
     */
    void merge_multi_entity_forces(LogicalEntityState& state);
    
    /**
     * 加權平均策略的多實體合成
     */
    void merge_multi_entity_weighted_average(LogicalEntityState& state);
    
    /**
     * 物理模擬策略的多實體合成
     */
    void merge_multi_entity_physics_simulation(LogicalEntityState& state);
    
    /**
     * 最限制策略的多實體合成
     */
    void merge_multi_entity_restrictive(LogicalEntityState& state);
    
    /**
     * 分佈約束到鏈的各段
     */
    void distribute_constraints_across_chain(LogicalEntityState& state);
    
    /**
     * 協調分佈式運動約束
     */
    void coordinate_distributed_motion(LogicalEntityState& state);
    
    /**
     * 同步邏輯狀態到鏈的所有實體
     */
    bool sync_logical_to_chain_entities(LogicalEntityId logical_id);

    // === 内部实现 ===
    
    /**
     * 分配新的逻辑实体ID
     */
    LogicalEntityId allocate_logical_id();
    
    /**
     * 收集实体的物理状态
     */
    bool collect_entity_physics_state(EntityId entity_id, Transform& transform, PhysicsState& physics);
    
    /**
     * 合成变换信息
     */
    Transform merge_transforms(const Transform& main_transform, const Transform& ghost_transform,
                              PhysicsStateMergeStrategy strategy, float main_weight, float ghost_weight);
    
    /**
     * 合成物理状态
     */
    PhysicsState merge_physics_states(const PhysicsState& main_physics, const PhysicsState& ghost_physics,
                                     PhysicsStateMergeStrategy strategy, float main_weight, float ghost_weight);
    
    /**
     * 检测实体碰撞约束
     */
    bool detect_entity_collision_constraints(EntityId entity_id, PhysicsConstraintState& constraint);
    
    /**
     * 应用约束到物理状态
     */
    void apply_constraint_to_physics(PhysicsState& physics, const PhysicsConstraintState& constraint);
    
    /**
     * 通知事件处理器
     */
    void notify_event_handler(const std::function<void(IPortalEventHandler*)>& callback);
  };

} // namespace Portal

#endif // LOGICAL_ENTITY_MANAGER_H
