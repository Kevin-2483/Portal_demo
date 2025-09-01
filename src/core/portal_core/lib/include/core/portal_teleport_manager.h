#ifndef PORTAL_TELEPORT_MANAGER_H
#define PORTAL_TELEPORT_MANAGER_H

#include "../portal_types.h"
#include "../interfaces/portal_event_interfaces.h"
#include "logical_entity_manager.h"
#include <unordered_map>
#include <memory>
#include <functional>

// 前向声明
namespace Portal { class MultiSegmentClippingManager; }

namespace Portal
{

  /**
   * Portal 獲取回調類型
   * 用於讓 TeleportManager 能夠獲取 Portal 對象指針
   */
  using PortalGetterCallback = std::function<class Portal *(PortalId portal_id)>;

  /**
   * 傳送狀態管理器
   *
   * 職責：
   * - 管理所有正在進行的傳送狀態
   * - 處理幽靈實體的創建、更新、銷毀
   * - 處理實體角色互換
   * - 管理無縫傳送邏輯
   * - 集成邏輯實體統一控制
   *
   * 這個類別專門負責傳送相關的複雜邏輯，從主管理器中分離出來
   */
  class TeleportManager
  {
  private:
    // 介面引用
    IPhysicsDataProvider *physics_data_;
    IPhysicsManipulator *physics_manipulator_;
    IPortalEventHandler *event_handler_;
    PortalGetterCallback portal_getter_;  ///< Portal 獲取回調

    // 逻辑实体管理器
    std::unique_ptr<LogicalEntityManager> logical_entity_manager_;

    // **新增：多段裁切管理器**
    std::unique_ptr<MultiSegmentClippingManager> multi_segment_clipping_manager_;

    // === 實體鏈傳送管理（替代舊的二元管理） ===
    std::unordered_map<EntityId, EntityChainState> entity_chains_;     // 原始實體 -> 鏈狀態
    std::unordered_map<EntityId, EntityId> chain_node_to_original_;     // 鏈節點 -> 原始實體
    
    // === 向後兼容：保留舊的二元系統支持 ===
    std::unordered_map<EntityId, TeleportState> active_teleports_;      // 活躍的傳送狀態（向後兼容）
    std::unordered_map<EntityId, GhostEntitySnapshot> ghost_snapshots_; // 幽靈實體快照
    std::unordered_map<EntityId, EntityId> main_to_ghost_mapping_;      // 主體 -> 幽靈（向後兼容）
    std::unordered_map<EntityId, EntityId> ghost_to_main_mapping_;      // 幽靈 -> 主體（向後兼容）

    // 同步控制
    float ghost_sync_timer_;
    float sync_frequency_;

    // 逻辑实体控制
    bool use_logical_entity_control_;  // 是否启用逻辑实体控制模式

  public:
    TeleportManager(IPhysicsDataProvider *physics_data,
                    IPhysicsManipulator *physics_manipulator,
                    IPortalEventHandler *event_handler = nullptr);

    ~TeleportManager(); // 声明，定义在实现文件中

    // 禁用拷貝
    TeleportManager(const TeleportManager &) = delete;
    TeleportManager &operator=(const TeleportManager &) = delete;

    // === 更新循環 ===

    /**
     * 更新所有活躍的傳送狀態
     */
    void update(float delta_time);

    // === 事件处理（更新为完整A/B面支持） ===

    /**
     * 處理實體開始與傳送門相交（完整版）
     */
    void handle_entity_intersect_start(EntityId entity_id, PortalId portal_id, class Portal *portal,
                                       PortalId target_portal_id, class Portal *target_portal);

    /**
     * 處理實體質心穿越傳送門（完整A/B面支持）
     */
    void handle_entity_center_crossed(EntityId entity_id, PortalId portal_id, PortalFace crossed_face, class Portal *portal,
                                      PortalId target_portal_id, PortalFace target_face, class Portal *target_portal);

    /**
     * 處理實體完全穿過傳送門（完整版）
     */
    void handle_entity_fully_passed(EntityId entity_id, PortalId portal_id, class Portal *portal,
                                    PortalId target_portal_id, class Portal *target_portal);

    /**
     * 處理實體離開傳送門
     */
    void handle_entity_exit_portal(EntityId entity_id, PortalId portal_id);

    // === 狀態查詢 ===

    /**
     * 獲取實體的傳送狀態
     */
    const TeleportState *get_teleport_state(EntityId entity_id) const;

    /**
     * 獲取幽靈實體快照
     */
    const GhostEntitySnapshot *get_ghost_snapshot(EntityId entity_id) const;

    /**
     * 檢查實體是否正在傳送
     */
    bool is_entity_teleporting(EntityId entity_id) const;

    /**
     * 獲取正在傳送的實體數量
     */
    size_t get_teleporting_entity_count() const;

    // === 實體清理 ===

    /**
     * 清理指定實體的所有傳送相關狀態
     */
    void cleanup_entity(EntityId entity_id);

