#include "../../include/math/portal_math.h"
#include <cmath>
#include <algorithm>

namespace Portal
{
  namespace Math
  {

    Vector3 PortalMath::transform_point_through_portal(
        const Vector3 &point,
        const PortalPlane &source_plane,
        const PortalPlane &target_plane,
        PortalFace source_face,
        PortalFace target_face)
    {
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
        const Vector3 &point,
        const PortalPlane &source_plane,
        const PortalPlane &target_plane)
    {
      return transform_point_through_portal(point, source_plane, target_plane, PortalFace::A, PortalFace::B);
    }

    Vector3 PortalMath::transform_direction_through_portal(
        const Vector3 &direction,
        const PortalPlane &source_plane,
        const PortalPlane &target_plane,
        PortalFace source_face,
        PortalFace target_face)
    {
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
        const Vector3 &direction,
        const PortalPlane &source_plane,
        const PortalPlane &target_plane)
    {
      return transform_direction_through_portal(direction, source_plane, target_plane, PortalFace::A, PortalFace::B);
    }

    Transform PortalMath::transform_through_portal(
        const Transform &transform,
        const PortalPlane &source_plane,
        const PortalPlane &target_plane,
        PortalFace source_face,
        PortalFace target_face)
    {
      // 转换位置
      Vector3 new_position = transform_point_through_portal(
          transform.position, source_plane, target_plane, source_face, target_face);

      // 获取实际的面法向量
      Vector3 source_normal = source_plane.get_face_normal(source_face);
      Vector3 target_normal = target_plane.get_face_normal(target_face);

      // 计算旋转变换
      // 从源传送门面到目标传送门面的旋转（注意面对面的映射）
      Quaternion portal_rotation = rotation_between_vectors(source_normal, target_normal * -1.0f);

      // 应用传送门旋转到对象旋转
      Quaternion new_rotation = portal_rotation * transform.rotation;

      // 计算缩放
      float scale_factor = calculate_scale_factor(source_plane, target_plane);
      Vector3 new_scale = transform.scale * scale_factor;

      return Transform(new_position, new_rotation, new_scale);
    }

    // 向后兼容的重载版本 - 默认A面对B面
    Transform PortalMath::transform_through_portal(
        const Transform &transform,
        const PortalPlane &source_plane,
        const PortalPlane &target_plane)
    {
      return transform_through_portal(transform, source_plane, target_plane, PortalFace::A, PortalFace::B);
    }

    PhysicsState PortalMath::transform_physics_state_through_portal(
        const PhysicsState &physics_state,
        const PortalPlane &source_plane,
        const PortalPlane &target_plane,
        PortalFace source_face,
        PortalFace target_face)
    {
      PhysicsState new_state;

      // 转换线性速度
      new_state.linear_velocity = transform_direction_through_portal(
                                      physics_state.linear_velocity, source_plane, target_plane, source_face, target_face) *
                                  physics_state.linear_velocity.length();

      // 转换角速度
      new_state.angular_velocity = transform_direction_through_portal(
                                       physics_state.angular_velocity, source_plane, target_plane, source_face, target_face) *
                                   physics_state.angular_velocity.length();

      // 质量保持不变
      new_state.mass = physics_state.mass;

      return new_state;
    }

    // 向後兼容的重載版本 - 默认A面对B面
    PhysicsState PortalMath::transform_physics_state_through_portal(
        const PhysicsState &physics_state,
        const PortalPlane &source_plane,
        const PortalPlane &target_plane)
    {
      return transform_physics_state_through_portal(physics_state, source_plane, target_plane, PortalFace::A, PortalFace::B);
    }

