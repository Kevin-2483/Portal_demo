#ifndef RENDER_SYSTEM_H
#define RENDER_SYSTEM_H

#include <entt/entt.hpp>
#include "../components/physics_components.h"
#include "../portal_core/lib/include/portal_interfaces.h"
#include <vector>

namespace Portal {

/**
 * 基礎渲染系統 - 實現Portal渲染接口
 */
class RenderSystem : public IRenderQuery, public IRenderManipulator {
public:
    RenderSystem();
    virtual ~RenderSystem() = default;

    // 初始化和清理
    bool initialize();
    void shutdown();
    
    // ECS整合
    void set_registry(entt::registry* registry) { registry_ = registry; }
    entt::registry* get_registry() const { return registry_; }

    // 相機管理
    void set_main_camera(const CameraParams& camera) { main_camera_ = camera; }
    const CameraParams& get_main_camera_ref() const { return main_camera_; }

    // === IRenderQuery 實現 ===
    CameraParams get_main_camera() const override { return main_camera_; }
    bool is_point_in_view_frustum(const Vector3& point, const CameraParams& camera) const override;
    Frustum calculate_frustum(const CameraParams& camera) const override;

    // === IRenderManipulator 實現 ===
    void set_portal_render_texture(PortalId portal_id, const CameraParams& virtual_camera) override;
    void set_entity_render_enabled(EntityId entity_id, bool enabled) override;
    void configure_stencil_buffer(bool enable, int ref_value = 1) override;
    void set_clipping_plane(const ClippingPlane& plane) override;
    void disable_clipping_plane() override;
    void reset_render_state() override;
    void render_portal_recursive_view(PortalId portal_id, int recursion_depth) override;

    // 渲染更新
    void update_render_components();

private:
    // 視錐體計算輔助方法
    void calculate_frustum_planes(const CameraParams& camera, Vector3 planes[6], float distances[6]) const;
    bool point_in_frustum_plane(const Vector3& point, const Vector3& plane_normal, float plane_distance) const;

    entt::registry* registry_;
    CameraParams main_camera_;
    
    // 渲染狀態
    bool stencil_enabled_;
    int stencil_ref_value_;
    ClippingPlane active_clipping_plane_;
    
    // 傳送門渲染相關
    struct PortalRenderData {
        PortalId portal_id;
        CameraParams virtual_camera;
        bool is_active;
    };
    std::vector<PortalRenderData> portal_render_data_;
    
    bool is_initialized_;
};

/**
 * 渲染更新系統 - 處理渲染組件的更新
 */
class RenderUpdateSystem {
public:
    explicit RenderUpdateSystem(RenderSystem* render_system)
        : render_system_(render_system) {}

    void update();

private:
    void update_visibility();
    void update_portal_textures();

    RenderSystem* render_system_;
};

} // namespace Portal

#endif // RENDER_SYSTEM_H
