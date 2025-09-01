#ifndef PORTAL_EVENT_INTERFACES_H
#define PORTAL_EVENT_INTERFACES_H

#include "../portal_types.h"

namespace Portal
{

  /**
   * 物理引擎事件接收介面
   * 外部物理引擎通過此介面通知傳送門庫發生的碰撞/相交事件
   *
   * 這是新架構的核心：庫不再主動檢測，而是響應外部事件
   */
  class IPortalPhysicsEventReceiver
  {
  public:
    virtual ~IPortalPhysicsEventReceiver() = default;

    /**
     * 實體開始與傳送門相交（包圍盒相交）
     * 外部物理引擎檢測到實體AABB與傳送門平面相交時調用
     *
     * @param entity_id 相交的實體ID
     * @param portal_id 相交的傳送門ID
     */
    virtual void on_entity_intersect_portal_start(EntityId entity_id, PortalId portal_id) = 0;

    /**
     * 實體質心穿越傳送門平面（瞬時事件）
     * 外部物理引擎檢測到實體質心跨過傳送門平面時調用
     * 這是觸發身份互換的關鍵事件
     *
     * @param entity_id 穿越的實體ID
     * @param portal_id 穿越的傳送門ID
     * @param crossed_face 穿越的傳送門面（A或B）
     */
    virtual void on_entity_center_crossed_portal(EntityId entity_id, PortalId portal_id, PortalFace crossed_face) = 0;

    /**
     * 實體完全穿過傳送門
     * 外部物理引擎檢測到實體完全通過傳送門時調用
     *
     * @param entity_id 穿過的實體ID
     * @param portal_id 穿過的傳送門ID
     */
    virtual void on_entity_fully_passed_portal(EntityId entity_id, PortalId portal_id) = 0;

    /**
     * 實體與傳送門分離
     * 外部物理引擎檢測到實體不再與傳送門相交時調用
     *
     * @param entity_id 分離的實體ID
     * @param portal_id 分離的傳送門ID
     */
    virtual void on_entity_exit_portal(EntityId entity_id, PortalId portal_id) = 0;
  };

  /**
   * 庫向外部引擎請求物理操作的介面
   * 完整版，支持所有傳送門系統需要的操作
   */
  class IPhysicsManipulator
  {
  public:
    virtual ~IPhysicsManipulator() = default;

    // === 基礎物理操作 ===

    /**
     * 設定實體的變換
     */
    virtual void set_entity_transform(EntityId entity_id, const Transform &transform) = 0;

    /**
     * 設定實體的物理狀態（速度等）
     */
    virtual void set_entity_physics_state(EntityId entity_id, const PhysicsState &physics_state) = 0;

    /**
     * 設定實體碰撞檢測開關
     */
    virtual void set_entity_collision_enabled(EntityId entity_id, bool enabled) = 0;

    /**
     * 設定實體可見性
     */
    virtual void set_entity_visible(EntityId entity_id, bool visible) = 0;

    /**
     * 分離的速度設置（精確控制）
     */
    virtual void set_entity_velocity(EntityId entity_id, const Vector3 &velocity) = 0;

    /**
     * 設置實體角速度
     */
    virtual void set_entity_angular_velocity(EntityId entity_id, const Vector3 &angular_velocity) = 0;

    // === 幽靈實體管理 ===

    /**
     * 創建幽靈實體
     * @param source_entity_id 源實體ID
     * @param ghost_transform 幽靈實體的變換
     * @param ghost_physics 幽靈實體的物理狀態
     * @return 創建的幽靈實體ID，失敗返回INVALID_ENTITY_ID
     */
    virtual EntityId create_ghost_entity(EntityId source_entity_id,
                                         const Transform &ghost_transform,
                                         const PhysicsState &ghost_physics) = 0;

    /**
     * 創建全功能幽靈實體（支持A/B面配置）
     * @param entity_desc 實體的完整描述
     * @param ghost_transform 幽靈實體變換
     * @param ghost_physics 幽靈實體物理狀態
     * @param source_face 源傳送門面
     * @param target_face 目標傳送門面
     * @return 創建的幽靈實體ID
     */
    virtual EntityId create_full_functional_ghost(
        const EntityDescription &entity_desc,
        const Transform &ghost_transform,
        const PhysicsState &ghost_physics,
        PortalFace source_face = PortalFace::A,
        PortalFace target_face = PortalFace::B) = 0;