    PhysicsState PortalMath::transform_physics_state_with_portal_velocity(
        const PhysicsState &entity_physics_state,
        const PhysicsState &source_portal_physics,
        const PhysicsState &target_portal_physics,
        const PortalPlane &source_plane,
        const PortalPlane &target_plane,
        float delta_time)
    {
      // 首先计算基础的物理状态变换
      PhysicsState base_transformed = transform_physics_state_through_portal(
          entity_physics_state, source_plane, target_plane);

      // 计算传送门的相对速度贡献
      Vector3 source_portal_velocity = source_portal_physics.linear_velocity;
      Vector3 target_portal_velocity = target_portal_physics.linear_velocity;

      // 变换源传送门速度到目标传送门坐标系
      Vector3 transformed_source_velocity = transform_direction_through_portal(
                                                source_portal_velocity, source_plane, target_plane) *
                                            source_portal_velocity.length();

      // 计算速度差异（目标传送门速度 - 变换后的源传送门速度）
      Vector3 portal_velocity_difference = target_portal_velocity - transformed_source_velocity;

      // 将传送门速度差异添加到实体速度上
      base_transformed.linear_velocity = base_transformed.linear_velocity + portal_velocity_difference;

      // 如果传送门有角速度，也需要考虑
      Vector3 source_angular_velocity = source_portal_physics.angular_velocity;
      Vector3 target_angular_velocity = target_portal_physics.angular_velocity;

      Vector3 transformed_source_angular = transform_direction_through_portal(
                                               source_angular_velocity, source_plane, target_plane) *
                                           source_angular_velocity.length();

      Vector3 angular_velocity_difference = target_angular_velocity - transformed_source_angular;
      base_transformed.angular_velocity = base_transformed.angular_velocity + angular_velocity_difference;

      return base_transformed;
    }

    Vector3 PortalMath::calculate_relative_velocity(
        const Vector3 &entity_velocity,
        const Vector3 &portal_velocity,
        const Vector3 &contact_point,
        const PortalPlane &portal_plane)
    {
      // 计算实体相对于传送门的速度
      Vector3 relative_velocity = entity_velocity - portal_velocity;

      // 考虑传送门旋转引起的速度影响
      // 如果传送门有角速度，计算接触点处的切向速度
      // 这里增强了实现，支持动态传送门的角速度影响
      
      // 注意：为了完全支持动态传送门，需要从 PortalPlane 中获取角速度信息
      // 目前结构体不包含角速度，但保留了扩展点
      
      // 计算从传送门中心到接触点的向量
      Vector3 radius_vector = contact_point - portal_plane.center;
      
      // 如果将来要支持角速度，可以这样计算：
      // Vector3 angular_velocity = portal_plane.angular_velocity; // 需要扩展 PortalPlane
      // Vector3 tangential_velocity = angular_velocity.cross(radius_vector);
      // relative_velocity -= tangential_velocity;

      return relative_velocity;
    }

    bool PortalMath::is_point_in_portal_bounds(
        const Vector3 &point,
        const PortalPlane &portal_plane)
    {
      Vector3 relative_point = point - portal_plane.center;

      float right_distance = std::abs(relative_point.dot(portal_plane.right));
      float up_distance = std::abs(relative_point.dot(portal_plane.up));

      return right_distance <= portal_plane.width * 0.5f &&
             up_distance <= portal_plane.height * 0.5f;
    }

    bool PortalMath::line_intersects_portal_plane(
        const Vector3 &start,
        const Vector3 &end,
        const PortalPlane &portal_plane,
        Vector3 &intersection_point)
    {
      Vector3 line_direction = end - start;
      float line_length = line_direction.length();

      if (line_length < EPSILON)
      {
        return false;
      }

      line_direction = line_direction * (1.0f / line_length);

      // 检查线段是否与平面平行
      float denominator = line_direction.dot(portal_plane.normal);
      if (std::abs(denominator) < EPSILON)
      {
        return false;
      }

      // 计算交点参数
      Vector3 to_plane = portal_plane.center - start;
      float t = to_plane.dot(portal_plane.normal) / denominator;

      // 检查交点是否在线段范围内
      if (t < 0.0f || t > line_length)
      {
        return false;
      }

      // 计算交点
      intersection_point = start + line_direction * t;

      // 检查交点是否在传送门范围内
      return is_point_in_portal_bounds(intersection_point, portal_plane);
    }

