#ifndef RENDER_PROXY_COMPONENT_H
#define RENDER_PROXY_COMPONENT_H

namespace portal_core {

/**
 * 渲染代理组件基类
 * 
 * 这是所有渲染代理组件的基类，用于存储纯渲染数据。
 * 渲染代理组件不包含业务逻辑，只存储用于渲染的数据，
 * 支持双缓冲机制以实现平滑的插值渲染。
 * 
 * 设计原则：
 * 1. 只存储渲染相关的数据
 * 2. 支持当前帧和上一帧的数据存储
 * 3. 提供插值计算的基础接口
 * 4. 与业务逻辑组件完全解耦
 */
struct RenderProxyComponent {
    // 标记是否需要更新
    bool needs_update = true;
    
    // 标记是否已经初始化
    bool is_initialized = false;
    
    // 用于插值计算的时间戳
    double last_update_time = 0.0;
    
    virtual ~RenderProxyComponent() = default;
    
    /**
     * 标记组件需要更新
     * 当逻辑组件数据发生变化时调用
     */
    void mark_dirty() {
        needs_update = true;
    }
    
    /**
     * 清除更新标记
     * 在数据同步完成后调用
     */
    void clear_dirty() {
        needs_update = false;
    }
    
    /**
     * 检查是否需要更新
     */
    bool is_dirty() const {
        return needs_update;
    }
    
    /**
     * 设置初始化状态
     */
    void set_initialized(bool initialized = true) {
        is_initialized = initialized;
    }
    
    /**
     * 检查是否已初始化
     */
    bool initialized() const {
        return is_initialized;
    }
    
    /**
     * 更新时间戳
     */
    void update_timestamp(double time) {
        last_update_time = time;
    }
    
    /**
     * 获取上次更新时间
     */
    double get_last_update_time() const {
        return last_update_time;
    }
};

} // namespace portal_core

#endif // RENDER_PROXY_COMPONENT_H