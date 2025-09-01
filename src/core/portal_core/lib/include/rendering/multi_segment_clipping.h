#ifndef MULTI_SEGMENT_CLIPPING_H
#define MULTI_SEGMENT_CLIPPING_H

#include "../portal_types.h"
#include <vector>
#include <unordered_map>
#include <functional>

namespace Portal
{

    /**
     * 多段裁切描述符
     * 用于描述一个实体如何被裁切成多个片段
     */
    struct MultiSegmentClippingDescriptor
    {
        EntityId entity_id;                                // 实体ID
        std::vector<ClippingPlane> clipping_planes;        // 裁切平面列表
        std::vector<bool> plane_enabled;                   // 各平面是否启用
        std::vector<float> segment_alpha;                  // 各段透明度（0.0-1.0）
        std::vector<int> segment_stencil_values;           // 各段模板缓冲值
        std::vector<Vector3> segment_colors;               // 各段调试颜色（可选）
        bool use_advanced_stencil_technique;               // 是否使用高级模板技术
        
        MultiSegmentClippingDescriptor() 
            : entity_id(INVALID_ENTITY_ID), use_advanced_stencil_technique(true) {}
    };

    /**
     * 链式裁切配置
     * 专门用于实体链的多段裁切
     */
    struct ChainClippingConfig
    {
        EntityId original_entity_id;                       // 原始实体ID
        std::vector<EntityChainNode> chain_nodes;          // 链节点副本（用于计算）
        int main_position;                                 // 主体位置
        
        // 渲染配置
        std::vector<MultiSegmentClippingDescriptor> segment_descriptors; // 各段裁切描述
        bool enable_smooth_transitions;                    // 是否启用平滑过渡
        float transition_blend_distance;                   // 过渡混合距离
        
        // 性能优化
        bool use_batch_rendering;                          // 是否使用批量渲染
        int max_visible_segments;                          // 最大可见段数（LOD）
        float min_segment_visibility_threshold;            // 最小段可见性阈值
        
        ChainClippingConfig() 
            : original_entity_id(INVALID_ENTITY_ID), main_position(0),
              enable_smooth_transitions(true), transition_blend_distance(0.5f),
              use_batch_rendering(true), max_visible_segments(6),
              min_segment_visibility_threshold(0.05f) {}
    };

    /**
     * 多段裁切管理器
     * 负责管理实体的多段裁切渲染
     */
    class MultiSegmentClippingManager
    {
    public:
        MultiSegmentClippingManager();
        ~MultiSegmentClippingManager();

        // === 核心管理接口 ===

        /**
         * 为实体链设置多段裁切
         * 
         * @param chain_state 链状态
         * @param camera_position 相机位置（用于LOD计算）
         * @return 是否设置成功
         */
        bool setup_chain_clipping(const EntityChainState& chain_state, const Vector3& camera_position);

        /**
         * 更新实体链的裁切状态
         * 在链发生变化时调用
         */
        bool update_chain_clipping(const EntityChainState& chain_state);

        /**
         * 清理实体的所有裁切设置
         */
        void cleanup_entity_clipping(EntityId original_entity_id);

        /**
         * 强制刷新所有裁切状态
         */
        void refresh_all_clipping_states();

        // === 渲染接口 ===

        /**
         * 获取实体的多段裁切描述符
         * 供渲染系统使用
         */
        const std::vector<MultiSegmentClippingDescriptor>* get_entity_clipping_descriptors(EntityId original_entity_id) const;

        /**
         * 检查实体是否需要多段裁切
         */
        bool requires_multi_segment_clipping(EntityId original_entity_id) const;

        /**
         * 获取实体可见的段数量（LOD相关）
         */
        int get_visible_segment_count(EntityId original_entity_id, const Vector3& camera_position) const;

        // === 高级功能 ===

        /**
         * 设置实体的裁切质量级别
         * 0 = 最低质量（简单裁切），3 = 最高质量（复杂模板技术）
         */
        void set_entity_clipping_quality(EntityId original_entity_id, int quality_level);

        /**
         * 启用/禁用实体的平滑过渡效果
         */
        void set_smooth_transitions(EntityId original_entity_id, bool enable, float blend_distance = 0.5f);

