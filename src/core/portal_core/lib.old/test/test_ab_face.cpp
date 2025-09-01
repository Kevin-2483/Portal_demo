#include "../lib/include/portal.h"
#include <iostream>
#include <iomanip>

using namespace Portal;
using namespace Portal::Math;

void print_vector3(const std::string &name, const Vector3 &v)
{
  std::cout << name << ": ("
            << std::fixed << std::setprecision(2)
            << v.x << ", " << v.y << ", " << v.z << ")" << std::endl;
}

int main()
{
  std::cout << "=== Portal A/B Face Correspondence Test ===" << std::endl;

  // 创建两个传送门
  PortalPlane portal1, portal2;

  // Portal 1: 在原点，法向量指向+Z (A面朝向+Z，B面朝向-Z)
  portal1.center = Vector3(0, 0, 0);
  portal1.normal = Vector3(0, 0, 1); // A面法向量
  portal1.right = Vector3(1, 0, 0);
  portal1.up = Vector3(0, 1, 0);

  // Portal 2: 在(10, 0, 0)，法向量指向-X (A面朝向-X，B面朝向+X)
  portal2.center = Vector3(10, 0, 0);
  portal2.normal = Vector3(-1, 0, 0); // A面法向量
  portal2.right = Vector3(0, 0, 1);   // 右方向调整以匹配坐标系
  portal2.up = Vector3(0, 1, 0);

  std::cout << "\nPortal 1:" << std::endl;
  print_vector3("  Center", portal1.center);
  print_vector3("  Normal (A face)", portal1.normal);
  print_vector3("  B face normal", portal1.get_face_normal(PortalFace::B));

  std::cout << "\nPortal 2:" << std::endl;
  print_vector3("  Center", portal2.center);
  print_vector3("  Normal (A face)", portal2.normal);
  print_vector3("  B face normal", portal2.get_face_normal(PortalFace::B));

  // 测试点传送
  Vector3 test_point(0, 0, -1); // Portal 1的B面前方

  std::cout << "\n=== Test Point Transformation ===" << std::endl;
  print_vector3("Original point", test_point);

  // 测试 B面 -> A面 (从Portal1的B面进入，从Portal2的A面出现)
  Vector3 result_ba = PortalMath::transform_point_through_portal(
      test_point, portal1, portal2, PortalFace::B, PortalFace::A);
  print_vector3("B->A result", result_ba);

  // 测试 A面 -> B面 (从Portal1的A面进入，从Portal2的B面出现)
  Vector3 test_point2(0, 0, 1); // Portal 1的A面前方
  Vector3 result_ab = PortalMath::transform_point_through_portal(
      test_point2, portal1, portal2, PortalFace::A, PortalFace::B);
  print_vector3("A->B test point", test_point2);
  print_vector3("A->B result", result_ab);

  // 测试默认行为（A->B）
  Vector3 result_default = PortalMath::transform_point_through_portal(
      test_point2, portal1, portal2);
  print_vector3("Default (A->B) result", result_default);

  // 验证结果是否一致
  bool consistency_check = (result_ab.x == result_default.x &&
                            result_ab.y == result_default.y &&
                            result_ab.z == result_default.z);

  std::cout << "\n=== Consistency Check ===" << std::endl;
  std::cout << "A->B explicit vs default: " << (consistency_check ? "PASS" : "FAIL") << std::endl;

  // 测试方向变换
  std::cout << "\n=== Direction Transformation Test ===" << std::endl;
  Vector3 test_direction(0, 0, -1); // 向-Z方向
  print_vector3("Original direction", test_direction);

  Vector3 dir_result = PortalMath::transform_direction_through_portal(
      test_direction, portal1, portal2, PortalFace::B, PortalFace::A);
  print_vector3("B->A direction result", dir_result);

  return 0;
}
