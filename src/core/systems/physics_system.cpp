#include "physics_system.h"
#include "../components/physics_body_component.h"
#include "../components/transform_component.h"
#include "../components/physics_sync_component.h"
#include "../components/physics_event_component.h"
#include "../component_safety_manager.h"
#include <iostream>
#include <chrono>

namespace portal_core
{

  bool PhysicsSystem::initialize()
  {
    std::cout << "PhysicsSystem: Initializing..." << std::endl;

    // 初始化物理世界
    if (!initialize_physics_world())
    {
      std::cerr << "PhysicsSystem: Failed to initialize physics world!" << std::endl;
      return false;
    }

    // 重置統計數據
    stats_ = PhysicsSystemStats{};
    accumulator_time_ = 0.0f;
    frame_counter_ = 0;

    std::cout << "PhysicsSystem: Initialization complete." << std::endl;
    return true;
  }

  bool PhysicsSystem::initialize(entt::registry& registry)
  {
    if (!initialize())
    {
      return false;
    }

    // 設置 EnTT 組件監聽器
    physics_body_added_connection_ = registry.on_construct<PhysicsBodyComponent>().connect<&PhysicsSystem::on_physics_body_added>(*this);
    physics_body_removed_connection_ = registry.on_destroy<PhysicsBodyComponent>().connect<&PhysicsSystem::on_physics_body_removed>(*this);
    transform_updated_connection_ = registry.on_update<TransformComponent>().connect<&PhysicsSystem::on_transform_updated>(*this);

    std::cout << "PhysicsSystem: Component listeners set up." << std::endl;
    return true;
  }

  void PhysicsSystem::update(entt::registry &registry, float delta_time)
  {
    if (!physics_world_initialized_)
    {
      return;
    }

    auto start_time = std::chrono::high_resolution_clock::now();

    // 處理待創建和待銷毀的物理體
    process_pending_creations(registry);
    process_pending_destructions(registry);

    // 處理需要同步到物理的實體（運動學物體等）
    if (auto_sync_enabled_)
    {
      sync_transform_to_physics(registry);
    }

    // 步進物理世界
    auto physics_start = std::chrono::high_resolution_clock::now();
    physics_world_->update(delta_time);
    auto physics_end = std::chrono::high_resolution_clock::now();

    stats_.physics_step_time = std::chrono::duration<float>(physics_end - physics_start).count();

    // 同步物理結果到Transform
    if (auto_sync_enabled_)
    {
      auto sync_start = std::chrono::high_resolution_clock::now();
      sync_physics_to_transform(registry);
      auto sync_end = std::chrono::high_resolution_clock::now();

      stats_.sync_time = std::chrono::duration<float>(sync_end - sync_start).count();
    }

    // 處理碰撞和觸發事件
    handle_collision_events(registry);
    handle_trigger_events(registry);

    // 更新調試渲染
    if (debug_rendering_enabled_)
    {
      update_debug_rendering(registry);
    }

    // 更新統計信息
    update_statistics(registry, delta_time);

    auto end_time = std::chrono::high_resolution_clock::now();
    float total_time = std::chrono::duration<float>(end_time - start_time).count();

    // 每60幀輸出一次性能統計
    if (++frame_counter_ % 60 == 0)
    {
      std::cout << "PhysicsSystem: Bodies=" << stats_.num_physics_bodies
                << " Active=" << stats_.num_active_bodies
                << " PhysicsTime=" << stats_.physics_step_time * 1000.0f << "ms"
                << " SyncTime=" << stats_.sync_time * 1000.0f << "ms"
                << " TotalTime=" << total_time * 1000.0f << "ms" << std::endl;
    }
  }

  void PhysicsSystem::cleanup()
  {
    std::cout << "PhysicsSystem: Cleaning up..." << std::endl;

    // 斷開EnTT連接
    physics_body_added_connection_.release();
    physics_body_removed_connection_.release();
    transform_updated_connection_.release();

    // 清理映射
    entity_to_body_.clear();
    body_to_entity_.clear();
    pending_creation_.clear();
    pending_destruction_.clear();
    entities_needing_physics_sync_.clear();
    entities_needing_transform_sync_.clear();

    // 重置標誌
    physics_world_initialized_ = false;
    physics_world_ = nullptr;

    std::cout << "PhysicsSystem: Cleanup complete." << std::endl;
  }

