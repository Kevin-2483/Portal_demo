#include "portal_game_world.h"
#include "systems/x_rotation_system.h"
#include "systems/y_rotation_system.h"
#include "systems/z_rotation_system.h"
#include "systems/physics_system.h"
#include "systems/physics_command_system.h"
#include <iostream>

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
      
      // 註冊物理系統
      register_physics_systems();
      
      // 初始化系統管理器
      instance_->system_manager_.initialize();
      std::cout << "PortalGameWorld: Instance created and systems initialized." << std::endl;
    }
  }

  void PortalGameWorld::destroy_instance()
  {
    instance_.reset();
  }

  void PortalGameWorld::register_physics_systems()
  {
    // 註冊物理命令系統（在物理系統之前執行）
    SystemRegistry::register_system(
      "PhysicsCommandSystem",
      create_physics_command_system,
      {},  // 沒有依賴
      {},  // 沒有衝突
      10   // 較高優先級，早執行
    );
    
    // 註冊物理系統
    SystemRegistry::register_system(
      "PhysicsSystem", 
      create_physics_system,
      {"PhysicsCommandSystem"},  // 依賴物理命令系統
      {},                        // 沒有衝突
      20                         // 中等優先級
    );
    
    // 註冊物理查詢系統（在物理系統之後執行）
    SystemRegistry::register_system(
      "PhysicsQuerySystem",
      create_physics_query_system,
      {"PhysicsSystem"},  // 依賴物理系統
      {},                 // 沒有衝突
      30                  // 較低優先級，晚執行
    );
    
    std::cout << "PortalGameWorld: Physics systems registered." << std::endl;
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
    // 使用 SystemManager 和 entt::organizer 來管理系統執行
    system_manager_.update_systems(registry_, delta_time);
  }

} // namespace portal_core
