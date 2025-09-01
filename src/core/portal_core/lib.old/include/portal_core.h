#ifndef PORTAL_CORE_H
#define PORTAL_CORE_H

#include "portal_types.h"
#include "portal_interfaces.h"
#include "portal_math.h"
#include "portal_physics_interfaces.h"
#include "portal_detection_manager.h"
#include "portal_center_of_mass.h"
#include <unordered_map>
#include <unordered_set>
#include <vector>
#include <memory>

namespace Portal
{

  /**
   * 传送门对象
   * 表示一个传送门的完整状态
   */
  class Portal
  {
  public:
    Portal(PortalId id);
    ~Portal() = default;

    // 基本属性访问
    PortalId get_id() const { return id_; }
    const PortalPlane &get_plane() const { return plane_; }
    void set_plane(const PortalPlane &plane) { plane_ = plane; }

    // 链接管理
    PortalId get_linked_portal() const { return linked_portal_id_; }
    void set_linked_portal(PortalId portal_id) { linked_portal_id_ = portal_id; }
    bool is_linked() const { return linked_portal_id_ != INVALID_PORTAL_ID; }

    // 状态管理
    bool is_active() const { return is_active_; }
    void set_active(bool active) { is_active_ = active; }

    bool is_recursive() const { return is_recursive_; }
    void set_recursive(bool recursive) { is_recursive_ = recursive; }

    // 移动和物理状态
    const PhysicsState &get_physics_state() const { return physics_state_; }
    void set_physics_state(const PhysicsState &state) { physics_state_ = state; }

    // 渲染相关
    int get_max_recursion_depth() const { return max_recursion_depth_; }
    void set_max_recursion_depth(int depth) { max_recursion_depth_ = depth; }

  private:
    PortalId id_;
    PortalPlane plane_;
    PortalId linked_portal_id_;
    bool is_active_;
    bool is_recursive_;
    PhysicsState physics_state_; // 传送门自身的物理状态（如果需要移动）
    int max_recursion_depth_;    // 最大递归渲染深度
  };

  /**
   * 传送门核心管理器
   * 负责所有传送门的创建、销毁、更新和传送逻辑
   */
  class PortalManager
  {
  public:
    // === 構造函數 - 支援新舊兩種架構 ===
    
    /**
     * 構造函數（舊版介面，向後相容）
     */
    explicit PortalManager(const HostInterfaces &interfaces);
    
    /**
     * 構造函數（新版混合架構）
     * @param physics_interfaces 物理系統介面集合  
     * @param render_interfaces 渲染系統介面（可選，使用舊版介面）
     * @param event_handler 事件處理器（可選）
     */
    explicit PortalManager(const PortalCore::PortalInterfaces &physics_interfaces);
    
    /**
     * 構造函數（完全自定義）
     * 允許混合新舊介面
     */
    PortalManager(PortalCore::IPhysicsDataProvider* data_provider,
                  PortalCore::IPhysicsManipulator* physics_manipulator,
                  PortalCore::IRenderQuery* render_query = nullptr,
                  PortalCore::IRenderManipulator* render_manipulator = nullptr,
                  PortalCore::IPortalEventHandler* event_handler = nullptr,
                  PortalCore::IPortalDetectionOverride* detection_override = nullptr);
    
    ~PortalManager() = default;

    // 禁用拷贝构造和赋值
    PortalManager(const PortalManager &) = delete;
    PortalManager &operator=(const PortalManager &) = delete;

    /**
     * 初始化传送门系统
     * @return 是否初始化成功
     */
    bool initialize();

    /**
     * 关闭传送门系统
     */
    void shutdown();

    /**
     * 每帧更新，处理传送逻辑
     * @param delta_time 时间差
     */
    void update(float delta_time);

    // === 传送门管理 ===

    /**
     * 创建新的传送门
     * @param plane 传送门平面定义
     * @return 传送门ID
     */
    PortalId create_portal(const PortalPlane &plane);

    /**
     * 销毁传送门
     * @param portal_id 传送门ID
     */
    void destroy_portal(PortalId portal_id);

    /**
     * 链接两个传送门
     * @param portal1 第一个传送门ID
     * @param portal2 第二个传送门ID
     * @return 是否链接成功
     */
    bool link_portals(PortalId portal1, PortalId portal2);

    /**
     * 取消传送门链接
     * @param portal_id 传送门ID
     */
    void unlink_portal(PortalId portal_id);

