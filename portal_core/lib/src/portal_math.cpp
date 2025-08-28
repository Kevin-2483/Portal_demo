#include "portal_math.h"
#include <cmath>
#include <algorithm>

namespace Portal {
namespace Math {

    Vector3 PortalMath::transform_point_through_portal(
        const Vector3& point,
        const PortalPlane& source_plane,
        const PortalPlane& target_plane,
        PortalFace source_face,
        PortalFace target_face
    ) {
        // 获取实际的面法向量
        Vector3 source_normal = source_plane.get_face_normal(source_face);
        Vector3 target_normal = target_plane.get_face_normal(target_face);
        
        // 将点转换到源传送门的本地空间
        Vector3 relative_to_source = point - source_plane.center;
        
        // 投影到源传送门平面的本地坐标系
        float right_component = relative_to_source.dot(source_plane.right);
        float up_component = relative_to_source.dot(source_plane.up);
        float forward_component = relative_to_source.dot(source_normal);
        
        // 计算缩放因子
        float scale_factor = calculate_scale_factor(source_plane, target_plane);
        
        // A面对应A面，B面对应B面 - 正确的面对面映射
        // 由于面总是相互对应，目标面的法向量应该与源面相反
        Vector3 target_relative = 
            target_plane.right * (right_component * scale_factor) +
            target_plane.up * (up_component * scale_factor) +
            target_normal * (-forward_component * scale_factor);
        
        return target_plane.center + target_relative;
    }
    
    // 向後兼容的重載版本 - 默认A面对B面
    Vector3 PortalMath::transform_point_through_portal(
        const Vector3& point,
        const PortalPlane& source_plane,
        const PortalPlane& target_plane
    ) {
        return transform_point_through_portal(point, source_plane, target_plane, PortalFace::A, PortalFace::B);
    }

    Vector3 PortalMath::transform_direction_through_portal(
        const Vector3& direction,
        const PortalPlane& source_plane,
        const PortalPlane& target_plane,
        PortalFace source_face,
        PortalFace target_face
    ) {
        // 获取实际的面法向量
        Vector3 source_normal = source_plane.get_face_normal(source_face);
        Vector3 target_normal = target_plane.get_face_normal(target_face);
        
        // 分解方向向量到源传送门的本地坐标系
        float right_component = direction.dot(source_plane.right);
        float up_component = direction.dot(source_plane.up);
        float forward_component = direction.dot(source_normal);
        
        // A面对应A面，B面对应B面 - 方向变换保持一致性
        Vector3 transformed_direction = 
            target_plane.right * right_component +
            target_plane.up * up_component +
            target_normal * (-forward_component);
        
        return transformed_direction.normalized();
    }
    
    // 向後兼容的重載版本 - 默认A面对B面
    Vector3 PortalMath::transform_direction_through_portal(
        const Vector3& direction,
        const PortalPlane& source_plane,
        const PortalPlane& target_plane
    ) {
        return transform_direction_through_portal(direction, source_plane, target_plane, PortalFace::A, PortalFace::B);
    }

    Transform PortalMath::transform_through_portal(
        const Transform& transform,
        const PortalPlane& source_plane,
        const PortalPlane& target_plane
    ) {
        // 转换位置
        Vector3 new_position = transform_point_through_portal(
            transform.position, source_plane, target_plane
        );
        
        // 计算旋转变换
        // 首先计算从源传送门到目标传送门的旋转
        Quaternion portal_rotation = rotation_between_vectors(source_plane.normal, target_plane.normal * -1.0f);
        
        // 应用传送门旋转到对象旋转
        Quaternion new_rotation = portal_rotation * transform.rotation;
        
        // 计算缩放
        float scale_factor = calculate_scale_factor(source_plane, target_plane);
        Vector3 new_scale = transform.scale * scale_factor;
        
        return Transform(new_position, new_rotation, new_scale);
    }