    bool PortalMath::is_entity_fully_through_portal(
        const Vector3 &entity_bounds_min,
        const Vector3 &entity_bounds_max,
        const Transform &entity_transform,
        const PortalPlane &portal_plane)
    {
      // 获取实体包围盒的8个角点
      Vector3 corners[8] = {
          entity_transform.transform_point(Vector3(entity_bounds_min.x, entity_bounds_min.y, entity_bounds_min.z)),
          entity_transform.transform_point(Vector3(entity_bounds_max.x, entity_bounds_min.y, entity_bounds_min.z)),
          entity_transform.transform_point(Vector3(entity_bounds_min.x, entity_bounds_max.y, entity_bounds_min.z)),
          entity_transform.transform_point(Vector3(entity_bounds_max.x, entity_bounds_max.y, entity_bounds_min.z)),
          entity_transform.transform_point(Vector3(entity_bounds_min.x, entity_bounds_min.y, entity_bounds_max.z)),
          entity_transform.transform_point(Vector3(entity_bounds_max.x, entity_bounds_min.y, entity_bounds_max.z)),
          entity_transform.transform_point(Vector3(entity_bounds_min.x, entity_bounds_max.y, entity_bounds_max.z)),
          entity_transform.transform_point(Vector3(entity_bounds_max.x, entity_bounds_max.y, entity_bounds_max.z))};

      // 检查所有角点是否都在传送门的同一侧
      bool all_same_side = true;
      float first_distance = signed_distance_to_plane(corners[0], portal_plane.center, portal_plane.normal);
      bool first_side = first_distance > 0.0f;

      for (int i = 1; i < 8; ++i)
      {
        float distance = signed_distance_to_plane(corners[i], portal_plane.center, portal_plane.normal);
        bool current_side = distance > 0.0f;
        if (current_side != first_side)
        {
          all_same_side = false;
          break;
        }
      }

      return all_same_side && first_distance < -EPSILON;
    }

    Transform PortalMath::calculate_portal_to_portal_transform(
        const PortalPlane &source_plane,
        const PortalPlane &target_plane)
    {
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
        const CameraParams &original_camera,
        const PortalPlane &source_plane,
        const PortalPlane &target_plane,
        PortalFace source_face,
        PortalFace target_face)
    {
      CameraParams portal_camera = original_camera;

      // 转换相机位置
      portal_camera.position = transform_point_through_portal(
          original_camera.position, source_plane, target_plane, source_face, target_face);

      // 转换相机朝向
      Vector3 forward = original_camera.rotation.rotate_vector(Vector3(0, 0, -1));
      Vector3 up = original_camera.rotation.rotate_vector(Vector3(0, 1, 0));

      Vector3 new_forward = transform_direction_through_portal(forward, source_plane, target_plane, source_face, target_face);
      Vector3 new_up = transform_direction_through_portal(up, source_plane, target_plane, source_face, target_face);

      // 重新构建旋转四元数
      Vector3 new_right = new_forward.cross(new_up).normalized();
      new_up = new_right.cross(new_forward).normalized();

      // 构建旋转矩阵并转换为四元数
      // 使用精确的矩阵到四元数转换算法
      Vector3 neg_forward = Vector3(-new_forward.x, -new_forward.y, -new_forward.z);
      portal_camera.rotation = matrix_to_quaternion(new_right, new_up, neg_forward);

      // 调整FOV（如果传送门有缩放）
      float scale_factor = calculate_scale_factor(source_plane, target_plane);
      if (scale_factor != 1.0f)
      {
        portal_camera.fov = original_camera.fov; // 暂时保持不变，可根据需要调整
      }

      return portal_camera;
    }

    // 向后兼容的重载版本 - 默认A面对B面
    CameraParams PortalMath::calculate_portal_camera(
        const CameraParams &original_camera,
        const PortalPlane &source_plane,
        const PortalPlane &target_plane)
    {
      return calculate_portal_camera(original_camera, source_plane, target_plane, PortalFace::A, PortalFace::B);
    }

