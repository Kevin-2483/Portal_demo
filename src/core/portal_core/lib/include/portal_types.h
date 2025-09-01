#ifndef PORTAL_TYPES_H
#define PORTAL_TYPES_H

#include <cstdint>
#include <string>
#include <vector>

namespace Portal
{

  // 基础数学类型定义 - 保持引擎无关性
  struct Vector3
  {
    float x, y, z;

    Vector3() : x(0), y(0), z(0) {}
    Vector3(float x_, float y_, float z_) : x(x_), y(y_), z(z_) {}
    explicit Vector3(float value) : x(value), y(value), z(value) {}

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

    Vector3 operator/(float scalar) const
    {
      return Vector3(x / scalar, y / scalar, z / scalar);
    }

    Vector3& operator+=(const Vector3 &other)
    {
      x += other.x;
      y += other.y;
      z += other.z;
      return *this;
    }

    Vector3& operator-=(const Vector3 &other)
    {
      x -= other.x;
      y -= other.y;
      z -= other.z;
      return *this;
    }

    Vector3& operator*=(float scalar)
    {
      x *= scalar;
      y *= scalar;
      z *= scalar;
      return *this;
    }

    Vector3& operator/=(float scalar)
    {
      x /= scalar;
      y /= scalar;
      z /= scalar;
      return *this;
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
    float length_squared() const
    {
      return x * x + y * y + z * z;
    }
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

  // 扩展的物理状态定义
  struct PhysicsState
  {
    Vector3 linear_velocity;
    Vector3 angular_velocity;
    float mass;
    
    // === 新增：力和力矩属性 ===
    Vector3 applied_force;        // 作用在物体上的合力
    Vector3 applied_torque;       // 作用在物体上的合力矩
    Vector3 center_of_mass_local; // 本地坐标系的质心位置
    
    // 惯性张量（简化为对角元素）
    Vector3 inertia_tensor_diagonal; // Ixx, Iyy, Izz
    
    // 物理材质属性
    float friction_coefficient;
    float restitution_coefficient;
    float linear_damping;
    float angular_damping;

    PhysicsState() : mass(1.0f), applied_force(0, 0, 0), applied_torque(0, 0, 0),
                     center_of_mass_local(0, 0, 0), inertia_tensor_diagonal(1, 1, 1),
                     friction_coefficient(0.5f), restitution_coefficient(0.3f),
                     linear_damping(0.01f), angular_damping(0.01f) {}
                     
    PhysicsState(const Vector3 &linear_vel, const Vector3 &angular_vel, float m = 1.0f)
        : linear_velocity(linear_vel), angular_velocity(angular_vel), mass(m),
          applied_force(0, 0, 0), applied_torque(0, 0, 0), center_of_mass_local(0, 0, 0),
          inertia_tensor_diagonal(1, 1, 1), friction_coefficient(0.5f), restitution_coefficient(0.3f),
          linear_damping(0.01f), angular_damping(0.01f) {}
  };

  // 传送门ID类型
  using PortalId = uint32_t;
  constexpr PortalId INVALID_PORTAL_ID = 0;

  // 实体ID类型 - 由宿主应用程序定义
  using EntityId = uint64_t;
  constexpr EntityId INVALID_ENTITY_ID = 0;

  // 逻辑实体ID类型 - 传送门系统内部管理
  using LogicalEntityId = uint64_t;
  constexpr LogicalEntityId INVALID_LOGICAL_ENTITY_ID = 0;

  // 传送门面类型
  enum class PortalFace
  {
    A, // A面（定义为传送门的一面）
    B  // B面（定义为传送门的另一面，与A面相对）
  };

  // 实体类型枚举
  enum class EntityType
  {
    MAIN,     // 主体实体
    GHOST,    // 幽灵实体
    HYBRID,   // 混合状态（用于转换期间）
    LOGICAL   // 逻辑实体（统一控制层）
  };

  // 物理状态合成策略
  enum class PhysicsStateMergeStrategy
  {
    MAIN_PRIORITY,      // 主体优先（默认）
    GHOST_PRIORITY,     // 幽灵优先
    MOST_RESTRICTIVE,   // 最受限制的状态（用于碰撞约束）
    WEIGHTED_AVERAGE,   // 加权平均
    FORCE_SUMMATION,    // 力和力矩求和（适用于复杂刚体）
    PHYSICS_SIMULATION, // 使用物理引擎模拟合成后的力
    CUSTOM_LOGIC        // 自定义逻辑
  };

  // 复杂物理属性合成配置
  struct ComplexPhysicsMergeConfig
  {
    bool merge_forces;           // 是否合成力
    bool merge_torques;          // 是否合成力矩
    bool consider_leverage;      // 是否考虑杠杆效应
    bool use_physics_simulation; // 是否使用物理引擎模拟
    float main_entity_leverage;  // 主体的杠杆臂长度
    float ghost_entity_leverage; // 幽灵的杠杆臂长度
    Vector3 logical_pivot_point; // 逻辑支点位置（世界坐标）
    
    ComplexPhysicsMergeConfig() 
      : merge_forces(true), merge_torques(true), consider_leverage(true),
        use_physics_simulation(true), main_entity_leverage(1.0f), ghost_entity_leverage(1.0f),
        logical_pivot_point(0, 0, 0) {}
  };

  // 逻辑实体的物理约束状态
  struct PhysicsConstraintState
  {
    bool is_blocked;           // 是否被阻挡
    Vector3 blocking_normal;   // 阻挡面的法向量
    Vector3 allowed_velocity;  // 允许的速度向量
    Vector3 contact_point;     // 接触点位置
    EntityId blocking_entity;  // 阻挡的实体ID（主体或幽灵）
    
    PhysicsConstraintState() 
      : is_blocked(false), blocking_normal(0,0,0), allowed_velocity(0,0,0), 
        contact_point(0,0,0), blocking_entity(INVALID_ENTITY_ID) {}
  };

  // 逻辑实体状态
  struct LogicalEntityState
  {
    LogicalEntityId logical_id;
    EntityId main_entity_id;    // 主体实体ID（向後兼容）
    EntityId ghost_entity_id;   // 幽灵实体ID（向後兼容）
    
    // === 新增：實體鏈多實體支持 ===
    std::vector<EntityId> controlled_entities;      // 受控實體列表（鏈的所有節點）
    std::vector<float> entity_weights;              // 各實體的權重
    std::vector<Transform> entity_transforms;       // 各實體變換
    std::vector<PhysicsState> entity_physics;       // 各實體物理狀態
    EntityId primary_entity_id;                     // 主控實體ID（質心位置）
    
    // 鏈特定的物理計算
    float total_chain_mass;                         // 鏈總質量
    Vector3 chain_center_of_mass;                   // 鏈質心
    std::vector<Vector3> segment_forces;            // 各段受力
    std::vector<Vector3> segment_torques;           // 各段力矩
    
    // 約束傳播
    std::vector<PhysicsConstraintState> segment_constraints;  // 各段約束
    bool has_distributed_constraints;                        // 是否有分佈式約束
    
    // 合成的物理状态
    Transform unified_transform;
    PhysicsState unified_physics;
    PhysicsConstraintState constraint_state;
    
    // 复杂物理属性合成
    ComplexPhysicsMergeConfig complex_merge_config;
    Vector3 total_applied_force;   // 合成后的总作用力
    Vector3 total_applied_torque;  // 合成后的总力矩
    
    // 合成策略
    PhysicsStateMergeStrategy merge_strategy;
    
    // 状态来源权重
    float main_weight;          // 主体权重
    float ghost_weight;         // 幽灵权重
    
    // 控制标志
    bool physics_unified_mode;  // 是否启用统一物理模式
    bool ignore_engine_physics; // 是否忽略引擎物理（完全由逻辑控制）
    bool use_physics_simulation; // 是否使用物理引擎模拟合成状态
    
    // 物理引擎模拟相关
    EntityId simulation_proxy_entity; // 用于物理模拟的代理实体ID
    bool has_simulation_proxy;        // 是否有模拟代理实体
    
    LogicalEntityState() 
      : logical_id(INVALID_LOGICAL_ENTITY_ID), main_entity_id(INVALID_ENTITY_ID), 
        ghost_entity_id(INVALID_ENTITY_ID), primary_entity_id(INVALID_ENTITY_ID),
        total_chain_mass(0.0f), chain_center_of_mass(0, 0, 0), has_distributed_constraints(false),
        total_applied_force(0, 0, 0), total_applied_torque(0, 0, 0),
        merge_strategy(PhysicsStateMergeStrategy::FORCE_SUMMATION),
        main_weight(1.0f), ghost_weight(1.0f), physics_unified_mode(true), 
        ignore_engine_physics(false), use_physics_simulation(true),
        simulation_proxy_entity(INVALID_ENTITY_ID), has_simulation_proxy(false) {}
  };

  // 实体描述结构 - 支持无缝传送
  struct EntityDescription
  {
    EntityId entity_id;
    EntityType entity_type;
    Transform transform;
    PhysicsState physics;
    Vector3 center_of_mass; // 质心位置（相对于实体）
    Vector3 bounds_min;
    Vector3 bounds_max;
    EntityId counterpart_id;    // 对应实体ID（主体↔幽灵）
    PortalId associated_portal; // 关联的传送门ID
    bool is_fully_functional;   // 是否具有完整功能（碰撞、渲染等）

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
    float crossing_progress;  // 0.0 = 未开始, 1.0 = 完全穿过
    Vector3 crossing_point;   // 穿越点位置
    Vector3 center_world_pos; // 质心的世界位置
    bool just_started;        // 是否刚开始穿越
    bool just_completed;      // 是否刚完成穿越

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

  // 增强的传送状态（事件驱动版本）
  struct TeleportState
  {
    EntityId entity_id;
    LogicalEntityId logical_entity_id;  // 关联的逻辑实体ID
    PortalId source_portal;
    PortalId target_portal;
    PortalCrossingState crossing_state; // 当前穿越状态
    PortalCrossingState previous_state; // 上一帧状态
    float transition_progress;          // 传送进度 (0.0 到 1.0)
    bool is_teleporting;                // 是否正在传送

    // A/B面状态同步支持
    PortalFace source_face;       // 源传送门使用的面
    PortalFace target_face;       // 目标传送门使用的面
    bool enable_realtime_sync;    // 是否启用实时同步
    uint64_t last_sync_timestamp; // 上次同步时间戳

    // 无缝传送支持
    EntityId ghost_entity_id;        // 幽灵实体ID
    bool seamless_mode;              // 是否启用无缝传送模式
    bool auto_triggered;             // 是否自动触发的传送
    bool ready_for_swap;             // 是否准备好进行实体交换
    bool role_swapped;               // 是否已执行角色互换
    EntityType original_entity_type; // 原始实体类型

    // === V2 事件驱动新增字段 ===

    // A/B面详细支持
    PortalFace active_source_face;  // 当前活跃的源面
    PortalFace active_target_face;  // 当前活跃的目标面
    bool face_configuration_locked; // 面配置是否锁定

    // 批量操作支持
    bool enable_batch_sync; // 是否启用批量同步
    uint32_t sync_group_id; // 同步组ID

    // 性能优化标识
    bool requires_full_sync; // 需要完整同步
    bool is_high_priority;   // 是否高优先级

    // 质心穿越标记（由事件设置）
    bool center_has_crossed; // 质心是否已穿越
    Vector3 crossing_point;  // 质心穿越点位置

    // === V3 逻辑实体支持新增字段 ===
    
    // 统一物理控制
    bool use_logical_entity_physics; // 是否使用逻辑实体物理控制
    PhysicsStateMergeStrategy merge_strategy; // 物理状态合成策略

    TeleportState() : entity_id(INVALID_ENTITY_ID),
                      logical_entity_id(INVALID_LOGICAL_ENTITY_ID),
                      source_portal(INVALID_PORTAL_ID),
                      target_portal(INVALID_PORTAL_ID),
                      crossing_state(PortalCrossingState::NOT_TOUCHING),
                      previous_state(PortalCrossingState::NOT_TOUCHING),
                      transition_progress(0.0f),
                      is_teleporting(false),
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
                      // V2 新增字段初始化
                      active_source_face(PortalFace::A),
                      active_target_face(PortalFace::B),
                      face_configuration_locked(false),
                      enable_batch_sync(false),
                      sync_group_id(0),
                      requires_full_sync(false),
                      is_high_priority(false),
                      center_has_crossed(false),
                      crossing_point(0, 0, 0),
                      // V3 逻辑实体支持
                      use_logical_entity_physics(true),
                      merge_strategy(PhysicsStateMergeStrategy::MOST_RESTRICTIVE)
    {
    }

    // 添加拷贝构造和赋值运算符以确保可复制性
    TeleportState(const TeleportState &other) = default;
    TeleportState &operator=(const TeleportState &other) = default;
    TeleportState(TeleportState &&other) = default;
    TeleportState &operator=(TeleportState &&other) = default;
  };

  // 幽灵实体状态同步配置
  struct GhostSyncConfig
  {
    bool sync_transform;       // 同步变换（位置、旋转、缩放）
    bool sync_physics;         // 同步物理状态（速度、角速度）
    bool sync_bounds;          // 同步包围盒大小
    bool sync_properties;      // 同步其他属性
    float sync_frequency;      // 同步频率（Hz）
    float transform_threshold; // 变换变化阈值
    float velocity_threshold;  // 速度变化阈值

    GhostSyncConfig() : sync_transform(true),
                        sync_physics(true),
                        sync_bounds(true),
                        sync_properties(false),
                        sync_frequency(60.0f),
                        transform_threshold(0.001f),
                        velocity_threshold(0.01f) {}
  };

  // 实体属性定义（用于完整属性复制）
  struct EntityProperty
  {
    std::string name;  // 属性名
    std::string value; // 属性值（序列化后）
    std::string type;  // 属性类型

    EntityProperty() = default;
    EntityProperty(const std::string &n, const std::string &v, const std::string &t)
        : name(n), value(v), type(t) {}
  };

  // 幽灵实体状态快照（事件驱动版本）
  struct GhostEntitySnapshot
  {
    EntityId main_entity_id;    // 主体实体ID
    EntityId ghost_entity_id;   // 幽灵实体ID (由引擎分配)
    Transform main_transform;   // 主体变换
    Transform ghost_transform;  // 幽灵变换
    PhysicsState main_physics;  // 主体物理状态
    PhysicsState ghost_physics; // 幽灵物理状态
    Vector3 main_bounds_min;    // 主体包围盒最小值
    Vector3 main_bounds_max;    // 主体包围盒最大值
    Vector3 ghost_bounds_min;   // 幽灵包围盒最小值
    Vector3 ghost_bounds_max;   // 幽灵包围盒最大值
    uint64_t timestamp;         // 快照时间戳

    // === V2 事件驱动新增字段 ===

    // A/B面信息
    PortalFace source_face; // 源传送门面
    PortalFace target_face; // 目标传送门面

    // 完整属性快照
    std::vector<EntityProperty> custom_properties; // 自定义属性列表
    bool has_full_functionality;                   // 是否具有完整功能

    // 同步控制
    uint32_t sync_priority;       // 同步优先级
    bool requires_immediate_sync; // 需要立即同步

    GhostEntitySnapshot() : main_entity_id(INVALID_ENTITY_ID),
                            ghost_entity_id(INVALID_ENTITY_ID),
                            timestamp(0),
                            // V2 新增字段初始化
                            source_face(PortalFace::A),
                            target_face(PortalFace::B),
                            has_full_functionality(true),
                            sync_priority(0),
                            requires_immediate_sync(false)
    {
    }
  };

  // === V2 新增数据结构 ===

  // === 实体链传送系统数据结构 ===

  // 实体链节点
  struct EntityChainNode
  {
    EntityId entity_id;           // 实体ID
    EntityType entity_type;       // 实体类型（MAIN, GHOST）
    PortalId entry_portal;        // 进入的传送门ID
    PortalId exit_portal;         // 出去的传送门ID
    int chain_position;           // 在链中的位置（0=主体位置）
    float segment_length;         // 该段的物理长度
    Transform transform;          // 当前变换
    PhysicsState physics_state;   // 物理状态
    
    // 渲染裁切相关
    bool requires_clipping;       // 是否需要裁切
    ClippingPlane clipping_plane; // 裁切平面
    float clipping_ratio;         // 裁切比例（0.0-1.0）
    
    // A/B面支持
    PortalFace entry_face;        // 进入面
    PortalFace exit_face;         // 出去面
    
    // 物理约束
    bool is_constrained;          // 是否被约束
    PhysicsConstraintState constraint_state; // 约束状态
    
    EntityChainNode() 
      : entity_id(INVALID_ENTITY_ID), entity_type(EntityType::MAIN),
        entry_portal(INVALID_PORTAL_ID), exit_portal(INVALID_PORTAL_ID),
        chain_position(0), segment_length(0.0f), requires_clipping(false),
        clipping_ratio(1.0f), entry_face(PortalFace::A), exit_face(PortalFace::B),
        is_constrained(false) {}
  };

  // 实体链状态
  struct EntityChainState
  {
    LogicalEntityId logical_entity_id;   // 统一控制的逻辑实体ID
    EntityId original_entity_id;         // 原始实体ID（用户创建的实体）
    std::vector<EntityChainNode> chain;  // 链节点列表
    int main_position;                   // 质心当前位置（主体在链中的位置）
    float total_chain_length;            // 总链长度
    Vector3 center_of_mass_world_pos;    // 质心世界位置
    
    // 物理属性合成
    PhysicsState unified_physics_state;  // 统一的物理状态
    Vector3 total_applied_force;         // 总作用力
    Vector3 total_applied_torque;        // 总力矩
    
    // 链管理状态
    bool is_actively_teleporting;        // 是否正在进行传送
    uint32_t chain_version;              // 链版本号（用于检测变化）
    uint64_t last_update_timestamp;      // 最后更新时间戳
    
    // 同步控制
    bool enable_batch_sync;              // 是否启用批量同步
    uint32_t sync_group_id;              // 同步组ID
    
    EntityChainState() 
      : logical_entity_id(INVALID_LOGICAL_ENTITY_ID),
        original_entity_id(INVALID_ENTITY_ID), main_position(0),
        total_chain_length(0.0f), center_of_mass_world_pos(0, 0, 0),
        total_applied_force(0, 0, 0), total_applied_torque(0, 0, 0),
        is_actively_teleporting(false), chain_version(0), last_update_timestamp(0),
        enable_batch_sync(false), sync_group_id(0) {}
  };

  // 链节点创建描述符
  struct ChainNodeCreateDescriptor
  {
    EntityId source_entity_id;       // 源实体ID（用于复制属性）
    Transform target_transform;      // 目标变换
    PhysicsState target_physics;     // 目标物理状态
    PortalId through_portal;         // 通过的传送门
    PortalFace entry_face;           // 进入面
    PortalFace exit_face;            // 出去面
    bool full_functionality;         // 是否需要完整功能
    
    ChainNodeCreateDescriptor()
      : source_entity_id(INVALID_ENTITY_ID), through_portal(INVALID_PORTAL_ID),
        entry_face(PortalFace::A), exit_face(PortalFace::B), full_functionality(true) {}
  };

  // 质心配置类型
  enum class CenterOfMassType
  {
    GEOMETRIC_CENTER,  // 幾何中心（預設）
    PHYSICS_CENTER,    // 物理質心
    CUSTOM_POINT,      // 自定義點
    BONE_ATTACHMENT,   // 綁定到骨骼/節點
    WEIGHTED_AVERAGE,  // 多點加權平均
    DYNAMIC_CALCULATED // 動態計算（基於密度分布等）
  };

  // 骨骼/節點附著配置
  struct BoneAttachment
  {
    std::string bone_name; // 骨骼/節點名稱
    Vector3 offset;        // 相對於骨骼的偏移量

    BoneAttachment() : offset(0, 0, 0) {}
    BoneAttachment(const std::string &name, const Vector3 &off = Vector3(0, 0, 0))
        : bone_name(name), offset(off) {}
  };

  // 加權點配置
  struct WeightedPoint
  {
    Vector3 position; // 本地坐標位置
    float weight;     // 權重值

    WeightedPoint() : weight(1.0f) {}
    WeightedPoint(const Vector3 &pos, float w = 1.0f)
        : position(pos), weight(w) {}
  };

  // 質心配置結構
  struct CenterOfMassConfig
  {
    CenterOfMassType type;
    Vector3 custom_point;                       // 自定義點（本地坐標）
    BoneAttachment bone_attachment;             // 骨骼附着
    std::vector<WeightedPoint> weighted_points; // 多點加權配置

    // 動態計算參數
    bool consider_physics_mass;      // 是否考慮物理質量分布
    bool auto_update_on_mesh_change; // 是否在網格改變時自動更新
    float update_frequency;          // 更新頻率（秒）

    CenterOfMassConfig() : type(CenterOfMassType::GEOMETRIC_CENTER),
                           custom_point(0, 0, 0),
                           consider_physics_mass(false),
                           auto_update_on_mesh_change(false),
                           update_frequency(0.1f) {}
  };

  // 質心計算結果
  struct CenterOfMassResult
  {
    Vector3 local_position;    // 本地坐標系中的質心位置
    Vector3 world_position;    // 世界坐標系中的質心位置
    bool is_valid;             // 計算是否成功
    uint64_t calculation_time; // 計算時間戳

    CenterOfMassResult() : local_position(0, 0, 0), world_position(0, 0, 0),
                           is_valid(false), calculation_time(0) {}
  };

  // 传送策略配置（事件驱动版本）
  struct TeleportStrategy
  {
    bool enable_seamless_mode;         // 启用无缝模式
    bool enable_a_b_face_optimization; // 启用A/B面优化
    bool enable_batch_operations;      // 启用批量操作
    float sync_frequency;              // 同步频率
    int max_concurrent_teleports;      // 最大并发传送数
    bool auto_create_ghosts;           // 自动创建幽灵实体
    bool auto_swap_on_center_cross;    // 质心穿越时自动互换身份

    TeleportStrategy()
        : enable_seamless_mode(true), enable_a_b_face_optimization(true), enable_batch_operations(false), sync_frequency(60.0f), max_concurrent_teleports(10), auto_create_ghosts(true), auto_swap_on_center_cross(true) {}
  };

} // namespace Portal

#endif // PORTAL_TYPES_H
