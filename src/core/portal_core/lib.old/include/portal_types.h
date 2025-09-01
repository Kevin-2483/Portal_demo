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

  // 实体类型枚举
  enum class EntityType
  {
    MAIN,       // 主体实体
    GHOST,      // 幽灵实体
    HYBRID      // 混合状态（用于转换期间）
  };

  // 实体描述结构 - 支持无缝传送
  struct EntityDescription
  {
    EntityId entity_id;
    EntityType entity_type;
    Transform transform;
    PhysicsState physics;
    Vector3 center_of_mass;        // 质心位置（相对于实体）
    Vector3 bounds_min;
    Vector3 bounds_max;
    EntityId counterpart_id;       // 对应实体ID（主体↔幽灵）
    PortalId associated_portal;    // 关联的传送门ID
    bool is_fully_functional;      // 是否具有完整功能（碰撞、渲染等）

    EntityDescription() : entity_id(INVALID_ENTITY_ID),
                         entity_type(EntityType::MAIN),
                         center_of_mass(0, 0, 0),
                         bounds_min(-0.5f, -0.5f, -0.5f),
                         bounds_max(0.5f, 0.5f, 0.5f),
                         counterpart_id(INVALID_ENTITY_ID),
                         associated_portal(INVALID_PORTAL_ID),
                         is_fully_functional(true) {}
  };

  // 质心穿越检测结果
  struct CenterOfMassCrossing
  {
    EntityId entity_id;
    PortalId portal_id;
    PortalFace crossed_face;
    PortalFace target_face;
    float crossing_progress;       // 0.0 = 未开始, 1.0 = 完全穿过
    Vector3 crossing_point;        // 穿越点位置
    Vector3 center_world_pos;      // 质心的世界位置
    bool just_started;             // 是否刚开始穿越
    bool just_completed;           // 是否刚完成穿越

    CenterOfMassCrossing() : entity_id(INVALID_ENTITY_ID),
                            portal_id(INVALID_PORTAL_ID),
                            crossed_face(PortalFace::A),
                            target_face(PortalFace::B),
                            crossing_progress(0.0f),
                            crossing_point(0, 0, 0),
                            center_world_pos(0, 0, 0),
                            just_started(false),
                            just_completed(false) {}
  };

  // 传送门面类型
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

    // A/B面状态同步支持
    PortalFace source_face;             // 源传送门使用的面
    PortalFace target_face;             // 目标传送门使用的面
    bool enable_realtime_sync;          // 是否启用实时同步
    uint64_t last_sync_timestamp;       // 上次同步时间戳

    // 新增：无缝传送支持
    EntityId ghost_entity_id;           // 幽灵实体ID
    CenterOfMassCrossing center_crossing; // 质心穿越检测
    bool seamless_mode;                 // 是否启用无缝传送模式
    bool auto_triggered;                // 是否自动触发的传送
    bool ready_for_swap;                // 是否准备好进行实体交换
    bool role_swapped;                  // 是否已执行角色互换
    EntityType original_entity_type;    // 原始实体类型
    float center_crossing_threshold;    // 质心穿越阈值 (0.0-1.0)

    TeleportState() : entity_id(INVALID_ENTITY_ID),
                      source_portal(INVALID_PORTAL_ID),
                      target_portal(INVALID_PORTAL_ID),
                      crossing_state(PortalCrossingState::NOT_TOUCHING),
                      previous_state(PortalCrossingState::NOT_TOUCHING),
                      transition_progress(0.0f),
                      is_teleporting(false),
                      has_ghost_collider(false),
                      source_face(PortalFace::A),
                      target_face(PortalFace::B),
                      enable_realtime_sync(true),
                      last_sync_timestamp(0),
                      ghost_entity_id(INVALID_ENTITY_ID),
                      seamless_mode(true),
                      auto_triggered(true),
                      ready_for_swap(false),
                      role_swapped(false),
                      original_entity_type(EntityType::MAIN),
                      center_crossing_threshold(0.5f) {}

    // 添加拷贝构造和赋值运算符以确保可复制性
    TeleportState(const TeleportState& other) = default;
    TeleportState& operator=(const TeleportState& other) = default;
    TeleportState(TeleportState&& other) = default;
    TeleportState& operator=(TeleportState&& other) = default;
  };

  // 幽灵实体状态同步配置
  struct GhostSyncConfig
  {
    bool sync_transform;          // 同步变换（位置、旋转、缩放）
    bool sync_physics;            // 同步物理状态（速度、角速度）
    bool sync_bounds;             // 同步包围盒大小
    bool sync_properties;         // 同步其他属性
    float sync_frequency;         // 同步频率（Hz）
    float transform_threshold;    // 变换变化阈值
    float velocity_threshold;     // 速度变化阈值

    GhostSyncConfig() : sync_transform(true),
                       sync_physics(true),
                       sync_bounds(true),
                       sync_properties(false),
                       sync_frequency(60.0f),
                       transform_threshold(0.001f),
                       velocity_threshold(0.01f) {}
  };

  // 幽灵实体状态快照
  struct GhostEntitySnapshot
  {
    EntityId main_entity_id;      // 主体实体ID
    EntityId ghost_entity_id;     // 幽灵实体ID (由引擎分配)
    Transform main_transform;     // 主体变换
    Transform ghost_transform;    // 幽灵变换
    PhysicsState main_physics;    // 主体物理状态
    PhysicsState ghost_physics;   // 幽灵物理状态
    Vector3 main_bounds_min;      // 主体包围盒最小值
    Vector3 main_bounds_max;      // 主体包围盒最大值
    Vector3 ghost_bounds_min;     // 幽灵包围盒最小值
    Vector3 ghost_bounds_max;     // 幽灵包围盒最大值
    uint64_t timestamp;           // 快照时间戳

    GhostEntitySnapshot() : main_entity_id(INVALID_ENTITY_ID),
                           ghost_entity_id(INVALID_ENTITY_ID),
                           timestamp(0) {}
  };

} // namespace Portal

#endif // PORTAL_TYPES_H
