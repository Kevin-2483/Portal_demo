#include "physics_command_system.h"
#include "../components/physics_body_component.h"
#include "../components/transform_component.h"
#include "../components/physics_command_component.h"
#include <iostream>
#include <chrono>
#include <algorithm>

namespace portal_core
{

  // PhysicsCommandSystem 實現

  bool PhysicsCommandSystem::initialize()
  {
    std::cout << "PhysicsCommandSystem: Initializing..." << std::endl;

    // 獲取物理世界管理器
    physics_world_ = &PhysicsWorldManager::get_instance();

    // 重置統計數據
    stats_ = CommandSystemStats{};
    commands_executed_this_frame_ = 0;

    initialized_ = true;
    std::cout << "PhysicsCommandSystem: Initialization complete." << std::endl;
    return true;
  }

  void PhysicsCommandSystem::update(entt::registry &registry, float delta_time)
  {
    if (!initialized_ || !enabled_)
    {
      return;
    }

    auto start_time = std::chrono::high_resolution_clock::now();

    // 重置每幀統計
    commands_executed_this_frame_ = 0;
    stats_.commands_executed_this_frame = 0;
    entities_processed_this_frame_.clear();

    // 更新延遲命令計時
    delta_time_accumulator_ += delta_time;

    // 執行不同時機的命令
    execute_immediate_commands(registry);
    execute_delayed_commands(registry, delta_time);
    execute_before_physics_commands(registry);
    execute_recurring_commands(registry);

    // 清理已執行的命令
    cleanup_executed_commands(registry);

    auto end_time = std::chrono::high_resolution_clock::now();
    stats_.execution_time = std::chrono::duration<float>(end_time - start_time).count();

    // 更新統計數據
    auto command_view = registry.view<PhysicsCommandComponent>();
    stats_.entities_with_commands = 0;
    for (auto entity : command_view)
    {
      auto &cmd_comp = command_view.get<PhysicsCommandComponent>(entity);
      if (cmd_comp.has_pending_commands())
      {
        stats_.entities_with_commands++;
      }
    }
  }

  void PhysicsCommandSystem::cleanup()
  {
    std::cout << "PhysicsCommandSystem: Cleaning up..." << std::endl;

    entities_processed_this_frame_.clear();
    physics_world_ = nullptr;
    initialized_ = false;

    std::cout << "PhysicsCommandSystem: Cleanup complete." << std::endl;
  }

  void PhysicsCommandSystem::execute_immediate_commands(entt::registry &registry)
  {
    auto view = registry.view<PhysicsCommandComponent>();

    view.each([&](auto entity, auto &cmd_comp)
              {
        if (commands_executed_this_frame_ >= max_commands_per_frame_) {
            return;
        }

        if (!cmd_comp.enabled || cmd_comp.immediate_commands.empty()) {
            return;
        }

        // 執行立即命令
        auto& commands = cmd_comp.immediate_commands;
        for (auto it = commands.begin(); it != commands.end() && commands_executed_this_frame_ < max_commands_per_frame_;) {
            const PhysicsCommand& command = *it;
            
            if (execute_command(command, entity, registry)) {
                stats_.total_commands_executed++;
                stats_.commands_executed_this_frame++;
                commands_executed_this_frame_++;
            } else {
                stats_.commands_failed++;
            }

            if (command.auto_remove) {
                it = commands.erase(it);
            } else {
                ++it;
            }
        }

        entities_processed_this_frame_.insert(entity); });
  }

  void PhysicsCommandSystem::execute_before_physics_commands(entt::registry &registry)
  {
    auto view = registry.view<PhysicsCommandComponent>();

    view.each([&](auto entity, auto &cmd_comp)
              {
        if (commands_executed_this_frame_ >= max_commands_per_frame_) {
            return;
        }

        if (!cmd_comp.enabled || cmd_comp.before_physics_commands.empty()) {
            return;
        }

        // 執行物理步進前命令
        auto& commands = cmd_comp.before_physics_commands;
        for (auto it = commands.begin(); it != commands.end() && commands_executed_this_frame_ < max_commands_per_frame_;) {
            const PhysicsCommand& command = *it;
            
            if (execute_command(command, entity, registry)) {
                stats_.total_commands_executed++;
                stats_.commands_executed_this_frame++;
                commands_executed_this_frame_++;
            } else {
                stats_.commands_failed++;
            }

            if (command.auto_remove) {
                it = commands.erase(it);
            } else {
                ++it;
            }
        }

        entities_processed_this_frame_.insert(entity); });
  }

