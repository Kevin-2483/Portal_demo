#include "ecs_entity_link_manager.h"
#include <godot_cpp/core/class_db.hpp>
#include <godot_cpp/classes/time.hpp>
#include <godot_cpp/variant/utility_functions.hpp>
#include "core/portal_game_world.h"

using namespace godot;

// 静态成员初始化
ECSEntityLinkManager* ECSEntityLinkManager::instance_ = nullptr;

void ECSEntityLinkManager::_bind_methods() {
    // 注意：LinkStatus枚举不绑定到Godot，因为自定义枚举类型不被支持
    
    // 绑定核心方法
    ClassDB::bind_method(D_METHOD("create_link", "entity_id", "godot_node", "template_name"), &ECSEntityLinkManager::create_link);
    ClassDB::bind_method(D_METHOD("remove_link_by_entity", "entity_id", "destroy_counterpart"), &ECSEntityLinkManager::remove_link_by_entity, DEFVAL(false));
    ClassDB::bind_method(D_METHOD("remove_link_by_node", "godot_node", "destroy_counterpart"), &ECSEntityLinkManager::remove_link_by_node, DEFVAL(false));
    
    // 绑定查询方法
    ClassDB::bind_method(D_METHOD("get_node_by_entity", "entity_id"), &ECSEntityLinkManager::get_node_by_entity);
    ClassDB::bind_method(D_METHOD("get_entity_by_node", "godot_node"), &ECSEntityLinkManager::get_entity_by_node);
    ClassDB::bind_method(D_METHOD("is_link_active", "entity_id"), &ECSEntityLinkManager::is_link_active);
    ClassDB::bind_method(D_METHOD("is_node_linked", "godot_node"), &ECSEntityLinkManager::is_node_linked);
    
    // 绑定模板相关查询方法
    ClassDB::bind_method(D_METHOD("get_entities_by_template", "template_name"), &ECSEntityLinkManager::get_entities_by_template);
    ClassDB::bind_method(D_METHOD("get_template_by_entity", "entity_id"), &ECSEntityLinkManager::get_template_by_entity);
    
    // 绑定生命周期管理方法
    ClassDB::bind_method(D_METHOD("on_ecs_entity_destroyed", "entity_id"), &ECSEntityLinkManager::on_ecs_entity_destroyed);
    ClassDB::bind_method(D_METHOD("on_godot_node_destroyed", "godot_node"), &ECSEntityLinkManager::on_godot_node_destroyed);
    
    // 绑定批量操作方法
    ClassDB::bind_method(D_METHOD("clear_all_links", "destroy_entities", "destroy_nodes"), &ECSEntityLinkManager::clear_all_links, DEFVAL(false), DEFVAL(false));
    ClassDB::bind_method(D_METHOD("cleanup_invalid_links"), &ECSEntityLinkManager::cleanup_invalid_links);
    ClassDB::bind_method(D_METHOD("process_pending_destroys"), &ECSEntityLinkManager::process_pending_destroys);
    
    // 绑定统计和调试方法
    ClassDB::bind_method(D_METHOD("get_active_link_count"), &ECSEntityLinkManager::get_active_link_count);
    ClassDB::bind_method(D_METHOD("get_statistics"), &ECSEntityLinkManager::get_statistics);
    ClassDB::bind_method(D_METHOD("debug_print_all_links"), &ECSEntityLinkManager::debug_print_all_links);
    ClassDB::bind_method(D_METHOD("validate_link_integrity"), &ECSEntityLinkManager::validate_link_integrity);
    
    // 绑定单例访问方法
    ClassDB::bind_static_method("ECSEntityLinkManager", D_METHOD("get_instance"), &ECSEntityLinkManager::get_instance);
    ClassDB::bind_static_method("ECSEntityLinkManager", D_METHOD("destroy_instance"), &ECSEntityLinkManager::destroy_instance);
}

ECSEntityLinkManager::ECSEntityLinkManager() 
    : total_links_created_(0), total_links_destroyed_(0), last_cleanup_time_(0.0) {
    UtilityFunctions::print("[ECSEntityLinkManager] Instance created");
}

ECSEntityLinkManager::~ECSEntityLinkManager() {
    clear_all_links(false, false);
    UtilityFunctions::print("[ECSEntityLinkManager] Instance destroyed");
}

