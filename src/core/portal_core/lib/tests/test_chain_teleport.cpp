#include <iostream>
#include <cassert>
#include <vector>
#include <memory>
#include <string>
#include <algorithm>
#include <unordered_map>

#include "../include/portal_core_v2.h"

// 命名空间别名来避免冲突
namespace PortalLib = Portal;
using PortalManager = PortalLib::PortalManager;
using EntityId = PortalLib::EntityId;
using PortalId = PortalLib::PortalId;
using PortalFace = PortalLib::PortalFace;
using Transform = PortalLib::Transform;
using PhysicsState = PortalLib::PhysicsState;
using Vector3 = PortalLib::Vector3;
using PortalPlane = PortalLib::PortalPlane;
using IPhysicsDataProvider = PortalLib::IPhysicsDataProvider;
using IPhysicsManipulator = PortalLib::IPhysicsManipulator;
using IPortalEventHandler = PortalLib::IPortalEventHandler;
using PortalInterfaces = PortalLib::PortalInterfaces;
using EntityDescription = PortalLib::EntityDescription;
using GhostEntitySnapshot = PortalLib::GhostEntitySnapshot;
using ChainNodeCreateDescriptor = PortalLib::ChainNodeCreateDescriptor;
using ClippingPlane = PortalLib::ClippingPlane;
using PhysicsConstraintState = PortalLib::PhysicsConstraintState;
using EntityType = PortalLib::EntityType;
using CenterOfMassConfig = PortalLib::CenterOfMassConfig;

constexpr EntityId INVALID_ENTITY_ID = PortalLib::INVALID_ENTITY_ID;
constexpr PortalId INVALID_PORTAL_ID = PortalLib::INVALID_PORTAL_ID;

// 前向声明
void test_interleaved_chain_sequence_with_assertions(PortalManager &manager,
                                                     class MockPhysicsManipulator *manipulator_ptr,
                                                     class MockEventHandler *event_handler_ptr,
                                                     PortalId portal1, PortalId portal3,
                                                     PortalId portal5, PortalId portal7);

// === 模拟的物理引擎接口实现 ===

class MockPhysicsDataProvider : public IPhysicsDataProvider
{
private:
  std::unordered_map<EntityId, Transform> entity_transforms_;
  std::unordered_map<EntityId, PhysicsState> entity_physics_;
  std::unordered_map<EntityId, EntityDescription> entity_descriptions_;
  std::unordered_map<EntityId, CenterOfMassConfig> center_of_mass_configs_;

public:
  void add_mock_entity(EntityId entity_id, const Transform &transform, const PhysicsState &physics)
  {
    entity_transforms_[entity_id] = transform;
    entity_physics_[entity_id] = physics;

    EntityDescription desc;
    desc.entity_id = entity_id;
    desc.entity_type = EntityType::MAIN;
    desc.transform = transform;
    desc.physics = physics;
    desc.center_of_mass = Vector3(0, 0, 0);
    desc.bounds_min = Vector3(-0.5f, -0.5f, -0.5f);
    desc.bounds_max = Vector3(0.5f, 0.5f, 0.5f);
    entity_descriptions_[entity_id] = desc;
  }

