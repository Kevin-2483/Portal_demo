#include "render_system.h"
#include <cmath>
#include <algorithm>

namespace Portal {

RenderSystem::RenderSystem()
    : registry_(nullptr)
    , stencil_enabled_(false)
    , stencil_ref_value_(1)
    , is_initialized_(false)
{
    // 設置默認相機參數
    main_camera_.position = Vector3(0, 0, 5);
    main_camera_.rotation = Quaternion(0, 0, 0, 1);
    main_camera_.fov = 75.0f;
    main_camera_.near_plane = 0.1f;
    main_camera_.far_plane = 1000.0f;
    main_camera_.aspect_ratio = 16.0f / 9.0f;
}

bool RenderSystem::initialize() {
    if (is_initialized_) {
        return true;
    }

    reset_render_state();
    is_initialized_ = true;
    return true;
}

void RenderSystem::shutdown() {
    if (!is_initialized_) {
        return;
    }

    portal_render_data_.clear();
    reset_render_state();
    is_initialized_ = false;
}

// === IRenderQuery 實現 ===

bool RenderSystem::is_point_in_view_frustum(const Vector3& point, const CameraParams& camera) const {
    // 計算視錐體平面
    Vector3 planes[6];
    float distances[6];
    calculate_frustum_planes(camera, planes, distances);

    // 檢查點是否在所有平面的內側
    for (int i = 0; i < 6; ++i) {
        if (!point_in_frustum_plane(point, planes[i], distances[i])) {
            return false;
        }
    }

    return true;
}

Frustum RenderSystem::calculate_frustum(const CameraParams& camera) const {
    Frustum frustum;

    // 計算視錐體頂點
    float half_fov_rad = camera.fov * 0.5f * M_PI / 180.0f;
    float tan_half_fov = std::tan(half_fov_rad);
    
    float near_height = 2.0f * tan_half_fov * camera.near_plane;
    float near_width = near_height * camera.aspect_ratio;
    float far_height = 2.0f * tan_half_fov * camera.far_plane;
    float far_width = far_height * camera.aspect_ratio;

    // 相機的前向、右向、上向向量
    Vector3 forward = camera.rotation.rotate_vector(Vector3(0, 0, -1));
    Vector3 right = camera.rotation.rotate_vector(Vector3(1, 0, 0));
    Vector3 up = camera.rotation.rotate_vector(Vector3(0, 1, 0));

    // 近平面中心和遠平面中心
    Vector3 near_center = camera.position + forward * camera.near_plane;
    Vector3 far_center = camera.position + forward * camera.far_plane;

    // 計算8個頂點
    // 近平面的4個頂點
    frustum.vertices[0] = near_center + up * (near_height * 0.5f) - right * (near_width * 0.5f); // 左上
    frustum.vertices[1] = near_center + up * (near_height * 0.5f) + right * (near_width * 0.5f); // 右上
    frustum.vertices[2] = near_center - up * (near_height * 0.5f) - right * (near_width * 0.5f); // 左下
    frustum.vertices[3] = near_center - up * (near_height * 0.5f) + right * (near_width * 0.5f); // 右下

    // 遠平面的4個頂點
    frustum.vertices[4] = far_center + up * (far_height * 0.5f) - right * (far_width * 0.5f); // 左上
    frustum.vertices[5] = far_center + up * (far_height * 0.5f) + right * (far_width * 0.5f); // 右上
    frustum.vertices[6] = far_center - up * (far_height * 0.5f) - right * (far_width * 0.5f); // 左下
    frustum.vertices[7] = far_center - up * (far_height * 0.5f) + right * (far_width * 0.5f); // 右下

    // 計算6個平面的法向量和距離
    calculate_frustum_planes(camera, frustum.planes, frustum.plane_distances);

    return frustum;
}

void RenderSystem::calculate_frustum_planes(const CameraParams& camera, Vector3 planes[6], float distances[6]) const {
    // 相機的前向、右向、上向向量
    Vector3 forward = camera.rotation.rotate_vector(Vector3(0, 0, -1));
    Vector3 right = camera.rotation.rotate_vector(Vector3(1, 0, 0));
    Vector3 up = camera.rotation.rotate_vector(Vector3(0, 1, 0));

    float half_fov_rad = camera.fov * 0.5f * M_PI / 180.0f;
    float tan_half_fov = std::tan(half_fov_rad);
    float half_width = tan_half_fov * camera.aspect_ratio;

    // 近平面和遠平面
    planes[0] = forward;                    // 近平面法向量
    distances[0] = forward.dot(camera.position + forward * camera.near_plane);
    
    planes[1] = forward * -1.0f;           // 遠平面法向量
    distances[1] = planes[1].dot(camera.position + forward * camera.far_plane);

    // 左、右、上、下平面
    Vector3 left_normal = (forward + right * tan_half_fov).cross(up).normalized();
    Vector3 right_normal = up.cross(forward + right * (-tan_half_fov)).normalized();
    Vector3 top_normal = (forward + up * (-tan_half_fov)).cross(right).normalized();
    Vector3 bottom_normal = right.cross(forward + up * tan_half_fov).normalized();

    planes[2] = left_normal;               // 左平面
    distances[2] = left_normal.dot(camera.position);
    
    planes[3] = right_normal;              // 右平面
    distances[3] = right_normal.dot(camera.position);
    
    planes[4] = top_normal;                // 上平面
    distances[4] = top_normal.dot(camera.position);
    
    planes[5] = bottom_normal;             // 下平面
    distances[5] = bottom_normal.dot(camera.position);
}

bool RenderSystem::point_in_frustum_plane(const Vector3& point, const Vector3& plane_normal, float plane_distance) const {
    return plane_normal.dot(point) >= plane_distance;
}

// === IRenderManipulator 實現 ===

void RenderSystem::set_portal_render_texture(PortalId portal_id, const CameraParams& virtual_camera) {
    // 查找現有的傳送門渲染數據
    for (auto& data : portal_render_data_) {
        if (data.portal_id == portal_id) {
            data.virtual_camera = virtual_camera;
            data.is_active = true;
            return;
        }
    }

    // 添加新的傳送門渲染數據
    PortalRenderData data;
    data.portal_id = portal_id;
    data.virtual_camera = virtual_camera;
    data.is_active = true;
    portal_render_data_.push_back(data);
}

void RenderSystem::set_entity_render_enabled(EntityId entity_id, bool enabled) {
    if (!registry_) {
        return;
    }

    // 在實際實現中，這裡應該通過某種ID映射找到對應的ECS實體
    // 目前這是一個簡化的實現
    auto view = registry_->view<RenderComponent>();
    for (auto entity : view) {
        auto& render_comp = view.get<RenderComponent>(entity);
        render_comp.visible = enabled;
        break; // 簡化實現：只處理第一個找到的實體
    }
}

void RenderSystem::configure_stencil_buffer(bool enable, int ref_value) {
    stencil_enabled_ = enable;
    stencil_ref_value_ = ref_value;
    
    // 在實際實現中，這裡應該調用圖形API來配置stencil buffer
    // 例如：glStencilFunc, glStencilOp 等
}

void RenderSystem::set_clipping_plane(const ClippingPlane& plane) {
    active_clipping_plane_ = plane;
    
    // 在實際實現中，這裡應該設置硬件裁剪平面
    // 例如：glClipPlane 或現代API的等價功能
}

void RenderSystem::disable_clipping_plane() {
    active_clipping_plane_.enabled = false;
    
    // 在實際實現中，這裡應該禁用硬件裁剪平面
}

void RenderSystem::reset_render_state() {
    stencil_enabled_ = false;
    stencil_ref_value_ = 1;
    active_clipping_plane_.enabled = false;
    
    // 清理傳送門渲染數據
    for (auto& data : portal_render_data_) {
        data.is_active = false;
    }
    
    // 在實際實現中，這裡應該重置所有渲染狀態到默認值
}

void RenderSystem::render_portal_recursive_view(PortalId portal_id, int recursion_depth) {
    // 在實際實現中，這裡應該：
    // 1. 設置適當的stencil buffer值
    // 2. 配置裁剪平面
    // 3. 使用虛擬相機渲染場景
    // 4. 遞歸處理下一層傳送門（如果需要）
    
    // 簡化實現：記錄渲染請求
    for (auto& data : portal_render_data_) {
        if (data.portal_id == portal_id && data.is_active) {
            // 這裡應該執行實際的渲染操作
            break;
        }
    }
}

void RenderSystem::update_render_components() {
    if (!registry_) {
        return;
    }

    // 更新所有渲染組件
    auto view = registry_->view<RenderComponent, TransformComponent>();
    view.each([&](auto entity, auto& render_comp, const auto& transform_comp) {
        // 執行視錐體裁剪檢查
        if (render_comp.visible) {
            bool in_frustum = is_point_in_view_frustum(transform_comp.position, main_camera_);
            // 在實際實現中，這裡可能需要更複雜的可見性檢查
            // 例如遮擋裁剪、距離裁剪等
        }
    });
}

// === RenderUpdateSystem 實現 ===

void RenderUpdateSystem::update() {
    update_visibility();
    update_portal_textures();
    
    if (render_system_) {
        render_system_->update_render_components();
    }
}

void RenderUpdateSystem::update_visibility() {
    if (!render_system_ || !render_system_->get_registry()) {
        return;
    }

    entt::registry* registry = render_system_->get_registry();
    const CameraParams& main_camera = render_system_->get_main_camera_ref();

    // 更新實體的可見性
    auto view = registry->view<RenderComponent, TransformComponent>();
    view.each([&](auto entity, auto& render_comp, const auto& transform_comp) {
        // 基本的視錐體裁剪
        bool was_visible = render_comp.visible;
        bool should_be_visible = render_system_->is_point_in_view_frustum(transform_comp.position, main_camera);
        
        if (was_visible != should_be_visible) {
            render_comp.visible = should_be_visible;
        }
    });
}

void RenderUpdateSystem::update_portal_textures() {
    // 在實際實現中，這裡應該：
    // 1. 檢查哪些傳送門在視野內
    // 2. 更新傳送門的渲染紋理
    // 3. 處理遞歸渲染
}

} // namespace Portal
