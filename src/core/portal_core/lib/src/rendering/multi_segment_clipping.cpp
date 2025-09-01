#include "../../include/rendering/multi_segment_clipping.h"
#include "../../include/math/portal_math.h"
#include <algorithm>
#include <chrono>
#include <iostream>

namespace Portal
{

    MultiSegmentClippingManager::MultiSegmentClippingManager()
        : debug_mode_(false)
    {
        std::cout << "MultiSegmentClippingManager: Initialized for chain-based multi-segment clipping" << std::endl;
    }

    MultiSegmentClippingManager::~MultiSegmentClippingManager()
    {
        active_clipping_configs_.clear();
        std::cout << "MultiSegmentClippingManager: Destroyed" << std::endl;
    }

    bool MultiSegmentClippingManager::setup_chain_clipping(const EntityChainState& chain_state, const Vector3& camera_position)
    {
        auto start_time = std::chrono::high_resolution_clock::now();

        if (chain_state.chain.empty())
        {
            std::cout << "MultiSegmentClippingManager: Empty chain provided for entity " << chain_state.original_entity_id << std::endl;
            return false;
        }

        // 检查是否需要多段裁切
        if (chain_state.chain.size() <= 1)
        {
            // 单节点不需要多段裁切，清理配置
            cleanup_entity_clipping(chain_state.original_entity_id);
            return true;
        }

        std::cout << "MultiSegmentClippingManager: Setting up multi-segment clipping for entity " 
                  << chain_state.original_entity_id << " with " << chain_state.chain.size() << " segments" << std::endl;

        // 创建或更新裁切配置
        ChainClippingConfig& config = active_clipping_configs_[chain_state.original_entity_id];
        config.original_entity_id = chain_state.original_entity_id;
        config.chain_nodes = chain_state.chain;  // 复制节点数据
        config.main_position = chain_state.main_position;

        // 计算节点间的裁切平面
        std::vector<ClippingPlane> inter_node_planes = calculate_inter_node_clipping_planes(chain_state);
        
        // 优化裁切平面（移除冗余）
        optimize_clipping_planes(inter_node_planes);

        std::cout << "MultiSegmentClippingManager: Generated " << inter_node_planes.size() 
                  << " clipping planes for " << chain_state.chain.size() << " chain segments" << std::endl;

        // 为每个链节点创建裁切描述符
        config.segment_descriptors.clear();
        config.segment_descriptors.reserve(chain_state.chain.size());

        for (size_t i = 0; i < chain_state.chain.size(); ++i)
        {
            const EntityChainNode& node = chain_state.chain[i];
            MultiSegmentClippingDescriptor descriptor;
            
            descriptor.entity_id = node.entity_id;
            descriptor.use_advanced_stencil_technique = true;

            // 为每个节点确定相关的裁切平面
            // 规则：第i个节点受到第i-1和第i个裁切平面的影响
            if (i > 0 && i - 1 < inter_node_planes.size())
            {
                // 前方裁切平面（裁掉前面的部分）
                descriptor.clipping_planes.push_back(inter_node_planes[i - 1]);
                descriptor.plane_enabled.push_back(true);
            }
            
            if (i < inter_node_planes.size())
            {
                // 后方裁切平面（裁掉后面的部分）
                ClippingPlane back_plane = inter_node_planes[i];
                back_plane.normal = back_plane.normal * -1.0f;  // 反向法线
                back_plane.distance = -back_plane.distance;     // 反向距离
                descriptor.clipping_planes.push_back(back_plane);
                descriptor.plane_enabled.push_back(true);
            }

            // 设置段的透明度（主体段完全不透明，其他段根据距离调整）
            float alpha = 1.0f;
            if (static_cast<int>(i) != chain_state.main_position)
            {
                // 非主体段：根据与主体的距离调整透明度
                int distance_from_main = abs(static_cast<int>(i) - chain_state.main_position);
                alpha = std::max(0.3f, 1.0f - distance_from_main * 0.2f);
            }
            descriptor.segment_alpha.push_back(alpha);

            // 生成模板缓冲值
            std::vector<int> stencil_values = generate_stencil_values(descriptor.clipping_planes.size());
            descriptor.segment_stencil_values = stencil_values;

            // 调试颜色
            if (debug_mode_)
            {
                // 为不同段分配不同颜色
                Vector3 debug_color;
                if (static_cast<int>(i) == chain_state.main_position)
                {
                    debug_color = Vector3(1.0f, 1.0f, 1.0f);  // 主体：白色
                }
                else
                {
                    float hue = (float(i) / float(chain_state.chain.size())) * 360.0f;
                    // 简单的HSV到RGB转换
                    debug_color = Vector3(
                        0.5f + 0.5f * cos(hue * 3.14159f / 180.0f),
                        0.5f + 0.5f * cos((hue + 120.0f) * 3.14159f / 180.0f),
                        0.5f + 0.5f * cos((hue + 240.0f) * 3.14159f / 180.0f)
                    );
                }
                descriptor.segment_colors.push_back(debug_color);
            }

            config.segment_descriptors.push_back(descriptor);
        }

        // 计算段可见性
        calculate_segment_visibility(config, camera_position);

        // 更新配置版本
        clipping_config_versions_[chain_state.original_entity_id] = chain_state.chain_version;

        // 应用裁切设置到物理引擎
        if (apply_clipping_callback_)
        {
            for (const auto& descriptor : config.segment_descriptors)
            {
                apply_clipping_callback_(descriptor.entity_id, descriptor);
            }
        }

        auto end_time = std::chrono::high_resolution_clock::now();
        float setup_time = std::chrono::duration<float, std::milli>(end_time - start_time).count();

        // 更新统计信息
        last_frame_stats_.active_entity_count = static_cast<int>(active_clipping_configs_.size());
        last_frame_stats_.total_clipping_planes += static_cast<int>(inter_node_planes.size());
        last_frame_stats_.total_visible_segments += static_cast<int>(config.segment_descriptors.size());
        last_frame_stats_.frame_setup_time_ms = setup_time;

        std::cout << "MultiSegmentClippingManager: Successfully set up multi-segment clipping in " 
                  << setup_time << "ms" << std::endl;

        return true;
    }