  Transform get_entity_transform(EntityId entity_id) override
  {
    auto it = entity_transforms_.find(entity_id);
    return (it != entity_transforms_.end()) ? it->second : Transform();
  }
  PhysicsState get_entity_physics_state(EntityId entity_id) override
  {
    auto it = entity_physics_.find(entity_id);
    return (it != entity_physics_.end()) ? it->second : PhysicsState();
  }
  void get_entity_bounds(EntityId entity_id, Vector3 &bounds_min, Vector3 &bounds_max) override
  {
    bounds_min = Vector3(-0.5f, -0.5f, -0.5f);
    bounds_max = Vector3(0.5f, 0.5f, 0.5f);
  }
  bool is_entity_valid(EntityId entity_id) override
  {
    return entity_transforms_.find(entity_id) != entity_transforms_.end();
  }
  EntityDescription get_entity_description(EntityId entity_id) override
  {
    auto it = entity_descriptions_.find(entity_id);
    return (it != entity_descriptions_.end()) ? it->second : EntityDescription();
  }
  std::vector<Transform> get_entities_transforms(const std::vector<EntityId> &entity_ids) override
  {
    std::vector<Transform> transforms;
    for (EntityId id : entity_ids)
      transforms.push_back(get_entity_transform(id));
    return transforms;
  }
  std::vector<PhysicsState> get_entities_physics_states(const std::vector<EntityId> &entity_ids) override
  {
    std::vector<PhysicsState> states;
    for (EntityId id : entity_ids)
      states.push_back(get_entity_physics_state(id));
    return states;
  }
  std::vector<EntityDescription> get_entities_descriptions(const std::vector<EntityId> &entity_ids) override
  {
    std::vector<EntityDescription> descriptions;
    for (EntityId id : entity_ids)
      descriptions.push_back(get_entity_description(id));
    return descriptions;
  }
  Vector3 calculate_entity_center_of_mass(EntityId entity_id) override
  {
    return get_entity_transform(entity_id).position;
  }
  Vector3 get_entity_center_of_mass_world_pos(EntityId entity_id) override
  {
    return calculate_entity_center_of_mass(entity_id);
  }
  bool has_center_of_mass_config(EntityId entity_id) override
  {
    return center_of_mass_configs_.find(entity_id) != center_of_mass_configs_.end();
  }
  CenterOfMassConfig get_entity_center_of_mass_config(EntityId entity_id) override
  {
    auto it = center_of_mass_configs_.find(entity_id);
    return (it != center_of_mass_configs_.end()) ? it->second : CenterOfMassConfig();
  }
};

class MockPhysicsManipulator : public IPhysicsManipulator
{
private:
  MockPhysicsDataProvider *data_provider_;
  EntityId next_ghost_id_;
  std::vector<EntityId> created_ghosts_;

public:
  explicit MockPhysicsManipulator(MockPhysicsDataProvider *data_provider)
      : data_provider_(data_provider), next_ghost_id_(10000) {}

  const std::vector<EntityId> &get_created_ghosts() const { return created_ghosts_; }

  EntityId create_chain_node_entity(const ChainNodeCreateDescriptor &descriptor) override
  {
    EntityId node_id = next_ghost_id_++;
    created_ghosts_.push_back(node_id);
    data_provider_->add_mock_entity(node_id, descriptor.target_transform, descriptor.target_physics);
    std::cout << "  MockPhysics: Created chain node entity " << node_id
              << " from source " << descriptor.source_entity_id << std::endl;
    return node_id;
  }

  void destroy_chain_node_entity(EntityId node_entity_id) override
  {
    auto it = std::find(created_ghosts_.begin(), created_ghosts_.end(), node_entity_id);
    if (it != created_ghosts_.end())
    {
      created_ghosts_.erase(it);
      std::cout << "  MockPhysics: Destroyed chain node entity " << node_entity_id << std::endl;
    }
  }

  bool swap_entity_roles_with_faces(EntityId main_entity_id, EntityId ghost_entity_id, PortalFace source_face, PortalFace target_face) override
  {
    std::cout << "  MockPhysics: Swapped roles between " << main_entity_id << " and " << ghost_entity_id << std::endl;

    auto it = std::find(created_ghosts_.begin(), created_ghosts_.end(), ghost_entity_id);
    if (it != created_ghosts_.end())
    {
      created_ghosts_.erase(it);
    }
    created_ghosts_.push_back(main_entity_id);

    return true;
  }