    bool PortalMath::is_portal_recursive(
        const PortalPlane &portal1,
        const PortalPlane &portal2,
        const CameraParams &camera)
    {
      // 简化检测：如果相机能够通过portal1看到portal2，并且portal2能够看到portal1，则为递归

      // 计算相机通过portal1的虚拟位置
      Vector3 virtual_camera_pos = transform_point_through_portal(
          camera.position, portal1, portal2);

      // 检查虚拟相机是否能看到portal1
      Vector3 to_portal1 = portal1.center - virtual_camera_pos;
      float distance_to_portal1 = to_portal1.length();

      if (distance_to_portal1 < 0.1f)
      {
        return true; // 相机太接近传送门
      }

      Vector3 direction_to_portal1 = to_portal1 * (1.0f / distance_to_portal1);

      // 检查方向是否朝向传送门正面
      float dot_with_normal = direction_to_portal1.dot(portal1.normal);

      return dot_with_normal > 0.0f; // 如果朝向正面则可能递归
    }

    float PortalMath::calculate_scale_factor(
        const PortalPlane &source_plane,
        const PortalPlane &target_plane)
    {
      // 基于传送门面积计算缩放因子
      float source_area = source_plane.width * source_plane.height;
      float target_area = target_plane.width * target_plane.height;

      if (source_area < EPSILON)
        return 1.0f;

      return std::sqrt(target_area / source_area);
    }

