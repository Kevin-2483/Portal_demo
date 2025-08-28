#ifndef PORTAL_MATH_H
#define PORTAL_MATH_H

#include "portal_types.h"

namespace Portal
{
  namespace Math
  {

    // 数学常量
    constexpr float PI = 3.14159265358979323846f;
    constexpr float EPSILON = 1e-6f;

    // 角度转弧度
    inline float deg_to_rad(float degrees)
    {
      return degrees * PI / 180.0f;
    }

    // 弧度转角度
    inline float rad_to_deg(float radians)
    {
      return radians * 180.0f / PI;
    }

    /**
     * 传送门核心数学计算类
     * 包含所有传送门相关的纯数学运算
     */
    class PortalMath
    {
    public:
      /**
       * 计算点穿过传送门后的新位置
       * @param point 原始点
       * @param source_plane 源传送门平面
       * @param target_plane 目标传送门平面
       * @param source_face 源传送门面（A或B）
       * @param target_face 目标传送门面（A或B）
       * @return 传送后的点位置
       */
      static Vector3 transform_point_through_portal(
          const Vector3 &point,
          const PortalPlane &source_plane,
          const PortalPlane &target_plane,
          PortalFace source_face,
          PortalFace target_face);

      /**
       * 计算点穿过传送门后的新位置（默认A面对B面）
       */
      static Vector3 transform_point_through_portal(
          const Vector3 &point,
          const PortalPlane &source_plane,
          const PortalPlane &target_plane);

      /**
       * 计算方向向量穿过传送门后的新方向
       */
      static Vector3 transform_direction_through_portal(
          const Vector3 &direction,
          const PortalPlane &source_plane,
          const PortalPlane &target_plane,
          PortalFace source_face,
          PortalFace target_face);

      /**
       * 计算方向向量穿过传送门后的新方向（默认A面对B面）
       */
      static Vector3 transform_direction_through_portal(
          const Vector3 &direction,
          const PortalPlane &source_plane,
          const PortalPlane &target_plane);

      /**
       * 计算变换矩阵穿过传送门后的新变换
       */
      static Transform transform_through_portal(
          const Transform &transform,
          const PortalPlane &source_plane,
          const PortalPlane &target_plane);

      /**
       * 计算物理状态穿过传送门后的新状态
       */
      static PhysicsState transform_physics_state_through_portal(
          const PhysicsState &physics_state,
          const PortalPlane &source_plane,
          const PortalPlane &target_plane,
          PortalFace source_face,
          PortalFace target_face);

      /**
       * 计算物理状态穿过传送门后的新状态（默认A面对B面）
       */
      static PhysicsState transform_physics_state_through_portal(
          const PhysicsState &physics_state,
          const PortalPlane &source_plane,
          const PortalPlane &target_plane);

      /**
       * 计算考虑传送门相对速度的物理状态变换
       * @param entity_physics_state 实体的物理状态
       * @param source_portal_physics 源传送门的物理状态
       * @param target_portal_physics 目标传送门的物理状态
       * @param source_plane 源传送门平面
       * @param target_plane 目标传送门平面
       * @param delta_time 时间差（用于计算相对运动）
       * @return 变换后的物理状态
       */
      static PhysicsState transform_physics_state_with_portal_velocity(
          const PhysicsState &entity_physics_state,
          const PhysicsState &source_portal_physics,
          const PhysicsState &target_portal_physics,
          const PortalPlane &source_plane,
          const PortalPlane &target_plane,
          float delta_time = 0.0f);

      /**
       * 计算实体相对于传送门的相对速度
       * @param entity_velocity 实体速度
       * @param portal_velocity 传送门速度
       * @param contact_point 接触点
       * @param portal_plane 传送门平面
       * @return 相对速度
       */
      static Vector3 calculate_relative_velocity(
          const Vector3 &entity_velocity,
          const Vector3 &portal_velocity,
          const Vector3 &contact_point,
          const PortalPlane &portal_plane);

      /**
       * 检查点是否在传送门平面的有效区域内
       */
      static bool is_point_in_portal_bounds(
          const Vector3 &point,
          const PortalPlane &portal_plane);