ECSEntityLinkManager* ECSEntityLinkManager::get_instance() {
    if (!instance_) {
        instance_ = memnew(ECSEntityLinkManager);
    }
    return instance_;
}

void ECSEntityLinkManager::destroy_instance() {
    if (instance_) {
        memdelete(instance_);
        instance_ = nullptr;
    }
}

// === 核心链接管理方法实现 ===

bool ECSEntityLinkManager::create_link(uint32_t entity_id, Node* godot_node, const String& template_name) {
    if (entity_id == 0 || !godot_node || !is_node_valid(godot_node)) {
        UtilityFunctions::print("[ECSEntityLinkManager] Invalid parameters for create_link");
        return false;
    }
    
    // 检查是否已存在链接
    if (entity_to_link_.find(entity_id) != entity_to_link_.end()) {
        UtilityFunctions::print("[ECSEntityLinkManager] Entity ", entity_id, " already linked");
        return false;
    }
    
    if (node_to_entity_.find(godot_node) != node_to_entity_.end()) {
        UtilityFunctions::print("[ECSEntityLinkManager] Node already linked to another entity");
        return false;
    }
    
    // 创建链接信息
    LinkInfo link_info(entity_id, godot_node, template_name);
    
    // 建立双向映射
    entity_to_link_[entity_id] = link_info;
    node_to_entity_[godot_node] = entity_id;
    
    total_links_created_++;
    
    UtilityFunctions::print("[ECSEntityLinkManager] Created link: Entity ", entity_id, " <-> Node ", godot_node->get_name());
    return true;
}

bool ECSEntityLinkManager::remove_link_by_entity(uint32_t entity_id, bool destroy_counterpart) {
    auto it = entity_to_link_.find(entity_id);
    if (it == entity_to_link_.end()) {
        return false;
    }
    
    LinkInfo& link_info = it->second;
    Node* godot_node = link_info.godot_node;
    
    // 更新状态为正在销毁
    link_info.status = LINK_DESTROYING;
    
    // 移除双向映射
    if (godot_node) {
        node_to_entity_.erase(godot_node);
        
        // 如果需要销毁对应的Godot节点
        if (destroy_counterpart && is_node_valid(godot_node)) {
            pending_node_destroys_.insert(godot_node);
        }
    }
    
    entity_to_link_.erase(it);
    total_links_destroyed_++;
    
    UtilityFunctions::print("[ECSEntityLinkManager] Removed link for entity ", entity_id);
    return true;
}

bool ECSEntityLinkManager::remove_link_by_node(Node* godot_node, bool destroy_counterpart) {
    if (!godot_node) {
        return false;
    }
    
    auto it = node_to_entity_.find(godot_node);
    if (it == node_to_entity_.end()) {
        return false;
    }
    
    uint32_t entity_id = it->second;
    
    // 如果需要销毁对应的ECS实体
    if (destroy_counterpart) {
        pending_entity_destroys_.insert(entity_id);
    }
    
    return remove_link_by_entity(entity_id, false);
}

// === 查询方法实现 ===

Node* ECSEntityLinkManager::get_node_by_entity(uint32_t entity_id) const {
    auto it = entity_to_link_.find(entity_id);
    if (it != entity_to_link_.end() && it->second.status == LINK_ACTIVE) {
        return it->second.godot_node;
    }
    return nullptr;
}

uint32_t ECSEntityLinkManager::get_entity_by_node(Node* godot_node) const {
    if (!godot_node) {
        return 0;
    }
    
    auto it = node_to_entity_.find(godot_node);
    if (it != node_to_entity_.end()) {
        // 验证链接状态
        auto link_it = entity_to_link_.find(it->second);
        if (link_it != entity_to_link_.end() && link_it->second.status == LINK_ACTIVE) {
            return it->second;
        }
    }
    return 0;
}

bool ECSEntityLinkManager::is_link_active(uint32_t entity_id) const {
    auto it = entity_to_link_.find(entity_id);
    return it != entity_to_link_.end() && it->second.status == LINK_ACTIVE;
}

bool ECSEntityLinkManager::is_node_linked(Node* godot_node) const {
    return get_entity_by_node(godot_node) != 0;
}

ECSEntityLinkManager::LinkInfo ECSEntityLinkManager::get_link_info(uint32_t entity_id) const {
    auto it = entity_to_link_.find(entity_id);
    if (it != entity_to_link_.end()) {
        return it->second;
    }
    return LinkInfo();
}