    bool PortalMath::does_entity_intersect_portal(
        const Vector3 &entity_bounds_min,
        const Vector3 &entity_bounds_max,
        const Transform &entity_transform,
        const PortalPlane &portal_plane)
    {
      // 获取实体包围盒的8个角点
      Vector3 corners[8] = {
          entity_transform.transform_point(Vector3(entity_bounds_min.x, entity_bounds_min.y, entity_bounds_min.z)),
          entity_transform.transform_point(Vector3(entity_bounds_max.x, entity_bounds_min.y, entity_bounds_min.z)),
          entity_transform.transform_point(Vector3(entity_bounds_min.x, entity_bounds_max.y, entity_bounds_min.z)),
          entity_transform.transform_point(Vector3(entity_bounds_max.x, entity_bounds_max.y, entity_bounds_min.z)),
          entity_transform.transform_point(Vector3(entity_bounds_min.x, entity_bounds_min.y, entity_bounds_max.z)),
          entity_transform.transform_point(Vector3(entity_bounds_max.x, entity_bounds_min.y, entity_bounds_max.z)),
          entity_transform.transform_point(Vector3(entity_bounds_min.x, entity_bounds_max.y, entity_bounds_max.z)),
          entity_transform.transform_point(Vector3(entity_bounds_max.x, entity_bounds_max.y, entity_bounds_max.z))};

      // 检查是否有角点在传送门平面两侧
      bool has_positive = false, has_negative = false;

      for (int i = 0; i < 8; ++i)
      {
        float distance = signed_distance_to_plane(corners[i], portal_plane.center, portal_plane.normal);

        if (distance > EPSILON)
        {
          has_positive = true;
        }
        else if (distance < -EPSILON)
        {
          has_negative = true;
        }
        else
        {
          // 頂點在平面上，為了保守起見，算作兩側都有
          has_positive = true;
          has_negative = true;
        }
      }

      if (has_positive && has_negative)
      {
        // 包围盒跨越平面，检查投影是否与传送门矩形重叠

        // 将所有角点投影到传送门平面
        Vector3 projected_points[8];
        for (int k = 0; k < 8; ++k)
        {
          projected_points[k] = project_point_on_plane(corners[k], portal_plane.center, portal_plane.normal);
        }

        // 计算投影后点群在传送门坐标系中的边界
        float min_right = 1e6f, max_right = -1e6f;
        float min_up = 1e6f, max_up = -1e6f;

        for (int k = 0; k < 8; ++k)
        {
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

        if (overlaps_width && overlaps_height)
        {
          return true;
        }
      }

      return false;
    }

    float PortalMath::signed_distance_to_plane(
        const Vector3 &point,
        const Vector3 &plane_center,
        const Vector3 &plane_normal)
    {
      return (point - plane_center).dot(plane_normal);
    }

    void PortalMath::get_portal_corners(
        const PortalPlane &portal_plane,
        Vector3 corners[4])
    {
      Vector3 right_offset = portal_plane.right * (portal_plane.width * 0.5f);
      Vector3 up_offset = portal_plane.up * (portal_plane.height * 0.5f);

      corners[0] = portal_plane.center - right_offset - up_offset; // 左下
      corners[1] = portal_plane.center + right_offset - up_offset; // 右下
      corners[2] = portal_plane.center + right_offset + up_offset; // 右上
      corners[3] = portal_plane.center - right_offset + up_offset; // 左上
    }

    // 私有辅助函数实现
    Vector3 PortalMath::project_point_on_plane(
        const Vector3 &point,
        const Vector3 &plane_center,
        const Vector3 &plane_normal)
    {
      Vector3 to_point = point - plane_center;
      float distance = to_point.dot(plane_normal);
      return point - plane_normal * distance;
    }

    Quaternion PortalMath::rotation_between_vectors(
        const Vector3 &from,
        const Vector3 &to)
    {
      Vector3 from_normalized = from.normalized();
      Vector3 to_normalized = to.normalized();

      float dot_product = from_normalized.dot(to_normalized);

      // 检查向量是否相同
      if (dot_product > 0.99999f)
      {
        return Quaternion(0, 0, 0, 1);
      }

      // 检查向量是否相反
      if (dot_product < -0.99999f)
      {
        // 找一个垂直向量
        Vector3 axis = Vector3(1, 0, 0).cross(from_normalized);
        if (axis.length() < EPSILON)
        {
          axis = Vector3(0, 1, 0).cross(from_normalized);
        }
        axis = axis.normalized();
        return Quaternion(axis.x, axis.y, axis.z, 0);
      }

      Vector3 cross_product = from_normalized.cross(to_normalized);
      float w = 1.0f + dot_product;

      return Quaternion(cross_product.x, cross_product.y, cross_product.z, w).normalized();
    }

    // === 精確的矩陣到四元數轉換實現 ===
    Quaternion PortalMath::matrix_to_quaternion(const Vector3 &right, const Vector3 &up, const Vector3 &forward)
    {
      // 構建3x3旋轉矩陣
      // 矩陣形式（列優先）：
      // [ right.x   up.x   forward.x ]
      // [ right.y   up.y   forward.y ]
      // [ right.z   up.z   forward.z ]
      
      float m00 = right.x, m01 = up.x, m02 = forward.x;
      float m10 = right.y, m11 = up.y, m12 = forward.y;  
      float m20 = right.z, m21 = up.z, m22 = forward.z;
      
      // 計算矩陣的跡（trace）
      float trace = m00 + m11 + m22;
      
      if (trace > 0.0f) {
        // 使用標準算法：trace > 0的情況
        float s = sqrt(trace + 1.0f) * 2.0f; // s = 4 * w
        float w = 0.25f * s;
        float x = (m21 - m12) / s;
        float y = (m02 - m20) / s;
        float z = (m10 - m01) / s;
        return Quaternion(x, y, z, w);
      } else if (m00 > m11 && m00 > m22) {
        // m00是最大的對角元素
        float s = sqrt(1.0f + m00 - m11 - m22) * 2.0f; // s = 4 * x
        float w = (m21 - m12) / s;
        float x = 0.25f * s;
        float y = (m01 + m10) / s;
        float z = (m02 + m20) / s;
        return Quaternion(x, y, z, w);
      } else if (m11 > m22) {
        // m11是最大的對角元素
        float s = sqrt(1.0f + m11 - m00 - m22) * 2.0f; // s = 4 * y
        float w = (m02 - m20) / s;
        float x = (m01 + m10) / s;
        float y = 0.25f * s;
        float z = (m12 + m21) / s;
        return Quaternion(x, y, z, w);
      } else {
        // m22是最大的對角元素
        float s = sqrt(1.0f + m22 - m00 - m11) * 2.0f; // s = 4 * z
        float w = (m10 - m01) / s;
        float x = (m02 + m20) / s;
        float y = (m12 + m21) / s;
        float z = 0.25f * s;
        return Quaternion(x, y, z, w);
      }
    }

    // === 新增：基于包围盒的穿越状态分析实现 ===

    BoundingBoxAnalysis PortalMath::analyze_entity_bounding_box(
        const Vector3 &entity_bounds_min,
        const Vector3 &entity_bounds_max,
        const Transform &entity_transform,
        const PortalPlane &portal_plane)
    {
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
          Vector3(entity_bounds_max.x, entity_bounds_max.y, entity_bounds_max.z)};

      // 转换到世界坐标并分析每个顶点
      analysis.total_vertices = 8;
      analysis.front_vertices_count = 0;
      analysis.back_vertices_count = 0;

      for (int i = 0; i < 8; ++i)
      {
        // 转换到世界坐标
        Vector3 world_vertex = entity_transform.transform_point(local_vertices[i]);

        // 计算到传送门平面的有符号距离
        float distance = signed_distance_to_plane(world_vertex, portal_plane.center, portal_plane.normal);

        if (distance > EPSILON)
        {
          analysis.front_vertices_count++;
        }
        else if (distance < -EPSILON)
        {
          analysis.back_vertices_count++;
        }
        else
        {
          // 顶点恰好在平面上，為了避免狀態抖動
          // 同時計入前後兩個計數（保守處理，確保 CROSSING 狀態維持）
          analysis.front_vertices_count++;
          analysis.back_vertices_count++;
        }
      }

      // 计算穿越比例
      if (analysis.front_vertices_count > 0)
      {
        analysis.crossing_ratio = static_cast<float>(analysis.back_vertices_count) /
                                  static_cast<float>(analysis.total_vertices);
      }
      else
      {
        analysis.crossing_ratio = 0.0f;
      }

      return analysis;
    }