  // --- 其他接口的简化实现 ---
  void set_entity_transform(EntityId entity_id, const Transform &transform) override {}
  void set_entity_physics_state(EntityId entity_id, const PhysicsState &physics_state) override {}
  void set_entity_collision_enabled(EntityId entity_id, bool enabled) override {}
  void set_entity_visible(EntityId entity_id, bool visible) override {}
  void set_entity_velocity(EntityId entity_id, const Vector3 &velocity) override {}
  void set_entity_angular_velocity(EntityId entity_id, const Vector3 &angular_velocity) override {}
  EntityId create_ghost_entity(EntityId source_entity_id, const Transform &ghost_transform, const PhysicsState &ghost_physics) override { return INVALID_ENTITY_ID; }
  EntityId create_full_functional_ghost(const EntityDescription &entity_desc, const Transform &ghost_transform, const PhysicsState &ghost_physics, PortalFace source_face, PortalFace target_face) override { return create_chain_node_entity({}); }
  void destroy_ghost_entity(EntityId ghost_entity_id) override {}
  void update_ghost_entity(EntityId ghost_entity_id, const Transform &transform, const PhysicsState &physics) override {}
  void set_ghost_entity_bounds(EntityId ghost_entity_id, const Vector3 &bounds_min, const Vector3 &bounds_max) override {}
  void sync_ghost_entities(const std::vector<GhostEntitySnapshot> &snapshots) override {}
  void set_entity_clipping_plane(EntityId entity_id, const ClippingPlane &clipping_plane) override {}
  void disable_entity_clipping(EntityId entity_id) override {}
  void set_entities_clipping_states(const std::vector<EntityId> &entity_ids, const std::vector<ClippingPlane> &clipping_planes, const std::vector<bool> &enable_clipping) override {}
  bool swap_entity_roles(EntityId main_entity_id, EntityId ghost_entity_id) override { return false; }
  void set_entity_functional_state(EntityId entity_id, bool is_fully_functional) override {}
  bool copy_all_entity_properties(EntityId source_entity_id, EntityId target_entity_id) override { return true; }
  void set_entity_center_of_mass(EntityId entity_id, const Vector3 &center_offset) override {}
  void set_entity_physics_engine_controlled(EntityId entity_id, bool engine_controlled) override {}
  bool detect_entity_collision_constraints(EntityId entity_id, PhysicsConstraintState &constraint_info) override { return false; }
  void force_set_entity_physics_state(EntityId entity_id, const Transform &transform, const PhysicsState &physics) override {}
  void force_set_entities_physics_states(const std::vector<EntityId> &entity_ids, const std::vector<Transform> &transforms, const std::vector<PhysicsState> &physics_states) override {}
  EntityId create_physics_simulation_proxy(EntityId template_entity_id, const Transform &initial_transform, const PhysicsState &initial_physics) override { return INVALID_ENTITY_ID; }
  void destroy_physics_simulation_proxy(EntityId proxy_entity_id) override {}
  void apply_force_to_proxy(EntityId proxy_entity_id, const Vector3 &force, const Vector3 &application_point) override {}
  void apply_torque_to_proxy(EntityId proxy_entity_id, const Vector3 &torque) override {}
  void clear_forces_on_proxy(EntityId proxy_entity_id) override {}
  void set_proxy_physics_material(EntityId proxy_entity_id, float friction, float restitution, float linear_damping, float angular_damping) override {}
  bool get_entity_applied_forces(EntityId entity_id, Vector3 &total_force, Vector3 &total_torque) override { return false; }
};

class MockEventHandler : public IPortalEventHandler
{
public:
  struct Event
  {
    std::string type;
    EntityId entity_id;
    EntityId ghost_entity_id;
    PortalId portal_id;
  };

  std::vector<Event> events;
  EntityId last_created_ghost_id = INVALID_ENTITY_ID;

  bool on_ghost_entity_created(EntityId main_entity, EntityId ghost_entity, PortalId portal) override
  {
    events.push_back({"ghost_created", main_entity, ghost_entity, portal});
    last_created_ghost_id = ghost_entity;
    std::cout << "  Event: Ghost entity created - Main " << main_entity << " Ghost " << ghost_entity << std::endl;
    return true;
  }

  bool on_ghost_entity_destroyed(EntityId main_entity, EntityId ghost_entity, PortalId portal) override
  {
    events.push_back({"ghost_destroyed", main_entity, ghost_entity, portal});
    std::cout << "  Event: Ghost entity destroyed - Main " << main_entity << " Ghost " << ghost_entity << std::endl;
    return true;
  }