    bool MultiSegmentClippingManager::update_chain_clipping(const EntityChainState& chain_state)
    {
        auto config_it = active_clipping_configs_.find(chain_state.original_entity_id);
        if (config_it == active_clipping_configs_.end())
        {
            std::cout << "MultiSegmentClippingManager: No existing config for entity " << chain_state.original_entity_id << std::endl;
            return false;
        }

        // 检查版本号，如果没有变化则跳过更新
        auto version_it = clipping_config_versions_.find(chain_state.original_entity_id);
        if (version_it != clipping_config_versions_.end() && 
            version_it->second == chain_state.chain_version)
        {
            return true;  // 无需更新
        }

        std::cout << "MultiSegmentClippingManager: Updating clipping config for entity " 
                  << chain_state.original_entity_id << " (version " << chain_state.chain_version << ")" << std::endl;

        // 重新设置裁切（使用相机位置的近似值）
        Vector3 approximate_camera_position(0, 0, 0);  // 可以通过回调获取实际相机位置
        return setup_chain_clipping(chain_state, approximate_camera_position);
    }

    void MultiSegmentClippingManager::cleanup_entity_clipping(EntityId original_entity_id)
    {
        auto config_it = active_clipping_configs_.find(original_entity_id);
        if (config_it == active_clipping_configs_.end())
        {
            return;  // 没有配置需要清理
        }

        const ChainClippingConfig& config = config_it->second;

        std::cout << "MultiSegmentClippingManager: Cleaning up clipping for entity " << original_entity_id 
                  << " with " << config.segment_descriptors.size() << " segments" << std::endl;

        // 清理所有相关实体的裁切设置
        if (clear_clipping_callback_)
        {
            for (const auto& descriptor : config.segment_descriptors)
            {
                clear_clipping_callback_(descriptor.entity_id);
            }
        }

        // 移除配置
        active_clipping_configs_.erase(config_it);
        clipping_config_versions_.erase(original_entity_id);
    }

    void MultiSegmentClippingManager::refresh_all_clipping_states()
    {
        std::cout << "MultiSegmentClippingManager: Refreshing all " << active_clipping_configs_.size() 
                  << " clipping states" << std::endl;

        for (auto& [entity_id, config] : active_clipping_configs_)
        {
            if (apply_clipping_callback_)
            {
                for (const auto& descriptor : config.segment_descriptors)
                {
                    apply_clipping_callback_(descriptor.entity_id, descriptor);
                }
            }
        }
    }

    const std::vector<MultiSegmentClippingDescriptor>* MultiSegmentClippingManager::get_entity_clipping_descriptors(EntityId original_entity_id) const
    {
        auto it = active_clipping_configs_.find(original_entity_id);
        return (it != active_clipping_configs_.end()) ? &it->second.segment_descriptors : nullptr;
    }

