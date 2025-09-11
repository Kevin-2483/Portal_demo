#pragma once

#include "core/render/unified_render_types.h"
#include <godot_cpp/classes/node3d.hpp>
#include <godot_cpp/classes/immediate_mesh.hpp>
#include <godot_cpp/classes/mesh_instance3d.hpp>
#include <godot_cpp/classes/standard_material3d.hpp>
#include <godot_cpp/variant/vector3.hpp>
#include <vector>

namespace portal_gdext {
namespace render {

/**
 * Godot 3D渲染器
 * 处理3D世界空间的调试绘制命令
 */
class GodotRenderer3D {
private:
    godot::Node3D* world_node_;
    godot::MeshInstance3D* mesh_instance_;
    godot::Ref<godot::ImmediateMesh> immediate_mesh_;
    godot::Ref<godot::StandardMaterial3D> material_;
    
    std::vector<portal_core::render::UnifiedRenderCommand> commands_;
    bool enabled_;
    
public:
    GodotRenderer3D();
    ~GodotRenderer3D();
    
    // 初始化
    bool initialize(godot::Node3D* world_node);
    void shutdown();
    
    // 命令处理
    void submit_command(const portal_core::render::UnifiedRenderCommand& command);
    void clear_commands();
    void render();
    void update(float delta_time);
    
    // 状态
    bool is_enabled() const { return enabled_; }
    void set_enabled(bool enabled) { enabled_ = enabled; }
    
    // 统计
    size_t get_command_count() const { return commands_.size(); }
    
private:
    void setup_mesh();
    void render_line_3d(const portal_core::render::Line3DData& data);
    void render_box_3d(const portal_core::render::Box3DData& data);
    void render_sphere_3d(const portal_core::render::Sphere3DData& data);
    
    // 转换辅助函数
    godot::Vector3 to_godot_vector3(const portal_core::Vector3& vec) const;
    godot::Color to_godot_color(const portal_core::render::Color4f& color) const;
    void add_line_to_mesh(const godot::Vector3& start, const godot::Vector3& end, const godot::Color& color);
    void add_box_wireframe_to_mesh(const godot::Vector3& center, const godot::Vector3& size, const godot::Color& color);
    void add_sphere_wireframe_to_mesh(const godot::Vector3& center, float radius, const godot::Color& color, int segments);
};

}} // namespace portal_gdext::render