    PortalCrossingState PortalMath::determine_crossing_state(
        const BoundingBoxAnalysis &analysis,
        PortalCrossingState previous_state)
    {
      // 状态判断逻辑
      bool has_front_vertices = analysis.front_vertices_count > 0;
      bool has_back_vertices = analysis.back_vertices_count > 0;
      bool all_vertices_back = analysis.back_vertices_count == analysis.total_vertices;

      if (has_front_vertices && has_back_vertices)
      {
        // 实体跨越传送门两侧 -> 正在穿越
        return PortalCrossingState::CROSSING;
      }
      else if (all_vertices_back && previous_state == PortalCrossingState::CROSSING)
      {
        // 从穿越状态完全进入背面 -> 传送完成
        return PortalCrossingState::TELEPORTED;
      }
      else if (analysis.front_vertices_count == analysis.total_vertices)
      {
        // 所有顶点都在正面 -> 未接触
        return PortalCrossingState::NOT_TOUCHING;
      }
      else
      {
        // 保持之前的状态（避免状态抖动）
        return previous_state;
      }
    }

    Transform PortalMath::calculate_ghost_transform(
        const Transform &entity_transform,
        const PortalPlane &source_plane,
        const PortalPlane &target_plane,
        float crossing_ratio,
        PortalFace source_face,
        PortalFace target_face)
    {
      // 基于穿越比例计算幽灵实体应该在目标侧的位置

      // 1. 计算实体中心穿过传送门后的位置
      Vector3 ghost_position = transform_point_through_portal(
          entity_transform.position, source_plane, target_plane, source_face, target_face);

      // 2. 计算旋转变换（使用现有的变换方法）
      Transform full_transform = transform_through_portal(
          entity_transform, source_plane, target_plane);
      Quaternion ghost_rotation = full_transform.rotation;

      // 3. 根据穿越比例调整位置（可选的高级特性）
      // 这里可以实现更复杂的插值逻辑，比如让幽灵实体逐渐"显现"

      return Transform(ghost_position, ghost_rotation, entity_transform.scale);
    }