    bool MultiSegmentClippingManager::requires_multi_segment_clipping(EntityId original_entity_id) const
    {
        auto it = active_clipping_configs_.find(original_entity_id);
        return (it != active_clipping_configs_.end()) && (it->second.segment_descriptors.size() > 1);
    }

    int MultiSegmentClippingManager::get_visible_segment_count(EntityId original_entity_id, const Vector3& camera_position) const
    {
        auto it = active_clipping_configs_.find(original_entity_id);
        if (it == active_clipping_configs_.end())
        {
            return 0;
        }

        const ChainClippingConfig& config = it->second;
        int visible_count = 0;

        for (const auto& descriptor : config.segment_descriptors)
        {
            // 简单的可见性检查：基于透明度阈值
            if (!descriptor.segment_alpha.empty() && 
                descriptor.segment_alpha[0] >= config.min_segment_visibility_threshold)
            {
                visible_count++;
            }
        }

        return std::min(visible_count, config.max_visible_segments);
    }

    void MultiSegmentClippingManager::set_entity_clipping_quality(EntityId original_entity_id, int quality_level)
    {
        auto it = active_clipping_configs_.find(original_entity_id);
        if (it == active_clipping_configs_.end())
        {
            return;
        }

        ChainClippingConfig& config = it->second;
        
        // 根据质量级别调整配置
        switch (quality_level)
        {
            case 0:  // 最低质量
                config.use_batch_rendering = true;
                config.enable_smooth_transitions = false;
                config.max_visible_segments = 2;
                break;
            case 1:  // 低质量
                config.use_batch_rendering = true;
                config.enable_smooth_transitions = false;
                config.max_visible_segments = 4;
                break;
            case 2:  // 中等质量
                config.use_batch_rendering = true;
                config.enable_smooth_transitions = true;
                config.max_visible_segments = 6;
                break;
            case 3:  // 最高质量
                config.use_batch_rendering = false;
                config.enable_smooth_transitions = true;
                config.max_visible_segments = 8;
                break;
        }

        std::cout << "MultiSegmentClippingManager: Set clipping quality level " << quality_level 
                  << " for entity " << original_entity_id << std::endl;
    }

    void MultiSegmentClippingManager::set_smooth_transitions(EntityId original_entity_id, bool enable, float blend_distance)
    {
        auto it = active_clipping_configs_.find(original_entity_id);
        if (it == active_clipping_configs_.end())
        {
            return;
        }

        ChainClippingConfig& config = it->second;
        config.enable_smooth_transitions = enable;
        config.transition_blend_distance = blend_distance;

        std::cout << "MultiSegmentClippingManager: Set smooth transitions " 
                  << (enable ? "enabled" : "disabled") << " for entity " << original_entity_id << std::endl;
    }

    void MultiSegmentClippingManager::set_debug_mode(bool enable)
    {
        debug_mode_ = enable;
        std::cout << "MultiSegmentClippingManager: Debug mode " 
                  << (enable ? "enabled" : "disabled") << std::endl;
    }

    void MultiSegmentClippingManager::set_apply_clipping_callback(const std::function<void(EntityId, const MultiSegmentClippingDescriptor&)>& callback)
    {
        apply_clipping_callback_ = callback;
        std::cout << "MultiSegmentClippingManager: Apply clipping callback set" << std::endl;
    }

    void MultiSegmentClippingManager::set_clear_clipping_callback(const std::function<void(EntityId)>& callback)
    {
        clear_clipping_callback_ = callback;
        std::cout << "MultiSegmentClippingManager: Clear clipping callback set" << std::endl;
    }

    MultiSegmentClippingManager::ClippingStats MultiSegmentClippingManager::get_clipping_stats() const
    {
        ClippingStats stats = last_frame_stats_;
        
        // 重新计算实时统计
        stats.active_entity_count = static_cast<int>(active_clipping_configs_.size());
        stats.total_clipping_planes = 0;
        stats.total_visible_segments = 0;
        
        for (const auto& [entity_id, config] : active_clipping_configs_)
        {
            for (const auto& descriptor : config.segment_descriptors)
            {
                stats.total_clipping_planes += static_cast<int>(descriptor.clipping_planes.size());
                stats.total_visible_segments++;
            }
        }

        stats.average_segments_per_entity = (stats.active_entity_count > 0) 
            ? static_cast<float>(stats.total_visible_segments) / static_cast<float>(stats.active_entity_count) 
            : 0.0f;

        return stats;
    }

    // === 私有方法实现 ===