    PhysicsState PortalMath::transform_physics_state_through_portal(
        const PhysicsState& physics_state,
        const PortalPlane& source_plane,
        const PortalPlane& target_plane,
        PortalFace source_face,
        PortalFace target_face
    ) {
        PhysicsState new_state;
        
        // 转换线性速度
        new_state.linear_velocity = transform_direction_through_portal(
            physics_state.linear_velocity, source_plane, target_plane, source_face, target_face
        ) * physics_state.linear_velocity.length();
        
        // 转换角速度
        new_state.angular_velocity = transform_direction_through_portal(
            physics_state.angular_velocity, source_plane, target_plane, source_face, target_face
        ) * physics_state.angular_velocity.length();
        
        // 质量保持不变
        new_state.mass = physics_state.mass;
        
        return new_state;
    }
    
    // 向後兼容的重載版本 - 默认A面对B面
    PhysicsState PortalMath::transform_physics_state_through_portal(
        const PhysicsState& physics_state,
        const PortalPlane& source_plane,
        const PortalPlane& target_plane
    ) {
        return transform_physics_state_through_portal(physics_state, source_plane, target_plane, PortalFace::A, PortalFace::B);
    }

    PhysicsState PortalMath::transform_physics_state_with_portal_velocity(
        const PhysicsState& entity_physics_state,
        const PhysicsState& source_portal_physics,
        const PhysicsState& target_portal_physics,
        const PortalPlane& source_plane,
        const PortalPlane& target_plane,
        float delta_time
    ) {
        // 首先计算基础的物理状态变换
        PhysicsState base_transformed = transform_physics_state_through_portal(
            entity_physics_state, source_plane, target_plane
        );
        
        // 计算传送门的相对速度贡献
        Vector3 source_portal_velocity = source_portal_physics.linear_velocity;
        Vector3 target_portal_velocity = target_portal_physics.linear_velocity;
        
        // 变换源传送门速度到目标传送门坐标系
        Vector3 transformed_source_velocity = transform_direction_through_portal(
            source_portal_velocity, source_plane, target_plane
        ) * source_portal_velocity.length();
        
        // 计算速度差异（目标传送门速度 - 变换后的源传送门速度）
        Vector3 portal_velocity_difference = target_portal_velocity - transformed_source_velocity;
        
        // 将传送门速度差异添加到实体速度上
        base_transformed.linear_velocity = base_transformed.linear_velocity + portal_velocity_difference;
        
        // 如果传送门有角速度，也需要考虑
        Vector3 source_angular_velocity = source_portal_physics.angular_velocity;
        Vector3 target_angular_velocity = target_portal_physics.angular_velocity;
        
        Vector3 transformed_source_angular = transform_direction_through_portal(
            source_angular_velocity, source_plane, target_plane
        ) * source_angular_velocity.length();
        
        Vector3 angular_velocity_difference = target_angular_velocity - transformed_source_angular;
        base_transformed.angular_velocity = base_transformed.angular_velocity + angular_velocity_difference;
        
        return base_transformed;
    }

    Vector3 PortalMath::calculate_relative_velocity(
        const Vector3& entity_velocity,
        const Vector3& portal_velocity,
        const Vector3& contact_point,
        const PortalPlane& portal_plane
    ) {
        // 计算实体相对于传送门的速度
        Vector3 relative_velocity = entity_velocity - portal_velocity;
        
        // 如果传送门在旋转，还需要考虑旋转引起的速度
        // 这里简化处理，只考虑线性速度差异
        // 在更复杂的实现中，可以加入角速度的影响
        
        return relative_velocity;
    }

    bool PortalMath::is_point_in_portal_bounds(
        const Vector3& point,
        const PortalPlane& portal_plane
    ) {
        Vector3 relative_point = point - portal_plane.center;
        
        float right_distance = std::abs(relative_point.dot(portal_plane.right));
        float up_distance = std::abs(relative_point.dot(portal_plane.up));
        
        return right_distance <= portal_plane.width * 0.5f && 
               up_distance <= portal_plane.height * 0.5f;
    }

