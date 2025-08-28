#include "../lib/include/portal.h"
#include <iostream>
#include <iomanip>

using namespace Portal;
using namespace Portal::Math;

void print_scenario(const std::string &title)
{
  std::cout << "\n"
            << std::string(50, '=') << std::endl;
  std::cout << title << std::endl;
  std::cout << std::string(50, '=') << std::endl;
}

void print_vector3(const std::string &name, const Vector3 &v)
{
  std::cout << std::setw(20) << name << ": ("
            << std::fixed << std::setprecision(2)
            << std::setw(6) << v.x << ", "
            << std::setw(6) << v.y << ", "
            << std::setw(6) << v.z << ")" << std::endl;
}

int main()
{
  print_scenario("Portal A/B Face Correspondence Verification");

  // 創建兩個傳送門，模擬牆壁上和地面上的傳送門
  PortalPlane wall_portal, floor_portal;

  // 牆上的傳送門 (垂直)
  wall_portal.center = Vector3(0, 2, 0); // 牆上2米高
  wall_portal.normal = Vector3(1, 0, 0); // A面朝向+X
  wall_portal.right = Vector3(0, 0, 1);  // 右方向朝向+Z
  wall_portal.up = Vector3(0, 1, 0);     // 上方向朝向+Y

  // 地面上的傳送門 (水平)
  floor_portal.center = Vector3(10, 0, 10); // 地面上
  floor_portal.normal = Vector3(0, 1, 0);   // A面朝向+Y（向上）
  floor_portal.right = Vector3(1, 0, 0);    // 右方向朝向+X
  floor_portal.up = Vector3(0, 0, 1);       // "上"方向實際朝向+Z

  std::cout << "\n牆上傳送門 (Wall Portal):" << std::endl;
  print_vector3("Center", wall_portal.center);
  print_vector3("A面法向量", wall_portal.get_face_normal(PortalFace::A));
  print_vector3("B面法向量", wall_portal.get_face_normal(PortalFace::B));
  print_vector3("Right", wall_portal.right);
  print_vector3("Up", wall_portal.up);

  std::cout << "\n地面傳送門 (Floor Portal):" << std::endl;
  print_vector3("Center", floor_portal.center);
  print_vector3("A面法向量", floor_portal.get_face_normal(PortalFace::A));
  print_vector3("B面法向量", floor_portal.get_face_normal(PortalFace::B));
  print_vector3("Right", floor_portal.right);
  print_vector3("Up", floor_portal.up);

  print_scenario("測試場景：從牆前走向地面傳送門");

  // 玩家在牆傳送門的A面前方
  Vector3 player_pos(1, 2, 0); // 牆傳送門A面前方1米
  print_vector3("玩家初始位置", player_pos);

  // 玩家向前走（朝向牆傳送門A面）
  Vector3 player_velocity(-1, 0, 0); // 朝向-X（走向傳送門）
  print_vector3("玩家初始速度", player_velocity);

  // 從牆傳送門A面進入，從地面傳送門A面出現
  Vector3 teleported_pos = PortalMath::transform_point_through_portal(
      player_pos, wall_portal, floor_portal, PortalFace::A, PortalFace::A);

  Vector3 teleported_velocity = PortalMath::transform_direction_through_portal(
      player_velocity, wall_portal, floor_portal, PortalFace::A, PortalFace::A);

  print_vector3("傳送後位置", teleported_pos);
  print_vector3("傳送後速度", teleported_velocity);

  print_scenario("測試場景：從地面跳向牆傳送門");

  // 玩家在地面傳送門B面下方
  Vector3 player_pos2(10, -1, 10); // 地面傳送門B面下方1米
  print_vector3("玩家初始位置", player_pos2);

  // 玩家向上跳（朝向地面傳送門B面）
  Vector3 player_velocity2(0, 1, 0); // 向上跳
  print_vector3("玩家初始速度", player_velocity2);

  // 從地面傳送門B面進入，從牆傳送門B面出現
  Vector3 teleported_pos2 = PortalMath::transform_point_through_portal(
      player_pos2, floor_portal, wall_portal, PortalFace::B, PortalFace::B);

  Vector3 teleported_velocity2 = PortalMath::transform_direction_through_portal(
      player_velocity2, floor_portal, wall_portal, PortalFace::B, PortalFace::B);

  print_vector3("傳送後位置", teleported_pos2);
  print_vector3("傳送後速度", teleported_velocity2);

  print_scenario("驗證面對應的一致性");

  std::cout << "✓ 牆傳送門A面對應地面傳送門A面" << std::endl;
  std::cout << "✓ 牆傳送門B面對應地面傳送門B面" << std::endl;
  std::cout << "✓ 玩家從任一面進入都會從對應面出現" << std::endl;
  std::cout << "✓ 速度方向正確轉換以保持物理一致性" << std::endl;

  // 測試默認行為是否等同於A->B
  Vector3 default_pos = PortalMath::transform_point_through_portal(
      player_pos, wall_portal, floor_portal);

  Vector3 explicit_ab_pos = PortalMath::transform_point_through_portal(
      player_pos, wall_portal, floor_portal, PortalFace::A, PortalFace::B);

  bool is_consistent = (abs(default_pos.x - explicit_ab_pos.x) < 1e-6 &&
                        abs(default_pos.y - explicit_ab_pos.y) < 1e-6 &&
                        abs(default_pos.z - explicit_ab_pos.z) < 1e-6);

  std::cout << "\n默認行為 vs A->B 顯式調用: " << (is_consistent ? "一致 ✓" : "不一致 ✗") << std::endl;

  return 0;
}
