#pragma once

#include "../system_base.h"
#include "../physics_world_manager.h"
#include "../components/physics_body_component.h"
#include "../components/transform_component.h"
#include "../components/physics_sync_component.h"
#include "../components/physics_event_component.h"
#include <entt/entt.hpp>
#include <unordered_map>
#include <unordered_set>

namespace portal_core
{

    /**
     * 物理系統
     * 負責物理世界的步進、物理體的創建/銷毀、以及物理與Transform的同步
     */
    class PhysicsSystem : public ISystem
    {
    public:
        PhysicsSystem() = default;
        virtual ~PhysicsSystem() = default;

        // ISystem 接口實現
        virtual bool initialize() override;
        virtual void update(entt::registry &registry, float delta_time) override;
        virtual void cleanup() override;
        virtual const char *get_name() const override { return "PhysicsSystem"; }

        // 擴展的初始化方法（設置組件監聽器）
        bool initialize(entt::registry &registry);

        // 物理體生命周期管理
        void create_physics_body(entt::entity entity, entt::registry &registry);
        void destroy_physics_body(entt::entity entity, entt::registry &registry);
        void update_physics_body_properties(entt::entity entity, entt::registry &registry);

        // 同步方法
        void sync_physics_to_transform(entt::registry &registry);
        void sync_transform_to_physics(entt::registry &registry);

        // 物理體查詢
        entt::entity get_entity_by_body_id(JPH::BodyID body_id) const;
        JPH::BodyID get_body_id_by_entity(entt::entity entity) const;

        // 統計信息
        struct PhysicsSystemStats
        {
            uint32_t num_physics_bodies = 0;
            uint32_t num_active_bodies = 0;
            uint32_t num_sleeping_bodies = 0;
            uint32_t num_sync_operations = 0;
            float physics_step_time = 0.0f;
            float sync_time = 0.0f;
        };

        const PhysicsSystemStats &get_stats() const { return stats_; }

        // 配置選項
        void set_auto_create_bodies(bool enable) { auto_create_bodies_ = enable; }
        void set_auto_sync_enabled(bool enable) { auto_sync_enabled_ = enable; }
        void set_debug_rendering_enabled(bool enable) { debug_rendering_enabled_ = enable; }

    protected:
        // 內部組件檢測和處理
        void on_physics_body_added(entt::registry &registry, entt::entity entity);
        void on_physics_body_removed(entt::registry &registry, entt::entity entity);
        void on_transform_updated(entt::registry &registry, entt::entity entity);

        // 物理體創建輔助
        bool create_jolt_body(entt::entity entity, PhysicsBodyComponent &physics_body,
                              const TransformComponent &transform);

        // 同步輔助方法
        void sync_single_entity_to_transform(entt::entity entity, entt::registry &registry);
        void sync_single_entity_to_physics(entt::entity entity, entt::registry &registry);

        // 碰撞事件處理
        void handle_collision_events(entt::registry &registry);
        void handle_trigger_events(entt::registry &registry);

    private:
        // 物理世界管理器引用
        PhysicsWorldManager *physics_world_ = nullptr;

        // 實體與物理體的映射
        std::unordered_map<entt::entity, JPH::BodyID> entity_to_body_;
        std::unordered_map<JPH::BodyID, entt::entity> body_to_entity_;

        // 待創建和待銷毀的物理體
        std::unordered_set<entt::entity> pending_creation_;
        std::unordered_set<entt::entity> pending_destruction_;

        // 需要同步的實體
        std::unordered_set<entt::entity> entities_needing_physics_sync_;
        std::unordered_set<entt::entity> entities_needing_transform_sync_;

        // 系統配置
        bool auto_create_bodies_ = true;
        bool auto_sync_enabled_ = true;
        bool debug_rendering_enabled_ = false;
        bool physics_world_initialized_ = false;

        // 性能統計
        mutable PhysicsSystemStats stats_;

        // 計時器
        float accumulator_time_ = 0.0f;
        uint32_t frame_counter_ = 0;

        // EnTT 連接器（用於監聽組件添加/移除事件）
        entt::connection physics_body_added_connection_;
        entt::connection physics_body_removed_connection_;
        entt::connection transform_updated_connection_;

        // 內部方法

        /**
         * 初始化物理世界
         */
        bool initialize_physics_world();

        /**
         * 處理待創建的物理體
         */
        void process_pending_creations(entt::registry &registry);

        /**
         * 處理待銷毀的物理體
         */
        void process_pending_destructions(entt::registry &registry);

        /**
         * 更新統計信息
         */
        void update_statistics(entt::registry &registry, float delta_time);

        /**
         * 驗證物理體組件的有效性
         */
        bool validate_physics_body_component(const PhysicsBodyComponent &component) const;

        /**
         * 設置物理體的用戶數據為entity ID
         */
        void set_body_user_data(JPH::BodyID body_id, entt::entity entity);

        /**
         * 從用戶數據獲取entity
         */
        entt::entity get_entity_from_user_data(uint64_t user_data) const;

        /**
         * 清理映射
         */
        void cleanup_entity_mapping(entt::entity entity);

        /**
         * 處理物理體屬性變更
         */
        void handle_physics_properties_changed(entt::entity entity, entt::registry &registry);

        /**
         * 檢查是否需要重新創建物理體
         */
        bool needs_body_recreation(const PhysicsBodyComponent &old_component,
                                   const PhysicsBodyComponent &new_component) const;

        /**
         * 應用物理體設定到Jolt物理體
         */
        void apply_physics_settings(JPH::BodyID body_id, const PhysicsBodyComponent &component);

        /**
         * 處理調試渲染
         */
        void update_debug_rendering(entt::registry &registry);
    };

    /**
     * 物理系統的工廠函數
     */
    std::unique_ptr<ISystem> create_physics_system();

    // 自動註冊物理系統
    REGISTER_SYSTEM(PhysicsSystem, {"PhysicsCommandSystem"}, {}, 20);

} // namespace portal_core