  bool on_entity_roles_swapped(EntityId old_main_entity, EntityId old_ghost_entity, EntityId new_main_entity, EntityId new_ghost_entity,
                               PortalId portal_id, const Transform &main_transform, const Transform &ghost_transform) override
  {
    events.push_back({"roles_swapped", old_main_entity, new_main_entity, portal_id});
    std::cout << "  Event: Entity roles swapped - Old main " << old_main_entity << " -> New main " << new_main_entity << std::endl;
    return true;
  }

  // --- 其他接口的简化实现 ---
  bool on_entity_teleport_begin(EntityId entity_id, PortalId from_portal, PortalId to_portal) override { return true; }
  bool on_entity_teleport_complete(EntityId entity_id, PortalId from_portal, PortalId to_portal) override { return true; }
};

// === 单元测试主函数 ===

void test_chain_teleport_four_portals()
{
  std::cout << "\n=== 链式传送测试：实体依次穿越四个传送门 (交错事件序列 + 断言) ===" << std::endl;

  // === 1. 初始化系统 ===
  auto mock_data_provider = std::make_unique<MockPhysicsDataProvider>();
  auto mock_manipulator = std::make_unique<MockPhysicsManipulator>(mock_data_provider.get());
  auto mock_event_handler = std::make_unique<MockEventHandler>();

  MockPhysicsDataProvider *data_provider_ptr = mock_data_provider.get();
  MockPhysicsManipulator *manipulator_ptr = mock_manipulator.get();
  MockEventHandler *event_handler_ptr = mock_event_handler.get();

  PortalInterfaces interfaces;
  interfaces.physics_data = mock_data_provider.release();
  interfaces.physics_manipulator = mock_manipulator.release();
  interfaces.event_handler = mock_event_handler.release();

  PortalManager manager(interfaces);
  assert(manager.initialize());

  // === 2. 创建4对传送门 ===
  // ** FIX: Reverted to explicit member assignment for PortalPlane creation **
  PortalPlane plane1;
  plane1.center = Vector3(10, 0, 0);
  plane1.normal = Vector3(1, 0, 0);
  PortalId portal1 = manager.create_portal(plane1);
  PortalPlane plane2;
  plane2.center = Vector3(20, 0, 0);
  plane2.normal = Vector3(-1, 0, 0);
  PortalId portal2 = manager.create_portal(plane2);
  PortalPlane plane3;
  plane3.center = Vector3(30, 0, 0);
  plane3.normal = Vector3(1, 0, 0);
  PortalId portal3 = manager.create_portal(plane3);
  PortalPlane plane4;
  plane4.center = Vector3(40, 0, 0);
  plane4.normal = Vector3(-1, 0, 0);
  PortalId portal4 = manager.create_portal(plane4);
  PortalPlane plane5;
  plane5.center = Vector3(50, 0, 0);
  plane5.normal = Vector3(1, 0, 0);
  PortalId portal5 = manager.create_portal(plane5);
  PortalPlane plane6;
  plane6.center = Vector3(60, 0, 0);
  plane6.normal = Vector3(-1, 0, 0);
  PortalId portal6 = manager.create_portal(plane6);
  PortalPlane plane7;
  plane7.center = Vector3(70, 0, 0);
  plane7.normal = Vector3(1, 0, 0);
  PortalId portal7 = manager.create_portal(plane7);
  PortalPlane plane8;
  plane8.center = Vector3(80, 0, 0);
  plane8.normal = Vector3(-1, 0, 0);
  PortalId portal8 = manager.create_portal(plane8);

  assert(manager.link_portals(portal1, portal2));
  assert(manager.link_portals(portal3, portal4));
  assert(manager.link_portals(portal5, portal6));
  assert(manager.link_portals(portal7, portal8));

  // === 3. 创建测试实体 ===
  EntityId test_entity = 1001;
  // ** FIX: Reverted to explicit member assignment for Transform and PhysicsState **
  Transform initial_transform;
  initial_transform.position = Vector3(0, 0, 0);
  PhysicsState initial_physics;
  initial_physics.linear_velocity = Vector3(1, 0, 0);

  data_provider_ptr->add_mock_entity(test_entity, initial_transform, initial_physics);
  manager.register_entity(test_entity);

  // === 4. 执行交错序列的链式传送测试 ===
  test_interleaved_chain_sequence_with_assertions(manager, manipulator_ptr, event_handler_ptr,
                                                  portal1, portal3, portal5, portal7);

  // === 5. 最终状态验证 ===
  std::cout << "\n=== 测试结束，最终状态验证 ===" << std::endl;
  std::cout << "DEBUG: Final ghost count: " << manipulator_ptr->get_created_ghosts().size() << std::endl;
  std::cout << "DEBUG: Total events: " << event_handler_ptr->events.size() << std::endl;
  
  // Debug: Print all events
  for (size_t i = 0; i < event_handler_ptr->events.size(); ++i) {
    std::cout << "DEBUG: Event " << i+1 << ": " << event_handler_ptr->events[i].type << std::endl;
  }
  
  assert(manipulator_ptr->get_created_ghosts().empty()); // 确保所有幽灵都已被销毁
  assert(event_handler_ptr->events.size() == 12);        // 确保触发了12个核心事件
  std::cout << "- 验证通过: 所有幽灵实体都已销毁。" << std::endl;
  std::cout << "- 验证通过: 共触发了12次核心链式事件。" << std::endl;

  // === 6. 清理 ===
  manager.shutdown();
  delete interfaces.physics_data;
  delete interfaces.physics_manipulator;
  delete interfaces.event_handler;

  std::cout << "\n=== 链式传送测试完成 ===" << std::endl;
}