    /**
     * 获取传送门对象
     * @param portal_id 传送门ID
     * @return 传送门指针（可能为空）
     */
    Portal *get_portal(PortalId portal_id);
    const Portal *get_portal(PortalId portal_id) const;

    /**
     * 更新传送门平面
     * @param portal_id 传送门ID
     * @param plane 新的平面定义
     */
    void update_portal_plane(PortalId portal_id, const PortalPlane &plane);

    // === 实体传送管理 ===

    /**
     * 注册需要传送检测的实体
     * @param entity_id 实体ID
     */
    void register_entity(EntityId entity_id);

    /**
     * 取消注册实体
     * @param entity_id 实体ID
     */
    void unregister_entity(EntityId entity_id);

    /**
     * 手动触发实体传送（考虑传送门速度）
     * @param entity_id 实体ID
     * @param source_portal 源传送门
     * @param target_portal 目标传送门
     * @param consider_portal_velocity 是否考虑传送门速度
     * @return 传送结果
     */
    TeleportResult teleport_entity(EntityId entity_id, PortalId source_portal, PortalId target_portal);
    TeleportResult teleport_entity_with_velocity(EntityId entity_id, PortalId source_portal, PortalId target_portal);

    /**
     * 更新传送门的物理状态（位置和速度）
     * @param portal_id 传送门ID
     * @param physics_state 新的物理状态
     */
    void update_portal_physics_state(PortalId portal_id, const PhysicsState &physics_state);

    /**
     * 获取实体的传送状态
     * @param entity_id 实体ID
     * @return 传送状态
     */
    const TeleportState *get_entity_teleport_state(EntityId entity_id) const;

    // === 渲染支持 ===

    /**
     * 计算渲染通道描述符列表
     * @param main_camera 主相机参数
     * @param max_recursion_depth 最大递归深度
     * @return 渲染通道描述符列表
     */
    std::vector<RenderPassDescriptor> calculate_render_passes(
        const CameraParams &main_camera,
        int max_recursion_depth = 3) const;

    /**
     * 获取正在穿越传送门的实体的裁剪平面
     * @param entity_id 实体ID
     * @param clipping_plane 输出的裁剪平面
     * @return 是否需要裁剪
     */
    bool get_entity_clipping_plane(EntityId entity_id, ClippingPlane &clipping_plane) const;

    /**
     * 获取传送门的递归渲染相机列表（已弃用，使用calculate_render_passes代替）
     * @param portal_id 传送门ID
     * @param base_camera 基础相机
     * @param max_depth 最大递归深度
     * @return 相机列表
     */
    std::vector<CameraParams> get_portal_render_cameras(
        PortalId portal_id,
        const CameraParams &base_camera,
        int max_depth = 3) const;

    /**
     * 检查传送门是否在相机视野内
     * @param portal_id 传送门ID
     * @param camera 相机参数
     * @return 是否可见
     */
    bool is_portal_visible(PortalId portal_id, const CameraParams &camera) const;

    // === 调试和统计 ===

    /**
     * 获取传送门数量
     */
    size_t get_portal_count() const { return portals_.size(); }

    /**
     * 获取注册实体数量
     */
    size_t get_registered_entity_count() const { return registered_entities_.size(); }

    /**
     * 获取当前传送中的实体数量
     */
    size_t get_teleporting_entity_count() const;

    // === 新增：幽灵状态同步管理 ===

    /**
     * 设置实体的幽灵同步配置
     * @param entity_id 实体ID
     * @param config 同步配置
     */
    void set_ghost_sync_config(EntityId entity_id, const GhostSyncConfig &config);

    /**
     * 获取实体的幽灵同步配置
     * @param entity_id 实体ID
     * @return 同步配置
     */
    const GhostSyncConfig *get_ghost_sync_config(EntityId entity_id) const;

    // === 新增：質心管理系統 ===

    /**
     * 設置實體的質心配置
     * @param entity_id 實體ID
     * @param config 質心配置
     */
    void set_entity_center_of_mass_config(EntityId entity_id, const CenterOfMassConfig& config);

    /**
     * 獲取實體的質心配置
     * @param entity_id 實體ID
     * @return 質心配置（可能為空）
     */
    const CenterOfMassConfig* get_entity_center_of_mass_config(EntityId entity_id) const;

    // === 新增：時間戳系統配置 ===