  void PhysicsSystem::create_physics_body(entt::entity entity, entt::registry &registry)
  {
    // 檢查是否已經有物理體
    if (entity_to_body_.count(entity))
    {
      std::cout << "PhysicsSystem: Entity already has physics body, skipping creation." << std::endl;
      return;
    }

    // 檢查必需的組件
    auto *physics_body = registry.try_get<PhysicsBodyComponent>(entity);
    auto *transform = registry.try_get<TransformComponent>(entity);

    if (!physics_body || !transform)
    {
      std::cerr << "PhysicsSystem: Entity missing required components for physics body creation." << std::endl;
      return;
    }

    // 使用安全管理器進行自動驗證和修正
    uint32_t entity_id = static_cast<uint32_t>(entity);
    bool physics_corrected = ComponentSafetyManager::validate_and_correct_physics_body(*physics_body, entity_id);
    bool transform_corrected = ComponentSafetyManager::validate_and_correct_transform(*transform, entity_id);
    
    if (physics_corrected || transform_corrected) {
      std::cout << "PhysicsSystem: Components auto-corrected for entity " << entity_id << std::endl;
    }

    // 檢查組件依賴關係
    if (!ComponentSafetyManager::validate_component_dependencies(registry, entity)) {
      std::cerr << "PhysicsSystem: Component dependency validation failed for entity " << entity_id << std::endl;
      return;
    }

    // 驗證物理體組件（現在應該都是有效的）
    if (!validate_physics_body_component(*physics_body))
    {
      std::cerr << "PhysicsSystem: Physics body component still invalid after auto-correction for entity " 
                << entity_id << std::endl;
      return;
    }

    // 創建Jolt物理體
    if (create_jolt_body(entity, *physics_body, *transform))
    {
      std::cout << "PhysicsSystem: Created physics body for entity " << static_cast<uint32_t>(entity) << std::endl;
    }
    else
    {
      std::cerr << "PhysicsSystem: Failed to create physics body for entity " << static_cast<uint32_t>(entity) << std::endl;
    }
  }

  void PhysicsSystem::destroy_physics_body(entt::entity entity, entt::registry &registry)
  {
    auto it = entity_to_body_.find(entity);
    if (it == entity_to_body_.end())
    {
      return; // 沒有物理體
    }

    JPH::BodyID body_id = it->second;

    // 從物理世界移除
    physics_world_->destroy_body(body_id);

    // 清理映射
    cleanup_entity_mapping(entity);

    // 重置組件中的body_id
    if (auto *physics_body = registry.try_get<PhysicsBodyComponent>(entity))
    {
      physics_body->body_id = JPH::BodyID();
    }

    std::cout << "PhysicsSystem: Destroyed physics body for entity " << static_cast<uint32_t>(entity) << std::endl;
  }

  void PhysicsSystem::sync_physics_to_transform(entt::registry &registry)
  {
    // 同步動態物體的位置和旋轉到Transform
    auto view = registry.view<PhysicsBodyComponent, TransformComponent, PhysicsSyncComponent>();

    stats_.num_sync_operations = 0;

    view.each([&](auto entity, auto &physics_body, auto &transform, auto &sync_comp)
              {
        // 檢查同步方向和條件
        if (sync_comp.sync_direction == PhysicsSyncComponent::TRANSFORM_TO_PHYSICS) {
            return; // 這個方向在另一個函數處理
        }
        
        if (!physics_body.is_valid() || (!sync_comp.sync_position && !sync_comp.sync_rotation)) {
            return;
        }
        
        sync_single_entity_to_transform(entity, registry);
        stats_.num_sync_operations++; });

    // 同步沒有PhysicsSyncComponent但有物理體的實體（使用默認同步）
    auto physics_view = registry.view<PhysicsBodyComponent, TransformComponent>(entt::exclude<PhysicsSyncComponent>);
    physics_view.each([&](auto entity, auto &physics_body, auto &transform)
                      {
        if (physics_body.is_valid() && physics_body.is_dynamic()) {
            sync_single_entity_to_transform(entity, registry);
            stats_.num_sync_operations++;
        } });
  }