    /**
     * 清理已完成的傳送
     */
    void cleanup_completed_teleports();

    // === 配置 ===

    /**
     * 設定幽靈同步頻率
     */
    void set_ghost_sync_frequency(float frequency) { sync_frequency_ = frequency; }

    /**
     * 設定 Portal 獲取回調
     * 用於讓 TeleportManager 能夠獲取 Portal 對象
     */
    void set_portal_getter(PortalGetterCallback getter) { portal_getter_ = std::move(getter); }

    // === 實體鏈管理接口 ===

    /**
     * 處理鏈節點進入傳送門
     * 可能是原始主體，也可能是鏈中的幽靈節點
     */
    void handle_chain_node_intersect_portal(EntityId node_entity_id, PortalId portal_id,
                                           Portal* portal, PortalId target_portal_id, Portal* target_portal);

    /**
     * 處理鏈節點質心穿越（可能觸發主體位置變更）
     */
    void handle_chain_node_center_crossed(EntityId node_entity_id, PortalId portal_id, 
                                         PortalFace crossed_face, Portal* portal,
                                         PortalId target_portal_id, PortalFace target_face, Portal* target_portal);

    /**
     * 處理鏈節點完全穿過傳送門
     */
    void handle_chain_node_fully_passed(EntityId node_entity_id, PortalId portal_id);

    /**
     * 處理鏈節點離開傳送門（可能銷毀該節點）
     */
    void handle_chain_node_exit_portal(EntityId node_entity_id, PortalId portal_id);

    /**
     * 獲取實體鏈狀態
     */
    const EntityChainState* get_entity_chain_state(EntityId original_entity_id) const;

    /**
     * 獲取鏈中的主體實體ID（質心位置的實體）
     */
    EntityId get_chain_main_entity(EntityId original_entity_id) const;

    /**
     * 獲取鏈的長度（節點數量）
     */
    size_t get_chain_length(EntityId original_entity_id) const;

    // === 向後兼容：保留舊接口 ===

    // === 逻辑实体控制 ===

    /**
     * 启用/禁用逻辑实体控制模式
     * 当启用时，主体和幽灵将由统一的逻辑实体控制
     */
    void set_logical_entity_control_mode(bool enabled);

    /**
     * 设置逻辑实体的合成策略
     */
    void set_logical_entity_merge_strategy(EntityId entity_id, PhysicsStateMergeStrategy strategy);

    /**
     * 检查逻辑实体是否被约束
     */
    bool is_logical_entity_constrained(EntityId entity_id) const;

    /**
     * 获取逻辑实体的约束状态
     */
    const PhysicsConstraintState* get_logical_entity_constraint(EntityId entity_id) const;

    /**
     * 强制更新逻辑实体状态
     */
    void force_update_logical_entity(EntityId entity_id);

    /**
     * 启用/禁用实体的批量同步
     */
    void set_entity_batch_sync(EntityId entity_id, bool enable_batch, uint32_t sync_group_id = 0);

    /**
     * 强制批量同步指定组的所有实体
     */
    void force_batch_sync_group(uint32_t sync_group_id);

    /**
     * 获取批量同步统计信息
     */
    struct BatchSyncStats
    {
      size_t total_entities;
      size_t batch_enabled_entities;
      size_t pending_sync_count;
      float last_batch_sync_time;
    };
    BatchSyncStats get_batch_sync_stats() const;

    // === 多段裁切控制（新增） ===

    /**
     * 启用/禁用实体的多段裁切
     * 当实体穿越多个传送门时，自动启用多段裁切
     */
    void set_entity_multi_segment_clipping(EntityId entity_id, bool enabled);

    /**
     * 设置实体的裁切质量级别
     * 0 = 最低质量，3 = 最高质量
     */
    void set_entity_clipping_quality(EntityId entity_id, int quality_level);

    /**
     * 启用/禁用多段裁切的平滑过渡效果
     */
    void set_multi_segment_smooth_transitions(EntityId entity_id, bool enabled, float blend_distance = 0.5f);

    /**
     * 获取实体当前的可见段数量（用于LOD）
     */
    int get_entity_visible_segment_count(EntityId entity_id, const Vector3& camera_position) const;

    /**
     * 启用/禁用多段裁切调试模式
     */
    void set_multi_segment_clipping_debug_mode(bool enabled);

    /**
     * 获取多段裁切统计信息
     */
    struct MultiSegmentClippingStats
    {
      int active_multi_segment_entities;
      int total_clipping_planes;
      int total_visible_segments;
      float average_segments_per_entity;
      float frame_setup_time_ms;
    };
    MultiSegmentClippingStats get_multi_segment_clipping_stats() const;

  private:
    // === 實體鏈管理核心邏輯 ===

