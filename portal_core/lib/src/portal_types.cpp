#include "portal_types.h"
#include <cmath>

namespace Portal {

    // Vector3 implementations
    float Vector3::length() const {
        return std::sqrt(x * x + y * y + z * z);
    }

    Vector3 Vector3::normalized() const {
        float len = length();
        if (len < 1e-8f) return Vector3(0, 0, 0);
        return Vector3(x / len, y / len, z / len);
    }

    // Quaternion implementations
    Quaternion Quaternion::operator*(const Quaternion& other) const {
        return Quaternion(
            w * other.x + x * other.w + y * other.z - z * other.y,
            w * other.y - x * other.z + y * other.w + z * other.x,
            w * other.z + x * other.y - y * other.x + z * other.w,
            w * other.w - x * other.x - y * other.y - z * other.z
        );
    }

    Vector3 Quaternion::rotate_vector(const Vector3& vec) const {
        // v' = q * v * q^(-1)
        // 优化的向量旋转公式
        Vector3 qvec(x, y, z);
        Vector3 uv = qvec.cross(vec);
        Vector3 uuv = qvec.cross(uv);
        
        uv = uv * (2.0f * w);
        uuv = uuv * 2.0f;
        
        return vec + uv + uuv;
    }

    Quaternion Quaternion::conjugate() const {
        return Quaternion(-x, -y, -z, w);
    }

    Quaternion Quaternion::normalized() const {
        float len = std::sqrt(x * x + y * y + z * z + w * w);
        if (len < 1e-8f) return Quaternion(0, 0, 0, 1);
        return Quaternion(x / len, y / len, z / len, w / len);
    }

    // Transform implementations
    Vector3 Transform::transform_point(const Vector3& point) const {
        Vector3 scaled_point = Vector3(point.x * scale.x, point.y * scale.y, point.z * scale.z);
        Vector3 rotated_point = rotation.rotate_vector(scaled_point);
        return rotated_point + position;
    }

    Vector3 Transform::inverse_transform_point(const Vector3& point) const {
        Vector3 translated_point = point - position;
        Vector3 inv_rotated_point = rotation.conjugate().rotate_vector(translated_point);
        return Vector3(
            inv_rotated_point.x / scale.x,
            inv_rotated_point.y / scale.y,
            inv_rotated_point.z / scale.z
        );
    }

    Transform Transform::inverse() const {
        Quaternion inv_rotation = rotation.conjugate();
        Vector3 inv_scale = Vector3(1.0f / scale.x, 1.0f / scale.y, 1.0f / scale.z);
        Vector3 inv_position = inv_rotation.rotate_vector(position * -1.0f);
        inv_position = Vector3(
            inv_position.x * inv_scale.x,
            inv_position.y * inv_scale.y,
            inv_position.z * inv_scale.z
        );
        
        return Transform(inv_position, inv_rotation, inv_scale);
    }

} // namespace Portal