    // === 包围盒变换相关实现 ===

    void PortalMath::transform_bounds_through_portal(
        const Vector3 &bounds_min,
        const Vector3 &bounds_max,
        const Transform &entity_transform,
        const PortalPlane &source_plane,
        const PortalPlane &target_plane,
        PortalFace source_face,
        PortalFace target_face,
        Vector3 &new_bounds_min,
        Vector3 &new_bounds_max,
        Transform &new_transform)
    {
      // 变换实体变换
      new_transform = transform_through_portal(entity_transform, source_plane, target_plane, source_face, target_face);

      // 获取包围盒的8个角点（本地坐标）
      Vector3 corners[8] = {
          Vector3(bounds_min.x, bounds_min.y, bounds_min.z),
          Vector3(bounds_max.x, bounds_min.y, bounds_min.z),
          Vector3(bounds_min.x, bounds_max.y, bounds_min.z),
          Vector3(bounds_max.x, bounds_max.y, bounds_min.z),
          Vector3(bounds_min.x, bounds_min.y, bounds_max.z),
          Vector3(bounds_max.x, bounds_min.y, bounds_max.z),
          Vector3(bounds_min.x, bounds_max.y, bounds_max.z),
          Vector3(bounds_max.x, bounds_max.y, bounds_max.z)
      };

      // 变换所有角点到世界坐标，然后通过传送门，再转回本地坐标
      Vector3 transformed_corners[8];
      for (int i = 0; i < 8; ++i)
      {
        // 转换到原始世界坐标
        Vector3 world_corner = entity_transform.transform_point(corners[i]);
        
        // 通过传送门变换
        Vector3 portal_transformed = transform_point_through_portal(
            world_corner, source_plane, target_plane, source_face, target_face);
        
        // 转换到新的本地坐标
        transformed_corners[i] = new_transform.inverse_transform_point(portal_transformed);
      }

      // 计算变换后的包围盒
      new_bounds_min = transformed_corners[0];
      new_bounds_max = transformed_corners[0];

      for (int i = 1; i < 8; ++i)
      {
        if (transformed_corners[i].x < new_bounds_min.x) new_bounds_min.x = transformed_corners[i].x;
        if (transformed_corners[i].y < new_bounds_min.y) new_bounds_min.y = transformed_corners[i].y;
        if (transformed_corners[i].z < new_bounds_min.z) new_bounds_min.z = transformed_corners[i].z;

        if (transformed_corners[i].x > new_bounds_max.x) new_bounds_max.x = transformed_corners[i].x;
        if (transformed_corners[i].y > new_bounds_max.y) new_bounds_max.y = transformed_corners[i].y;
        if (transformed_corners[i].z > new_bounds_max.z) new_bounds_max.z = transformed_corners[i].z;
      }
    }

    void PortalMath::transform_bounds_through_portal(
        const Vector3 &bounds_min,
        const Vector3 &bounds_max,
        const Transform &entity_transform,
        const PortalPlane &source_plane,
        const PortalPlane &target_plane,
        Vector3 &new_bounds_min,
        Vector3 &new_bounds_max,
        Transform &new_transform)
    {
      transform_bounds_through_portal(bounds_min, bounds_max, entity_transform,
                                    source_plane, target_plane,
                                    PortalFace::A, PortalFace::B,
                                    new_bounds_min, new_bounds_max, new_transform);
    }

    float PortalMath::calculate_transform_distance(
        const Transform &t1,
        const Transform &t2)
    {
      // 位置距离
      Vector3 pos_diff = t1.position - t2.position;
      float pos_distance = pos_diff.length();

      // 旋转距离（四元数点积，越接近1越相似）
      float rot_dot = std::abs(t1.rotation.x * t2.rotation.x +
                              t1.rotation.y * t2.rotation.y +
                              t1.rotation.z * t2.rotation.z +
                              t1.rotation.w * t2.rotation.w);
      float rot_distance = 1.0f - std::min(rot_dot, 1.0f);

      // 缩放距离
      Vector3 scale_diff = t1.scale - t2.scale;
      float scale_distance = scale_diff.length();

      // 加权总距离
      return pos_distance + rot_distance * 10.0f + scale_distance;
    }

