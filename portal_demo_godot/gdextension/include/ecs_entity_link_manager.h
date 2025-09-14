#ifndef ECS_ENTITY_LINK_MANAGER_H
#define ECS_ENTITY_LINK_MANAGER_H

#include <godot_cpp/classes/node.hpp>
#include <godot_cpp/classes/ref_counted.hpp>
#include <godot_cpp/classes/time.hpp>
#include <godot_cpp/variant/utility_functions.hpp>
#include <unordered_map>
#include <unordered_set>
#include <functional>

// 前向声明
namespace portal_core {
    class PortalGameWorld;
}

namespace godot {

/**
 * ECS实体与Godot节点的双向链接管理器
 * 负责维护实体和节点之间的映射关系，处理生命周期同步
 */
class ECSEntityLinkManager : public RefCounted {
    GDCLASS(ECSEntityLinkManager, RefCounted)

public:
    // 链接状态枚举
    enum LinkStatus {
        LINK_ACTIVE,        // 链接活跃
        LINK_PENDING,       // 等待链接
        LINK_BROKEN,        // 链接断开
        LINK_DESTROYING     // 正在销毁
    };

    // 链接信息结构
    struct LinkInfo {
        uint32_t entity_id;
        Node* godot_node;
        LinkStatus status;
        String template_name;
        double creation_time;
        
        LinkInfo() : entity_id(0), godot_node(nullptr), status(LINK_PENDING), creation_time(0.0) {}
        LinkInfo(uint32_t eid, Node* node, const String& tmpl_name) 
            : entity_id(eid), godot_node(node), status(LINK_ACTIVE), template_name(tmpl_name) {
            creation_time = Time::get_singleton()->get_time_dict_from_system()["unix"];
        }
    };

    // 销毁回调函数类型
    using DestroyCallback = std::function<void(uint32_t entity_id, Node* node)>;

private:
    // 双向映射表
    std::unordered_map<uint32_t, LinkInfo> entity_to_link_;     // ECS实体ID -> 链接信息
    std::unordered_map<Node*, uint32_t> node_to_entity_;        // Godot节点 -> ECS实体ID
    
    // 待处理的链接操作队列
    std::unordered_set<uint32_t> pending_entity_destroys_;      // 待销毁的实体
    std::unordered_set<Node*> pending_node_destroys_;           // 待销毁的节点
    
    // 回调函数
    std::vector<DestroyCallback> entity_destroy_callbacks_;
    std::vector<DestroyCallback> node_destroy_callbacks_;
    
    // 单例实例
    static ECSEntityLinkManager* instance_;
    
    // 统计信息
    int total_links_created_;
    int total_links_destroyed_;
    double last_cleanup_time_;

protected:
    static void _bind_methods();

public:
    ECSEntityLinkManager();
    ~ECSEntityLinkManager();
    
    // 单例访问
    static ECSEntityLinkManager* get_instance();
    static void destroy_instance();
    
    // === 核心链接管理方法 ===
    
    /**
     * 创建ECS实体与Godot节点的双向链接
     * @param entity_id ECS实体ID
     * @param godot_node Godot节点指针
     * @param template_name 模板名称
     * @return 是否成功创建链接
     */
    bool create_link(uint32_t entity_id, Node* godot_node, const String& template_name);
    
    /**
     * 移除双向链接
     * @param entity_id ECS实体ID
     * @param destroy_counterpart 是否同时销毁对应的节点/实体
     * @return 是否成功移除链接
     */
    bool remove_link_by_entity(uint32_t entity_id, bool destroy_counterpart = false);
    
    /**
     * 通过Godot节点移除链接
     * @param godot_node Godot节点指针
     * @param destroy_counterpart 是否同时销毁对应的实体
     * @return 是否成功移除链接
     */
    bool remove_link_by_node(Node* godot_node, bool destroy_counterpart = false);
    
    // === 查询方法 ===
    
    /**
     * 根据ECS实体ID获取对应的Godot节点
     */
    Node* get_node_by_entity(uint32_t entity_id) const;
    
    /**
     * 根据Godot节点获取对应的ECS实体ID
     */
    uint32_t get_entity_by_node(Node* godot_node) const;
    
    /**
     * 检查链接是否存在且活跃
     */
    bool is_link_active(uint32_t entity_id) const;
    bool is_node_linked(Node* godot_node) const;
    
    /**
     * 获取链接信息
     */
    LinkInfo get_link_info(uint32_t entity_id) const;
    
    /**
     * 根据模板名称获取所有相关实体
     */
    Array get_entities_by_template(const String& template_name) const;
    
    /**
     * 获取实体的模板名称
     */
    String get_template_by_entity(uint32_t entity_id) const;
    
    // === 生命周期管理 ===
    
    /**
     * 处理ECS实体销毁事件
     * 当ECS系统销毁实体时调用，自动销毁对应的Godot节点
     */
    void on_ecs_entity_destroyed(uint32_t entity_id);
    
    /**
     * 处理Godot节点销毁事件
     * 当Godot节点被销毁时调用，自动销毁对应的ECS实体
     */
    void on_godot_node_destroyed(Node* godot_node);
    
    /**
     * 注册销毁回调函数
     */
    void register_entity_destroy_callback(const DestroyCallback& callback);
    void register_node_destroy_callback(const DestroyCallback& callback);
    
    // === 批量操作 ===
    
    /**
     * 清理所有链接
     */
    void clear_all_links(bool destroy_entities = false, bool destroy_nodes = false);
    
    /**
     * 清理无效链接（节点已被销毁但链接仍存在）
     */
    int cleanup_invalid_links();
    
    /**
     * 处理待销毁队列
     */
    void process_pending_destroys();
    
    // === 调试和统计 ===
    
    /**
     * 获取当前活跃链接数量
     */
    int get_active_link_count() const;
    
    /**
     * 获取统计信息
     */
    Dictionary get_statistics() const;
    
    /**
     * 打印所有链接信息（调试用）
     */
    void debug_print_all_links() const;
    
    /**
     * 验证链接完整性
     */
    bool validate_link_integrity() const;

private:
    // 内部辅助方法
    void internal_destroy_entity(uint32_t entity_id);
    void internal_destroy_node(Node* godot_node);
    bool is_node_valid(Node* node) const;
    void update_link_status(uint32_t entity_id, LinkStatus status);
};

} // namespace godot

#endif // ECS_ENTITY_LINK_MANAGER_H