    std::vector<ClippingPlane> MultiSegmentClippingManager::calculate_inter_node_clipping_planes(const EntityChainState& chain_state) const
    {
        std::vector<ClippingPlane> planes;
        
        if (chain_state.chain.size() <= 1)
        {
            return planes;
        }

        // 为相邻节点之间生成裁切平面
        for (size_t i = 0; i < chain_state.chain.size() - 1; ++i)
        {
            const EntityChainNode& current_node = chain_state.chain[i];
            const EntityChainNode& next_node = chain_state.chain[i + 1];

            // 计算两个节点之间的中点和法向量
            Vector3 current_pos = current_node.transform.position;
            Vector3 next_pos = next_node.transform.position;
            Vector3 midpoint = (current_pos + next_pos) * 0.5f;

            // 计算从当前节点指向下一个节点的方向
            Vector3 direction = (next_pos - current_pos).normalized();

            // 创建垂直于连接方向的裁切平面
            ClippingPlane plane = ClippingPlane::from_point_and_normal(midpoint, direction);
            
            std::cout << "MultiSegmentClippingManager: Generated clipping plane between nodes " 
                      << i << " and " << i + 1 << " at midpoint (" 
                      << midpoint.x << ", " << midpoint.y << ", " << midpoint.z << ")" << std::endl;
            
            planes.push_back(plane);
        }

        return planes;
    }

    void MultiSegmentClippingManager::calculate_segment_visibility(ChainClippingConfig& config, const Vector3& camera_position)
    {
        if (config.segment_descriptors.empty())
        {
            return;
        }

        // 根据与相机的距离计算每段的可见性
        for (size_t i = 0; i < config.segment_descriptors.size(); ++i)
        {
            if (i < config.chain_nodes.size())
            {
                const EntityChainNode& node = config.chain_nodes[i];
                float distance_to_camera = (node.transform.position - camera_position).length();
                
                // 距离越远，透明度越低（LOD效果）
                float distance_factor = std::max(0.1f, 1.0f - (distance_to_camera * 0.01f));
                
                // 更新透明度
                if (!config.segment_descriptors[i].segment_alpha.empty())
                {
                    config.segment_descriptors[i].segment_alpha[0] *= distance_factor;
                }
            }
        }
    }

    void MultiSegmentClippingManager::optimize_clipping_planes(std::vector<ClippingPlane>& planes) const
    {
        if (planes.size() <= 1)
        {
            return;
        }

        // 只移除真正重複的平面（法向量和距離都幾乎相同）
        // 注意：不要移除僅僅是平行的平面，因為它們可能服務於不同的段分隔
        auto it = planes.begin();
        while (it != planes.end())
        {
            bool should_remove = false;
            
            for (auto other_it = planes.begin(); other_it != planes.end(); ++other_it)
            {
                if (it != other_it)
                {
                    // 檢查是否為真正的重複平面（法向量和距離都相似）
                    if (are_planes_nearly_parallel(*it, *other_it))
                    {
                        // 計算距離差異
                        float distance_diff = abs(abs(it->distance) - abs(other_it->distance));
                        const float DISTANCE_THRESHOLD = 0.01f; // 1cm容差
                        
                        // 只有當距離也非常接近時才認為是重複
                        if (distance_diff < DISTANCE_THRESHOLD)
                        {
                            // 保留距離更保守的平面
                            if (abs(it->distance) < abs(other_it->distance))
                            {
                                should_remove = true;
                                break;
                            }
                        }
                    }
                }
            }
            
            if (should_remove)
            {
                it = planes.erase(it);
                std::cout << "MultiSegmentClippingManager: Removed truly duplicate clipping plane (same normal and distance)" << std::endl;
            }
            else
            {
                ++it;
            }
        }
        
        std::cout << "MultiSegmentClippingManager: Optimization complete, " << planes.size() << " clipping planes retained" << std::endl;
    }

    std::vector<int> MultiSegmentClippingManager::generate_stencil_values(int segment_count) const
    {
        std::vector<int> stencil_values;
        stencil_values.reserve(segment_count);
        
        // 为每个段生成唯一的模板值
        for (int i = 0; i < segment_count; ++i)
        {
            stencil_values.push_back(i + 1);  // 从1开始（0通常保留）
        }
        
        return stencil_values;
    }

    bool MultiSegmentClippingManager::are_planes_nearly_parallel(const ClippingPlane& plane1, const ClippingPlane& plane2, float tolerance) const
    {
        float dot_product = abs(plane1.normal.dot(plane2.normal));
        return dot_product >= tolerance;
    }