    bool PortalMath::line_intersects_portal_plane(
        const Vector3& start,
        const Vector3& end,
        const PortalPlane& portal_plane,
        Vector3& intersection_point
    ) {
        Vector3 line_direction = end - start;
        float line_length = line_direction.length();
        
        if (line_length < EPSILON) {
            return false;
        }
        
        line_direction = line_direction * (1.0f / line_length);
        
        // 检查线段是否与平面平行
        float denominator = line_direction.dot(portal_plane.normal);
        if (std::abs(denominator) < EPSILON) {
            return false;
        }
        
        // 计算交点参数
        Vector3 to_plane = portal_plane.center - start;
        float t = to_plane.dot(portal_plane.normal) / denominator;
        
        // 检查交点是否在线段范围内
        if (t < 0.0f || t > line_length) {
            return false;
        }
        
        // 计算交点
        intersection_point = start + line_direction * t;
        
        // 检查交点是否在传送门范围内
        return is_point_in_portal_bounds(intersection_point, portal_plane);
    }

    bool PortalMath::is_entity_fully_through_portal(
        const Vector3& entity_bounds_min,
        const Vector3& entity_bounds_max,
        const Transform& entity_transform,
        const PortalPlane& portal_plane
    ) {
        // 获取实体包围盒的8个角点
        Vector3 corners[8] = {
            entity_transform.transform_point(Vector3(entity_bounds_min.x, entity_bounds_min.y, entity_bounds_min.z)),
            entity_transform.transform_point(Vector3(entity_bounds_max.x, entity_bounds_min.y, entity_bounds_min.z)),
            entity_transform.transform_point(Vector3(entity_bounds_min.x, entity_bounds_max.y, entity_bounds_min.z)),
            entity_transform.transform_point(Vector3(entity_bounds_max.x, entity_bounds_max.y, entity_bounds_min.z)),
            entity_transform.transform_point(Vector3(entity_bounds_min.x, entity_bounds_min.y, entity_bounds_max.z)),
            entity_transform.transform_point(Vector3(entity_bounds_max.x, entity_bounds_min.y, entity_bounds_max.z)),
            entity_transform.transform_point(Vector3(entity_bounds_min.x, entity_bounds_max.y, entity_bounds_max.z)),
            entity_transform.transform_point(Vector3(entity_bounds_max.x, entity_bounds_max.y, entity_bounds_max.z))
        };
        
        // 检查所有角点是否都在传送门的同一侧
        bool all_same_side = true;
        float first_distance = signed_distance_to_plane(corners[0], portal_plane.center, portal_plane.normal);
        bool first_side = first_distance > 0.0f;
        
        for (int i = 1; i < 8; ++i) {
            float distance = signed_distance_to_plane(corners[i], portal_plane.center, portal_plane.normal);
            bool current_side = distance > 0.0f;
            if (current_side != first_side) {
                all_same_side = false;
                break;
            }
        }
        
        return all_same_side && first_distance < -EPSILON;
    }

    Transform PortalMath::calculate_portal_to_portal_transform(
        const PortalPlane& source_plane,
        const PortalPlane& target_plane
    ) {
        // 计算从源传送门到目标传送门的变换矩阵
        // 位置：目标传送门中心
        Vector3 position = target_plane.center;
        
        // 旋转：从源传送门法向量到目标传送门反向法向量的旋转
        Quaternion rotation = rotation_between_vectors(source_plane.normal, target_plane.normal * -1.0f);
        
        // 缩放：基于传送门大小比例
        float scale_factor = calculate_scale_factor(source_plane, target_plane);
        Vector3 scale(scale_factor, scale_factor, scale_factor);
        
        return Transform(position, rotation, scale);
    }

    CameraParams PortalMath::calculate_portal_camera(
        const CameraParams& original_camera,
        const PortalPlane& source_plane,
        const PortalPlane& target_plane
    ) {
        CameraParams portal_camera = original_camera;
        
        // 转换相机位置
        portal_camera.position = transform_point_through_portal(
            original_camera.position, source_plane, target_plane
        );
        
        // 转换相机朝向
        Vector3 forward = original_camera.rotation.rotate_vector(Vector3(0, 0, -1));
        Vector3 up = original_camera.rotation.rotate_vector(Vector3(0, 1, 0));
        
        Vector3 new_forward = transform_direction_through_portal(forward, source_plane, target_plane);
        Vector3 new_up = transform_direction_through_portal(up, source_plane, target_plane);
        
        // 重新构建旋转四元数
        Vector3 new_right = new_forward.cross(new_up).normalized();
        new_up = new_right.cross(new_forward).normalized();
        
        // 构建旋转矩阵并转换为四元数
        // 这里简化处理，实际实现需要更精确的矩阵到四元数转换
        portal_camera.rotation = rotation_between_vectors(Vector3(0, 0, -1), new_forward);
        
        // 调整FOV（如果传送门有缩放）
        float scale_factor = calculate_scale_factor(source_plane, target_plane);
        if (scale_factor != 1.0f) {
            portal_camera.fov = original_camera.fov; // 暂时保持不变，可根据需要调整
        }
        
        return portal_camera;
    }

