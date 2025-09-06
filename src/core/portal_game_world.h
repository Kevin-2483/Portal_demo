#ifndef PORTAL_GAME_WORLD_H
#define PORTAL_GAME_WORLD_H

#include <entt/entt.hpp>
#include <memory>
#include <unordered_map>
#include "system_manager.h"

namespace portal_core
{

  class PortalGameWorld
  {
  private:
    entt::registry registry_;
    SystemManager system_manager_;
    static std::unique_ptr<PortalGameWorld> instance_;

    // 映射：Godot 節點ID <-> Entt 實體ID
    std::unordered_map<uint64_t, entt::entity> godot_to_entt_;
    std::unordered_map<entt::entity, uint64_t> entt_to_godot_;

  public:
    PortalGameWorld() = default;
    ~PortalGameWorld() = default;

    // 單例模式
    static PortalGameWorld *get_instance();
    static void create_instance();
    static void destroy_instance();

    // 註冊表訪問
    entt::registry &get_registry() { return registry_; }
    const entt::registry &get_registry() const { return registry_; }

    // 實體管理
    entt::entity create_entity();
    void destroy_entity(entt::entity entity);

    // Godot <-> Entt 映射管理
    void bind_godot_node(uint64_t godot_id, entt::entity entt_entity);
    void unbind_godot_node(uint64_t godot_id);

    entt::entity get_entt_entity(uint64_t godot_id) const;
    uint64_t get_godot_id(entt::entity entity) const;

    // 系統更新 - 使用 SystemManager 和 entt::organizer
    void update_systems(float delta_time);

    // 系統管理器訪問
    SystemManager &get_system_manager() { return system_manager_; }
    const SystemManager &get_system_manager() const { return system_manager_; }

  private:
  };

} // namespace portal_core

#endif // PORTAL_GAME_WORLD_H