  void PhysicsCommandSystem::execute_after_physics_commands(entt::registry &registry)
  {
    auto view = registry.view<PhysicsCommandComponent>();

    view.each([&](auto entity, auto &cmd_comp)
              {
        if (commands_executed_this_frame_ >= max_commands_per_frame_) {
            return;
        }

        if (!cmd_comp.enabled || cmd_comp.after_physics_commands.empty()) {
            return;
        }

        // 執行物理步進後命令
        auto& commands = cmd_comp.after_physics_commands;
        for (auto it = commands.begin(); it != commands.end() && commands_executed_this_frame_ < max_commands_per_frame_;) {
            const PhysicsCommand& command = *it;
            
            if (execute_command(command, entity, registry)) {
                stats_.total_commands_executed++;
                stats_.commands_executed_this_frame++;
                commands_executed_this_frame_++;
            } else {
                stats_.commands_failed++;
            }

            if (command.auto_remove) {
                it = commands.erase(it);
            } else {
                ++it;
            }
        }

        entities_processed_this_frame_.insert(entity); });
  }

  void PhysicsCommandSystem::execute_delayed_commands(entt::registry &registry, float delta_time)
  {
    auto view = registry.view<PhysicsCommandComponent>();

    view.each([&](auto entity, auto &cmd_comp)
              {
        if (commands_executed_this_frame_ >= max_commands_per_frame_) {
            return;
        }

        if (!cmd_comp.enabled || cmd_comp.delayed_commands.empty()) {
            return;
        }

        // 更新延遲命令計時
        cmd_comp.update_delayed_commands(delta_time);

        // 獲取準備執行的延遲命令
        auto ready_commands = cmd_comp.get_ready_delayed_commands();
        
        for (const auto& command : ready_commands) {
            if (commands_executed_this_frame_ >= max_commands_per_frame_) {
                break;
            }

            if (execute_command(command, entity, registry)) {
                stats_.total_commands_executed++;
                stats_.commands_executed_this_frame++;
                commands_executed_this_frame_++;
            } else {
                stats_.commands_failed++;
            }
        }

        entities_processed_this_frame_.insert(entity); });
  }

  void PhysicsCommandSystem::execute_recurring_commands(entt::registry &registry)
  {
    auto view = registry.view<PhysicsCommandComponent>();

    view.each([&](auto entity, auto &cmd_comp)
              {
        if (commands_executed_this_frame_ >= max_commands_per_frame_) {
            return;
        }

        if (!cmd_comp.enabled || cmd_comp.recurring_commands.empty()) {
            return;
        }

        // 執行重複命令
        for (const auto& command : cmd_comp.recurring_commands) {
            if (commands_executed_this_frame_ >= max_commands_per_frame_) {
                break;
            }

            if (execute_command(command, entity, registry)) {
                stats_.total_commands_executed++;
                stats_.commands_executed_this_frame++;
                commands_executed_this_frame_++;
            } else {
                stats_.commands_failed++;
            }
        }

        entities_processed_this_frame_.insert(entity); });
  }