    bool PortalMath::is_portal_recursive(
        const PortalPlane& portal1,
        const PortalPlane& portal2,
        const CameraParams& camera
    ) {
        // 简化检测：如果相机能够通过portal1看到portal2，并且portal2能够看到portal1，则为递归
        
        // 计算相机通过portal1的虚拟位置
        Vector3 virtual_camera_pos = transform_point_through_portal(
            camera.position, portal1, portal2
        );
        
        // 检查虚拟相机是否能看到portal1
        Vector3 to_portal1 = portal1.center - virtual_camera_pos;
        float distance_to_portal1 = to_portal1.length();
        
        if (distance_to_portal1 < 0.1f) {
            return true; // 相机太接近传送门
        }
        
        Vector3 direction_to_portal1 = to_portal1 * (1.0f / distance_to_portal1);
        
        // 检查方向是否朝向传送门正面
        float dot_with_normal = direction_to_portal1.dot(portal1.normal);
        
        return dot_with_normal > 0.0f; // 如果朝向正面则可能递归
    }

    float PortalMath::calculate_scale_factor(
        const PortalPlane& source_plane,
        const PortalPlane& target_plane
    ) {
        // 基于传送门面积计算缩放因子
        float source_area = source_plane.width * source_plane.height;
        float target_area = target_plane.width * target_plane.height;
        
        if (source_area < EPSILON) return 1.0f;
        
        return std::sqrt(target_area / source_area);
    }

    bool PortalMath::does_entity_intersect_portal(
        const Vector3& entity_bounds_min,
        const Vector3& entity_bounds_max,
        const Transform& entity_transform,
        const PortalPlane& portal_plane
    ) {
        // 获取实体包围盒的8个角点
        Vector3 corners[8] = {
            entity_transform.transform_point(Vector3(entity_bounds_min.x, entity_bounds_min.y, entity_bounds_min.z)),
            entity_transform.transform_point(Vector3(entity_bounds_max.x, entity_bounds_min.y, entity_bounds_min.z)),
            entity_transform.transform_point(Vector3(entity_bounds_min.x, entity_bounds_max.y, entity_bounds_min.z)),
            entity_transform.transform_point(Vector3(entity_bounds_max.x, entity_bounds_max.y, entity_bounds_min.z)),
            entity_transform.transform_point(Vector3(entity_bounds_min.x, entity_bounds_min.y, entity_bounds_max.z)),
            entity_transform.transform_point(Vector3(entity_bounds_max.x, entity_bounds_min.y, entity_bounds_max.z)),
            entity_transform.transform_point(Vector3(entity_bounds_min.x, entity_bounds_max.y, entity_bounds_max.z)),
            entity_transform.transform_point(Vector3(entity_bounds_max.x, entity_bounds_max.y, entity_bounds_max.z))
        };
        
        // 检查是否有角点在传送门平面两侧
        bool has_positive = false, has_negative = false;
        
        for (int i = 0; i < 8; ++i) {
            float distance = signed_distance_to_plane(corners[i], portal_plane.center, portal_plane.normal);
            
            if (distance > EPSILON) {
                has_positive = true;
            } else if (distance < -EPSILON) {
                has_negative = true;
            } else {
                // 頂點在平面上，為了保守起見，算作兩側都有
                has_positive = true;
                has_negative = true;
            }
        }
            
            if (has_positive && has_negative) {
                // 包围盒跨越平面，检查投影是否与传送门矩形重叠
                
                // 将所有角点投影到传送门平面
                Vector3 projected_points[8];
                for (int k = 0; k < 8; ++k) {
                    projected_points[k] = project_point_on_plane(corners[k], portal_plane.center, portal_plane.normal);
                }
                
                // 计算投影后点群在传送门坐标系中的边界
                float min_right = 1e6f, max_right = -1e6f;
                float min_up = 1e6f, max_up = -1e6f;
                
                for (int k = 0; k < 8; ++k) {
                    Vector3 relative = projected_points[k] - portal_plane.center;
                    float right_coord = relative.dot(portal_plane.right);
                    float up_coord = relative.dot(portal_plane.up);
                    
                    min_right = std::min(min_right, right_coord);
                    max_right = std::max(max_right, right_coord);
                    min_up = std::min(min_up, up_coord);
                    max_up = std::max(max_up, up_coord);
                }
                
                // 检查是否与传送门矩形重叠
                float portal_half_width = portal_plane.width * 0.5f;
                float portal_half_height = portal_plane.height * 0.5f;
                
                bool overlaps_width = (max_right >= -portal_half_width) && (min_right <= portal_half_width);
                bool overlaps_height = (max_up >= -portal_half_height) && (min_up <= portal_half_height);
                
                if (overlaps_width && overlaps_height) {
                    return true;
                }
            }
        
        return false;
    }