        /**
         * 设置调试模式（显示裁切平面和段边界）
         */
        void set_debug_mode(bool enable);

        // === 回调设置接口 ===
        
        /**
         * 设置应用裁切的回调函数
         * 用于将裁切配置应用到物理/渲染引擎
         */
        void set_apply_clipping_callback(const std::function<void(EntityId, const MultiSegmentClippingDescriptor&)>& callback);

        /**
         * 设置清除裁切的回调函数
         * 用于清除实体的裁切设置
         */
        void set_clear_clipping_callback(const std::function<void(EntityId)>& callback);

        // === 性能监控 ===
        
        struct ClippingStats
        {
            int active_entity_count;              // 活跃实体数量
            int total_clipping_planes;            // 总裁切平面数
            int total_visible_segments;           // 总可见段数
            float average_segments_per_entity;    // 每实体平均段数
            float frame_setup_time_ms;            // 帧设置时间（毫秒）
        };

        ClippingStats get_clipping_stats() const;

    private:
        // === 内部计算方法 ===

        /**
         * 计算链节点之间的裁切平面
         */
        std::vector<ClippingPlane> calculate_inter_node_clipping_planes(const EntityChainState& chain_state) const;

        /**
         * 计算段的可见性和透明度
         */
        void calculate_segment_visibility(ChainClippingConfig& config, const Vector3& camera_position);

        /**
         * 优化裁切平面（移除冗余平面）
         */
        void optimize_clipping_planes(std::vector<ClippingPlane>& planes) const;

        /**
         * 生成模板缓冲区配置
         */
        std::vector<int> generate_stencil_values(int segment_count) const;

        /**
         * 检查两个平面是否近似平行
         */
        bool are_planes_nearly_parallel(const ClippingPlane& plane1, const ClippingPlane& plane2, float tolerance = 0.95f) const;

        /**
         * 计算段之间的过渡权重
         */
        float calculate_transition_weight(const EntityChainNode& node1, const EntityChainNode& node2, const Vector3& test_point) const;

        // === 数据成员 ===
        
        std::unordered_map<EntityId, ChainClippingConfig> active_clipping_configs_;  // 活跃的裁切配置
        std::unordered_map<EntityId, uint32_t> clipping_config_versions_;           // 配置版本号
        bool debug_mode_;                                                           // 调试模式开关
        mutable ClippingStats last_frame_stats_;                                   // 上帧统计信息
        
        // 回调接口（需要由外部设置）
        std::function<void(EntityId, const MultiSegmentClippingDescriptor&)> apply_clipping_callback_;
        std::function<void(EntityId)> clear_clipping_callback_;
    };

    /**
     * 多段裁切渲染辅助函数
     * 提供常用的裁切计算功能
     */
    namespace MultiSegmentClippingUtils
    {
        /**
         * 从传送门创建裁切平面
         */
        ClippingPlane create_clipping_plane_from_portal(const PortalPlane& portal_plane, PortalFace face);

        /**
         * 计算两个节点之间的过渡区域
         */
        struct TransitionRegion
        {
            Vector3 start_point;
            Vector3 end_point;
            Vector3 blend_direction;
            float blend_distance;
        };
        
        TransitionRegion calculate_transition_region(const EntityChainNode& node1, const EntityChainNode& node2);

        /**
         * 检查点是否在裁切平面的可见侧
         */
        bool is_point_visible(const Vector3& point, const std::vector<ClippingPlane>& clipping_planes);

        /**
         * 计算实体包围盒在裁切平面下的可见比例
         */
        float calculate_visibility_ratio(const Vector3& bounds_min, const Vector3& bounds_max, 
                                       const std::vector<ClippingPlane>& clipping_planes);

        /**
         * 生成调试用的裁切平面可视化数据
         */
        struct DebugPlaneVisualization
        {
            std::vector<Vector3> plane_vertices;     // 平面顶点
            std::vector<Vector3> plane_normals;      // 平面法线
            std::vector<Vector3> plane_colors;       // 平面颜色
        };
        
        DebugPlaneVisualization generate_debug_visualization(const std::vector<ClippingPlane>& planes);
    }

} // namespace Portal

#endif // MULTI_SEGMENT_CLIPPING_H
