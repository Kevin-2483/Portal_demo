#ifndef MATH_TYPES_H
#define MATH_TYPES_H

#include <cmath>

// 直接使用 JPH 的数学类型作为项目统一的数学库
#include <Jolt/Jolt.h>
#include <Jolt/Math/Vec3.h>
#include <Jolt/Math/Quat.h>
#include <Jolt/Math/DVec3.h>
#include <Jolt/Math/Real.h>
#include <Jolt/Math/Float2.h>

JPH_SUPPRESS_WARNING_PUSH
JPH_SUPPRESS_WARNINGS

namespace portal_core
{
  // 直接使用JPH数学类型，无需扩展
  using Vector3 = JPH::Vec3;
  using Vector2 = JPH::Float2;  // JPH提供的2D向量
  using Quaternion = JPH::Quat;
  using Vector3d = JPH::DVec3;  // 双精度向量
  using Real = JPH::Real;
  
  // 使用JPH的基础类型
  using Vec3 = JPH::Vec3;
  using RVec3 = JPH::RVec3;
  using DVec3 = JPH::DVec3;
  using Mat44 = JPH::Mat44;
  using Float2 = JPH::Float2;
  
  // 颜色类型（仍需自定义，因为JPH没有颜色类型）
  class ColorExtended {
  public:
    float r, g, b, a;
    
    ColorExtended() : r(1.0f), g(1.0f), b(1.0f), a(1.0f) {}
    ColorExtended(float pr, float pg, float pb, float pa = 1.0f) : r(pr), g(pg), b(pb), a(pa) {}
    
    // 常用颜色
    static const ColorExtended WHITE;
    static const ColorExtended BLACK;
    static const ColorExtended RED;
    static const ColorExtended GREEN;
    static const ColorExtended BLUE;
    static const ColorExtended YELLOW;
    static const ColorExtended CYAN;
    static const ColorExtended MAGENTA;
  };
  
  // 颜色常量定义
  inline const ColorExtended ColorExtended::WHITE = ColorExtended(1.0f, 1.0f, 1.0f, 1.0f);
  inline const ColorExtended ColorExtended::BLACK = ColorExtended(0.0f, 0.0f, 0.0f, 1.0f);
  inline const ColorExtended ColorExtended::RED = ColorExtended(1.0f, 0.0f, 0.0f, 1.0f);
  inline const ColorExtended ColorExtended::GREEN = ColorExtended(0.0f, 1.0f, 0.0f, 1.0f);
  inline const ColorExtended ColorExtended::BLUE = ColorExtended(0.0f, 0.0f, 1.0f, 1.0f);
  inline const ColorExtended ColorExtended::YELLOW = ColorExtended(1.0f, 1.0f, 0.0f, 1.0f);
  inline const ColorExtended ColorExtended::CYAN = ColorExtended(0.0f, 1.0f, 1.0f, 1.0f);
  inline const ColorExtended ColorExtended::MAGENTA = ColorExtended(1.0f, 0.0f, 1.0f, 1.0f);

  // 便利函数用于创建向量
  inline Vector3 make_vector3(float x, float y, float z) {
    return JPH::Vec3(x, y, z);
  }
  
  inline Vector2 make_vector2(float x, float y) {
    return JPH::Float2(x, y);
  }
  
  inline Quaternion make_quaternion(float x, float y, float z, float w) {
    return JPH::Quat(x, y, z, w);
  }
  
  // 便利函数用于向量转换
  inline Vector3 to_vector3(const RVec3& rvec) {
    return JPH::Vec3(rvec);
  }
  
  inline RVec3 to_rvec3(const Vector3& vec) {
    return JPH::RVec3(vec);
  }
}

JPH_SUPPRESS_WARNING_POP

#endif // MATH_TYPES_H
