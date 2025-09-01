#ifndef PORTAL_MANAGER_H
#define PORTAL_MANAGER_H

#include "../portal_types.h"
#include "../interfaces/portal_event_interfaces.h"
#include "portal.h"
#include "portal_teleport_manager.h"
#include "portal_center_of_mass.h"
#include <unordered_map>
#include <unordered_set>
#include <memory>

namespace Portal
{

  /**
   * 傳送門管理器（重構版）
   *
   * 新架構特點：
   * 1. 事件驅動：不再主動檢測，響應外部物理引擎事件
   * 2. 模塊化設計：將複雜邏輯分離到專門的管理器中
   * 3. 職責清晰：專注於傳送門的生命週期管理和事件分發
   * 4. 簡化介面：移除所有檢測相關的複雜介面
   */
  class PortalManager : public IPortalPhysicsEventReceiver
  {
  private:
    // 核心介面
    PortalInterfaces interfaces_;

    // 模組化管理器
    std::unique_ptr<TeleportManager> teleport_manager_;           // 傳送狀態管理
    std::unique_ptr<CenterOfMassManager> center_of_mass_manager_; // 質心管理

    // 傳送門管理
    std::unordered_map<PortalId, std::unique_ptr<Portal>> portals_;
    std::unordered_set<EntityId> registered_entities_;
    PortalId next_portal_id_;

    // 系統狀態
    bool is_initialized_;
    int default_max_recursion_depth_;

  public:
    /**
     * 構造函數
     * @param interfaces 外部引擎提供的介面集合
     */
    explicit PortalManager(const PortalInterfaces &interfaces);

    ~PortalManager();

    // 禁用拷貝
    PortalManager(const PortalManager &) = delete;
    PortalManager &operator=(const PortalManager &) = delete;

    // === 系統生命週期 ===

    /**
     * 初始化傳送門系統
     */
    bool initialize();

    /**
     * 關閉傳送門系統
     */
    void shutdown();

    /**
     * 每幀更新
     * 注意：新架構中不再包含主動檢測，只處理狀態更新
     */
    void update(float delta_time);

    // === 傳送門管理 ===

    /**
     * 創建新的傳送門
     */
    PortalId create_portal(const PortalPlane &plane);

    /**
     * 銷毀傳送門
     */
    void destroy_portal(PortalId portal_id);

    /**
     * 鏈接兩個傳送門
     */
    bool link_portals(PortalId portal1, PortalId portal2);

    /**
     * 取消傳送門鏈接
     */
    void unlink_portal(PortalId portal_id);

    /**
     * 獲取傳送門對象
     */
    Portal *get_portal(PortalId portal_id);
    const Portal *get_portal(PortalId portal_id) const;

    /**
     * 更新傳送門平面
     */
    void update_portal_plane(PortalId portal_id, const PortalPlane &plane);

    /**
     * 更新傳送門物理狀態（用於移動的傳送門）
     */
    void update_portal_physics_state(PortalId portal_id, const PhysicsState &physics_state);

    // === 實體管理 ===

    /**
     * 註冊需要傳送檢測的實體
     * 注意：新架構中這主要用於內部狀態管理，不再觸發主動檢測
     */
    void register_entity(EntityId entity_id);

    /**
     * 取消註冊實體
     */
    void unregister_entity(EntityId entity_id);

    // === IPortalPhysicsEventReceiver 介面實現 ===
    // 這些方法由外部物理引擎調用

    void on_entity_intersect_portal_start(EntityId entity_id, PortalId portal_id) override;
    void on_entity_center_crossed_portal(EntityId entity_id, PortalId portal_id, PortalFace crossed_face) override;
    void on_entity_fully_passed_portal(EntityId entity_id, PortalId portal_id) override;
    void on_entity_exit_portal(EntityId entity_id, PortalId portal_id) override;

    // === 質心管理 ===

    /**
     * 設定實體的質心配置
     */
    void set_entity_center_of_mass_config(EntityId entity_id, const CenterOfMassConfig &config);

    /**
     * 獲取實體的質心配置
     */
    const CenterOfMassConfig *get_entity_center_of_mass_config(EntityId entity_id) const;

    // === 渲染支援 ===

