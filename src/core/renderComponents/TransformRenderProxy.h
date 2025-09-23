#ifndef TRANSFORM_RENDER_PROXY_H
#define TRANSFORM_RENDER_PROXY_H

#include "RenderProxyComponent.h"
#include "../math_types.h"
#include <iostream>
#include "../debug/portal_debug_logging.h"

namespace portal_core {

/**
 * 变换渲染代理组件
 * 
 * 专门用于存储变换相关的渲染数据，支持双缓冲机制。
 * 这个组件从TransformComponent同步数据，并提供插值计算功能。
 * 
 * 数据流：
 * TransformComponent -> RenderSyncSystem -> TransformRenderProxy -> Godot节点
 * 
 * 特性：
 * 1. 双缓冲：存储当前帧和上一帧的变换数据
 * 2. 插值：支持位置、旋转、缩放的平滑插值
 * 3. 优化：只在数据变化时更新
 */
struct TransformRenderProxy : public RenderProxyComponent {
    // 当前帧的变换数据
    struct TransformData {
        Vector3 position{0.0f, 0.0f, 0.0f};
        Quaternion rotation{1.0f, 0.0f, 0.0f, 0.0f}; // w, x, y, z
        Vector3 scale{1.0f, 1.0f, 1.0f};
        
        TransformData() = default;
        
        TransformData(const Vector3& pos, const Quaternion& rot, const Vector3& scl)
            : position(pos), rotation(rot), scale(scl) {}
        
        // 检查两个变换数据是否相等（用于优化）
        bool equals(const TransformData& other, float epsilon = 1e-6f) const {
            return (position - other.position).LengthSq() < epsilon * epsilon &&
                   (rotation - other.rotation).LengthSq() < epsilon * epsilon &&
                   (scale - other.scale).LengthSq() < epsilon * epsilon;
        }
    };
    
    // 当前帧数据
    TransformData current;
    
    // 上一帧数据（用于插值）
    TransformData previous;
    
    // 构造函数
    TransformRenderProxy() = default;
    
    /**
     * 从TransformComponent更新数据
     * @param position 新的位置
     * @param rotation 新的旋转
     * @param scale 新的缩放
     * @param timestamp 更新时间戳
     */
    void update_from_transform(const Vector3& position, const Quaternion& rotation, 
                              const Vector3& scale, double timestamp) {
        // 保存上一帧数据
        previous = current;
        
        // 更新当前帧数据
        current.position = position;
        current.rotation = rotation;
        current.scale = scale;
        
        // 更新时间戳和状态
        update_timestamp(timestamp);
        set_initialized(true);
        clear_dirty();
    }
    
    /**
     * 检查数据是否发生变化
     * @param position 要检查的位置
     * @param rotation 要检查的旋转
     * @param scale 要检查的缩放
     * @param epsilon 比较精度
     * @return 如果数据发生变化返回true
     */
    bool has_changed(const Vector3& position, const Quaternion& rotation, 
                    const Vector3& scale, float epsilon = 1e-6f) const {
        TransformData new_data(position, rotation, scale);
        return !current.equals(new_data, epsilon);
    }
    
    /**
     * 获取插值后的变换数据
     * @param alpha 插值因子 (0.0 = previous, 1.0 = current)
     * @return 插值后的变换数据
     */
    TransformData get_interpolated(float alpha) const {
        // 添加调试信息
        PORTAL_DEBUG_LOG("TransformRenderProxy::get_interpolated - alpha: " << alpha 
                  << ", is_initialized: " << is_initialized);
        
        if (!is_initialized || alpha >= 1.0f) {
            PORTAL_DEBUG_LOG("  -> Returning current (no interpolation)");
            return current;
        }
        
        if (alpha <= 0.0f) {
            PORTAL_DEBUG_LOG("  -> Returning previous (alpha <= 0)");
            return previous;
        }
        
        PORTAL_DEBUG_LOG("  -> Performing interpolation");
        TransformData result;
        
        // 位置线性插值
        result.position = previous.position + (current.position - previous.position) * alpha;
        
        // 旋转球面线性插值
        result.rotation = previous.rotation.SLERP(current.rotation, alpha);
        
        // 缩放线性插值
        result.scale = previous.scale + (current.scale - previous.scale) * alpha;
        
        return result;
    }
    
    /**
     * 获取当前变换数据
     */
    const TransformData& get_current() const {
        return current;
    }
    
    /**
     * 获取上一帧变换数据
     */
    const TransformData& get_previous() const {
        return previous;
    }
    
    /**
     * 重置到初始状态
     */
    void reset() {
        current = TransformData();
        previous = TransformData();
        set_initialized(false);
        mark_dirty();
    }
    
    /**
     * 强制设置当前和上一帧为相同数据（用于初始化）
     */
    void set_initial_transform(const Vector3& position, const Quaternion& rotation, 
                              const Vector3& scale, double timestamp) {
        current = TransformData(position, rotation, scale);
        previous = current; // 初始化时两帧数据相同
        update_timestamp(timestamp);
        set_initialized(true);
        clear_dirty();
    }
};

} // namespace portal_core

#endif // TRANSFORM_RENDER_PROXY_H