// === 生命周期管理实现 ===

void ECSEntityLinkManager::on_ecs_entity_destroyed(uint32_t entity_id) {
    UtilityFunctions::print("[ECSEntityLinkManager] ECS entity ", entity_id, " destroyed");
    
    // 触发实体销毁回调
    Node* linked_node = get_node_by_entity(entity_id);
    for (const auto& callback : entity_destroy_callbacks_) {
        callback(entity_id, linked_node);
    }
    
    // 移除链接并销毁对应的Godot节点
    remove_link_by_entity(entity_id, true);
}

void ECSEntityLinkManager::on_godot_node_destroyed(Node* godot_node) {
    if (!godot_node) {
        return;
    }
    
    uint32_t entity_id = get_entity_by_node(godot_node);
    UtilityFunctions::print("[ECSEntityLinkManager] Godot node destroyed, linked entity: ", entity_id);
    
    // 触发节点销毁回调
    for (const auto& callback : node_destroy_callbacks_) {
        callback(entity_id, godot_node);
    }
    
    // 移除链接并销毁对应的ECS实体
    remove_link_by_node(godot_node, true);
}

void ECSEntityLinkManager::register_entity_destroy_callback(const DestroyCallback& callback) {
    entity_destroy_callbacks_.push_back(callback);
}

void ECSEntityLinkManager::register_node_destroy_callback(const DestroyCallback& callback) {
    node_destroy_callbacks_.push_back(callback);
}

// === 批量操作实现 ===

void ECSEntityLinkManager::clear_all_links(bool destroy_entities, bool destroy_nodes) {
    UtilityFunctions::print("[ECSEntityLinkManager] Clearing all links...");
    
    if (destroy_entities || destroy_nodes) {
        for (const auto& pair : entity_to_link_) {
            const LinkInfo& link_info = pair.second;
            
            if (destroy_entities) {
                pending_entity_destroys_.insert(link_info.entity_id);
            }
            
            if (destroy_nodes && link_info.godot_node && is_node_valid(link_info.godot_node)) {
                pending_node_destroys_.insert(link_info.godot_node);
            }
        }
    }
    
    entity_to_link_.clear();
    node_to_entity_.clear();
    
    UtilityFunctions::print("[ECSEntityLinkManager] All links cleared");
}

int ECSEntityLinkManager::cleanup_invalid_links() {
    int cleaned_count = 0;
    std::vector<uint32_t> invalid_entities;
    
    // 检查无效的节点链接
    for (const auto& pair : entity_to_link_) {
        const LinkInfo& link_info = pair.second;
        if (link_info.godot_node && !is_node_valid(link_info.godot_node)) {
            invalid_entities.push_back(pair.first);
        }
    }
    
    // 移除无效链接
    for (uint32_t entity_id : invalid_entities) {
        remove_link_by_entity(entity_id, false);
        cleaned_count++;
    }
    
    if (cleaned_count > 0) {
        UtilityFunctions::print("[ECSEntityLinkManager] Cleaned ", cleaned_count, " invalid links");
    }
    
    last_cleanup_time_ = Time::get_singleton()->get_time_dict_from_system()["unix"];
    return cleaned_count;
}

void ECSEntityLinkManager::process_pending_destroys() {
    // 处理待销毁的ECS实体
    if (!pending_entity_destroys_.empty()) {
        auto* game_world = portal_core::PortalGameWorld::get_instance();
        if (game_world) {
            auto& registry = game_world->get_registry();
            for (uint32_t entity_id : pending_entity_destroys_) {
                if (registry.valid(entt::entity(entity_id))) {
                    registry.destroy(entt::entity(entity_id));
                    UtilityFunctions::print("[ECSEntityLinkManager] Destroyed ECS entity ", entity_id);
                }
            }
        }
        pending_entity_destroys_.clear();
    }
    
    // 处理待销毁的Godot节点
    if (!pending_node_destroys_.empty()) {
        for (Node* node : pending_node_destroys_) {
            if (is_node_valid(node)) {
                node->queue_free();
                UtilityFunctions::print("[ECSEntityLinkManager] Queued Godot node for destruction: ", node->get_name());
            }
        }
        pending_node_destroys_.clear();
    }
}

// === 调试和统计实现 ===