    float PortalMath::signed_distance_to_plane(
        const Vector3& point,
        const Vector3& plane_center,
        const Vector3& plane_normal
    ) {
        return (point - plane_center).dot(plane_normal);
    }

    void PortalMath::get_portal_corners(
        const PortalPlane& portal_plane,
        Vector3 corners[4]
    ) {
        Vector3 right_offset = portal_plane.right * (portal_plane.width * 0.5f);
        Vector3 up_offset = portal_plane.up * (portal_plane.height * 0.5f);
        
        corners[0] = portal_plane.center - right_offset - up_offset; // 左下
        corners[1] = portal_plane.center + right_offset - up_offset; // 右下
        corners[2] = portal_plane.center + right_offset + up_offset; // 右上
        corners[3] = portal_plane.center - right_offset + up_offset; // 左上
    }

    // 私有辅助函数实现
    Vector3 PortalMath::project_point_on_plane(
        const Vector3& point,
        const Vector3& plane_center,
        const Vector3& plane_normal
    ) {
        Vector3 to_point = point - plane_center;
        float distance = to_point.dot(plane_normal);
        return point - plane_normal * distance;
    }

    Quaternion PortalMath::rotation_between_vectors(
        const Vector3& from,
        const Vector3& to
    ) {
        Vector3 from_normalized = from.normalized();
        Vector3 to_normalized = to.normalized();
        
        float dot_product = from_normalized.dot(to_normalized);
        
        // 检查向量是否相同
        if (dot_product > 0.99999f) {
            return Quaternion(0, 0, 0, 1);
        }
        
        // 检查向量是否相反
        if (dot_product < -0.99999f) {
            // 找一个垂直向量
            Vector3 axis = Vector3(1, 0, 0).cross(from_normalized);
            if (axis.length() < EPSILON) {
                axis = Vector3(0, 1, 0).cross(from_normalized);
            }
            axis = axis.normalized();
            return Quaternion(axis.x, axis.y, axis.z, 0);
        }
        
        Vector3 cross_product = from_normalized.cross(to_normalized);
        float w = 1.0f + dot_product;
        
        return Quaternion(cross_product.x, cross_product.y, cross_product.z, w).normalized();
    }

    // === 新增：基于包围盒的穿越状态分析实现 ===
    