    /**
     * 銷毀幽靈實體
     */
    virtual void destroy_ghost_entity(EntityId ghost_entity_id) = 0;

    /**
     * 更新幽靈實體狀態
     */
    virtual void update_ghost_entity(EntityId ghost_entity_id,
                                     const Transform &transform,
                                     const PhysicsState &physics) = 0;

    /**
     * 設置幽靈實體的邊界
     */
    virtual void set_ghost_entity_bounds(
        EntityId ghost_entity_id,
        const Vector3 &bounds_min,
        const Vector3 &bounds_max) = 0;

    /**
     * 批量同步幽靈實體狀態（性能優化）
     */
    virtual void sync_ghost_entities(const std::vector<GhostEntitySnapshot> &snapshots) = 0;

    // === 實體鏈支持接口 ===

    /**
     * 創建鏈節點實體
     * @param descriptor 鏈節點創建描述符
     * @return 創建的鏈節點實體ID，失敗返回INVALID_ENTITY_ID
     */
    virtual EntityId create_chain_node_entity(const ChainNodeCreateDescriptor& descriptor) = 0;

    /**
     * 銷毀鏈節點實體
     */
    virtual void destroy_chain_node_entity(EntityId node_entity_id) = 0;

    /**
     * 設置實體的裁切平面（用於傳送門裁切渲染）
     */
    virtual void set_entity_clipping_plane(EntityId entity_id, const ClippingPlane& clipping_plane) = 0;

    /**
     * 禁用實體的裁切平面
     */
    virtual void disable_entity_clipping(EntityId entity_id) = 0;

    /**
     * 批量設置多個實體的裁切狀態
     * 用於實體鏈的高效裁切管理
     */
    virtual void set_entities_clipping_states(const std::vector<EntityId>& entity_ids,
                                             const std::vector<ClippingPlane>& clipping_planes,
                                             const std::vector<bool>& enable_clipping) = 0;

    // === 無縫傳送支援 ===

    /**
     * 實體角色互換（幽靈變主體，主體變幽靈或銷毀）
     * 這是無縫傳送的核心操作
     *
     * @param main_entity_id 當前的主體實體ID
     * @param ghost_entity_id 當前的幽靈實體ID
     * @return 操作是否成功
     */
    virtual bool swap_entity_roles(EntityId main_entity_id, EntityId ghost_entity_id) = 0;

    /**
     * 增強的身份互換（支持A/B面特定配置）
     * 
     * 關鍵：無感角色互換邏輯
     * 1. 角色邏輯互換：原幽靈成為新主體，原主體成為新幽靈
     * 2. 物理狀態保持：各自保持原來的位置、速度、加速度等狀態
     * 3. 無感體驗：玩家感受不到任何跳躍或視覺中斷
     * 
     * 實現要求：
     * - 只改變實體的控制角色（誰是主控，誰是跟隨）
     * - 絕對不能交換物理狀態（位置、速度等）
     * - 確保運動的連續性和流暢性
     */
    virtual bool swap_entity_roles_with_faces(
        EntityId main_entity_id,
        EntityId ghost_entity_id,
        PortalFace source_face,
        PortalFace target_face) = 0;

    /**
     * 設置實體的功能狀態（是否具備完整功能）
     */
    virtual void set_entity_functional_state(EntityId entity_id, bool is_fully_functional) = 0;

    /**
     * 複製實體的所有屬性到另一個實體
     */
    virtual bool copy_all_entity_properties(EntityId source_entity_id, EntityId target_entity_id) = 0;

    // === 質心管理支援 ===
    // 注意：這裡是"設置"質心位置，讓物理引擎知道質心在哪裡
    // 物理引擎會基於這些設置來進行質心穿越檢測

    /**
     * 設置實體的質心位置（本地坐標偏移）
     * 物理引擎會使用這個設置來進行質心穿越檢測
     */
    virtual void set_entity_center_of_mass(EntityId entity_id, const Vector3 &center_offset) = 0;

    // === 逻辑实体支持 ===

    /**
     * 启用/禁用实体的物理引擎控制
     * 当逻辑实体接管控制时，需要禁用物理引擎的直接控制
     * @param entity_id 实体ID
     * @param engine_controlled true=物理引擎控制，false=外部控制
     */
    virtual void set_entity_physics_engine_controlled(EntityId entity_id, bool engine_controlled) = 0;

