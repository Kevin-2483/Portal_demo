#pragma once

#include "../math_types.h"

namespace portal_core {

/**
 * 物理體同步組件
 * 用於同步物理世界和遊戲世界的位置和旋轉
 */
struct PhysicsSyncComponent {
    // 同步標誌
    bool sync_position = true;
    bool sync_rotation = true;
    bool sync_velocity = false;  // 是否同步速度到Transform
    
    // 同步方向
    enum SyncDirection {
        PHYSICS_TO_TRANSFORM,  // 從物理世界同步到Transform（默認）
        TRANSFORM_TO_PHYSICS,  // 從Transform同步到物理世界（運動學物體）
        BIDIRECTIONAL          // 雙向同步（小心使用）
    } sync_direction = PHYSICS_TO_TRANSFORM;
    
    // 同步偏移（物理體相對於Transform的偏移）
    Vec3 position_offset = Vec3(0.0f, 0.0f, 0.0f);
    Quat rotation_offset = Quat(0.0f, 0.0f, 0.0f, 1.0f);
    
    // 插值設定（用於平滑同步）
    bool enable_interpolation = false;
    float interpolation_speed = 10.0f;
    
    // 同步閾值（避免微小變化的同步）
    float position_threshold = 0.001f;
    float rotation_threshold = 0.001f;
    
    // 內部狀態
    Vec3 last_synced_position = Vec3(0.0f, 0.0f, 0.0f);
    Quat last_synced_rotation = Quat(0.0f, 0.0f, 0.0f, 1.0f);
    
    PhysicsSyncComponent() = default;
    
    /**
     * 檢查位置是否需要同步
     */
    bool should_sync_position(const Vec3& current_position) const {
        if (!sync_position) return false;
        Vec3 diff = current_position - last_synced_position;
        return diff.Length() > position_threshold;
    }
    
    /**
     * 檢查旋轉是否需要同步
     */
    bool should_sync_rotation(const Quat& current_rotation) const {
        if (!sync_rotation) return false;
        // 計算四元數差異的角度
        float dot = last_synced_rotation.GetX() * current_rotation.GetX() + 
                   last_synced_rotation.GetY() * current_rotation.GetY() + 
                   last_synced_rotation.GetZ() * current_rotation.GetZ() + 
                   last_synced_rotation.GetW() * current_rotation.GetW();
        float angle = 2.0f * std::acos(std::abs(dot));
        return angle > rotation_threshold;
    }
    
    /**
     * 更新上次同步的狀態
     */
    void update_last_synced_state(const Vec3& position, const Quat& rotation) {
        last_synced_position = position;
        last_synced_rotation = rotation;
    }
};

} // namespace portal_core