  bool PhysicsCommandSystem::execute_command(const PhysicsCommand &command, entt::entity entity, entt::registry &registry)
  {
    if (!validate_command(command, entity, registry))
    {
      return false;
    }

    switch (command.type)
    {
    case PhysicsCommandType::ADD_FORCE:
    case PhysicsCommandType::ADD_IMPULSE:
    case PhysicsCommandType::ADD_TORQUE:
    case PhysicsCommandType::ADD_ANGULAR_IMPULSE:
    case PhysicsCommandType::ADD_FORCE_AT_POSITION:
    case PhysicsCommandType::ADD_IMPULSE_AT_POSITION:
      return execute_force_command(command, entity, registry);

    case PhysicsCommandType::SET_LINEAR_VELOCITY:
    case PhysicsCommandType::SET_ANGULAR_VELOCITY:
    case PhysicsCommandType::ADD_LINEAR_VELOCITY:
    case PhysicsCommandType::ADD_ANGULAR_VELOCITY:
      return execute_velocity_command(command, entity, registry);

    case PhysicsCommandType::SET_POSITION:
    case PhysicsCommandType::SET_ROTATION:
    case PhysicsCommandType::TRANSLATE:
    case PhysicsCommandType::ROTATE:
    case PhysicsCommandType::TELEPORT:
      return execute_position_command(command, entity, registry);

    case PhysicsCommandType::ACTIVATE:
    case PhysicsCommandType::DEACTIVATE:
    case PhysicsCommandType::SET_GRAVITY_SCALE:
    case PhysicsCommandType::SET_LINEAR_DAMPING:
    case PhysicsCommandType::SET_ANGULAR_DAMPING:
    case PhysicsCommandType::SET_FRICTION:
    case PhysicsCommandType::SET_RESTITUTION:
      return execute_state_command(command, entity, registry);

    case PhysicsCommandType::RAYCAST:
    case PhysicsCommandType::OVERLAP_TEST:
      return execute_query_command(command, entity, registry);

    case PhysicsCommandType::CUSTOM:
      return execute_custom_command(command, entity, registry);

    default:
      std::cerr << "PhysicsCommandSystem: Unknown command type: " << static_cast<int>(command.type) << std::endl;
      return false;
    }
  }

  bool PhysicsCommandSystem::execute_force_command(const PhysicsCommand &command, entt::entity entity, entt::registry &registry)
  {
    auto *physics_body = get_physics_body(entity, registry);
    if (!physics_body || !physics_body->is_valid())
    {
      return false;
    }

    JPH::BodyID body_id = physics_body->body_id;

    switch (command.type)
    {
    case PhysicsCommandType::ADD_FORCE:
    {
      Vec3 force = command.get_data<Vec3>();
      physics_world_->add_force(body_id, force);
      return true;
    }
    case PhysicsCommandType::ADD_IMPULSE:
    {
      Vec3 impulse = command.get_data<Vec3>();
      physics_world_->add_impulse(body_id, impulse);
      return true;
    }
    case PhysicsCommandType::ADD_TORQUE:
    {
      Vec3 torque = command.get_data<Vec3>();
      physics_world_->add_torque(body_id, torque);
      return true;
    }
    case PhysicsCommandType::ADD_ANGULAR_IMPULSE:
    {
      Vec3 impulse = command.get_data<Vec3>();
      physics_world_->add_angular_impulse(body_id, impulse);
      return true;
    }
    case PhysicsCommandType::ADD_FORCE_AT_POSITION:
    {
      auto force_pos = command.get_data<std::pair<Vec3, Vec3>>();
      Vec3 force = force_pos.first;
      Vec3 position = force_pos.second;

      // 計算相對於質心的位置向量
      JPH::RVec3 com_pos = physics_world_->get_body_position(body_id);
      Vec3 relative_pos(position.GetX() - com_pos.GetX(), position.GetY() - com_pos.GetY(), position.GetZ() - com_pos.GetZ());

      // 添加力和相應的扭矩
      physics_world_->add_force(body_id, force);
      Vec3 torque = relative_pos.Cross(force);
      physics_world_->add_torque(body_id, torque);
      return true;
    }
    case PhysicsCommandType::ADD_IMPULSE_AT_POSITION:
    {
      auto impulse_pos = command.get_data<std::pair<Vec3, Vec3>>();
      Vec3 impulse = impulse_pos.first;
      Vec3 position = impulse_pos.second;

      // 計算相對於質心的位置向量
      JPH::RVec3 com_pos = physics_world_->get_body_position(body_id);
      Vec3 relative_pos(position.GetX() - com_pos.GetX(), position.GetY() - com_pos.GetY(), position.GetZ() - com_pos.GetZ());

      // 添加衝量和相應的角衝量
      physics_world_->add_impulse(body_id, impulse);
      Vec3 angular_impulse = relative_pos.Cross(impulse);
      physics_world_->add_angular_impulse(body_id, angular_impulse);
      return true;
    }
    default:
      return false;
    }
  }