    /**
     * 計算渲染通道描述符列表（用於遞歸渲染）
     */
    std::vector<RenderPassDescriptor> calculate_render_passes(
        const CameraParams &main_camera,
        int max_recursion_depth = 3) const;

    /**
     * 獲取實體的裁剪平面（用於模型裁切）
     */
    bool get_entity_clipping_plane(EntityId entity_id, ClippingPlane &clipping_plane) const;

    /**
     * 檢查傳送門是否在相機視野內
     */
    bool is_portal_visible(PortalId portal_id, const CameraParams &camera) const;

    // === 狀態查詢 ===

    /**
     * 獲取傳送門數量
     */
    size_t get_portal_count() const { return portals_.size(); }

    /**
     * 獲取註冊實體數量
     */
    size_t get_registered_entity_count() const { return registered_entities_.size(); }

    /**
     * 獲取正在傳送的實體數量
     */
    size_t get_teleporting_entity_count() const;

    /**
     * 检查系统是否已初始化
     */
    bool is_initialized() const { return is_initialized_; }

    // === 批量操作控制 ===

    /**
     * 启用/禁用实体的批量同步
     */
    void set_entity_batch_sync(EntityId entity_id, bool enable_batch, uint32_t sync_group_id = 0);

    /**
     * 强制同步指定传送门的所有幽灵实体
     */
    void force_sync_portal_ghosts(PortalId portal_id);

    /**
     * 获取批量同步性能统计
     */
    struct BatchSyncStats
    {
      size_t total_entities;
      size_t batch_enabled_entities;
      size_t pending_sync_count;
      float last_batch_sync_time;
    };
    BatchSyncStats get_batch_sync_stats() const;

    // === 多段裁切系统控制 ===

    /**
     * 多段裁切统计信息
     */
    struct MultiSegmentClippingStats
    {
      int active_multi_segment_entities;    // 活跃的多段实体数
      int total_clipping_planes;           // 总裁切平面数
      int total_visible_segments;          // 总可见段数
      float average_segments_per_entity;   // 每实体平均段数
      float frame_setup_time_ms;           // 帧设置时间（毫秒）
    };

    /**
     * 获取多段裁切系统的统计信息
     */
    MultiSegmentClippingStats get_multi_segment_clipping_stats() const;

    /**
     * 设置实体的裁切质量级别
     * @param entity_id 实体ID
     * @param quality_level 质量级别 (0-3, 0=最低, 3=最高)
     */
    void set_entity_clipping_quality(EntityId entity_id, int quality_level);

    /**
     * 启用/禁用实体的平滑过渡效果
     * @param entity_id 实体ID
     * @param enable 是否启用
     * @param blend_distance 混合距离
     */
    void set_multi_segment_smooth_transitions(EntityId entity_id, bool enable, float blend_distance = 0.5f);

    /**
     * 获取实体在指定相机位置的可见段数量
     * @param entity_id 实体ID
     * @param camera_position 相机位置
     * @return 可见段数量
     */
    int get_entity_visible_segment_count(EntityId entity_id, const Vector3& camera_position) const;

    /**
     * 设置多段裁切系统的调试模式
     * @param enable 是否启用调试模式
     */
    void set_multi_segment_clipping_debug_mode(bool enable);

    // === 手動傳送介面（向後相容） ===

    /**
     * 手動觸發實體傳送
     * 注意：這是為了向後相容保留的介面，新架構中推薦使用事件驅動方式
     */
    TeleportResult teleport_entity(EntityId entity_id, PortalId source_portal, PortalId target_portal);

  private:
    // === 內部輔助方法 ===

    /**
     * 生成新的傳送門ID
     */
    PortalId generate_portal_id();

    /**
     * 更新傳送門遞歸狀態
     */
    void update_portal_recursive_states();

    /**
     * 計算遞歸渲染通道
     */
    void calculate_recursive_render_passes(
        PortalId portal_id,
        const CameraParams &current_camera,
        int current_depth,
        int max_depth,
        std::vector<RenderPassDescriptor> &render_passes) const;

    /**
     * 通知事件處理器
     */
    void notify_event_handler(const std::function<void(IPortalEventHandler *)> &callback);

    /**
     * 驗證傳送門ID是否有效
     */
    bool is_valid_portal_id(PortalId portal_id) const;
  };

} // namespace Portal

#endif // PORTAL_MANAGER_H