    float PortalMath::calculate_physics_distance(
        const PhysicsState &p1,
        const PhysicsState &p2)
    {
      // 线性速度距离
      Vector3 lin_vel_diff = p1.linear_velocity - p2.linear_velocity;
      float lin_vel_distance = lin_vel_diff.length();

      // 角速度距离
      Vector3 ang_vel_diff = p1.angular_velocity - p2.angular_velocity;
      float ang_vel_distance = ang_vel_diff.length();

      // 质量差异
      float mass_diff = std::abs(p1.mass - p2.mass);

      return lin_vel_distance + ang_vel_distance + mass_diff;
    }

    // === 质心穿越检测数学函数实现 ===

    float PortalMath::calculate_point_crossing_progress(
        const Vector3 &point,
        const PortalPlane &portal_plane,
        const Vector3 &entity_bounds_min,
        const Vector3 &entity_bounds_max)
    {
      // 计算点到传送门平面的距离
      float distance_to_plane = (point - portal_plane.center).dot(portal_plane.normal);

      // 计算实体在传送门法线方向上的尺寸
      Vector3 bounds_size = entity_bounds_max - entity_bounds_min;
      float entity_size_along_normal = std::abs(bounds_size.dot(portal_plane.normal));

      // 如果实体尺寸为0，直接根据点的位置判断
      if (entity_size_along_normal < 1e-6f)
      {
        return distance_to_plane >= 0.0f ? 1.0f : 0.0f;
      }

      // 计算穿越进度（0.0 = 完全在一侧，1.0 = 完全在另一侧）
      float half_size = entity_size_along_normal * 0.5f;
      float progress = (distance_to_plane + half_size) / entity_size_along_normal;
      
      // 限制在 [0, 1] 范围内
      return std::max(0.0f, std::min(1.0f, progress));
    }

    bool PortalMath::detect_center_crossing_start(
        const Vector3 &center_pos,
        const Vector3 &prev_center_pos,
        const PortalPlane &portal_plane)
    {
      // 计算当前和之前位置到平面的距离
      float current_distance = (center_pos - portal_plane.center).dot(portal_plane.normal);
      float prev_distance = (prev_center_pos - portal_plane.center).dot(portal_plane.normal);

      // 检测是否从负侧穿越到正侧（或反之）
      bool crossed_positive = (prev_distance <= 0.0f && current_distance > 0.0f);
      bool crossed_negative = (prev_distance >= 0.0f && current_distance < 0.0f);

      return crossed_positive || crossed_negative;
    }

    bool PortalMath::detect_center_crossing_completion(
        const Vector3 &center_pos,
        const Vector3 &prev_center_pos,
        const PortalPlane &portal_plane,
        const Vector3 &entity_bounds_min,
        const Vector3 &entity_bounds_max)
    {
      // 计算当前和之前的穿越进度
      float current_progress = calculate_point_crossing_progress(
          center_pos, portal_plane, entity_bounds_min, entity_bounds_max);
      float prev_progress = calculate_point_crossing_progress(
          prev_center_pos, portal_plane, entity_bounds_min, entity_bounds_max);

      // 检测是否刚完成完全穿越（进度从 < 1.0 变为 >= 1.0，或从 > 0.0 变为 <= 0.0）
      bool completed_forward = (prev_progress < 1.0f && current_progress >= 1.0f);
      bool completed_backward = (prev_progress > 0.0f && current_progress <= 0.0f);

      return completed_forward || completed_backward;
    }

    Vector3 PortalMath::calculate_center_of_mass_world_pos(
        const Transform &entity_transform,
        const Vector3 &center_offset)
    {
      return entity_transform.transform_point(center_offset);
    }

  } // namespace Math
} // namespace Portal