    /**
     * 检测实体的碰撞约束状态
     * 用于检测实体是否被阻挡、接触面信息等
     * @param entity_id 实体ID
     * @param constraint_info 输出约束信息
     * @return 是否检测到约束
     */
    virtual bool detect_entity_collision_constraints(EntityId entity_id, struct PhysicsConstraintState& constraint_info) = 0;

    /**
     * 强制设置实体的物理状态（忽略物理引擎计算）
     * 用于逻辑实体直接控制物理实体
     */
    virtual void force_set_entity_physics_state(EntityId entity_id, const Transform& transform, const PhysicsState& physics) = 0;

    /**
     * 批量强制设置多个实体的物理状态
     * 性能优化的批量操作
     */
    virtual void force_set_entities_physics_states(const std::vector<EntityId>& entity_ids,
                                                  const std::vector<Transform>& transforms,
                                                  const std::vector<PhysicsState>& physics_states) = 0;

    // === 复杂物理属性支持 ===

    /**
     * 创建物理模拟代理实体
     * 用于模拟逻辑实体的合成物理状态
     * @param template_entity_id 模板实体ID（复制物理属性）
     * @param initial_transform 初始变换
     * @param initial_physics 初始物理状态
     * @return 代理实体ID
     */
    virtual EntityId create_physics_simulation_proxy(EntityId template_entity_id,
                                                     const Transform& initial_transform,
                                                     const PhysicsState& initial_physics) = 0;

    /**
     * 销毁物理模拟代理实体
     */
    virtual void destroy_physics_simulation_proxy(EntityId proxy_entity_id) = 0;

    /**
     * 对代理实体施加力
     * @param proxy_entity_id 代理实体ID
     * @param force 作用力（世界坐标）
     * @param application_point 作用点（世界坐标，可选）
     */
    virtual void apply_force_to_proxy(EntityId proxy_entity_id, const Vector3& force, 
                                     const Vector3& application_point = Vector3(0, 0, 0)) = 0;

    /**
     * 对代理实体施加力矩
     */
    virtual void apply_torque_to_proxy(EntityId proxy_entity_id, const Vector3& torque) = 0;

    /**
     * 清除代理实体上的所有作用力
     */
    virtual void clear_forces_on_proxy(EntityId proxy_entity_id) = 0;

    /**
     * 设置代理实体的物理材质属性
     */
    virtual void set_proxy_physics_material(EntityId proxy_entity_id, 
                                           float friction, float restitution, 
                                           float linear_damping, float angular_damping) = 0;

    /**
     * 获取实体上的当前作用力和力矩
     * 用于复杂物理状态合成
     */
    virtual bool get_entity_applied_forces(EntityId entity_id, Vector3& total_force, Vector3& total_torque) = 0;
  };

  /**
   * 庫從外部引擎獲取物理數據的介面
   * 完整版，支持所有必需的查詢功能，但不包含主動檢測
   */
  class IPhysicsDataProvider
  {
  public:
    virtual ~IPhysicsDataProvider() = default;

    // === 基礎數據查詢 ===

    /**
     * 獲取實體的變換
     */
    virtual Transform get_entity_transform(EntityId entity_id) = 0;

    /**
     * 獲取實體的物理狀態
     */
    virtual PhysicsState get_entity_physics_state(EntityId entity_id) = 0;

    /**
     * 獲取實體的包圍盒（本地坐標）
     */
    virtual void get_entity_bounds(EntityId entity_id, Vector3 &bounds_min, Vector3 &bounds_max) = 0;

    /**
     * 檢查實體是否有效
     */
    virtual bool is_entity_valid(EntityId entity_id) = 0;

    /**
     * 獲取實體的完整描述（包含質心、類型等信息）
     */
    virtual EntityDescription get_entity_description(EntityId entity_id) = 0;

    // === 批量查詢優化 ===

    /**
     * 批量獲取實體變換（性能優化）
     */
    virtual std::vector<Transform> get_entities_transforms(const std::vector<EntityId> &entity_ids) = 0;

    /**
     * 批量獲取實體物理狀態（性能優化）
     */
    virtual std::vector<PhysicsState> get_entities_physics_states(const std::vector<EntityId> &entity_ids) = 0;

    /**
     * 批量獲取實體描述
     */
    virtual std::vector<EntityDescription> get_entities_descriptions(const std::vector<EntityId> &entity_ids) = 0;

