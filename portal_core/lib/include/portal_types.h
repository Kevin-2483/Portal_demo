#ifndef PORTAL_TYPES_H
#define PORTAL_TYPES_H

#include <cstdint>

namespace Portal
{

  // 基础数学类型定义 - 保持引擎无关性
  struct Vector3
  {
    float x, y, z;

    Vector3() : x(0), y(0), z(0) {}
    Vector3(float x_, float y_, float z_) : x(x_), y(y_), z(z_) {}

    Vector3 operator+(const Vector3 &other) const
    {
      return Vector3(x + other.x, y + other.y, z + other.z);
    }

    Vector3 operator-(const Vector3 &other) const
    {
      return Vector3(x - other.x, y - other.y, z - other.z);
    }

    Vector3 operator*(float scalar) const
    {
      return Vector3(x * scalar, y * scalar, z * scalar);
    }

    float dot(const Vector3 &other) const
    {
      return x * other.x + y * other.y + z * other.z;
    }

    Vector3 cross(const Vector3 &other) const
    {
      return Vector3(
          y * other.z - z * other.y,
          z * other.x - x * other.z,
          x * other.y - y * other.x);
    }

    float length() const;
    Vector3 normalized() const;
  };

  struct Quaternion
  {
    float x, y, z, w;

    Quaternion() : x(0), y(0), z(0), w(1) {}
    Quaternion(float x_, float y_, float z_, float w_) : x(x_), y(y_), z(z_), w(w_) {}

    Quaternion operator*(const Quaternion &other) const;
    Vector3 rotate_vector(const Vector3 &vec) const;
    Quaternion conjugate() const;
    Quaternion normalized() const;
  };

  struct Transform
  {
    Vector3 position;
    Quaternion rotation;
    Vector3 scale;

    Transform() : scale(1.0f, 1.0f, 1.0f) {}
    Transform(const Vector3 &pos, const Quaternion &rot, const Vector3 &scl = Vector3(1, 1, 1))
        : position(pos), rotation(rot), scale(scl) {}

    Vector3 transform_point(const Vector3 &point) const;
    Vector3 inverse_transform_point(const Vector3 &point) const;
    Transform inverse() const;
  };

  // 物理状态定义
  struct PhysicsState
  {
    Vector3 linear_velocity;
    Vector3 angular_velocity;
    float mass;

    PhysicsState() : mass(1.0f) {}
    PhysicsState(const Vector3 &linear_vel, const Vector3 &angular_vel, float m = 1.0f)
        : linear_velocity(linear_vel), angular_velocity(angular_vel), mass(m) {}
  };

  // 传送门ID类型
  using PortalId = uint32_t;
  constexpr PortalId INVALID_PORTAL_ID = 0;

  // 实体ID类型 - 由宿主应用程序定义
  using EntityId = uint64_t;
  constexpr EntityId INVALID_ENTITY_ID = 0;

  // 传送门面类型
  enum class PortalFace
  {
    A, // A面（定义为传送门的一面）
    B  // B面（定义为传送门的另一面，与A面相对）
  };

  // 传送门平面定义
  struct PortalPlane
  {
    Vector3 center;         // 传送门中心位置
    Vector3 normal;         // 传送门法向量（指向A面）
    Vector3 up;             // 传送门向上方向
    Vector3 right;          // 传送门向右方向
    float width;            // 传送门宽度
    float height;           // 传送门高度
    PortalFace active_face; // 当前活跃的面（A或B）

    PortalPlane() : width(2.0f), height(3.0f), active_face(PortalFace::A) {}

    // 获取指定面的法向量
    Vector3 get_face_normal(PortalFace face) const
    {
      return (face == PortalFace::A) ? normal : (normal * -1.0f);
    }
  };

  // 相机参数
  struct CameraParams
  {
    Vector3 position;
    Quaternion rotation;
    float fov; // 视野角度 (度)
    float near_plane;
    float far_plane;
    float aspect_ratio;

    CameraParams() : fov(75.0f), near_plane(0.1f), far_plane(1000.0f), aspect_ratio(16.0f / 9.0f) {}
  };

  // 裁剪平面定义
  struct ClippingPlane
  {
    Vector3 normal; // 平面法向量
    float distance; // 平面到原点的距离
    bool enabled;   // 是否启用裁剪

    ClippingPlane() : distance(0.0f), enabled(false) {}
    ClippingPlane(const Vector3 &n, float d) : normal(n), distance(d), enabled(true) {}

    // 从点和法向量构造裁剪平面
    static ClippingPlane from_point_and_normal(const Vector3 &point, const Vector3 &normal)
    {
      ClippingPlane plane;
      plane.normal = normal.normalized();
      plane.distance = plane.normal.dot(point);
      plane.enabled = true;
      return plane;
    }
  };

  // 渲染通道描述符
  struct RenderPassDescriptor
  {
    CameraParams virtual_camera;  // 虚拟相机参数
    ClippingPlane clipping_plane; // 裁剪平面
    bool should_clip;             // 是否需要裁剪
    bool use_stencil_buffer;      // 是否使用模板缓冲
    int stencil_ref_value;        // 模板参考值
    PortalId source_portal_id;    // 源传送门ID
    int recursion_depth;          // 递归深度

    RenderPassDescriptor()
        : should_clip(false), use_stencil_buffer(true),
          stencil_ref_value(1), source_portal_id(INVALID_PORTAL_ID), recursion_depth(0) {}
  };

  // 视锥体定义
  struct Frustum
  {
    Vector3 vertices[8];      // 视锥体8个顶点
    Vector3 planes[6];        // 视锥体6个平面的法向量
    float plane_distances[6]; // 平面到原点的距离
  };

  // 传送结果
  enum class TeleportResult
  {
    SUCCESS,
    FAILED_NO_LINKED_PORTAL,
    FAILED_INVALID_PORTAL,
    FAILED_BLOCKED,
    FAILED_TOO_LARGE
  };

  // 传送门穿越状态
  enum class PortalCrossingState
  {
    NOT_TOUCHING, // 未接触
    CROSSING,     // 正在穿越
    TELEPORTED    // 已传送
  };

  // 包围盒顶点分布信息
  struct BoundingBoxAnalysis
  {
    int front_vertices_count; // 传送门正面顶点数
    int back_vertices_count;  // 传送门背面顶点数
    int total_vertices;       // 总顶点数 (通常是8)
    float crossing_ratio;     // 穿越比例 (0.0-1.0)

    BoundingBoxAnalysis() : front_vertices_count(0), back_vertices_count(0),
                            total_vertices(8), crossing_ratio(0.0f) {}
  };

  // 增强的传送状态
  struct TeleportState
  {
    EntityId entity_id;
    PortalId source_portal;
    PortalId target_portal;
    PortalCrossingState crossing_state; // 当前穿越状态
    PortalCrossingState previous_state; // 上一帧状态
    BoundingBoxAnalysis bbox_analysis;  // 包围盒分析结果
    float transition_progress;          // 传送进度 (0.0 到 1.0)
    bool is_teleporting;                // 是否正在传送
    bool has_ghost_collider;            // 是否在目标世界有幽灵碰撞体

    TeleportState() : entity_id(INVALID_ENTITY_ID),
                      source_portal(INVALID_PORTAL_ID),
                      target_portal(INVALID_PORTAL_ID),
                      crossing_state(PortalCrossingState::NOT_TOUCHING),
                      previous_state(PortalCrossingState::NOT_TOUCHING),
                      transition_progress(0.0f),
                      is_teleporting(false),
                      has_ghost_collider(false) {}
  };

} // namespace Portal

#endif // PORTAL_TYPES_H
