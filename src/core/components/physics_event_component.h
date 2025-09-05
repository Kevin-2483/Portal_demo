#pragma once

#include "../math_types.h"
#include <entt/entt.hpp>
#include <functional>
#include <unordered_set>

namespace portal_core {

/**
 * 物理體事件組件
 * 存儲物理體事件回調和狀態
 */
struct PhysicsEventComponent {
    // 碰撞事件回調類型
    using CollisionCallback = std::function<void(entt::entity self, entt::entity other, const Vec3& contact_point, const Vec3& contact_normal)>;
    using TriggerCallback = std::function<void(entt::entity self, entt::entity other, bool is_entering)>;
    
    // 事件回調
    CollisionCallback on_collision_enter;
    CollisionCallback on_collision_stay;
    CollisionCallback on_collision_exit;
    TriggerCallback on_trigger_enter;
    TriggerCallback on_trigger_exit;
    
    // 事件過濾
    uint32_t collision_event_mask = 0xFFFFFFFF;  // 只對特定層的碰撞產生事件
    bool enable_collision_events = true;
    bool enable_trigger_events = true;
    
    // 事件狀態跟踪
    std::unordered_set<entt::entity> current_collisions;
    std::unordered_set<entt::entity> current_triggers;
    
    PhysicsEventComponent() = default;
    
    /**
     * 設置碰撞事件回調
     */
    void set_collision_callbacks(CollisionCallback enter, CollisionCallback stay = nullptr, CollisionCallback exit = nullptr) {
        on_collision_enter = std::move(enter);
        on_collision_stay = std::move(stay);
        on_collision_exit = std::move(exit);
        enable_collision_events = true;
    }
    
    /**
     * 設置觸發器事件回調
     */
    void set_trigger_callbacks(TriggerCallback enter, TriggerCallback exit = nullptr) {
        on_trigger_enter = std::move(enter);
        on_trigger_exit = std::move(exit);
        enable_trigger_events = true;
    }
    
    /**
     * 處理碰撞進入
     */
    void handle_collision_enter(entt::entity self, entt::entity other, const Vec3& contact_point, const Vec3& contact_normal) {
        if (enable_collision_events && on_collision_enter) {
            current_collisions.insert(other);
            on_collision_enter(self, other, contact_point, contact_normal);
        }
    }
    
    /**
     * 處理碰撞退出
     */
    void handle_collision_exit(entt::entity self, entt::entity other) {
        if (enable_collision_events && current_collisions.count(other)) {
            current_collisions.erase(other);
            if (on_collision_exit) {
                on_collision_exit(self, other, Vec3(0,0,0), Vec3(0,0,0));
            }
        }
    }
    
    /**
     * 處理觸發器進入
     */
    void handle_trigger_enter(entt::entity self, entt::entity other) {
        if (enable_trigger_events && on_trigger_enter) {
            current_triggers.insert(other);
            on_trigger_enter(self, other, true);
        }
    }
    
    /**
     * 處理觸發器退出
     */
    void handle_trigger_exit(entt::entity self, entt::entity other) {
        if (enable_trigger_events && current_triggers.count(other)) {
            current_triggers.erase(other);
            if (on_trigger_exit) {
                on_trigger_exit(self, other, false);
            }
        }
    }
};

} // namespace portal_core
