#include "../lib/include/portal.h"
#include "../lib/include/portal_example.h"
#include <iostream>
#include <iomanip>
#include <thread>
#include <chrono>

using namespace Portal;
using namespace Portal::Example;

void print_vector3(const std::string &name, const Vector3 &v)
{
  std::cout << name << ": ("
            << std::fixed << std::setprecision(3)
            << v.x << ", " << v.y << ", " << v.z << ")" << std::endl;
}

void print_entity_description(const EntityDescription &desc)
{
  std::cout << "Entity " << desc.entity_id << ":" << std::endl;
  std::cout << "  Type: " << (desc.entity_type == EntityType::MAIN ? "MAIN" : 
                             desc.entity_type == EntityType::GHOST ? "GHOST" : "HYBRID") << std::endl;
  print_vector3("  Position", desc.transform.position);
  print_vector3("  Center of Mass", desc.center_of_mass);
  std::cout << "  Fully Functional: " << (desc.is_fully_functional ? "Yes" : "No") << std::endl;
  std::cout << "  Counterpart ID: " << desc.counterpart_id << std::endl;
}

void test_center_crossing_detection()
{
  std::cout << "\n=== 測試質心穿越檢測 ===" << std::endl;
  
  // 創建物理查詢接口
  ExamplePhysicsQuery physics_query;
  
  // 添加測試實體
  EntityId entity_id = 100;
  Transform entity_transform;
  entity_transform.position = Vector3(-2.0f, 0, 0); // 開始在傳送門左側
  PhysicsState physics_state;
  
  physics_query.add_test_entity(entity_id, entity_transform);
  
  // 創建傳送門平面
  PortalPlane portal_plane;
  portal_plane.center = Vector3(0, 0, 0);
  portal_plane.normal = Vector3(1, 0, 0); // 指向+X方向
  portal_plane.right = Vector3(0, 0, 1);
  portal_plane.up = Vector3(0, 1, 0);
  
  std::cout << "傳送門位置: (0, 0, 0), 法向量: (1, 0, 0)" << std::endl;
  std::cout << "實體初始位置: (-2, 0, 0)" << std::endl;
  
  // 模擬實體移動穿過傳送門
  for (int step = 0; step < 10; ++step) {
    float x_pos = -2.0f + step * 0.5f;
    entity_transform.position.x = x_pos;
    physics_query.update_entity_transform(entity_id, entity_transform);
    
    // 檢測質心穿越
    CenterOfMassCrossing crossing = physics_query.check_center_crossing(
      entity_id, portal_plane, PortalFace::A);
    
    std::cout << "Step " << step << ": Entity X=" << x_pos 
              << ", Crossing Progress=" << crossing.crossing_progress
              << ", Just Started=" << (crossing.just_started ? "YES" : "NO") << std::endl;
    
    if (crossing.just_started) {
      std::cout << "  *** 檢測到質心開始穿越！***" << std::endl;
    }
  }
}

void test_seamless_teleport_flow()
{
  std::cout << "\n=== 測試完整無縫傳送流程 ===" << std::endl;
  
  // 創建所有必需的接口
  ExamplePhysicsQuery physics_query;
  ExamplePhysicsManipulator physics_manipulator(&physics_query);
  ExampleRenderQuery render_query;
  ExampleRenderManipulator render_manipulator;
  
  // 設置主機接口
  HostInterfaces interfaces;
  interfaces.physics_query = &physics_query;
  interfaces.physics_manipulator = &physics_manipulator;
  interfaces.render_query = &render_query;
  interfaces.render_manipulator = &render_manipulator;
  
  // 創建Portal管理器
  PortalManager portal_manager(interfaces);
  if (!portal_manager.initialize()) {
    std::cout << "Failed to initialize portal manager" << std::endl;
    return;
  }
  
  // 創建兩個傳送門
  PortalPlane portal1_plane;
  portal1_plane.center = Vector3(0, 0, 0);
  portal1_plane.normal = Vector3(1, 0, 0);
  portal1_plane.up = Vector3(0, 1, 0);
  portal1_plane.right = Vector3(0, 0, 1);
  
  PortalPlane portal2_plane;
  portal2_plane.center = Vector3(10, 0, 0);
  portal2_plane.normal = Vector3(-1, 0, 0);
  portal2_plane.up = Vector3(0, 1, 0);
  portal2_plane.right = Vector3(0, 0, 1);
  
  auto portal1_id = portal_manager.create_portal(portal1_plane);
  auto portal2_id = portal_manager.create_portal(portal2_plane);  std::cout << "創建傳送門 " << portal1_id << " 在 (0, 0, 0)" << std::endl;
  std::cout << "創建傳送門 " << portal2_id << " 在 (10, 0, 0)" << std::endl;
  
  // 鏈接傳送門
  if (portal_manager.link_portals(portal1_id, portal2_id)) {
    std::cout << "成功鏈接傳送門" << std::endl;
  }
  
  // 註冊並設置測試實體
  EntityId entity_id = 12345;
  
  Transform entity_transform;
  entity_transform.position = Vector3(-3.0f, 0, 0); // 開始位置在傳送門左側
  PhysicsState entity_physics;
  entity_physics.linear_velocity = Vector3(2.0f, 0, 0); // 向右移動

  // 先添加實體到物理查詢中（這很重要！）
  physics_query.add_test_entity(entity_id, entity_transform);
  
  // 然後註冊到 portal manager
  portal_manager.register_entity(entity_id);
  
  std::cout << "註冊實體 " << entity_id << " 在初始位置 (-3, 0, 0)" << std::endl;  // 模擬幀更新循環
  float delta_time = 1.0f / 60.0f; // 60 FPS
  
  for (int frame = 0; frame < 120; ++frame) { // 2秒模擬
    // 獲取實體當前的實際位置（可能已被Portal系統修改）
    entity_transform = physics_query.get_entity_transform(entity_id);
    
    // 更新實體位置（簡單的物理模擬）
    entity_transform.position.x += entity_physics.linear_velocity.x * delta_time;
    physics_query.update_entity_transform(entity_id, entity_transform);
    
    // 更新Portal系統
    portal_manager.update(delta_time);
    
    // 每10幀輸出一次狀態
    if (frame % 10 == 0) {
      std::cout << "Frame " << frame << ": Entity at ("
                << entity_transform.position.x << ", "
                << entity_transform.position.y << ", "
                << entity_transform.position.z << ")" << std::endl;
      
      // 檢查是否有活躍的傳送狀態
      if (portal_manager.get_teleporting_entity_count() > 0) {
        std::cout << "  -> 檢測到 " << portal_manager.get_teleporting_entity_count() 
                  << " 個實體正在傳送" << std::endl;
      }
    }
    
    // 模擬幀時間
    std::this_thread::sleep_for(std::chrono::milliseconds(16));
  }
  
  std::cout << "模擬完成。最終實體位置: (" << entity_transform.position.x 
            << ", " << entity_transform.position.y << ", " << entity_transform.position.z << ")" << std::endl;
}