  bool PhysicsCommandSystem::execute_velocity_command(const PhysicsCommand &command, entt::entity entity, entt::registry &registry)
  {
    auto *physics_body = get_physics_body(entity, registry);
    if (!physics_body || !physics_body->is_valid())
    {
      return false;
    }

    JPH::BodyID body_id = physics_body->body_id;

    switch (command.type)
    {
    case PhysicsCommandType::SET_LINEAR_VELOCITY:
    {
      Vec3 velocity = command.get_data<Vec3>();
      physics_world_->set_body_linear_velocity(body_id, velocity);
      physics_body->linear_velocity = velocity;
      return true;
    }
    case PhysicsCommandType::SET_ANGULAR_VELOCITY:
    {
      Vec3 velocity = command.get_data<Vec3>();
      physics_world_->set_body_angular_velocity(body_id, velocity);
      physics_body->angular_velocity = velocity;
      return true;
    }
    case PhysicsCommandType::ADD_LINEAR_VELOCITY:
    {
      Vec3 velocity_delta = command.get_data<Vec3>();
      Vec3 current_vel = physics_world_->get_body_linear_velocity(body_id);
      Vec3 new_vel = Vec3(current_vel.GetX() + velocity_delta.GetX(), current_vel.GetY() + velocity_delta.GetY(), current_vel.GetZ() + velocity_delta.GetZ());
      physics_world_->set_body_linear_velocity(body_id, new_vel);
      physics_body->linear_velocity = new_vel;
      return true;
    }
    case PhysicsCommandType::ADD_ANGULAR_VELOCITY:
    {
      Vec3 velocity_delta = command.get_data<Vec3>();
      Vec3 current_vel = physics_world_->get_body_angular_velocity(body_id);
      Vec3 new_vel = Vec3(current_vel.GetX() + velocity_delta.GetX(), current_vel.GetY() + velocity_delta.GetY(), current_vel.GetZ() + velocity_delta.GetZ());
      physics_world_->set_body_angular_velocity(body_id, new_vel);
      physics_body->angular_velocity = new_vel;
      return true;
    }
    default:
      return false;
    }
  }

  bool PhysicsCommandSystem::execute_position_command(const PhysicsCommand &command, entt::entity entity, entt::registry &registry)
  {
    auto *physics_body = get_physics_body(entity, registry);
    auto *transform = get_transform(entity, registry);

    if (!physics_body || !physics_body->is_valid() || !transform)
    {
      return false;
    }

    JPH::BodyID body_id = physics_body->body_id;

    switch (command.type)
    {
    case PhysicsCommandType::SET_POSITION:
    {
      Vec3 position = command.get_data<Vec3>();
      physics_world_->set_body_position(body_id, JPH::RVec3(position.GetX(), position.GetY(), position.GetZ()));
      transform->position = position;
      return true;
    }
    case PhysicsCommandType::SET_ROTATION:
    {
      Vec3 euler = command.get_data<Vec3>();
      Quat rotation = Quat::sEulerAngles(euler);
      physics_world_->set_body_rotation(body_id, rotation);
      transform->rotation = rotation;
      return true;
    }
    case PhysicsCommandType::TRANSLATE:
    {
      Vec3 translation = command.get_data<Vec3>();
      JPH::RVec3 current_pos = physics_world_->get_body_position(body_id);
      JPH::RVec3 new_pos = current_pos + JPH::RVec3(translation.GetX(), translation.GetY(), translation.GetZ());
      physics_world_->set_body_position(body_id, new_pos);
      transform->position = Vec3(new_pos.GetX(), new_pos.GetY(), new_pos.GetZ());
      return true;
    }
    case PhysicsCommandType::ROTATE:
    {
      Vec3 rotation_delta = command.get_data<Vec3>();
      Quat current_rot = physics_world_->get_body_rotation(body_id);
      Quat delta_quat = Quat::sEulerAngles(rotation_delta);
      Quat new_rot = current_rot * delta_quat;
      physics_world_->set_body_rotation(body_id, new_rot);
      transform->rotation = new_rot;
      return true;
    }
    case PhysicsCommandType::TELEPORT:
    {
      auto pos_rot = command.get_data<std::pair<Vec3, Quat>>();
      Vec3 position = pos_rot.first;
      Quat rotation = pos_rot.second;
      physics_world_->set_body_position(body_id, JPH::RVec3(position.GetX(), position.GetY(), position.GetZ()));
      physics_world_->set_body_rotation(body_id, rotation);
      transform->position = position;
      transform->rotation = rotation;
      return true;
    }
    default:
      return false;
    }
  }