    /**
     * 擴展實體鏈（創建新的幽靈節點）
     */
    bool extend_entity_chain(EntityId original_entity_id, EntityId extending_node_id, 
                            PortalId entry_portal, PortalId exit_portal,
                            PortalFace entry_face, PortalFace exit_face);

    /**
     * 收縮實體鏈（移除不需要的幽靈節點）
     */
    void shrink_entity_chain(EntityId original_entity_id, EntityId removing_node_id);

    /**
     * 更新主體位置（質心移動到新的鏈位置）
     */
    bool shift_main_entity_position(EntityId original_entity_id, int new_main_position);

    /**
     * 計算鏈節點的物理狀態
     */
    bool calculate_chain_node_state(const EntityChainState& chain_state, int node_position,
                                   const Portal* through_portal, PortalFace entry_face, PortalFace exit_face,
                                   Transform& node_transform, PhysicsState& node_physics);

    /**
     * 更新所有鏈節點的裁切狀態
     */
    void update_chain_clipping_states(EntityChainState& chain_state);

    /**
     * 創建鏈節點實體
     */
    EntityId create_chain_node_entity(const ChainNodeCreateDescriptor& descriptor);

    /**
     * 銷毀鏈節點實體
     */
    void destroy_chain_node_entity(EntityId node_entity_id);

    /**
     * 檢查是否需要質心位置遷移
     */
    bool should_migrate_main_position(const EntityChainState& chain_state, 
                                     EntityId node_entity_id, PortalId crossed_portal);

    /**
     * 獲取或創建實體鏈狀態
     */
    EntityChainState* get_or_create_chain_state(EntityId original_entity_id);

    /**
     * 同步實體鏈狀態到邏輯實體
     */
    void sync_chain_to_logical_entity(EntityChainState& chain_state);

    // === 向後兼容：舊的邏輯處理方法 ===

    // === 內部邏輯處理 ===

    /**
     * 創建幽靈實體（完整A/B面支持）
     */
    bool create_ghost_entity(EntityId entity_id, PortalId portal_id, const class Portal *source_portal,
                             const class Portal *target_portal, PortalFace source_face, PortalFace target_face);

    /**
     * 更新幽靈實體狀態（支持A/B面配置）
     */
    void update_ghost_entity(EntityId entity_id, const class Portal *source_portal, const class Portal *target_portal);

    /**
     * 銷毀幽靈實體
     */
    void destroy_ghost_entity(EntityId entity_id);

    /**
     * 執行實體角色互換（A/B面支持）
     */
    bool execute_entity_role_swap(EntityId main_entity_id, EntityId ghost_entity_id,
                                  PortalFace source_face, PortalFace target_face);

    /**
     * 計算幽靈實體的變換和物理狀態（A/B面支持）
     */
    bool calculate_ghost_state(EntityId main_entity_id,
                               const class Portal *source_portal,
                               const class Portal *target_portal,
                               PortalFace source_face,
                               PortalFace target_face,
                               Transform &ghost_transform,
                               PhysicsState &ghost_physics);

    /**
     * 同步所有幽靈實體
     */
    void sync_all_ghost_entities(float delta_time);

    /**
     * 檢查是否需要同步特定幽靈實體
     */
    bool should_sync_ghost_entity(EntityId entity_id, float delta_time);

    /**
     * 通知事件處理器（如果可用）
     */
    void notify_event_handler(const std::function<void(IPortalEventHandler *)> &callback);

    /**
     * 獲取或創建傳送狀態
     */
    TeleportState *get_or_create_teleport_state(EntityId entity_id, PortalId portal_id);

    // === 逻辑实体管理 ===

    /**
     * 为传送实体创建逻辑实体控制
     */
    bool create_logical_entity_for_teleport(EntityId main_entity_id, EntityId ghost_entity_id);

    /**
     * 销毁传送实体的逻辑实体控制
     */
    void destroy_logical_entity_for_teleport(EntityId main_entity_id);

    /**
     * 更新逻辑实体控制的传送状态
     */
    void update_logical_entity_teleport_states(float delta_time);

    /**
     * 处理逻辑实体约束事件
     */
    void handle_logical_entity_constraint(LogicalEntityId logical_id, const PhysicsConstraintState& constraint);
    
    // === 多段裁切内部方法（新增） ===

    /**
     * 应用多段裁切到指定实体
     */
    void apply_multi_segment_clipping_to_entity(EntityId entity_id, const struct MultiSegmentClippingDescriptor& descriptor);

    /**
     * 清除实体的多段裁切设置
     */
    void clear_entity_multi_segment_clipping(EntityId entity_id);

    /**
     * 估算相机位置（用于LOD计算）
     * 可以通过外部回调优化
     */
    Vector3 estimate_camera_position(const EntityChainState& chain_state) const;

    // === 輔助函數 ===
    
    /**
     * 獲取當前時間戳（毫秒）
     */
    uint64_t get_current_time_ms() const;
  };

} // namespace Portal

#endif // PORTAL_TELEPORT_MANAGER_H