  void PhysicsSystem::sync_transform_to_physics(entt::registry &registry)
  {
    // 同步Transform到物理（主要用於運動學物體）
    auto view = registry.view<PhysicsBodyComponent, TransformComponent, PhysicsSyncComponent>();

    view.each([&](auto entity, auto &physics_body, auto &transform, auto &sync_comp)
              {
        // 檢查同步方向
        if (sync_comp.sync_direction == PhysicsSyncComponent::PHYSICS_TO_TRANSFORM) {
            return; // 這個方向在另一個函數處理
        }
        
        if (!physics_body.is_valid()) {
            return;
        }
        
        // 運動學物體或明確要求Transform到Physics同步
        if (physics_body.is_kinematic || 
            sync_comp.sync_direction == PhysicsSyncComponent::TRANSFORM_TO_PHYSICS ||
            sync_comp.sync_direction == PhysicsSyncComponent::BIDIRECTIONAL) {
            
            sync_single_entity_to_physics(entity, registry);
        } });
  }

  entt::entity PhysicsSystem::get_entity_by_body_id(JPH::BodyID body_id) const
  {
    auto it = body_to_entity_.find(body_id);
    return (it != body_to_entity_.end()) ? it->second : entt::null;
  }

  JPH::BodyID PhysicsSystem::get_body_id_by_entity(entt::entity entity) const
  {
    auto it = entity_to_body_.find(entity);
    return (it != entity_to_body_.end()) ? it->second : JPH::BodyID();
  }

  bool PhysicsSystem::initialize_physics_world()
  {
    // 獲取物理世界管理器實例
    physics_world_ = &PhysicsWorldManager::get_instance();

    // 如果還未初始化，則初始化物理世界
    if (!physics_world_->is_initialized())
    {
      JPH::PhysicsSettings settings;
      settings.mBaumgarte = 0.2f;
      settings.mSpeculativeContactDistance = 0.02f;
      settings.mPenetrationSlop = 0.02f; // 修正名稱：原來的 mPenetrationSlop
      settings.mLinearCastThreshold = 0.75f;
      settings.mBodyPairCacheMaxDeltaPositionSq = JPH::Square(0.002f);                      // 修正名稱和函數
      settings.mBodyPairCacheCosMaxDeltaRotationDiv2 = 0.99984769515639123915701155881391f; // 修正名稱：cos(2度/2)
      settings.mContactNormalCosMaxDeltaRotation = 0.99619469809174553229501040247389f;     // 修正名稱
      settings.mContactPointPreserveLambdaMaxDistSq = JPH::Square(0.01f);                   // 修正名稱和函數
      settings.mNumVelocitySteps = 10;
      settings.mNumPositionSteps = 5;
      settings.mPointVelocitySleepThreshold = 0.03f;
      settings.mDeterministicSimulation = false;
      settings.mConstraintWarmStart = true;
      settings.mUseBodyPairContactCache = true;
      settings.mUseManifoldReduction = true;
      settings.mUseLargeIslandSplitter = true;
      settings.mAllowSleeping = true;
      settings.mCheckActiveEdges = true;

      if (!physics_world_->initialize(settings))
      {
        return false;
      }

      // 設置碰撞事件回調
      physics_world_->set_contact_added_callback(
          [this](JPH::BodyID body1, JPH::BodyID body2)
          {
            // 處理碰撞進入事件
            entt::entity entity1 = get_entity_by_body_id(body1);
            entt::entity entity2 = get_entity_by_body_id(body2);

            if (entity1 != entt::null && entity2 != entt::null)
            {
              // 這裡可以添加碰撞事件處理邏輯
              // 暫時只輸出調試信息
              // std::cout << "Collision between entities " << static_cast<uint32_t>(entity1)
              //          << " and " << static_cast<uint32_t>(entity2) << std::endl;
            }
          });

      physics_world_->set_contact_removed_callback(
          [this](JPH::BodyID body1, JPH::BodyID body2)
          {
            // 處理碰撞退出事件
            entt::entity entity1 = get_entity_by_body_id(body1);
            entt::entity entity2 = get_entity_by_body_id(body2);

            if (entity1 != entt::null && entity2 != entt::null)
            {
              // 這裡可以添加碰撞退出事件處理邏輯
            }
          });
    }

    physics_world_initialized_ = true;
    return true;
  }

