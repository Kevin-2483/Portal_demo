#pragma once

#include "debug_config.h"

#ifdef PORTAL_DEBUG_ENABLED

#include "i_debuggable.h"
#include <vector>
#include <unordered_map>
#include <string>
#include <memory>

namespace portal_core {
namespace debug {

/**
 * 可调试对象注册管理器
 * 
 * 负责管理所有实现了IDebuggable接口的对象：
 * - 自动发现和注册可调试对象
 * - 批量调用调试渲染方法
 * - 提供查找和管理功能
 * - 与DebugGUISystem集成，统一调试界面
 * 
 * 设计原则：
 * - 最小侵入：不强制任何对象必须注册
 * - 自动管理：对象可以随时注册和注销
 * - 性能友好：只在调试启用时进行渲染调用
 */
class DebuggableRegistry {
public:
    /**
     * 获取单例实例
     */
    static DebuggableRegistry& instance();
    
    /**
     * 注册可调试对象
     * 
     * 将对象添加到调试系统中。对象将在下次调试更新时开始显示。
     * 注意：这里存储的是原始指针，调用者负责对象的生命周期管理。
     * 
     * @param debuggable 要注册的可调试对象（不能为nullptr）
     */
    void register_debuggable(IDebuggable* debuggable);
    
    /**
     * 注销可调试对象
     * 
     * 从调试系统中移除对象。
     * 
     * @param debuggable 要注销的可调试对象
     */
    void unregister_debuggable(IDebuggable* debuggable);
    
    /**
     * 批量渲染所有GUI调试界面
     * 
     * 调用所有已注册对象的render_debug_gui()方法。
     * 每帧由DebugGUISystem调用一次。
     */
    void render_all_gui();
    
    /**
     * 批量渲染所有世界空间调试图形
     * 
     * 调用所有已注册对象的render_debug_world()方法。
     * 每帧调用一次。
     */
    void render_all_world();
    
    /**
     * 渲染调试对象列表窗口
     * 
     * 显示所有已注册对象的列表，允许单独启用/禁用。
     */
    void render_debuggable_list();
    
    /**
     * 按名称查找可调试对象
     * 
     * @param name 调试名称（IDebuggable::get_debug_name()的返回值）
     * @return 找到的对象指针，如果未找到则返回nullptr
     */
    IDebuggable* find_by_name(const std::string& name);
    
    /**
     * 获取已注册对象数量
     */
    size_t get_registered_count() const { return registered_objects_.size(); }
    
    /**
     * 检查是否已注册某个对象
     */
    bool is_registered(IDebuggable* debuggable) const;
    
    /**
     * 清空所有注册
     * 
     * 移除所有已注册的对象。通常在系统关闭时调用。
     */
    void clear_all();
    
    /**
     * 设置全局调试启用状态
     * 
     * 当为false时，render_all_*方法将跳过所有渲染调用。
     */
    void set_debug_enabled(bool enabled) { debug_enabled_ = enabled; }
    bool is_debug_enabled() const { return debug_enabled_; }

private:
    DebuggableRegistry() = default;
    ~DebuggableRegistry() = default;
    DebuggableRegistry(const DebuggableRegistry&) = delete;
    DebuggableRegistry& operator=(const DebuggableRegistry&) = delete;
    
    // 已注册的对象列表
    std::vector<IDebuggable*> registered_objects_;
    
    // 按名称索引的快速查找表
    std::unordered_map<std::string, IDebuggable*> named_objects_;
    
    // 全局调试启用状态
    bool debug_enabled_ = true;
};

/**
 * 便利宏定义
 * 
 * 简化IDebuggable接口的声明和注册过程。
 */

/**
 * 在类中声明IDebuggable接口的必要方法
 * 
 * 用法：
 * class MySystem : public IDebuggable {
 * public:
 *     PORTAL_DECLARE_DEBUGGABLE()
 *     // ... 其他代码
 * };
 */
#define PORTAL_DECLARE_DEBUGGABLE() \
    void render_debug_gui() override; \
    void render_debug_world() override; \
    std::string get_debug_name() const override;

/**
 * 注册对象到调试系统
 * 
 * 通常在构造函数中调用：
 * MySystem::MySystem() {
 *     PORTAL_REGISTER_DEBUGGABLE(this);
 * }
 */
#define PORTAL_REGISTER_DEBUGGABLE(obj) \
    portal_core::debug::DebuggableRegistry::instance().register_debuggable(obj)

/**
 * 从调试系统注销对象
 * 
 * 通常在析构函数中调用：
 * MySystem::~MySystem() {
 *     PORTAL_UNREGISTER_DEBUGGABLE(this);
 * }
 */
#define PORTAL_UNREGISTER_DEBUGGABLE(obj) \
    portal_core::debug::DebuggableRegistry::instance().unregister_debuggable(obj)

} // namespace debug
} // namespace portal_core

#else

// 当调试功能未启用时，宏定义为空操作
#define PORTAL_DECLARE_DEBUGGABLE()
#define PORTAL_REGISTER_DEBUGGABLE(obj)
#define PORTAL_UNREGISTER_DEBUGGABLE(obj)

#endif // PORTAL_DEBUG_ENABLED