    /**
     * 設置自定義時間戳提供者
     * @param timestamp_provider 時間戳提供函數，返回毫秒級時間戳
     */
    void set_timestamp_provider(std::function<uint64_t()> timestamp_provider);

    /**
     * 重置為預設時間戳提供者（簡單計數器）
     */
    void reset_timestamp_provider();

    /**
     * 强制同步幽灵实体状态
     * @param entity_id 主体实体ID
     * @param source_face 源传送门面
     * @param target_face 目标传送门面
     * @return 是否同步成功
     */
    bool force_sync_ghost_state(EntityId entity_id, PortalFace source_face = PortalFace::A, PortalFace target_face = PortalFace::B);

    /**
     * 批量同步所有活跃的幽灵实体
     * @param delta_time 时间差
     * @param force_sync 是否强制同步（忽略频率限制）
     */
    void sync_all_ghost_entities(float delta_time, bool force_sync = false);

    /**
     * 获取幽灵实体快照
     * @param entity_id 主体实体ID
     * @return 幽灵快照（如果存在）
     */
    const GhostEntitySnapshot *get_ghost_snapshot(EntityId entity_id) const;

    /**
     * 计算主体到幽灵的完整变换（支持A/B面）
     * @param main_transform 主体变换
     * @param main_physics 主体物理状态
     * @param main_bounds_min 主体包围盒最小值
     * @param main_bounds_max 主体包围盒最大值
     * @param portal_id 传送门ID
     * @param source_face 源传送门面
     * @param target_face 目标传送门面
     * @param ghost_transform 输出：幽灵变换
     * @param ghost_physics 输出：幽灵物理状态
     * @param ghost_bounds_min 输出：幽灵包围盒最小值
     * @param ghost_bounds_max 输出：幽灵包围盒最大值
     * @return 是否计算成功
     */
    bool calculate_ghost_state(
        const Transform &main_transform,
        const PhysicsState &main_physics,
        const Vector3 &main_bounds_min,
        const Vector3 &main_bounds_max,
        PortalId portal_id,
        PortalFace source_face,
        PortalFace target_face,
        Transform &ghost_transform,
        PhysicsState &ghost_physics,
        Vector3 &ghost_bounds_min,
        Vector3 &ghost_bounds_max) const;

    // === 新增：无缝传送管理方法 ===

    /**
     * 检测并处理质心穿越传送门
     * @param entity_id 实体ID
     * @param delta_time 时间差
     * @return 是否发生了质心穿越
     */
    bool detect_and_handle_center_crossing(EntityId entity_id, float delta_time);

    /**
     * 创建无缝传送（自动触发传送）
     * @param entity_id 实体ID
     * @param portal_id 传送门ID
     * @param crossed_face 穿越的传送门面
     * @return 是否成功创建
     */
    bool create_seamless_teleport(EntityId entity_id, PortalId portal_id, PortalFace crossed_face);

    /**
     * 促进幽灵实体成为主体实体
     * @param ghost_id 幽灵实体ID
     * @param old_main_id 原主体实体ID
     * @return 是否成功交换
     */
    bool promote_ghost_to_main(EntityId ghost_id, EntityId old_main_id);

    /**
     * 检查是否准备好进行实体交换
     * @param entity_id 实体ID
     * @return 是否准备好
     */
    bool is_ready_for_entity_swap(EntityId entity_id) const;

    /**
     * 执行实体角色交换
     * @param main_id 主体实体ID
     * @param ghost_id 幽灵实体ID
     * @return 是否成功交换
     */
    bool execute_entity_role_swap(EntityId main_id, EntityId ghost_id);

    /**
     * 处理质心穿越事件
     * @param entity_id 实体ID
     * @param portal_id 传送门ID
     * @param crossed_face 穿越的面
     * @return 是否成功处理
     */
    bool handle_center_crossing_event(EntityId entity_id, PortalId portal_id, PortalFace crossed_face);

  private:
    // 内部更新方法
    void update_entity_teleportation(float delta_time);
    void check_entity_portal_intersections();
    void update_portal_recursive_states();
    void cleanup_completed_teleports();

    // 传送逻辑
    bool can_entity_teleport(EntityId entity_id, PortalId portal_id) const;
    void start_entity_teleport(EntityId entity_id, PortalId source_portal);
    void complete_entity_teleport(EntityId entity_id);
    void cancel_entity_teleport(EntityId entity_id);