    BoundingBoxAnalysis PortalMath::analyze_entity_bounding_box(
        const Vector3& entity_bounds_min,
        const Vector3& entity_bounds_max,
        const Transform& entity_transform,
        const PortalPlane& portal_plane
    ) {
        BoundingBoxAnalysis analysis;
        
        // 获取AABB的8个顶点（本地坐标）
        Vector3 local_vertices[8] = {
            Vector3(entity_bounds_min.x, entity_bounds_min.y, entity_bounds_min.z),
            Vector3(entity_bounds_max.x, entity_bounds_min.y, entity_bounds_min.z),
            Vector3(entity_bounds_min.x, entity_bounds_max.y, entity_bounds_min.z),
            Vector3(entity_bounds_max.x, entity_bounds_max.y, entity_bounds_min.z),
            Vector3(entity_bounds_min.x, entity_bounds_min.y, entity_bounds_max.z),
            Vector3(entity_bounds_max.x, entity_bounds_min.y, entity_bounds_max.z),
            Vector3(entity_bounds_min.x, entity_bounds_max.y, entity_bounds_max.z),
            Vector3(entity_bounds_max.x, entity_bounds_max.y, entity_bounds_max.z)
        };
        
        // 转换到世界坐标并分析每个顶点
        analysis.total_vertices = 8;
        analysis.front_vertices_count = 0;
        analysis.back_vertices_count = 0;
        
        for (int i = 0; i < 8; ++i) {
            // 转换到世界坐标
            Vector3 world_vertex = entity_transform.transform_point(local_vertices[i]);
            
            // 计算到传送门平面的有符号距离
            float distance = signed_distance_to_plane(world_vertex, portal_plane.center, portal_plane.normal);
            
            if (distance > EPSILON) {
                analysis.front_vertices_count++;
            } else if (distance < -EPSILON) {
                analysis.back_vertices_count++;
            } else {
                // 顶点恰好在平面上，為了避免狀態抖動
                // 同時計入前後兩個計數（保守處理，確保 CROSSING 狀態維持）
                analysis.front_vertices_count++;
                analysis.back_vertices_count++;
            }
        }
        
        // 计算穿越比例
        if (analysis.front_vertices_count > 0) {
            analysis.crossing_ratio = static_cast<float>(analysis.back_vertices_count) / 
                                     static_cast<float>(analysis.total_vertices);
        } else {
            analysis.crossing_ratio = 0.0f;
        }
        
        return analysis;
    }
    
    PortalCrossingState PortalMath::determine_crossing_state(
        const BoundingBoxAnalysis& analysis,
        PortalCrossingState previous_state
    ) {
        // 状态判断逻辑
        bool has_front_vertices = analysis.front_vertices_count > 0;
        bool has_back_vertices = analysis.back_vertices_count > 0;
        bool all_vertices_back = analysis.back_vertices_count == analysis.total_vertices;
        
        if (has_front_vertices && has_back_vertices) {
            // 实体跨越传送门两侧 -> 正在穿越
            return PortalCrossingState::CROSSING;
        } else if (all_vertices_back && previous_state == PortalCrossingState::CROSSING) {
            // 从穿越状态完全进入背面 -> 传送完成
            return PortalCrossingState::TELEPORTED;
        } else if (analysis.front_vertices_count == analysis.total_vertices) {
            // 所有顶点都在正面 -> 未接触
            return PortalCrossingState::NOT_TOUCHING;
        } else {
            // 保持之前的状态（避免状态抖动）
            return previous_state;
        }
    }
    
    Transform PortalMath::calculate_ghost_transform(
        const Transform& entity_transform,
        const PortalPlane& source_plane,
        const PortalPlane& target_plane,
        float crossing_ratio,
        PortalFace source_face,
        PortalFace target_face
    ) {
        // 基于穿越比例计算幽灵实体应该在目标侧的位置
        
        // 1. 计算实体中心穿过传送门后的位置
        Vector3 ghost_position = transform_point_through_portal(
            entity_transform.position, source_plane, target_plane, source_face, target_face
        );
        
        // 2. 计算旋转变换（使用现有的变换方法）
        Transform full_transform = transform_through_portal(
            entity_transform, source_plane, target_plane
        );
        Quaternion ghost_rotation = full_transform.rotation;
        
        // 3. 根据穿越比例调整位置（可选的高级特性）
        // 这里可以实现更复杂的插值逻辑，比如让幽灵实体逐渐"显现"
        
        return Transform(ghost_position, ghost_rotation, entity_transform.scale);
    }

} // namespace Math
} // namespace Portal
