#include "portal_game_world.h"
#include "systems/x_rotation_system.h"
#include "systems/y_rotation_system.h"
#include "systems/z_rotation_system.h"

namespace portal_core
{

  std::unique_ptr<PortalGameWorld> PortalGameWorld::instance_ = nullptr;

  PortalGameWorld *PortalGameWorld::get_instance()
  {
    return instance_.get();
  }

  void PortalGameWorld::create_instance()
  {
    if (!instance_)
    {
      instance_ = std::make_unique<PortalGameWorld>();
    }
  }

  void PortalGameWorld::destroy_instance()
  {
    instance_.reset();
  }

  entt::entity PortalGameWorld::create_entity()
  {
    return registry_.create();
  }

  void PortalGameWorld::destroy_entity(entt::entity entity)
  {
    // 清理映射
    auto godot_it = entt_to_godot_.find(entity);
    if (godot_it != entt_to_godot_.end())
    {
      godot_to_entt_.erase(godot_it->second);
      entt_to_godot_.erase(godot_it);
    }

    registry_.destroy(entity);
  }

  void PortalGameWorld::bind_godot_node(uint64_t godot_id, entt::entity entt_entity)
  {
    // 清理舊的映射
    auto old_entt_it = godot_to_entt_.find(godot_id);
    if (old_entt_it != godot_to_entt_.end())
    {
      entt_to_godot_.erase(old_entt_it->second);
    }

    auto old_godot_it = entt_to_godot_.find(entt_entity);
    if (old_godot_it != entt_to_godot_.end())
    {
      godot_to_entt_.erase(old_godot_it->second);
    }

    // 建立新映射
    godot_to_entt_[godot_id] = entt_entity;
    entt_to_godot_[entt_entity] = godot_id;
  }

  void PortalGameWorld::unbind_godot_node(uint64_t godot_id)
  {
    auto entt_it = godot_to_entt_.find(godot_id);
    if (entt_it != godot_to_entt_.end())
    {
      entt_to_godot_.erase(entt_it->second);
      godot_to_entt_.erase(entt_it);
    }
  }

  entt::entity PortalGameWorld::get_entt_entity(uint64_t godot_id) const
  {
    auto it = godot_to_entt_.find(godot_id);
    if (it != godot_to_entt_.end())
    {
      return it->second;
    }
    return entt::null;
  }

  uint64_t PortalGameWorld::get_godot_id(entt::entity entity) const
  {
    auto it = entt_to_godot_.find(entity);
    if (it != entt_to_godot_.end())
    {
      return it->second;
    }
    return 0; // 無效的 Godot ID
  }

  void PortalGameWorld::update_systems(float delta_time)
  {
    // 新的架構：使用分離的旋轉系統

    // 更新所有軸的旋轉系統
    portal_core::XRotationSystem::update(registry_, delta_time);
    portal_core::YRotationSystem::update(registry_, delta_time);
    portal_core::ZRotationSystem::update(registry_, delta_time);

    // 這裡可以添加更多系統的更新
    // PhysicsSystem::update(registry_, delta_time);
    // CollisionSystem::update(registry_, delta_time);
    // etc.
  }

} // namespace portal_core