  void PhysicsSystem::process_pending_creations(entt::registry &registry)
  {
    if (pending_creation_.empty() && auto_create_bodies_)
    {
      // 自動檢測需要創建物理體的實體
      auto view = registry.view<PhysicsBodyComponent, TransformComponent>();
      view.each([&](auto entity, auto &physics_body, auto &transform)
                {
            if (!physics_body.is_valid()) {
                pending_creation_.insert(entity);
            } });
    }

    for (auto entity : pending_creation_)
    {
      create_physics_body(entity, registry);
    }
    pending_creation_.clear();
  }

  void PhysicsSystem::process_pending_destructions(entt::registry &registry)
  {
    for (auto entity : pending_destruction_)
    {
      destroy_physics_body(entity, registry);
    }
    pending_destruction_.clear();
  }

  void PhysicsSystem::update_statistics(entt::registry &registry, float delta_time)
  {
    // 計算物理體數量
    auto physics_view = registry.view<PhysicsBodyComponent>();
    stats_.num_physics_bodies = 0;
    stats_.num_active_bodies = 0;
    stats_.num_sleeping_bodies = 0;

    for (auto entity : physics_view)
    {
      auto &physics_body = physics_view.get<PhysicsBodyComponent>(entity);
      if (physics_body.is_valid())
      {
        stats_.num_physics_bodies++;
        if (physics_world_->is_body_active(physics_body.body_id))
        {
          stats_.num_active_bodies++;
        }
        else
        {
          stats_.num_sleeping_bodies++;
        }
      }
    }
  }

  bool PhysicsSystem::validate_physics_body_component(const PhysicsBodyComponent &component) const
  {
    // 檢查形狀是否有效
    switch (component.shape.type)
    {
    case PhysicsShapeType::BOX:
      if (component.shape.size.GetX() <= 0 || component.shape.size.GetY() <= 0 || component.shape.size.GetZ() <= 0)
      {
        std::cerr << "PhysicsSystem: Invalid box size." << std::endl;
        return false;
      }
      break;
    case PhysicsShapeType::SPHERE:
      if (component.shape.radius <= 0)
      {
        std::cerr << "PhysicsSystem: Invalid sphere radius." << std::endl;
        return false;
      }
      break;
    case PhysicsShapeType::CAPSULE:
      if (component.shape.radius <= 0 || component.shape.height <= 0)
      {
        std::cerr << "PhysicsSystem: Invalid capsule dimensions." << std::endl;
        return false;
      }
      break;
    default:
      break;
    }

    // 檢查材質屬性
    if (component.material.friction < 0 || component.material.restitution < 0 || component.material.restitution > 1)
    {
      std::cerr << "PhysicsSystem: Invalid material properties." << std::endl;
      return false;
    }

    // 檢查質量（動態物體）
    if (component.is_dynamic() && component.mass <= 0)
    {
      std::cerr << "PhysicsSystem: Dynamic body must have positive mass." << std::endl;
      return false;
    }

    return true;
  }

  bool PhysicsSystem::create_jolt_body(entt::entity entity, PhysicsBodyComponent &physics_body,
                                       const TransformComponent &transform)
  {
    // 創建物理體描述符
    PhysicsBodyDesc desc = physics_body.create_physics_body_desc(transform.position, transform.rotation);
    desc.user_data = static_cast<uint64_t>(entity);

    // 創建物理體
    JPH::BodyID body_id = physics_world_->create_body(desc);
    if (body_id.IsInvalid())
    {
      return false;
    }

    // 更新組件
    physics_body.body_id = body_id;

    // 建立映射
    entity_to_body_[entity] = body_id;
    body_to_entity_[body_id] = entity;

    // 應用額外的物理設定
    apply_physics_settings(body_id, physics_body);

    return true;
  }