void test_ab_face_seamless_teleport()
{
  std::cout << "\n=== 測試A/B面無縫傳送 ===" << std::endl;
  
  ExamplePhysicsQuery physics_query;
  ExamplePhysicsManipulator physics_manipulator(&physics_query);
  ExampleRenderQuery render_query;
  ExampleRenderManipulator render_manipulator;
  
  HostInterfaces interfaces;
  interfaces.physics_query = &physics_query;
  interfaces.physics_manipulator = &physics_manipulator;
  interfaces.render_query = &render_query;
  interfaces.render_manipulator = &render_manipulator;
  
  PortalManager portal_manager(interfaces);
  portal_manager.initialize();
  
  // 創建傳送門
  // 創建傳送門
  PortalPlane portal_plane;
  portal_plane.center = Vector3(0, 0, 0);
  portal_plane.normal = Vector3(1, 0, 0);
  portal_plane.up = Vector3(0, 1, 0);
  portal_plane.right = Vector3(0, 0, 1);
  
  auto portal_id = portal_manager.create_portal(portal_plane);
  EntityId entity_id = 999;
  portal_manager.register_entity(entity_id);
  
  // 測試從A面接近
  std::cout << "測試從A面接近傳送門..." << std::endl;
  Transform transform_a;
  transform_a.position = Vector3(-1.0f, 0, 0);
  physics_query.add_test_entity(entity_id, transform_a);
  
  // 模擬質心檢測
  // 獲取傳送門平面
  const ::Portal::Portal *portal = portal_manager.get_portal(portal_id);
  if (!portal) {
    std::cout << "錯誤：無法獲取傳送門" << std::endl;
    return;
  }
  PortalPlane plane = portal->get_plane();
  
  CenterOfMassCrossing crossing_a = physics_query.check_center_crossing(entity_id, plane, PortalFace::A);
  std::cout << "A面穿越檢測: progress=" << crossing_a.crossing_progress 
            << ", started=" << (crossing_a.just_started ? "YES" : "NO") << std::endl;
  
  // 測試從B面接近
  std::cout << "測試從B面接近傳送門..." << std::endl;
  transform_a.position = Vector3(1.0f, 0, 0);
  physics_query.update_entity_transform(entity_id, transform_a);
  
  CenterOfMassCrossing crossing_b = physics_query.check_center_crossing(entity_id, plane, PortalFace::B);
  std::cout << "B面穿越檢測: progress=" << crossing_b.crossing_progress 
            << ", started=" << (crossing_b.just_started ? "YES" : "NO") << std::endl;
}

void test_entity_description()
{
  std::cout << "\n=== 測試實體描述功能 ===" << std::endl;
  
  ExamplePhysicsQuery physics_query;
  
  EntityId entity_id = 777;
  Transform transform;
  transform.position = Vector3(5, 2, -1);
  PhysicsState physics;
  physics.linear_velocity = Vector3(1, 0, 0);
  
  physics_query.add_test_entity(entity_id, transform);
  
  EntityDescription desc = physics_query.get_entity_description(entity_id);
  
  std::cout << "實體描述測試:" << std::endl;
  print_entity_description(desc);
}

int main()
{
  std::cout << "=== 無縫傳送系統測試 ===" << std::endl;
  std::cout << "測試包含：質心檢測、實體交換、A/B面支援" << std::endl;
  
  // 運行所有測試
  test_entity_description();
  test_center_crossing_detection();
  test_ab_face_seamless_teleport();
  test_seamless_teleport_flow();
  
  std::cout << "\n=== 所有測試完成 ===" << std::endl;
  
  return 0;
}