  bool PhysicsCommandSystem::execute_state_command(const PhysicsCommand &command, entt::entity entity, entt::registry &registry)
  {
    auto *physics_body = get_physics_body(entity, registry);
    if (!physics_body || !physics_body->is_valid())
    {
      return false;
    }

    JPH::BodyID body_id = physics_body->body_id;

    switch (command.type)
    {
    case PhysicsCommandType::ACTIVATE:
    {
      // 通過設置速度來激活物體
      physics_world_->set_body_linear_velocity(body_id, Vec3(0.0f, 0.0f, 0.0f));
      physics_body->is_active = true;
      return true;
    }
    case PhysicsCommandType::DEACTIVATE:
    {
      // 通過設置零速度並可能通過其他方式來停用物體
      physics_world_->set_body_linear_velocity(body_id, Vec3(0.0f, 0.0f, 0.0f));
      physics_world_->set_body_angular_velocity(body_id, Vec3(0.0f, 0.0f, 0.0f));
      physics_body->is_active = false;
      return true;
    }
    case PhysicsCommandType::SET_GRAVITY_SCALE:
    {
      float scale = command.get_data<float>();
      physics_body->gravity_scale = scale;
      // 重力縮放需要在物理體屬性中實現
      return true;
    }
    case PhysicsCommandType::SET_LINEAR_DAMPING:
    {
      float damping = command.get_data<float>();
      physics_body->linear_damping = damping;
      // 阻尼設置需要在物理體創建時或通過其他API設置
      return true;
    }
    case PhysicsCommandType::SET_ANGULAR_DAMPING:
    {
      float damping = command.get_data<float>();
      physics_body->angular_damping = damping;
      // 角阻尼設置需要在物理體創建時或通過其他API設置
      return true;
    }
    case PhysicsCommandType::SET_FRICTION:
    {
      float friction = command.get_data<float>();
      physics_body->material.friction = friction;
      // 摩擦力設置需要通過材質更新
      return true;
    }
    case PhysicsCommandType::SET_RESTITUTION:
    {
      float restitution = command.get_data<float>();
      physics_body->material.restitution = restitution;
      // 彈性設置需要通過材質更新
      return true;
    }
    default:
      return false;
    }
  }

  bool PhysicsCommandSystem::execute_query_command(const PhysicsCommand &command, entt::entity entity, entt::registry &registry)
  {
    // 查詢命令通常應該添加到PhysicsQueryComponent中而不是直接執行
    // 這裡我們可以創建或更新查詢組件

    // TODO: 實現查詢命令邏輯
    return true;
  }

  bool PhysicsCommandSystem::execute_custom_command(const PhysicsCommand &command, entt::entity entity, entt::registry &registry)
  {
    if (command.has_data())
    {
      // 替換 try-catch 為簡單的檢查
      auto custom_func = command.get_data<std::function<void()>>();
      if (custom_func)
      {
        custom_func();
        return true;
      }
      else
      {
        std::cerr << "PhysicsCommandSystem: Custom command execution failed: invalid function" << std::endl;
        return false;
      }
    }
    return false;
  }

  bool PhysicsCommandSystem::validate_command(const PhysicsCommand &command, entt::entity entity, entt::registry &registry) const
  {
    // 檢查實體是否有必需的組件
    if (!has_required_components(entity, registry))
    {
      return false;
    }

    // 檢查命令數據是否有效
    if (!command.has_data() && command.type != PhysicsCommandType::ACTIVATE && command.type != PhysicsCommandType::DEACTIVATE)
    {
      return false;
    }

    return true;
  }

  bool PhysicsCommandSystem::has_required_components(entt::entity entity, entt::registry &registry) const
  {
    return registry.all_of<PhysicsBodyComponent>(entity);
  }

  PhysicsBodyComponent *PhysicsCommandSystem::get_physics_body(entt::entity entity, entt::registry &registry) const
  {
    return registry.try_get<PhysicsBodyComponent>(entity);
  }

  TransformComponent *PhysicsCommandSystem::get_transform(entt::entity entity, entt::registry &registry) const
  {
    return registry.try_get<TransformComponent>(entity);
  }

  PhysicsWorldManager *PhysicsCommandSystem::get_physics_world() const
  {
    return physics_world_;
  }

  void PhysicsCommandSystem::cleanup_executed_commands(entt::registry &registry)
  {
    auto view = registry.view<PhysicsCommandComponent>();

    for (auto entity : view)
    {
      auto &cmd_comp = view.get<PhysicsCommandComponent>(entity);

      if (cmd_comp.clear_after_execution)
      {
        cmd_comp.clear_all_commands();
      }
    }
  }