  void PhysicsSystem::sync_single_entity_to_transform(entt::entity entity, entt::registry &registry)
  {
    auto *physics_body = registry.try_get<PhysicsBodyComponent>(entity);
    auto *transform = registry.try_get<TransformComponent>(entity);
    auto *sync_comp = registry.try_get<PhysicsSyncComponent>(entity);

    if (!physics_body || !transform || !physics_body->is_valid())
    {
      return;
    }

    // 獲取物理體位置和旋轉
    JPH::RVec3 physics_pos = physics_world_->get_body_position(physics_body->body_id);
    JPH::Quat physics_rot = physics_world_->get_body_rotation(physics_body->body_id);

    Vec3 new_position(physics_pos.GetX(), physics_pos.GetY(), physics_pos.GetZ());
    Quat new_rotation(physics_rot.GetX(), physics_rot.GetY(), physics_rot.GetZ(), physics_rot.GetW());

    // 應用偏移
    if (sync_comp)
    {
      new_position += sync_comp->position_offset;
      new_rotation = new_rotation * sync_comp->rotation_offset;

      // 檢查是否需要同步（閾值檢查）
      if (!sync_comp->should_sync_position(new_position) && !sync_comp->should_sync_rotation(new_rotation))
      {
        return;
      }

      // 插值處理
      if (sync_comp->enable_interpolation)
      {
        float t = sync_comp->interpolation_speed * 0.016f; // 假設60fps
        if (sync_comp->sync_position)
        {
          transform->position = transform->position.lerp(new_position, t);
        }
        if (sync_comp->sync_rotation)
        {
          transform->rotation = transform->rotation.slerp(new_rotation, t);
        }
      }
      else
      {
        // 直接設置
        if (sync_comp->sync_position)
        {
          transform->position = new_position;
        }
        if (sync_comp->sync_rotation)
        {
          transform->rotation = new_rotation;
        }
      }

      // 更新上次同步狀態
      sync_comp->update_last_synced_state(new_position, new_rotation);
    }
    else
    {
      // 默認同步行為
      transform->position = new_position;
      transform->rotation = new_rotation;
    }

    // 同步速度（如果需要）
    if (sync_comp && sync_comp->sync_velocity)
    {
      JPH::Vec3 linear_vel = physics_world_->get_body_linear_velocity(physics_body->body_id);
      JPH::Vec3 angular_vel = physics_world_->get_body_angular_velocity(physics_body->body_id);

      physics_body->linear_velocity = Vec3(linear_vel.GetX(), linear_vel.GetY(), linear_vel.GetZ());
      physics_body->angular_velocity = Vec3(angular_vel.GetX(), angular_vel.GetY(), angular_vel.GetZ());
    }
  }

  void PhysicsSystem::sync_single_entity_to_physics(entt::entity entity, entt::registry &registry)
  {
    auto *physics_body = registry.try_get<PhysicsBodyComponent>(entity);
    auto *transform = registry.try_get<TransformComponent>(entity);
    auto *sync_comp = registry.try_get<PhysicsSyncComponent>(entity);

    if (!physics_body || !transform || !physics_body->is_valid())
    {
      return;
    }

    Vec3 physics_position = transform->position;
    Quat physics_rotation = transform->rotation;

    // 應用偏移
    if (sync_comp)
    {
      physics_position -= sync_comp->position_offset;
      physics_rotation = physics_rotation * sync_comp->rotation_offset.conjugate();
    }

    // 同步到物理世界
    if (!sync_comp || sync_comp->sync_position)
    {
      JPH::RVec3 jolt_pos(physics_position.GetX(), physics_position.GetY(), physics_position.GetZ());
      physics_world_->set_body_position(physics_body->body_id, jolt_pos);
    }

    if (!sync_comp || sync_comp->sync_rotation)
    {
      JPH::Quat jolt_rot(physics_rotation.GetX(), physics_rotation.GetY(), physics_rotation.GetZ(), physics_rotation.GetW());
      physics_world_->set_body_rotation(physics_body->body_id, jolt_rot);
    }
  }

  void PhysicsSystem::apply_physics_settings(JPH::BodyID body_id, const PhysicsBodyComponent &component)
  {
    // 設置速度
    if (component.linear_velocity.length() > 0)
    {
      JPH::Vec3 vel(component.linear_velocity.GetX(), component.linear_velocity.GetY(), component.linear_velocity.GetZ());
      physics_world_->set_body_linear_velocity(body_id, vel);
    }

    if (component.angular_velocity.length() > 0)
    {
      JPH::Vec3 ang_vel(component.angular_velocity.GetX(), component.angular_velocity.GetY(), component.angular_velocity.GetZ());
      physics_world_->set_body_angular_velocity(body_id, ang_vel);
    }

    // 其他物理設定可以在這裡添加
    // 例如：阻尼、重力縮放等
  }

