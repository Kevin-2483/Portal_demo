#pragma once

#include "../system_base.h"
#include "../physics_world_manager.h"
#include "../components/physics_command_component.h"
#include "../components/physics_body_component.h"
#include "../components/transform_component.h"
#include <entt/entt.hpp>

namespace portal_core {

/**
 * 物理命令系統
 * 負責處理和執行PhysicsCommandComponent中的物理命令
 * 在物理系統之前運行，確保命令在物理步進前得到處理
 */
class PhysicsCommandSystem : public ISystem {
public:
    PhysicsCommandSystem() = default;
    virtual ~PhysicsCommandSystem() = default;

    // ISystem 接口實現
    virtual bool initialize() override;
    virtual void update(entt::registry& registry, float delta_time) override;
    virtual void cleanup() override;
    virtual const char* get_name() const override { return "PhysicsCommandSystem"; }

    // 命令執行控制
    void set_enabled(bool enabled) { enabled_ = enabled; }
    bool is_enabled() const { return enabled_; }

    // 批量命令執行控制
    void set_max_commands_per_frame(uint32_t max_commands) { max_commands_per_frame_ = max_commands; }
    uint32_t get_max_commands_per_frame() const { return max_commands_per_frame_; }

    // 統計信息
    struct CommandSystemStats {
        uint32_t commands_executed_this_frame = 0;
        uint32_t total_commands_executed = 0;
        uint32_t commands_skipped = 0;
        uint32_t commands_failed = 0;
        float execution_time = 0.0f;
        uint32_t entities_with_commands = 0;
    };

    const CommandSystemStats& get_stats() const { return stats_; }

protected:
    // 命令執行方法
    void execute_immediate_commands(entt::registry& registry);
    void execute_before_physics_commands(entt::registry& registry);
    void execute_after_physics_commands(entt::registry& registry);
    void execute_delayed_commands(entt::registry& registry, float delta_time);
    void execute_recurring_commands(entt::registry& registry);

    // 單個命令執行
    bool execute_command(const PhysicsCommand& command, entt::entity entity, entt::registry& registry);
    bool execute_force_command(const PhysicsCommand& command, entt::entity entity, entt::registry& registry);
    bool execute_velocity_command(const PhysicsCommand& command, entt::entity entity, entt::registry& registry);
    bool execute_position_command(const PhysicsCommand& command, entt::entity entity, entt::registry& registry);
    bool execute_state_command(const PhysicsCommand& command, entt::entity entity, entt::registry& registry);
    bool execute_query_command(const PhysicsCommand& command, entt::entity entity, entt::registry& registry);
    bool execute_custom_command(const PhysicsCommand& command, entt::entity entity, entt::registry& registry);

    // 命令驗證
    bool validate_command(const PhysicsCommand& command, entt::entity entity, entt::registry& registry) const;
    bool has_required_components(entt::entity entity, entt::registry& registry) const;

    // 輔助方法
    PhysicsBodyComponent* get_physics_body(entt::entity entity, entt::registry& registry) const;
    TransformComponent* get_transform(entt::entity entity, entt::registry& registry) const;
    PhysicsWorldManager* get_physics_world() const;

    // 命令清理
    void cleanup_executed_commands(entt::registry& registry);
    void remove_command_from_vector(std::vector<PhysicsCommand>& commands, uint64_t command_id);

private:
    // 物理世界管理器引用
    PhysicsWorldManager* physics_world_ = nullptr;

    // 系統狀態
    bool enabled_ = true;
    bool initialized_ = false;

    // 性能控制
    uint32_t max_commands_per_frame_ = 1000;  // 每幀最大執行命令數
    uint32_t commands_executed_this_frame_ = 0;

    // 統計數據
    mutable CommandSystemStats stats_;

    // 執行狀態跟踪
    std::unordered_set<entt::entity> entities_processed_this_frame_;

    // 延遲命令計時
    float delta_time_accumulator_ = 0.0f;
};

/**
 * 物理查詢系統
 * 負責處理PhysicsQueryComponent中的物理查詢請求
 * 通常在物理步進後運行，確保查詢基於最新的物理狀態
 */
class PhysicsQuerySystem : public ISystem {
public:
    PhysicsQuerySystem() = default;
    virtual ~PhysicsQuerySystem() = default;

    // ISystem 接口實現
    virtual bool initialize() override;
    virtual void update(entt::registry& registry, float delta_time) override;
    virtual void cleanup() override;
    virtual const char* get_name() const override { return "PhysicsQuerySystem"; }

    // 查詢執行控制
    void set_enabled(bool enabled) { enabled_ = enabled; }
    bool is_enabled() const { return enabled_; }

    void set_max_queries_per_frame(uint32_t max_queries) { max_queries_per_frame_ = max_queries; }
    uint32_t get_max_queries_per_frame() const { return max_queries_per_frame_; }

    // 統計信息
    struct QuerySystemStats {
        uint32_t raycast_queries_executed = 0;
        uint32_t overlap_queries_executed = 0;
        uint32_t distance_queries_executed = 0;
        uint32_t total_queries_executed = 0;
        uint32_t queries_failed = 0;
        float execution_time = 0.0f;
    };

    const QuerySystemStats& get_stats() const { return stats_; }

protected:
    // 查詢執行方法
    void execute_raycast_queries(entt::registry& registry);
    void execute_overlap_queries(entt::registry& registry);
    void execute_distance_queries(entt::registry& registry);

    // 單個查詢執行
    bool execute_raycast_query(PhysicsQueryComponent::RaycastQuery& query, entt::entity entity, entt::registry& registry);
    bool execute_overlap_query(PhysicsQueryComponent::OverlapQuery& query, entt::entity entity, entt::registry& registry);
    bool execute_distance_query(PhysicsQueryComponent::DistanceQuery& query, entt::entity entity, entt::registry& registry);

    // 輔助方法
    PhysicsWorldManager* get_physics_world() const;
    entt::entity body_id_to_entity(JPH::BodyID body_id, entt::registry& registry) const;
    bool passes_layer_filter(entt::entity entity, uint32_t layer_mask, entt::registry& registry) const;

private:
    // 物理世界管理器引用
    PhysicsWorldManager* physics_world_ = nullptr;

    // 系統狀態
    bool enabled_ = true;
    bool initialized_ = false;

    // 性能控制
    uint32_t max_queries_per_frame_ = 100;  // 每幀最大執行查詢數
    uint32_t queries_executed_this_frame_ = 0;

    // 統計數據
    mutable QuerySystemStats stats_;
};

/**
 * 工廠函數
 */
std::unique_ptr<ISystem> create_physics_command_system();
std::unique_ptr<ISystem> create_physics_query_system();

// 系統註冊宏
#define REGISTER_PHYSICS_COMMAND_SYSTEM() \
    SystemRegistry::register_system<PhysicsCommandSystem>("PhysicsCommandSystem", {"TransformSystem"}, create_physics_command_system)

#define REGISTER_PHYSICS_QUERY_SYSTEM() \
    SystemRegistry::register_system<PhysicsQuerySystem>("PhysicsQuerySystem", {"PhysicsSystem"}, create_physics_query_system)

} // namespace portal_core
