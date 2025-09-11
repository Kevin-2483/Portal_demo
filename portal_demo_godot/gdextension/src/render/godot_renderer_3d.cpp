#include "render/godot_renderer_3d.h"
#include <godot_cpp/core/class_db.hpp>
#include <godot_cpp/variant/utility_functions.hpp>
#include <cmath>

using namespace godot;

namespace portal_gdext
{
    namespace render
    {

        GodotRenderer3D::GodotRenderer3D()
            : world_node_(nullptr), mesh_instance_(nullptr), enabled_(true)
        {
        }

        GodotRenderer3D::~GodotRenderer3D()
        {
            shutdown();
        }

        bool GodotRenderer3D::initialize(godot::Node3D *world_node)
        {
            if (!world_node)
            {
                UtilityFunctions::printerr("GodotRenderer3D: world_node is null");
                return false;
            }

            world_node_ = world_node;

            // 创建MeshInstance3D节点
            mesh_instance_ = memnew(MeshInstance3D);
            mesh_instance_->set_name("DebugMeshInstance3D");
            world_node_->add_child(mesh_instance_);

            setup_mesh();

            UtilityFunctions::print("GodotRenderer3D initialized successfully");
            return true;
        }

        void GodotRenderer3D::shutdown()
        {
            if (mesh_instance_ && world_node_)
            {
                world_node_->remove_child(mesh_instance_);
                memdelete(mesh_instance_);
                mesh_instance_ = nullptr;
            }

            immediate_mesh_.unref();
            material_.unref();
            world_node_ = nullptr;
        }

        void GodotRenderer3D::setup_mesh()
        {
            // 创建ImmediateMesh用于动态绘制
            immediate_mesh_.instantiate();

            // 创建材质
            material_.instantiate();
            material_->set_albedo(Color(1.0, 1.0, 1.0, 1.0));
            material_->set_shading_mode(BaseMaterial3D::SHADING_MODE_UNSHADED);
            material_->set_flag(BaseMaterial3D::FLAG_ALBEDO_FROM_VERTEX_COLOR, true);
            material_->set_transparency(BaseMaterial3D::TRANSPARENCY_ALPHA);
            material_->set_flag(BaseMaterial3D::FLAG_DISABLE_DEPTH_TEST, false);

            // 设置材质到MeshInstance3D
            if (mesh_instance_)
            {
                mesh_instance_->set_material_override(material_);
                mesh_instance_->set_mesh(immediate_mesh_);
            }
        }

        void GodotRenderer3D::submit_command(const portal_core::render::UnifiedRenderCommand &command)
        {
            if (!enabled_)
                return;
            commands_.push_back(command);
        }

        void GodotRenderer3D::clear_commands()
        {
            commands_.clear();
        }

        void GodotRenderer3D::render()
        {
            if (!enabled_ || !immediate_mesh_.is_valid())
                return;

            // 清空网格
            immediate_mesh_->clear_surfaces();

            if (commands_.empty())
                return;

            // 开始绘制
            immediate_mesh_->surface_begin(Mesh::PRIMITIVE_LINES);

            // 处理所有命令
            for (const auto &command : commands_)
            {
                switch (command.type)
                {
                case portal_core::render::RenderCommandType::DRAW_LINE_3D:
                    if (command.data && command.data_size == sizeof(portal_core::render::Line3DData))
                    {
                        render_line_3d(*static_cast<const portal_core::render::Line3DData *>(command.data));
                    }
                    break;

                case portal_core::render::RenderCommandType::DRAW_BOX_3D:
                    if (command.data && command.data_size == sizeof(portal_core::render::Box3DData))
                    {
                        render_box_3d(*static_cast<const portal_core::render::Box3DData *>(command.data));
                    }
                    break;

                case portal_core::render::RenderCommandType::DRAW_SPHERE_3D:
                    if (command.data && command.data_size == sizeof(portal_core::render::Sphere3DData))
                    {
                        render_sphere_3d(*static_cast<const portal_core::render::Sphere3DData *>(command.data));
                    }
                    break;

                default:
                    // 不支持的命令类型，忽略
                    break;
                }
            }

            // 完成绘制
            immediate_mesh_->surface_end();
        }

        void GodotRenderer3D::update(float delta_time)
        {
            // 3D渲染器目前不需要特殊的更新逻辑
        }

        void GodotRenderer3D::render_line_3d(const portal_core::render::Line3DData &data)
        {
            Vector3 start = to_godot_vector3(data.start);
            Vector3 end = to_godot_vector3(data.end);
            Color color = to_godot_color(data.color);

            add_line_to_mesh(start, end, color);
        }