    // === 質心系統支援 ===
    // 注意：質心管理的作用是"配置"而不是"檢測"
    // 庫設置實體的質心位置，物理引擎基於此進行檢測

    /**
     * 計算實體的質心（世界坐標）
     * 基於實體的質心配置進行計算，供物理引擎使用
     */
    virtual Vector3 calculate_entity_center_of_mass(EntityId entity_id) = 0;

    /**
     * 獲取實體質心的世界位置（直接查詢，無計算）
     * 物理引擎可能需要實時查詢質心位置進行檢測
     */
    virtual Vector3 get_entity_center_of_mass_world_pos(EntityId entity_id) = 0;

    /**
     * 檢查實體是否有自定義質心配置
     * 物理引擎可據此決定使用自定義質心還是默認幾何中心
     */
    virtual bool has_center_of_mass_config(EntityId entity_id) = 0;

    /**
     * 獲取實體的質心配置
     * 物理引擎可據此了解質心的配置方式
     */
    virtual CenterOfMassConfig get_entity_center_of_mass_config(EntityId entity_id) = 0;
  };

  /**
   * 渲染相關查詢介面（可選）
   * 用於支援傳送門遞歸渲染
   */
  class IRenderQuery
  {
  public:
    virtual ~IRenderQuery() = default;

    /**
     * 獲取主相機參數
     */
    virtual CameraParams get_main_camera() = 0;

    /**
     * 檢查點是否在視錐體內
     */
    virtual bool is_point_in_view_frustum(const Vector3 &point, const CameraParams &camera) = 0;
  };

  /**
   * 渲染操作介面（可選）
   * 用於控制傳送門渲染
   */
  class IRenderManipulator
  {
  public:
    virtual ~IRenderManipulator() = default;

    /**
     * 設定實體渲染開關
     */
    virtual void set_entity_render_enabled(EntityId entity_id, bool enabled) = 0;

    /**
     * 設定裁剪平面
     */
    virtual void set_clipping_plane(const ClippingPlane &plane) = 0;

    /**
     * 禁用裁剪平面
     */
    virtual void disable_clipping_plane() = 0;

    /**
     * 渲染傳送門遞歸視圖
     */
    virtual void render_portal_recursive_view(PortalId portal_id, int recursion_depth) = 0;
  };

  /**
   * 事件通知介面
   * 庫通過此介面向外部應用程序發送事件通知
   */
  class IPortalEventHandler
  {
  public:
    virtual ~IPortalEventHandler() = default;

    /**
     * 實體開始傳送（遊戲引擎返回準備狀態）
     * @return true表示遊戲引擎已準備好處理傳送，false表示暫時無法處理
     */
    virtual bool on_entity_teleport_begin(EntityId entity_id, PortalId from_portal, PortalId to_portal) { return true; }

    /**
     * 實體傳送完成（遊戲引擎返回清理結果）
     * @return true表示遊戲引擎已完成所有清理工作，false表示需要額外處理
     */
    virtual bool on_entity_teleport_complete(EntityId entity_id, PortalId from_portal, PortalId to_portal) { return true; }

    /**
     * 幽靈實體被創建（遊戲引擎返回創建確認）
     * @return true表示遊戲引擎已識別並準備處理幽靈實體，false表示創建失敗
     */
    virtual bool on_ghost_entity_created(EntityId main_entity, EntityId ghost_entity, PortalId portal) { return true; }

    /**
     * 幽靈實體被銷毀（遊戲引擎返回銷毀確認）
     * @return true表示遊戲引擎已完成幽靈實體相關清理，false表示清理失敗
     */
    virtual bool on_ghost_entity_destroyed(EntityId main_entity, EntityId ghost_entity, PortalId portal) { return true; }

    /**
     * 實體角色互換發生（遊戲引擎返回處理結果）
     * 
     * 庫只負責提供角色互換的核心信息，遊戲引擎自行決定如何處理
     * 
     * @param old_main_entity 原主體實體ID（現在成為幽靈）
     * @param old_ghost_entity 原幽靈實體ID（現在成為主體）
     * @param new_main_entity 新主體實體ID（原幽靈）
     * @param new_ghost_entity 新幽靈實體ID（原主體）
     * @param portal_id 發生互換的傳送門ID
     * @param main_transform 新主體的當前變換
     * @param ghost_transform 新幽靈的當前變換
     * @return 返回true表示遊戲引擎成功處理了角色互換，false表示處理失敗
     */
    virtual bool on_entity_roles_swapped(
        EntityId old_main_entity, 
        EntityId old_ghost_entity,
        EntityId new_main_entity,
        EntityId new_ghost_entity, 
        PortalId portal_id,
        const Transform& main_transform,
        const Transform& ghost_transform) { return true; }