      /**
       * 检查线段是否与传送门平面相交
       * @param start 线段起点
       * @param end 线段终点
       * @param portal_plane 传送门平面
       * @param intersection_point 相交点（如果相交）
       * @return 是否相交
       */
      static bool line_intersects_portal_plane(
          const Vector3 &start,
          const Vector3 &end,
          const PortalPlane &portal_plane,
          Vector3 &intersection_point);

      /**
       * 检查物体是否完全穿过了传送门
       * @param entity_bounds_min 物体包围盒最小点
       * @param entity_bounds_max 物体包围盒最大点
       * @param entity_transform 物体变换
       * @param portal_plane 传送门平面
       * @return 是否完全穿过
       */
      static bool is_entity_fully_through_portal(
          const Vector3 &entity_bounds_min,
          const Vector3 &entity_bounds_max,
          const Transform &entity_transform,
          const PortalPlane &portal_plane);

      /**
       * 计算传送门之间的变换矩阵
       */
      static Transform calculate_portal_to_portal_transform(
          const PortalPlane &source_plane,
          const PortalPlane &target_plane);

      /**
       * 计算相机通过传送门看到的虚拟相机位置
       */
      static CameraParams calculate_portal_camera(
          const CameraParams &original_camera,
          const PortalPlane &source_plane,
          const PortalPlane &target_plane);

      /**
       * 检查传送门是否处于递归状态（传送门看到自己）
       */
      static bool is_portal_recursive(
          const PortalPlane &portal1,
          const PortalPlane &portal2,
          const CameraParams &camera);

      /**
       * 计算缩放因子（当传送门大小不同时）
       */
      static float calculate_scale_factor(
          const PortalPlane &source_plane,
          const PortalPlane &target_plane);

      /**
       * 检查物体是否与传送门平面相交
       */
      static bool does_entity_intersect_portal(
          const Vector3 &entity_bounds_min,
          const Vector3 &entity_bounds_max,
          const Transform &entity_transform,
          const PortalPlane &portal_plane);

      /**
       * === 新增：基于包围盒的穿越状态分析 ===
       */

      /**
       * 分析实体包围盒相对于传送门平面的分布
       * @param entity_bounds_min 实体包围盒最小点（本地坐标）
       * @param entity_bounds_max 实体包围盒最大点（本地坐标）
       * @param entity_transform 实体变换矩阵
       * @param portal_plane 传送门平面
       * @return 包围盒分析结果
       */
      static BoundingBoxAnalysis analyze_entity_bounding_box(
          const Vector3 &entity_bounds_min,
          const Vector3 &entity_bounds_max,
          const Transform &entity_transform,
          const PortalPlane &portal_plane);

      /**
       * 根据包围盒分析结果判断穿越状态
       * @param analysis 包围盒分析结果
       * @param previous_state 上一帧的状态
       * @return 当前应该的穿越状态
       */
      static PortalCrossingState determine_crossing_state(
          const BoundingBoxAnalysis &analysis,
          PortalCrossingState previous_state);

      /**
       * 计算实体在传送门两侧的变换
       * @param entity_transform 原始变换
       * @param source_plane 源传送门平面
       * @param target_plane 目标传送门平面
       * @param crossing_ratio 穿越比例(0.0-1.0)
       * @param source_face 源传送门面
       * @param target_face 目标传送门面
       * @return 目标侧的幽灵变换
       */
      static Transform calculate_ghost_transform(
          const Transform &entity_transform,
          const PortalPlane &source_plane,
          const PortalPlane &target_plane,
          float crossing_ratio,
          PortalFace source_face = PortalFace::A,
          PortalFace target_face = PortalFace::B);

      /**
       * 计算点到平面的带符号距离
       * 正值表示在平面法向量指向的一侧
       */
      static float signed_distance_to_plane(
          const Vector3 &point,
          const Vector3 &plane_center,
          const Vector3 &plane_normal);

      /**
       * 计算传送门平面的四个角点
       */
      static void get_portal_corners(
          const PortalPlane &portal_plane,
          Vector3 corners[4]);

    private:
      // 辅助函数
      static Vector3 project_point_on_plane(
          const Vector3 &point,
          const Vector3 &plane_center,
          const Vector3 &plane_normal);

      static Quaternion rotation_between_vectors(
          const Vector3 &from,
          const Vector3 &to);
    };

  } // namespace Math
} // namespace Portal

#endif // PORTAL_MATH_H
