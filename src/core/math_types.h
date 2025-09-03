#ifndef MATH_TYPES_H
#define MATH_TYPES_H

#include <cmath>

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

namespace portal_core
{

  // 簡單的 3D 向量
  struct Vector3
  {
    float x, y, z;

    Vector3() : x(0.0f), y(0.0f), z(0.0f) {}
    Vector3(float x_, float y_, float z_) : x(x_), y(y_), z(z_) {}

    Vector3 operator+(const Vector3 &other) const
    {
      return Vector3(x + other.x, y + other.y, z + other.z);
    }

    Vector3 operator-(const Vector3 &other) const
    {
      return Vector3(x - other.x, y - other.y, z - other.z);
    }

    Vector3 operator*(float scalar) const
    {
      return Vector3(x * scalar, y * scalar, z * scalar);
    }

    float length() const
    {
      return std::sqrt(x * x + y * y + z * z);
    }

    Vector3 normalized() const
    {
      float len = length();
      if (len > 0.0f)
      {
        return Vector3(x / len, y / len, z / len);
      }
      return Vector3();
    }
  };

  // 簡單的四元數
  struct Quaternion
  {
    float w, x, y, z;

    Quaternion() : w(1.0f), x(0.0f), y(0.0f), z(0.0f) {}
    Quaternion(float w_, float x_, float y_, float z_) : w(w_), x(x_), y(y_), z(z_) {}

    // 從軸角創建四元數
    static Quaternion from_axis_angle(const Vector3 &axis, float angle)
    {
      float half_angle = angle * 0.5f;
      float sin_half = std::sin(half_angle);
      float cos_half = std::cos(half_angle);

      Vector3 normalized_axis = axis.normalized();
      return Quaternion(
          cos_half,
          normalized_axis.x * sin_half,
          normalized_axis.y * sin_half,
          normalized_axis.z * sin_half);
    }

    // 從歐拉角創建四元數
    static Quaternion from_euler(const Vector3 &euler)
    {
      float cx = std::cos(euler.x * 0.5f);
      float sx = std::sin(euler.x * 0.5f);
      float cy = std::cos(euler.y * 0.5f);
      float sy = std::sin(euler.y * 0.5f);
      float cz = std::cos(euler.z * 0.5f);
      float sz = std::sin(euler.z * 0.5f);

      return Quaternion(
          cx * cy * cz + sx * sy * sz,
          sx * cy * cz - cx * sy * sz,
          cx * sy * cz + sx * cy * sz,
          cx * cy * sz - sx * sy * cz);
    }

    // 轉換為歐拉角
    Vector3 to_euler() const
    {
      float sinr_cosp = 2 * (w * x + y * z);
      float cosr_cosp = 1 - 2 * (x * x + y * y);
      float roll = std::atan2(sinr_cosp, cosr_cosp);

      float sinp = 2 * (w * y - z * x);
      float pitch;
      if (std::abs(sinp) >= 1)
      {
        pitch = std::copysign(M_PI / 2, sinp); // use 90 degrees if out of range
      }
      else
      {
        pitch = std::asin(sinp);
      }

      float siny_cosp = 2 * (w * z + x * y);
      float cosy_cosp = 1 - 2 * (y * y + z * z);
      float yaw = std::atan2(siny_cosp, cosy_cosp);

      return Vector3(roll, pitch, yaw);
    }

    Quaternion operator*(const Quaternion &other) const
    {
      return Quaternion(
          w * other.w - x * other.x - y * other.y - z * other.z,
          w * other.x + x * other.w + y * other.z - z * other.y,
          w * other.y - x * other.z + y * other.w + z * other.x,
          w * other.z + x * other.y - y * other.x + z * other.w);
    }

    Quaternion normalized() const
    {
      float len = std::sqrt(w * w + x * x + y * y + z * z);
      if (len > 0.0f)
      {
        return Quaternion(w / len, x / len, y / len, z / len);
      }
      return Quaternion();
    }
  };

} // namespace portal_core

#endif // MATH_TYPES_H