    // 辅助方法
    PortalId generate_portal_id();
    bool is_valid_portal_id(PortalId portal_id) const;
    void notify_event_handler_if_available(const std::function<void(IPortalEventHandler *)> &callback) const;

    // === 新增：三状态机辅助方法 ===

    // 获取或创建传送状态
    TeleportState *get_or_create_teleport_state(EntityId entity_id, PortalId portal_id);

    // 清理实体与传送门的状态
    void cleanup_entity_portal_state(EntityId entity_id, PortalId portal_id);

    // 处理穿越状态变化
    void handle_crossing_state_change(
        EntityId entity_id,
        PortalId portal_id,
        PortalCrossingState previous_state,
        PortalCrossingState new_state);

    // 管理幽灵碰撞体
    void create_ghost_collider_if_needed(EntityId entity_id, PortalId portal_id);
    void update_ghost_collider_position(EntityId entity_id, PortalId portal_id);
    void destroy_ghost_collider_if_exists(EntityId entity_id);

    // === 新增：幽灵状态同步辅助方法 ===

    // 创建幽灵实体（支持A/B面）
    bool create_ghost_entity_with_faces(EntityId entity_id, PortalId portal_id, 
                                       PortalFace source_face, PortalFace target_face);

    // 更新幽灵实体状态（支持A/B面）
    void update_ghost_entity_with_faces(EntityId entity_id, PortalId portal_id,
                                       PortalFace source_face, PortalFace target_face);

    // 销毁幽灵实体
    void destroy_ghost_entity_if_exists(EntityId entity_id);

    // 检查是否需要同步幽灵状态
    bool should_sync_ghost_state(EntityId entity_id, float delta_time) const;

    // 计算变换的差异程度
    float calculate_transform_difference(const Transform &t1, const Transform &t2) const;
    float calculate_physics_difference(const PhysicsState &p1, const PhysicsState &p2) const;

    // 获取当前时间戳（毫秒）
    uint64_t get_current_timestamp() const;

    // 渲染支持辅助方法
    void calculate_recursive_render_passes(
        PortalId portal_id,
        const CameraParams &current_camera,
        int current_depth,
        int max_depth,
        std::vector<RenderPassDescriptor> &render_passes) const;

    // 数据成员
    HostInterfaces interfaces_;  // 舊版介面（向後相容）
    
    // 新版混合架構支援
    std::unique_ptr<PortalCore::PortalDetectionManager> detection_manager_;
    PortalCore::IPhysicsManipulator* physics_manipulator_;      // 新版物理操作介面
    PortalCore::IRenderQuery* render_query_;                    // 新版渲染查詢介面
    PortalCore::IRenderManipulator* render_manipulator_;        // 新版渲染操作介面  
    PortalCore::IPortalEventHandler* event_handler_;            // 新版事件處理器
    
    // 質心管理系統
    std::unique_ptr<CenterOfMassManager> center_of_mass_manager_;
    
    std::unordered_map<PortalId, std::unique_ptr<Portal>> portals_;
    std::unordered_set<EntityId> registered_entities_;
    std::unordered_map<EntityId, TeleportState> active_teleports_;

    // 幽灵状态同步管理
    std::unordered_map<EntityId, GhostSyncConfig> ghost_sync_configs_;
    std::unordered_map<EntityId, GhostEntitySnapshot> ghost_snapshots_;
    float ghost_sync_timer_;  // 同步计时器

    // 新增：无缝传送管理
    std::unordered_map<EntityId, EntityDescription> entity_descriptions_;  // 实体描述缓存
    std::unordered_map<EntityId, EntityId> ghost_to_main_mapping_;         // 幽灵实体到主体实体的映射
    std::unordered_map<EntityId, EntityId> main_to_ghost_mapping_;         // 主体实体到幽灵实体的映射
    std::unordered_map<EntityId, CenterOfMassCrossing> center_crossings_;  // 质心穿越状态
    bool seamless_teleport_enabled_;                                       // 是否启用无缝传送
    float center_crossing_check_interval_;                                 // 质心检测间隔

    PortalId next_portal_id_;
    bool is_initialized_;

    // 配置参数
    float teleport_transition_duration_; // 传送过渡时间
    float portal_detection_distance_;    // 传送门检测距离
    int default_max_recursion_depth_;    // 默认最大递归深度
    
    // 時間戳提供者
    std::function<uint64_t()> timestamp_provider_;  // 可配置的時間戳提供者
  };

} // namespace Portal

#endif // PORTAL_CORE_H