// === 交错事件序列的链式传送测试 (带断言) ===
void test_interleaved_chain_sequence_with_assertions(PortalManager &manager,
                                                     MockPhysicsManipulator *manipulator_ptr,
                                                     MockEventHandler *event_handler_ptr,
                                                     PortalId p1, PortalId p3,
                                                     PortalId p5, PortalId p7)
{
  std::cout << "\n=== Testing Interleaved 12-Step Chain Teleport Sequence with Assertions ===\n"
            << std::endl;

  const EntityId original_entity = 1001;
  EntityId ghost1 = INVALID_ENTITY_ID, ghost2 = INVALID_ENTITY_ID,
           ghost3 = INVALID_ENTITY_ID, ghost4 = INVALID_ENTITY_ID;
  EntityId current_main = original_entity;

  // 步骤 1: 原始实体与 P1 相交 -> 创建幽灵1
  std::cout << "--- Step 1: Main(1001) intersects P1 ---" << std::endl;
  manager.on_entity_intersect_portal_start(current_main, p1);
  manager.update(0.016f);
  
  std::cout << "DEBUG: Checking manipulator_ptr..." << std::endl;
  if (!manipulator_ptr) {
    std::cout << "ERROR: manipulator_ptr is null!" << std::endl;
    return;
  }
  std::cout << "DEBUG: manipulator_ptr is valid" << std::endl;
  
  std::cout << "DEBUG: Getting created ghosts size..." << std::endl;
  size_t ghost_count = manipulator_ptr->get_created_ghosts().size();
  std::cout << "DEBUG: Ghost count: " << ghost_count << std::endl;
  
  assert(ghost_count == 1 && "Step 1 Failed: Ghost count should be 1.");
  
  std::cout << "DEBUG: Checking event_handler_ptr..." << std::endl;
  if (!event_handler_ptr) {
    std::cout << "ERROR: event_handler_ptr is null!" << std::endl;
    return;
  }
  std::cout << "DEBUG: event_handler_ptr is valid" << std::endl;
  
  std::cout << "DEBUG: Checking events vector..." << std::endl;
  if (event_handler_ptr->events.empty()) {
    std::cout << "ERROR: events vector is empty!" << std::endl;
    return;
  }
  std::cout << "DEBUG: Events count: " << event_handler_ptr->events.size() << std::endl;
  std::cout << "DEBUG: Last event type: " << event_handler_ptr->events.back().type << std::endl;
  
  assert(event_handler_ptr->events.back().type == "ghost_created" && "Step 1 Failed: Event should be ghost_created.");
  ghost1 = event_handler_ptr->last_created_ghost_id;
  std::cout << "DEBUG: Step 1 completed successfully" << std::endl;
  
  // Debug: Print ghost list after each step
  std::cout << "DEBUG: Ghost list after step 1: ";
  for (auto id : manipulator_ptr->get_created_ghosts()) {
    std::cout << id << " ";
  }
  std::cout << std::endl;

  // 步骤 2: 原始实体质心穿过 P1 -> 角色交换，幽灵1成为主体
  std::cout << "\n--- Step 2: Main(1001) crosses P1 ---" << std::endl;
  manager.on_entity_center_crossed_portal(current_main, p1, PortalFace::A);
  manager.update(0.016f);
  assert(manipulator_ptr->get_created_ghosts().size() == 1 && "Step 2 Failed: Ghost count should still be 1.");
  assert(event_handler_ptr->events.back().type == "roles_swapped" && "Step 2 Failed: Event should be roles_swapped.");
  current_main = ghost1; // 主体现在是幽灵1
  
  // Debug: Print ghost list after step 2
  std::cout << "DEBUG: Ghost list after step 2: ";
  for (auto id : manipulator_ptr->get_created_ghosts()) {
    std::cout << id << " ";
  }
  std::cout << std::endl;

  // 步骤 3: 新主体(幽灵1)与 P3 相交 -> 创建幽灵2
  std::cout << "\n--- Step 3: Main(" << current_main << ") intersects P3 ---" << std::endl;
  manager.on_entity_intersect_portal_start(current_main, p3);
  manager.update(0.016f);
  assert(manipulator_ptr->get_created_ghosts().size() == 2 && "Step 3 Failed: Ghost count should be 2.");
  assert(event_handler_ptr->events.back().type == "ghost_created" && "Step 3 Failed: Event should be ghost_created.");
  ghost2 = event_handler_ptr->last_created_ghost_id;
  
  // Debug: Print ghost list after step 3
  std::cout << "DEBUG: Ghost list after step 3: ";
  for (auto id : manipulator_ptr->get_created_ghosts()) {
    std::cout << id << " ";
  }
  std::cout << std::endl;

  // 步骤 4: 原始实体(现在是链尾)离开 P1 -> 销毁链尾
  std::cout << "\n--- Step 4: Tail(1001) exits P1 ---" << std::endl;
  manager.on_entity_exit_portal(original_entity, p1);
  manager.update(0.016f);
  
  // Debug: Print ghost list before step 4 assertion
  std::cout << "DEBUG: Ghost list before step 4 assertion: ";
  for (auto id : manipulator_ptr->get_created_ghosts()) {
    std::cout << id << " ";
  }
  std::cout << std::endl;
  
  assert(manipulator_ptr->get_created_ghosts().size() == 1 && "Step 4 Failed: Ghost count should be 1.");
  assert(event_handler_ptr->events.back().type == "ghost_destroyed" && "Step 4 Failed: Event should be ghost_destroyed.");

  // 步骤 5: 主体(幽灵1)质心穿过 P3 -> 角色交换，幽灵2成为主体
  std::cout << "\n--- Step 5: Main(" << current_main << ") crosses P3 ---" << std::endl;
  manager.on_entity_center_crossed_portal(current_main, p3, PortalFace::A);
  manager.update(0.016f);
  assert(manipulator_ptr->get_created_ghosts().size() == 1 && "Step 5 Failed: Ghost count should still be 1.");
  assert(event_handler_ptr->events.back().type == "roles_swapped" && "Step 5 Failed: Event should be roles_swapped.");
  current_main = ghost2; // 主体现在是幽灵2

  // 步骤 6: 新主体(幽灵2)与 P5 相交 -> 创建幽灵3
  std::cout << "\n--- Step 6: Main(" << current_main << ") intersects P5 ---" << std::endl;
  manager.on_entity_intersect_portal_start(current_main, p5);
  manager.update(0.016f);
  assert(manipulator_ptr->get_created_ghosts().size() == 2 && "Step 6 Failed: Ghost count should be 2.");
  assert(event_handler_ptr->events.back().type == "ghost_created" && "Step 6 Failed: Event should be ghost_created.");
  ghost3 = event_handler_ptr->last_created_ghost_id;

  // 步骤 7: 新主体(幽灵3)与 P7 相交 -> 创建幽灵4
  std::cout << "\n--- Step 7: Main(" << current_main << ")'s ghost intersects P7 ---" << std::endl;
  manager.on_entity_intersect_portal_start(ghost3, p7); // 注意：此时幽灵3还不是主体，是它在与P7相交
  manager.update(0.016f);
  assert(manipulator_ptr->get_created_ghosts().size() == 3 && "Step 7 Failed: Ghost count should be 3.");
  assert(event_handler_ptr->events.back().type == "ghost_created" && "Step 7 Failed: Event should be ghost_created.");
  ghost4 = event_handler_ptr->last_created_ghost_id;

  // 步骤 8: 主体(幽灵2)质心穿过 P5 -> 角色交换, 幽灵3成为主体
  std::cout << "\n--- Step 8: Main(" << current_main << ") crosses P5 ---" << std::endl;
  manager.on_entity_center_crossed_portal(current_main, p5, PortalFace::A);
  manager.update(0.016f);
  assert(manipulator_ptr->get_created_ghosts().size() == 3 && "Step 8 Failed: Ghost count should still be 3.");
  assert(event_handler_ptr->events.back().type == "roles_swapped" && "Step 8 Failed: Event should be roles_swapped.");
  current_main = ghost3;

  // 步骤 9: 链尾(幽灵1)离开 P3 -> 销毁链尾
  std::cout << "\n--- Step 9: Tail(" << ghost1 << ") exits P3 ---" << std::endl;
  manager.on_entity_exit_portal(ghost1, p3);
  manager.update(0.016f);
  assert(manipulator_ptr->get_created_ghosts().size() == 2 && "Step 9 Failed: Ghost count should be 2.");
  assert(event_handler_ptr->events.back().type == "ghost_destroyed" && "Step 9 Failed: Event should be ghost_destroyed.");

  // 步骤 10: 主体(幽灵3)质心穿过 P7 -> 角色交换, 幽灵4成为主体
  std::cout << "\n--- Step 10: Main(" << current_main << ") crosses P7 ---" << std::endl;
  manager.on_entity_center_crossed_portal(current_main, p7, PortalFace::A);
  manager.update(0.016f);
  assert(manipulator_ptr->get_created_ghosts().size() == 2 && "Step 10 Failed: Ghost count should still be 2.");
  assert(event_handler_ptr->events.back().type == "roles_swapped" && "Step 10 Failed: Event should be roles_swapped.");
  current_main = ghost4;

  // 步骤 11: 链尾(幽灵2)离开 P5 -> 销毁链尾
  std::cout << "\n--- Step 11: Tail(" << ghost2 << ") exits P5 ---" << std::endl;
  manager.on_entity_exit_portal(ghost2, p5);
  manager.update(0.016f);
  assert(manipulator_ptr->get_created_ghosts().size() == 1 && "Step 11 Failed: Ghost count should be 1.");
  assert(event_handler_ptr->events.back().type == "ghost_destroyed" && "Step 11 Failed: Event should be ghost_destroyed.");

  // 步骤 12: 最后的链尾(幽灵3)离开 P7 -> 销毁链尾, 链条结束
  std::cout << "\n--- Step 12: Tail(" << ghost3 << ") exits P7 ---" << std::endl;
  manager.on_entity_exit_portal(ghost3, p7);
  manager.update(0.016f);
  assert(manipulator_ptr->get_created_ghosts().empty() && "Step 12 Failed: Ghost count should be 0.");
  assert(event_handler_ptr->events.back().type == "ghost_destroyed" && "Step 12 Failed: Event should be ghost_destroyed.");
}

int main()
{
  try
  {
    test_chain_teleport_four_portals();
    std::cout << "\n🎉 所有测试通过！" << std::endl;
    return 0;
  }
  catch (const std::exception &e)
  {
    std::cerr << "❌ 测试失败: " << e.what() << std::endl;
    return 1;
  }
  catch (...)
  {
    std::cerr << "❌ 测试失败: 未知错误" << std::endl;
    return 1;
  }
}