int ECSEntityLinkManager::get_active_link_count() const {
    int active_count = 0;
    for (const auto& pair : entity_to_link_) {
        if (pair.second.status == LINK_ACTIVE) {
            active_count++;
        }
    }
    return active_count;
}

Dictionary ECSEntityLinkManager::get_statistics() const {
    Dictionary stats;
    stats["total_links_created"] = total_links_created_;
    stats["total_links_destroyed"] = total_links_destroyed_;
    stats["active_links"] = get_active_link_count();
    stats["pending_entity_destroys"] = (int)pending_entity_destroys_.size();
    stats["pending_node_destroys"] = (int)pending_node_destroys_.size();
    stats["last_cleanup_time"] = last_cleanup_time_;
    return stats;
}

void ECSEntityLinkManager::debug_print_all_links() const {
    UtilityFunctions::print("[ECSEntityLinkManager] === All Links Debug Info ===");
    UtilityFunctions::print("Total links: ", entity_to_link_.size());
    
    for (const auto& pair : entity_to_link_) {
        const LinkInfo& link_info = pair.second;
        String status_str;
        switch (link_info.status) {
            case LINK_ACTIVE: status_str = "ACTIVE"; break;
            case LINK_PENDING: status_str = "PENDING"; break;
            case LINK_BROKEN: status_str = "BROKEN"; break;
            case LINK_DESTROYING: status_str = "DESTROYING"; break;
        }
        
        String node_name = link_info.godot_node ? link_info.godot_node->get_name() : "NULL";
        UtilityFunctions::print("  Entity ", link_info.entity_id, " <-> Node '", node_name, "' (", link_info.template_name, ") - ", status_str);
    }
    UtilityFunctions::print("=== End Debug Info ===");
}

bool ECSEntityLinkManager::validate_link_integrity() const {
    bool is_valid = true;
    
    // 验证双向映射的一致性
    for (const auto& pair : entity_to_link_) {
        uint32_t entity_id = pair.first;
        const LinkInfo& link_info = pair.second;
        
        if (link_info.godot_node) {
            auto node_it = node_to_entity_.find(link_info.godot_node);
            if (node_it == node_to_entity_.end() || node_it->second != entity_id) {
                UtilityFunctions::print("[ECSEntityLinkManager] Integrity error: Entity ", entity_id, " mapping inconsistent");
                is_valid = false;
            }
        }
    }
    
    for (const auto& pair : node_to_entity_) {
        Node* node = pair.first;
        uint32_t entity_id = pair.second;
        
        auto entity_it = entity_to_link_.find(entity_id);
        if (entity_it == entity_to_link_.end() || entity_it->second.godot_node != node) {
            UtilityFunctions::print("[ECSEntityLinkManager] Integrity error: Node mapping inconsistent");
            is_valid = false;
        }
    }
    
    return is_valid;
}

// === 内部辅助方法实现 ===

void ECSEntityLinkManager::internal_destroy_entity(uint32_t entity_id) {
    auto* game_world = portal_core::PortalGameWorld::get_instance();
    if (game_world) {
        auto& registry = game_world->get_registry();
        if (registry.valid(entt::entity(entity_id))) {
            registry.destroy(entt::entity(entity_id));
        }
    }
}

void ECSEntityLinkManager::internal_destroy_node(Node* godot_node) {
    if (is_node_valid(godot_node)) {
        godot_node->queue_free();
    }
}

bool ECSEntityLinkManager::is_node_valid(Node* node) const {
    return node && ObjectDB::get_instance(node->get_instance_id()) != nullptr;
}

void ECSEntityLinkManager::update_link_status(uint32_t entity_id, LinkStatus status) {
    auto it = entity_to_link_.find(entity_id);
    if (it != entity_to_link_.end()) {
        it->second.status = status;
    }
}

// === 模板相关查询方法实现 ===
Array ECSEntityLinkManager::get_entities_by_template(const String& template_name) const {
    Array result;
    
    for (const auto& pair : entity_to_link_) {
        const LinkInfo& link_info = pair.second;
        if (link_info.status == LINK_ACTIVE && link_info.template_name == template_name) {
            result.push_back(link_info.entity_id);
        }
    }
    
    return result;
}

String ECSEntityLinkManager::get_template_by_entity(uint32_t entity_id) const {
    auto it = entity_to_link_.find(entity_id);
    if (it != entity_to_link_.end()) {
        return it->second.template_name;
    }
    return String(); // 返回空字符串表示未找到
}