        void GodotRenderer3D::render_box_3d(const portal_core::render::Box3DData &data)
        {
            Vector3 center = to_godot_vector3(data.center);
            Vector3 size = to_godot_vector3(data.size);
            Color color = to_godot_color(data.color);

            add_box_wireframe_to_mesh(center, size, color);
        }

        void GodotRenderer3D::render_sphere_3d(const portal_core::render::Sphere3DData &data)
        {
            Vector3 center = to_godot_vector3(data.center);
            Color color = to_godot_color(data.color);

            add_sphere_wireframe_to_mesh(center, data.radius, color, data.segments);
        }

        Vector3 GodotRenderer3D::to_godot_vector3(const portal_core::Vector3 &vec) const
        {
            return Vector3(vec.GetX(), vec.GetY(), vec.GetZ());
        }

        Color GodotRenderer3D::to_godot_color(const portal_core::render::Color4f &color) const
        {
            return Color(color.r, color.g, color.b, color.a);
        }

        void GodotRenderer3D::add_line_to_mesh(const Vector3 &start, const Vector3 &end, const Color &color)
        {
            immediate_mesh_->surface_set_color(color);
            immediate_mesh_->surface_add_vertex(start);

            immediate_mesh_->surface_set_color(color);
            immediate_mesh_->surface_add_vertex(end);
        }

        void GodotRenderer3D::add_box_wireframe_to_mesh(const Vector3 &center, const Vector3 &size, const Color &color)
        {
            Vector3 half_size = size * 0.5f;

            // 计算8个顶点
            Vector3 vertices[8] = {
                center + Vector3(-half_size.x, -half_size.y, -half_size.z), // 0: left-bottom-back
                center + Vector3(half_size.x, -half_size.y, -half_size.z),  // 1: right-bottom-back
                center + Vector3(half_size.x, half_size.y, -half_size.z),   // 2: right-top-back
                center + Vector3(-half_size.x, half_size.y, -half_size.z),  // 3: left-top-back
                center + Vector3(-half_size.x, -half_size.y, half_size.z),  // 4: left-bottom-front
                center + Vector3(half_size.x, -half_size.y, half_size.z),   // 5: right-bottom-front
                center + Vector3(half_size.x, half_size.y, half_size.z),    // 6: right-top-front
                center + Vector3(-half_size.x, half_size.y, half_size.z)    // 7: left-top-front
            };

            // 12条边
            int edges[12][2] = {
                // 后面的4条边
                {0, 1},
                {1, 2},
                {2, 3},
                {3, 0},
                // 前面的4条边
                {4, 5},
                {5, 6},
                {6, 7},
                {7, 4},
                // 连接前后的4条边
                {0, 4},
                {1, 5},
                {2, 6},
                {3, 7}};

            // 绘制所有边
            for (int i = 0; i < 12; ++i)
            {
                add_line_to_mesh(vertices[edges[i][0]], vertices[edges[i][1]], color);
            }
        }

        void GodotRenderer3D::add_sphere_wireframe_to_mesh(const Vector3 &center, float radius, const Color &color, int segments)
        {
            if (segments < 3)
                segments = 8;

            float angle_step = 2.0f * Math_PI / segments;

            // 绘制3个圆圈 (XY, XZ, YZ 平面)

            // XY平面圆圈 (Z = 0)
            for (int i = 0; i < segments; ++i)
            {
                float angle1 = i * angle_step;
                float angle2 = (i + 1) * angle_step;

                Vector3 p1 = center + Vector3(cos(angle1) * radius, sin(angle1) * radius, 0);
                Vector3 p2 = center + Vector3(cos(angle2) * radius, sin(angle2) * radius, 0);

                add_line_to_mesh(p1, p2, color);
            }

            // XZ平面圆圈 (Y = 0)
            for (int i = 0; i < segments; ++i)
            {
                float angle1 = i * angle_step;
                float angle2 = (i + 1) * angle_step;

                Vector3 p1 = center + Vector3(cos(angle1) * radius, 0, sin(angle1) * radius);
                Vector3 p2 = center + Vector3(cos(angle2) * radius, 0, sin(angle2) * radius);

                add_line_to_mesh(p1, p2, color);
            }

            // YZ平面圆圈 (X = 0)
            for (int i = 0; i < segments; ++i)
            {
                float angle1 = i * angle_step;
                float angle2 = (i + 1) * angle_step;

                Vector3 p1 = center + Vector3(0, cos(angle1) * radius, sin(angle1) * radius);
                Vector3 p2 = center + Vector3(0, cos(angle2) * radius, sin(angle2) * radius);

                add_line_to_mesh(p1, p2, color);
            }
        }

    }
} // namespace portal_gdext::render
