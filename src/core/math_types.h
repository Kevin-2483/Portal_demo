#ifndef MATH_TYPES_H
#define MATH_TYPES_H

// 使用 JPH 的数学类型作为项目统一的数学库
#include <Jolt/Jolt.h>
#include <Jolt/Math/Vec3.h>
#include <Jolt/Math/Quat.h>
#include <Jolt/Math/DVec3.h>
#include <Jolt/Math/Real.h>

JPH_SUPPRESS_WARNING_PUSH
JPH_SUPPRESS_WARNINGS

namespace portal_core
{
  // 前向聲明
  class Vector3Extended;
  class QuaternionExtended;

  // 為了向後兼容，保留一些旧的接口扩展
  class Vector3Extended : public JPH::Vec3
  {
  public:
    Vector3Extended() : JPH::Vec3() {}
    Vector3Extended(float x, float y, float z) : JPH::Vec3(x, y, z) {}
    Vector3Extended(const JPH::Vec3 &v) : JPH::Vec3(v) {}

    // 提供与原来兼容的接口
    float x() const { return GetX(); }
    float y() const { return GetY(); }
    float z() const { return GetZ(); }

    Vector3Extended lerp(const Vector3Extended &other, float t) const
    {
      return Vector3Extended((*this) * (1.0f - t) + other * t);
    }

    float dot(const Vector3Extended &other) const
    {
      return this->Dot(other);
    }

    Vector3Extended cross(const Vector3Extended &other) const
    {
      return Vector3Extended(this->Cross(other));
    }

    // 提供原来的length()方法名
    float length() const
    {
      return Length();
    }

    // 提供原来的normalized()方法名
    Vector3Extended normalized() const
    {
      return Vector3Extended(Normalized());
    }
  };

  // 扩展四元数类，提供向后兼容的接口
  class QuaternionExtended : public JPH::Quat
  {
  public:
    QuaternionExtended() : JPH::Quat() {}
    QuaternionExtended(float w, float x, float y, float z) : JPH::Quat(x, y, z, w) {} // 注意參數順序轉換
    QuaternionExtended(const JPH::Quat &q) : JPH::Quat(q) {}

    // 提供与原来兼容的接口
    float w() const { return GetW(); }
    float x() const { return GetX(); }
    float y() const { return GetY(); }
    float z() const { return GetZ(); }

    QuaternionExtended slerp(const QuaternionExtended &other, float t) const
    {
      return QuaternionExtended(this->SLERP(other, t));
    }

    QuaternionExtended conjugate() const
    {
      return QuaternionExtended(Conjugated());
    }

    // 提供原来的normalized()方法名
    QuaternionExtended normalized() const
    {
      return QuaternionExtended(Normalized());
    }

    // 从轴角创建四元数 - 保持原有接口
    static QuaternionExtended from_axis_angle(const Vector3Extended &axis, float angle)
    {
      return QuaternionExtended(JPH::Quat::sRotation(axis.Normalized(), angle));
    }

    // 从欧拉角创建四元数 - 保持原有接口
    static QuaternionExtended from_euler(const Vector3Extended &euler)
    {
      // 按照 XYZ 顺序应用旋转
      JPH::Quat qx = JPH::Quat::sRotation(JPH::Vec3::sAxisX(), euler.GetX());
      JPH::Quat qy = JPH::Quat::sRotation(JPH::Vec3::sAxisY(), euler.GetY());
      JPH::Quat qz = JPH::Quat::sRotation(JPH::Vec3::sAxisZ(), euler.GetZ());
      return QuaternionExtended(qz * qy * qx);
    }

    // 静态工厂方法 - 支持更多创建方式
    static QuaternionExtended FromEuler(const Vector3Extended &euler)
    {
      return from_euler(euler);
    }

    // 转换为欧拉角 - 保持原有接口
    Vector3Extended to_euler() const
    {
      JPH::Vec3 euler = GetEulerAngles();
      return Vector3Extended(euler);
    }

    // 获取欧拉角 - 提供另一个名称
    Vector3Extended GetEulerAngles() const
    {
      return to_euler();
    }
  };

  // 使用 JPH 的数学类型作为基础类型
  using Vector3 = Vector3Extended;       // 使用擴展版本以獲得便利方法
  using DVectorRe3 = JPH::DVec3;         // 双精度向量，用于精确位置计算
  using RealVector3 = JPH::RVec3;        // 实数向量，JPH中用于位置的类型
  using Quaternion = QuaternionExtended; // 使用擴展版本

  // 类型别名，用于物理系统和向后兼容
  using Vec3 = Vector3Extended;    // 直接使用擴展的Vec3
  using Quat = QuaternionExtended; // 直接使用擴展的Quat
  using RVec3 = JPH::RVec3;        // 用于位置的实数向量
  using DVec3 = JPH::DVec3;        // 双精度向量

  // 数学常量，与JPH兼容
  namespace Math
  {
    static constexpr float PI = JPH::JPH_PI;
    static constexpr float HALF_PI = JPH::JPH_PI * 0.5f;
    static constexpr float TWO_PI = JPH::JPH_PI * 2.0f;
  }

} // namespace portal_core

JPH_SUPPRESS_WARNING_POP

#endif // MATH_TYPES_H
