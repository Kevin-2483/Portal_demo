#ifndef PORTAL_GAME_WORLD_H
#define PORTAL_GAME_WORLD_H

#include <entt/entt.hpp>
#include <memory>

#include "portal_core/lib/include/portal_core.h"
#include "systems/jolt_physics_system.h"
#include "systems/render_system.h"

namespace Portal {

/**
 * 傳送門遊戲世界管理器
 * 整合ECS、物理系統、渲染系統和傳送門系統
 */
class PortalGameWorld {
public:
    PortalGameWorld();
    ~PortalGameWorld();

    // 禁用拷貝構造和賦值
    PortalGameWorld(const PortalGameWorld&) = delete;
    PortalGameWorld& operator=(const PortalGameWorld&) = delete;

    // === 初始化和清理 ===
    bool initialize();
    void shutdown();
    bool is_initialized() const { return is_initialized_; }

    // === 每幀更新 ===
    void update(float delta_time);

    // === ECS訪問 ===
    entt::registry& get_registry() { return registry_; }
    const entt::registry& get_registry() const { return registry_; }

    // === 系統訪問 ===
    JoltPhysicsSystem* get_physics_system() { return physics_system_.get(); }
    RenderSystem* get_render_system() { return render_system_.get(); }
    PortalManager* get_portal_manager() { return portal_manager_.get(); }

    const JoltPhysicsSystem* get_physics_system() const { return physics_system_.get(); }
    const RenderSystem* get_render_system() const { return render_system_.get(); }
    const PortalManager* get_portal_manager() const { return portal_manager_.get(); }

    // === 實體創建工廠方法 ===

    /**
     * 創建基本的物理實體
     */
    entt::entity create_physics_entity(
        const Vector3& position,
        const Quaternion& rotation = Quaternion(),
        CollisionShapeType shape_type = CollisionShapeType::Box,
        const Vector3& dimensions = Vector3(1, 1, 1),
        bool is_dynamic = true,
        float mass = 1.0f
    );

    /**
     * 創建傳送門實體
     */
    entt::entity create_portal_entity(
        const Vector3& center,
        const Vector3& normal,
        const Vector3& up = Vector3(0, 1, 0),
        float width = 2.0f,
        float height = 3.0f
    );

    /**
     * 創建靜態環境物體
     */
    entt::entity create_static_entity(
        const Vector3& position,
        const Quaternion& rotation = Quaternion(),
        CollisionShapeType shape_type = CollisionShapeType::Box,
        const Vector3& dimensions = Vector3(1, 1, 1)
    );

    /**
     * 銷毀實體
     */
    void destroy_entity(entt::entity entity);

    // === 傳送門管理輔助方法 ===

    /**
     * 鏈接兩個傳送門實體
     */
    bool link_portal_entities(entt::entity portal1, entt::entity portal2);

    /**
     * 註冊實體到傳送門系統
     */
    void register_entity_for_teleportation(entt::entity entity);

    /**
     * 取消註冊實體
     */
    void unregister_entity_for_teleportation(entt::entity entity);

    // === 相機管理 ===
    void set_main_camera(const CameraParams& camera);
    CameraParams get_main_camera() const;

    // === 調試和統計 ===
    size_t get_entity_count() const; // 在 .cpp 中實現
    size_t get_portal_count() const;
    size_t get_physics_body_count() const;

    // === 場景管理 ===
    void clear_scene();
    void reset_physics();

private:
    // 內部更新方法
    void update_physics(float delta_time);
    void update_portal_system(float delta_time);
    void update_render_system();

    // 系統初始化
    bool initialize_physics_system();
    bool initialize_render_system();
    bool initialize_portal_system();

    // ECS註冊表
    entt::registry registry_;

    // 核心系統
    std::unique_ptr<JoltPhysicsSystem> physics_system_;
    std::unique_ptr<RenderSystem> render_system_;
    std::unique_ptr<PortalManager> portal_manager_;

    // 更新系統
    std::unique_ptr<PhysicsUpdateSystem> physics_update_system_;
    std::unique_ptr<RenderUpdateSystem> render_update_system_;

    // 狀態
    bool is_initialized_;
    float accumulated_time_;
    
    // 配置參數
    static constexpr float FIXED_TIMESTEP = 1.0f / 60.0f; // 60 FPS固定時間步長
};

/**
 * 遊戲世界建造器 - 簡化遊戲世界的創建和配置
 */
class PortalGameWorldBuilder {
public:
    PortalGameWorldBuilder();

    // 物理配置
    PortalGameWorldBuilder& with_gravity(const Vector3& gravity);
    PortalGameWorldBuilder& with_max_bodies(unsigned int max_bodies);

    // 渲染配置
    PortalGameWorldBuilder& with_main_camera(const CameraParams& camera);

    // 傳送門配置
    PortalGameWorldBuilder& with_max_recursion_depth(int depth);

    // 建造遊戲世界
    std::unique_ptr<PortalGameWorld> build();

private:
    Vector3 gravity_;
    unsigned int max_bodies_;
    CameraParams main_camera_;
    int max_recursion_depth_;
};

} // namespace Portal

#endif // PORTAL_GAME_WORLD_H