    float MultiSegmentClippingManager::calculate_transition_weight(const EntityChainNode& node1, const EntityChainNode& node2, const Vector3& test_point) const
    {
        Vector3 node1_pos = node1.transform.position;
        Vector3 node2_pos = node2.transform.position;
        
        float dist1 = (test_point - node1_pos).length();
        float dist2 = (test_point - node2_pos).length();
        float total_dist = dist1 + dist2;
        
        return (total_dist > 0.001f) ? (dist1 / total_dist) : 0.5f;
    }

    // === 辅助函数实现 ===

    namespace MultiSegmentClippingUtils
    {
        ClippingPlane create_clipping_plane_from_portal(const PortalPlane& portal_plane, PortalFace face)
        {
            Vector3 normal = portal_plane.get_face_normal(face);
            return ClippingPlane::from_point_and_normal(portal_plane.center, normal);
        }

        TransitionRegion calculate_transition_region(const EntityChainNode& node1, const EntityChainNode& node2)
        {
            TransitionRegion region;
            region.start_point = node1.transform.position;
            region.end_point = node2.transform.position;
            region.blend_direction = (region.end_point - region.start_point).normalized();
            region.blend_distance = (region.end_point - region.start_point).length() * 0.2f; // 20%的过渡区域
            return region;
        }

        bool is_point_visible(const Vector3& point, const std::vector<ClippingPlane>& clipping_planes)
        {
            for (const ClippingPlane& plane : clipping_planes)
            {
                if (!plane.enabled) continue;
                
                float distance_to_plane = plane.normal.dot(point) - plane.distance;
                if (distance_to_plane < 0.0f)
                {
                    return false; // 点在裁切平面的背面
                }
            }
            return true;
        }

        float calculate_visibility_ratio(const Vector3& bounds_min, const Vector3& bounds_max, 
                                       const std::vector<ClippingPlane>& clipping_planes)
        {
            if (clipping_planes.empty()) return 1.0f;
            
            // 简化实现：检查包围盒的8个顶点
            std::vector<Vector3> box_vertices = {
                Vector3(bounds_min.x, bounds_min.y, bounds_min.z),
                Vector3(bounds_max.x, bounds_min.y, bounds_min.z),
                Vector3(bounds_min.x, bounds_max.y, bounds_min.z),
                Vector3(bounds_max.x, bounds_max.y, bounds_min.z),
                Vector3(bounds_min.x, bounds_min.y, bounds_max.z),
                Vector3(bounds_max.x, bounds_min.y, bounds_max.z),
                Vector3(bounds_min.x, bounds_max.y, bounds_max.z),
                Vector3(bounds_max.x, bounds_max.y, bounds_max.z)
            };
            
            int visible_vertices = 0;
            for (const Vector3& vertex : box_vertices)
            {
                if (is_point_visible(vertex, clipping_planes))
                {
                    visible_vertices++;
                }
            }
            
            return static_cast<float>(visible_vertices) / 8.0f;
        }

        DebugPlaneVisualization generate_debug_visualization(const std::vector<ClippingPlane>& planes)
        {
            DebugPlaneVisualization viz;
            
            for (size_t i = 0; i < planes.size(); ++i)
            {
                const ClippingPlane& plane = planes[i];
                
                // 生成平面的可视化四边形
                Vector3 center = plane.normal * plane.distance;
                Vector3 tangent = Vector3(1, 0, 0);
                if (abs(plane.normal.dot(tangent)) > 0.9f)
                {
                    tangent = Vector3(0, 1, 0);
                }
                Vector3 bitangent = plane.normal.cross(tangent).normalized();
                tangent = bitangent.cross(plane.normal).normalized();
                
                float plane_size = 2.0f;
                viz.plane_vertices.push_back(center + tangent * plane_size + bitangent * plane_size);
                viz.plane_vertices.push_back(center - tangent * plane_size + bitangent * plane_size);
                viz.plane_vertices.push_back(center - tangent * plane_size - bitangent * plane_size);
                viz.plane_vertices.push_back(center + tangent * plane_size - bitangent * plane_size);
                
                // 法线可视化
                for (int j = 0; j < 4; ++j)
                {
                    viz.plane_normals.push_back(plane.normal);
                }
                
                // 颜色编码
                Vector3 plane_color = Vector3(1.0f, 0.5f, 0.2f); // 橙色
                for (int j = 0; j < 4; ++j)
                {
                    viz.plane_colors.push_back(plane_color);
                }
            }
            
            return viz;
        }
    }

} // namespace Portal