  void PhysicsCommandSystem::remove_command_from_vector(std::vector<PhysicsCommand> &commands, uint64_t command_id)
  {
    auto it = std::remove_if(commands.begin(), commands.end(),
                             [command_id](const PhysicsCommand &command)
                             {
                               return command.command_id == command_id;
                             });

    if (it != commands.end())
    {
      commands.erase(it, commands.end());
    }
  }

  // PhysicsQuerySystem 實現

  bool PhysicsQuerySystem::initialize()
  {
    std::cout << "PhysicsQuerySystem: Initializing..." << std::endl;

    physics_world_ = &PhysicsWorldManager::get_instance();
    stats_ = QuerySystemStats{};

    initialized_ = true;
    std::cout << "PhysicsQuerySystem: Initialization complete." << std::endl;
    return true;
  }

  void PhysicsQuerySystem::update(entt::registry &registry, float delta_time)
  {
    if (!initialized_ || !enabled_)
    {
      return;
    }

    auto start_time = std::chrono::high_resolution_clock::now();

    queries_executed_this_frame_ = 0;
    stats_.raycast_queries_executed = 0;
    stats_.overlap_queries_executed = 0;
    stats_.distance_queries_executed = 0;

    // 執行各種查詢
    execute_raycast_queries(registry);
    execute_overlap_queries(registry);
    execute_distance_queries(registry);

    auto end_time = std::chrono::high_resolution_clock::now();
    stats_.execution_time = std::chrono::duration<float>(end_time - start_time).count();
    stats_.total_queries_executed = stats_.raycast_queries_executed +
                                    stats_.overlap_queries_executed +
                                    stats_.distance_queries_executed;
  }

  void PhysicsQuerySystem::cleanup()
  {
    std::cout << "PhysicsQuerySystem: Cleaning up..." << std::endl;

    physics_world_ = nullptr;
    initialized_ = false;

    std::cout << "PhysicsQuerySystem: Cleanup complete." << std::endl;
  }

  void PhysicsQuerySystem::execute_raycast_queries(entt::registry &registry)
  {
    auto view = registry.view<PhysicsQueryComponent>();

    for (auto entity : view)
    {
      if (queries_executed_this_frame_ >= max_queries_per_frame_)
      {
        break;
      }

      auto &query_comp = view.get<PhysicsQueryComponent>(entity);

      for (auto &query : query_comp.raycast_queries)
      {
        if (queries_executed_this_frame_ >= max_queries_per_frame_)
        {
          break;
        }

        if (execute_raycast_query(query, entity, registry))
        {
          stats_.raycast_queries_executed++;
          queries_executed_this_frame_++;
        }
        else
        {
          stats_.queries_failed++;
        }
      }

      query_comp.raycast_results_valid = true;
    }
  }

  void PhysicsQuerySystem::execute_overlap_queries(entt::registry &registry)
  {
    auto view = registry.view<PhysicsQueryComponent>();

    for (auto entity : view)
    {
      if (queries_executed_this_frame_ >= max_queries_per_frame_)
      {
        break;
      }

      auto &query_comp = view.get<PhysicsQueryComponent>(entity);

      for (auto &query : query_comp.overlap_queries)
      {
        if (queries_executed_this_frame_ >= max_queries_per_frame_)
        {
          break;
        }

        if (execute_overlap_query(query, entity, registry))
        {
          stats_.overlap_queries_executed++;
          queries_executed_this_frame_++;
        }
        else
        {
          stats_.queries_failed++;
        }
      }

      query_comp.overlap_results_valid = true;
    }
  }

  void PhysicsQuerySystem::execute_distance_queries(entt::registry &registry)
  {
    auto view = registry.view<PhysicsQueryComponent>();

    for (auto entity : view)
    {
      if (queries_executed_this_frame_ >= max_queries_per_frame_)
      {
        break;
      }

      auto &query_comp = view.get<PhysicsQueryComponent>(entity);

      for (auto &query : query_comp.distance_queries)
      {
        if (queries_executed_this_frame_ >= max_queries_per_frame_)
        {
          break;
        }

        if (execute_distance_query(query, entity, registry))
        {
          stats_.distance_queries_executed++;
          queries_executed_this_frame_++;
        }
        else
        {
          stats_.queries_failed++;
        }
      }

      query_comp.distance_results_valid = true;
    }
  }