  void PhysicsSystem::cleanup_entity_mapping(entt::entity entity)
  {
    auto it = entity_to_body_.find(entity);
    if (it != entity_to_body_.end())
    {
      JPH::BodyID body_id = it->second;
      body_to_entity_.erase(body_id);
      entity_to_body_.erase(it);
    }
  }

  void PhysicsSystem::handle_collision_events(entt::registry &registry)
  {
    // TODO: 實現碰撞事件處理
    // 這裡可以處理PhysicsEventComponent中的碰撞回調
  }

  void PhysicsSystem::handle_trigger_events(entt::registry &registry)
  {
    // TODO: 實現觸發器事件處理
    // 這裡可以處理PhysicsEventComponent中的觸發器回調
  }

  void PhysicsSystem::update_debug_rendering(entt::registry &registry)
  {
    // TODO: 實現調試渲染
    // 這裡可以添加物理體的可視化調試信息
  }

  void PhysicsSystem::on_physics_body_added(entt::registry &registry, entt::entity entity)
  {
    // 使用安全管理器自動驗證和修正新添加的組件
    if (auto* physics_body = registry.try_get<PhysicsBodyComponent>(entity)) {
      uint32_t entity_id = static_cast<uint32_t>(entity);
      bool corrected = ComponentSafetyManager::validate_and_correct_physics_body(*physics_body, entity_id);
      if (corrected) {
        std::cout << "PhysicsSystem: Auto-corrected PhysicsBodyComponent for entity " 
                  << entity_id << " on component addition" << std::endl;
      }
    }
    
    pending_creation_.insert(entity);
  }

  void PhysicsSystem::on_physics_body_removed(entt::registry &registry, entt::entity entity)
  {
    pending_destruction_.insert(entity);
  }

  void PhysicsSystem::on_transform_updated(entt::registry &registry, entt::entity entity)
  {
    entities_needing_physics_sync_.insert(entity);
  }

  void PhysicsSystem::set_body_user_data(JPH::BodyID body_id, entt::entity entity)
  {
    // 設置物理體的用戶數據為entity的數值表示
    // 注意：這裡需要確保entity能夠安全轉換為uint64_t
    uint64_t entity_value = static_cast<uint64_t>(entity);
    physics_world_->get_body_interface().SetUserData(body_id, entity_value);
  }

  entt::entity PhysicsSystem::get_entity_from_user_data(uint64_t user_data) const
  {
    // 從用戶數據恢復entity
    return static_cast<entt::entity>(user_data);
  }

  void PhysicsSystem::handle_physics_properties_changed(entt::entity entity, entt::registry &registry)
  {
    auto *physics_body = registry.try_get<PhysicsBodyComponent>(entity);
    if (!physics_body || !physics_body->is_valid())
    {
      return;
    }

    // 檢查是否需要重新創建物理體
    // 由於我們沒有舊的組件數據，這裡我們簡單地更新屬性
    apply_physics_settings(physics_body->body_id, *physics_body);
  }

  bool PhysicsSystem::needs_body_recreation(const PhysicsBodyComponent &old_component,
                                            const PhysicsBodyComponent &new_component) const
  {
    // 檢查需要重新創建物理體的屬性變更
    if (old_component.body_type != new_component.body_type)
    {
      return true;
    }

    // 檢查形狀類型變更
    if (old_component.shape.type != new_component.shape.type)
    {
      return true;
    }

    // 檢查形狀參數的變更 - 使用 PhysicsShapeDesc 的實際欄位
    if (old_component.shape.size != new_component.shape.size ||
        old_component.shape.radius != new_component.shape.radius)
    {
      return true;
    }

    // 檢查頂點數據變更（對於凸包和網格）
    if (old_component.shape.vertices.size() != new_component.shape.vertices.size() ||
        old_component.shape.indices.size() != new_component.shape.indices.size())
    {
      return true;
    }

    return false;
  }

  // 工廠函數實現
  std::unique_ptr<ISystem> create_physics_system()
  {
    return std::make_unique<PhysicsSystem>();
  }

} // namespace portal_core
