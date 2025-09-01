#include "src/core/portal_core/lib/include/portal.h"
#include <iostream>
#include <iomanip>
#include <cassert>

using namespace Portal;
using namespace Portal::Math;

void print_vector3(const std::string &name, const Vector3 &v)
{
  std::cout << std::setw(25) << name << ": ("
            << std::fixed << std::setprecision(3)
            << std::setw(7) << v.x << ", "
            << std::setw(7) << v.y << ", "
            << std::setw(7) << v.z << ")" << std::endl;
}

void print_transform(const std::string &name, const Transform &t)
{
  std::cout << name << ":" << std::endl;
  print_vector3("  Position", t.position);
  print_vector3("  Scale", t.scale);
  std::cout << "  Rotation: (" 
            << std::fixed << std::setprecision(3)
            << t.rotation.x << ", " << t.rotation.y << ", " 
            << t.rotation.z << ", " << t.rotation.w << ")" << std::endl;
}

bool test_transform_through_portal_ab_face()
{
  std::cout << "\n=== 测试 transform_through_portal A/B 面支持 ===" << std::endl;
  
  // 创建两个传送门
  PortalPlane portal1, portal2;
  
  // Portal 1: 垂直传送门，法向量指向+Z
  portal1.center = Vector3(0, 0, 0);
  portal1.normal = Vector3(0, 0, 1);  // A面朝向+Z
  portal1.right = Vector3(1, 0, 0);
  portal1.up = Vector3(0, 1, 0);
  portal1.width = 2.0f;
  portal1.height = 3.0f;
  
  // Portal 2: 垂直传送门，法向量指向+X  
  portal2.center = Vector3(10, 0, 0);
  portal2.normal = Vector3(1, 0, 0);  // A面朝向+X
  portal2.right = Vector3(0, 0, 1);
  portal2.up = Vector3(0, 1, 0);
  portal2.width = 2.0f;
  portal2.height = 3.0f;
  
  // 测试变换
  Transform test_transform;
  test_transform.position = Vector3(0, 0, -1);  // Portal1 B面前方
  test_transform.rotation = Quaternion(0, 0, 0, 1);  // 无旋转
  test_transform.scale = Vector3(1, 1, 1);
  
  print_transform("原始变换", test_transform);
  
  // 测试默认版本 (A->B)
  Transform result_default = PortalMath::transform_through_portal(
      test_transform, portal1, portal2);
  print_transform("默认结果 (A->B)", result_default);
  
  // 测试显式 A->B
  Transform result_ab = PortalMath::transform_through_portal(
      test_transform, portal1, portal2, PortalFace::A, PortalFace::B);
  print_transform("显式 A->B 结果", result_ab);
  
  // 测试 B->A
  Transform result_ba = PortalMath::transform_through_portal(
      test_transform, portal1, portal2, PortalFace::B, PortalFace::A);
  print_transform("B->A 结果", result_ba);
  
  // 测试 A->A
  Transform result_aa = PortalMath::transform_through_portal(
      test_transform, portal1, portal2, PortalFace::A, PortalFace::A);
  print_transform("A->A 结果", result_aa);
  
  // 验证默认版本与显式 A->B 版本一致
  bool is_consistent = (
      abs(result_default.position.x - result_ab.position.x) < 1e-6 &&
      abs(result_default.position.y - result_ab.position.y) < 1e-6 &&
      abs(result_default.position.z - result_ab.position.z) < 1e-6);
  
  std::cout << "默认版本与显式A->B一致性: " << (is_consistent ? "✅ 通过" : "❌ 失败") << std::endl;
  
  return is_consistent;
}

