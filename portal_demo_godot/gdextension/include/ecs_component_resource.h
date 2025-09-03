#pragma once

#include <godot_cpp/classes/resource.hpp>
#include <godot_cpp/classes/ref.hpp>
#include <godot_cpp/classes/node.hpp>
#include <godot_cpp/variant/utility_functions.hpp>
#include "entt/entt.hpp"

// 前向聲明ECS世界相關類型
namespace portal_core {
    class PortalGameWorld;
}

using namespace godot;

/**
 * ECS組件資源抽象基類
 * 定義所有組件資源都必須遵守的"契約"
 * 這是多態系統的核心，讓ECSNode能夠統一處理所有類型的組件
 */
class ECSComponentResource : public Resource
{
    GDCLASS(ECSComponentResource, Resource)

protected:
    static void _bind_methods();

public:
    ECSComponentResource();
    virtual ~ECSComponentResource();

    /**
     * 純虛函數：將此資源應用到指定的ECS實體
     * 每個具體的組件資源類型必須實現這個方法
     * @param registry ECS註冊表的引用
     * @param entity 要應用組件的實體ID
     * @return true 如果成功應用，false 否則
     */
    virtual bool apply_to_entity(entt::registry& registry, entt::entity entity) = 0;

    /**
     * 純虛函數：從指定的ECS實體移除此類型的組件
     * @param registry ECS註冊表的引用
     * @param entity 要移除組件的實體ID
     * @return true 如果成功移除，false 否則
     */
    virtual bool remove_from_entity(entt::registry& registry, entt::entity entity) = 0;

    /**
     * 🌟 新增的核心虛函數：從ECS同步數據到Godot節點
     * 這是讓框架支持任意自定義邏輯的關鍵！每個組件資源類型自己決定如何同步數據
     * @param registry ECS註冊表的引用
     * @param entity 要同步數據的實體ID
     * @param target_node 要同步到的目標Godot節點
     */
    virtual void sync_to_node(entt::registry& registry, entt::entity entity, Node* target_node) = 0;

    /**
     * 虛函數：獲取組件的類型名稱（用於調試和日誌）
     * 子類可以重寫這個方法來提供更具體的名稱
     * @return 組件類型的字符串表示
     */
    virtual String get_component_type_name() const;

    /**
     * 虛函數：檢查實體是否已經擁有此類型的組件
     * 子類可以重寫這個方法來實現特定的檢查邏輯
     * @param registry ECS註冊表的引用
     * @param entity 要檢查的實體ID
     * @return true 如果實體擁有此類型的組件，false 否則
     */
    virtual bool has_component(const entt::registry& registry, entt::entity entity) const = 0;
};