    // 移除了錯誤的遊戲引擎特定事件
    // 庫不應該知道攝像機、玩家控制等概念

    /**
     * 傳送門鏈接建立
     */
    virtual void on_portals_linked(PortalId portal1, PortalId portal2) {}

    /**
     * 傳送門鏈接斷開
     */
    virtual void on_portals_unlinked(PortalId portal1, PortalId portal2) {}

    /**
     * 傳送門進入遞歸狀態
     */
    virtual void on_portal_recursive_state(PortalId portal_id, bool is_recursive) {}

    // === 逻辑实体事件 ===

    /**
     * 逻辑实体创建事件
     */
    virtual void on_logical_entity_created(LogicalEntityId logical_id, EntityId main_entity, EntityId ghost_entity) {}

    /**
     * 逻辑实体销毁事件
     */
    virtual void on_logical_entity_destroyed(LogicalEntityId logical_id, EntityId main_entity, EntityId ghost_entity) {}

    /**
     * 逻辑实体被约束事件（碰撞阻挡）
     */
    virtual void on_logical_entity_constrained(LogicalEntityId logical_id, const struct PhysicsConstraintState& constraint) {}

    /**
     * 逻辑实体约束解除事件
     */
    virtual void on_logical_entity_constraint_released(LogicalEntityId logical_id) {}

    /**
     * 逻辑实体状态合成事件
     */
    virtual void on_logical_entity_state_merged(LogicalEntityId logical_id, PhysicsStateMergeStrategy strategy) {}
  };

  /**
   * 專門的幽靈管理事件接口
   * 用於更細粒度的幽靈實體管理事件通知
   */
  class IPortalGhostEventReceiver
  {
  public:
    virtual ~IPortalGhostEventReceiver() = default;

    /**
     * 幽靈實體狀態同步需求事件
     */
    virtual void on_ghost_sync_required(
        EntityId main_entity_id,
        EntityId ghost_entity_id,
        PortalId portal_id) = 0;

    /**
     * 批量同步完成事件
     */
    virtual void on_batch_ghost_sync_completed(
        const std::vector<EntityId> &entity_ids,
        bool success) = 0;

    /**
     * 身份互換準備事件
     */
    virtual void on_entity_swap_preparation(
        EntityId main_entity_id,
        EntityId ghost_entity_id,
        PortalFace source_face,
        PortalFace target_face) = 0;

    /**
     * 幽靈實體需要更新事件
     */
    virtual void on_ghost_entity_update_required(
        EntityId main_entity_id,
        EntityId ghost_entity_id,
        PortalId portal_id,
        bool urgent) = 0;
  };

  /**
   * 統一的介面集合
   * 完整版介面組合，支持所有傳送門功能
   */
  struct PortalInterfaces
  {
    // 必需的介面
    IPhysicsDataProvider *physics_data;       // 物理數據查詢
    IPhysicsManipulator *physics_manipulator; // 物理操作

    // 可選的介面
    IRenderQuery *render_query;                      // 渲染查詢（用於遞歸渲染）
    IRenderManipulator *render_manipulator;          // 渲染操作（用於裁剪等）
    IPortalEventHandler *event_handler;              // 事件通知
    IPortalGhostEventReceiver *ghost_event_receiver; // 幽靈管理事件接收器

    PortalInterfaces()
        : physics_data(nullptr), physics_manipulator(nullptr), render_query(nullptr), render_manipulator(nullptr), event_handler(nullptr), ghost_event_receiver(nullptr) {}

    /**
     * 檢查必需介面是否完整
     */
    bool is_valid() const
    {
      return physics_data && physics_manipulator;
    }

    /**
     * 檢查是否支援渲染功能
     */
    bool supports_rendering() const
    {
      return render_query && render_manipulator;
    }

    /**
     * 檢查是否支援高級幽靈管理
     */
    bool supports_advanced_ghost_management() const
    {
      return ghost_event_receiver != nullptr;
    }
  };

} // namespace Portal

#endif // PORTAL_EVENT_INTERFACES_H