  bool PhysicsQuerySystem::execute_raycast_query(PhysicsQueryComponent::RaycastQuery &query, entt::entity entity, entt::registry &registry)
  {
    auto result = physics_world_->raycast(
        JPH::RVec3(query.origin.GetX(), query.origin.GetY(), query.origin.GetZ()),
        query.direction,
        query.max_distance);

    query.hit = result.hit;
    if (result.hit)
    {
      query.hit_point = result.hit_point;
      query.hit_normal = result.hit_normal;
      query.hit_distance = result.distance;
      query.hit_entity = body_id_to_entity(result.body_id, registry);
    }

    return true;
  }

  bool PhysicsQuerySystem::execute_overlap_query(PhysicsQueryComponent::OverlapQuery &query, entt::entity entity, entt::registry &registry)
  {
    std::vector<JPH::BodyID> body_ids;

    switch (query.shape)
    {
    case PhysicsQueryComponent::OverlapQuery::SPHERE:
    {
      body_ids = physics_world_->overlap_sphere(
          JPH::RVec3(query.center.GetX(), query.center.GetY(), query.center.GetZ()),
          query.size.GetX());
      break;
    }
    case PhysicsQueryComponent::OverlapQuery::BOX:
    {
      body_ids = physics_world_->overlap_box(
          JPH::RVec3(query.center.GetX(), query.center.GetY(), query.center.GetZ()),
          query.size,
          query.rotation);
      break;
    }
    default:
      return false;
    }

    // 轉換BodyID到entity並應用層過濾
    query.overlapping_entities.clear();
    for (JPH::BodyID body_id : body_ids)
    {
      entt::entity overlapping_entity = body_id_to_entity(body_id, registry);
      if (overlapping_entity != entt::null && passes_layer_filter(overlapping_entity, query.layer_mask, registry))
      {
        query.overlapping_entities.push_back(overlapping_entity);
      }
    }

    return true;
  }

  bool PhysicsQuerySystem::execute_distance_query(PhysicsQueryComponent::DistanceQuery &query, entt::entity entity, entt::registry &registry)
  {
    // 使用球體重疊查詢來實現距離查詢
    auto body_ids = physics_world_->overlap_sphere(
        JPH::RVec3(query.point.GetX(), query.point.GetY(), query.point.GetZ()),
        query.max_distance);

    query.closest_entity = entt::null;
    query.closest_distance = std::numeric_limits<float>::max();

    for (JPH::BodyID body_id : body_ids)
    {
      entt::entity candidate_entity = body_id_to_entity(body_id, registry);
      if (candidate_entity != entt::null && passes_layer_filter(candidate_entity, query.layer_mask, registry))
      {
        JPH::RVec3 body_pos = physics_world_->get_body_position(body_id);
        Vec3 body_position(body_pos.GetX(), body_pos.GetY(), body_pos.GetZ());

        float distance = (query.point - body_position).Length();
        if (distance < query.closest_distance)
        {
          query.closest_distance = distance;
          query.closest_entity = candidate_entity;
          query.closest_point = body_position;
        }
      }
    }

    return true;
  }

  PhysicsWorldManager *PhysicsQuerySystem::get_physics_world() const
  {
    return physics_world_;
  }

  entt::entity PhysicsQuerySystem::body_id_to_entity(JPH::BodyID body_id, entt::registry &registry) const
  {
    // 這需要與PhysicsSystem協調，通過用戶數據或其他方式獲取entity
    // 暫時返回null，實際實現中需要適當的映射機制
    return entt::null;
  }

  bool PhysicsQuerySystem::passes_layer_filter(entt::entity entity, uint32_t layer_mask, entt::registry &registry) const
  {
    PhysicsBodyComponent *physics_body = registry.try_get<PhysicsBodyComponent>(entity);
    if (!physics_body)
    {
      return false;
    }

    return (physics_body->collision_filter.collision_layer & layer_mask) != 0;
  }

  // 工廠函數實現
  std::unique_ptr<ISystem> create_physics_command_system()
  {
    return std::make_unique<PhysicsCommandSystem>();
  }

  std::unique_ptr<ISystem> create_physics_query_system()
  {
    return std::make_unique<PhysicsQuerySystem>();
  }

} // namespace portal_core