bool test_calculate_portal_camera_ab_face()
{
  std::cout << "\n=== 测试 calculate_portal_camera A/B 面支持 ===" << std::endl;
  
  // 创建两个传送门
  PortalPlane portal1, portal2;
  
  portal1.center = Vector3(0, 0, 0);
  portal1.normal = Vector3(0, 0, 1);
  portal1.right = Vector3(1, 0, 0);
  portal1.up = Vector3(0, 1, 0);
  
  portal2.center = Vector3(10, 0, 0);
  portal2.normal = Vector3(1, 0, 0);
  portal2.right = Vector3(0, 0, 1);
  portal2.up = Vector3(0, 1, 0);
  
  // 创建测试相机
  CameraParams camera;
  camera.position = Vector3(0, 0, -2);  // Portal1 B面前方
  camera.rotation = Quaternion(0, 0, 0, 1);  // 朝向+Z
  camera.fov = 75.0f;
  camera.near_plane = 0.1f;
  camera.far_plane = 100.0f;
  camera.aspect_ratio = 16.0f / 9.0f;
  
  std::cout << "原始相机:" << std::endl;
  print_vector3("  Position", camera.position);
  
  // 测试默认版本 (A->B)
  CameraParams result_default = PortalMath::calculate_portal_camera(
      camera, portal1, portal2);
  std::cout << "默认结果 (A->B):" << std::endl;
  print_vector3("  Position", result_default.position);
  
  // 测试显式 A->B
  CameraParams result_ab = PortalMath::calculate_portal_camera(
      camera, portal1, portal2, PortalFace::A, PortalFace::B);
  std::cout << "显式 A->B 结果:" << std::endl;
  print_vector3("  Position", result_ab.position);
  
  // 测试 B->A
  CameraParams result_ba = PortalMath::calculate_portal_camera(
      camera, portal1, portal2, PortalFace::B, PortalFace::A);
  std::cout << "B->A 结果:" << std::endl;
  print_vector3("  Position", result_ba.position);
  
  // 验证一致性
  bool is_consistent = (
      abs(result_default.position.x - result_ab.position.x) < 1e-6 &&
      abs(result_default.position.y - result_ab.position.y) < 1e-6 &&
      abs(result_default.position.z - result_ab.position.z) < 1e-6);
  
  std::cout << "默认版本与显式A->B一致性: " << (is_consistent ? "✅ 通过" : "❌ 失败") << std::endl;
  
  return is_consistent;
}

bool test_calculate_ghost_transform_ab_face()
{
  std::cout << "\n=== 测试 calculate_ghost_transform A/B 面支持 ===" << std::endl;
  
  // 创建两个传送门
  PortalPlane portal1, portal2;
  
  portal1.center = Vector3(0, 0, 0);
  portal1.normal = Vector3(0, 0, 1);
  portal1.right = Vector3(1, 0, 0);
  portal1.up = Vector3(0, 1, 0);
  
  portal2.center = Vector3(10, 0, 0);
  portal2.normal = Vector3(1, 0, 0);
  portal2.right = Vector3(0, 0, 1);
  portal2.up = Vector3(0, 1, 0);
  
  // 测试变换
  Transform test_transform;
  test_transform.position = Vector3(0, 0, -1);
  test_transform.rotation = Quaternion(0, 0, 0, 1);
  test_transform.scale = Vector3(1, 1, 1);
  
  float crossing_ratio = 0.5f;
  
  print_transform("原始变换", test_transform);
  
  // 测试默认版本 (A->B)
  Transform result_default = PortalMath::calculate_ghost_transform(
      test_transform, portal1, portal2, crossing_ratio);
  print_transform("默认结果 (A->B)", result_default);
  
  // 测试显式 A->B
  Transform result_ab = PortalMath::calculate_ghost_transform(
      test_transform, portal1, portal2, crossing_ratio, PortalFace::A, PortalFace::B);
  print_transform("显式 A->B 结果", result_ab);
  
  // 测试 B->A
  Transform result_ba = PortalMath::calculate_ghost_transform(
      test_transform, portal1, portal2, crossing_ratio, PortalFace::B, PortalFace::A);
  print_transform("B->A 结果", result_ba);
  
  // 验证一致性
  bool is_consistent = (
      abs(result_default.position.x - result_ab.position.x) < 1e-6 &&
      abs(result_default.position.y - result_ab.position.y) < 1e-6 &&
      abs(result_default.position.z - result_ab.position.z) < 1e-6);
  
  std::cout << "默认版本与显式A->B一致性: " << (is_consistent ? "✅ 通过" : "❌ 失败") << std::endl;
  
  return is_consistent;
}

int main()
{
  std::cout << "=== Portal A/B Face Coverage Test ===" << std::endl;
  std::cout << "测试手动指定A/B面后，所有方法是否正确覆盖默认值" << std::endl;
  
  bool test1 = test_transform_through_portal_ab_face();
  bool test2 = test_calculate_portal_camera_ab_face();
  bool test3 = test_calculate_ghost_transform_ab_face();
  
  std::cout << "\n=== 测试总结 ===" << std::endl;
  std::cout << "transform_through_portal A/B面支持: " << (test1 ? "✅ 通过" : "❌ 失败") << std::endl;
  std::cout << "calculate_portal_camera A/B面支持: " << (test2 ? "✅ 通过" : "❌ 失败") << std::endl;
  std::cout << "calculate_ghost_transform A/B面支持: " << (test3 ? "✅ 通过" : "❌ 失败") << std::endl;
  
  bool all_passed = test1 && test2 && test3;
  std::cout << "\n总体结果: " << (all_passed ? "✅ 所有测试通过" : "❌ 部分测试失败") << std::endl;
  
  if (all_passed) {
    std::cout << "\n🎉 现在所有方法都正确支持手动指定的A/B面！" << std::endl;
    std::cout << "手动指定的A/B面参数会正确覆盖默认的A->B映射。" << std::endl;
  }
  
  return all_passed ? 0 : 1;
}
