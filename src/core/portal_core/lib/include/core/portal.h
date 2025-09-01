#ifndef PORTAL_H
#define PORTAL_H

#include "../portal_types.h"

namespace Portal {

/**
 * 傳送門類別
 * 表示一個傳送門的完整狀態和屬性
 * 
 * 職責：
 * - 管理傳送門的基本屬性（位置、大小、鏈接等）
 * - 提供傳送門相關的查詢介面
 * - 不包含複雜的邏輯處理
 */
class Portal {
private:
    PortalId id_;                    // 傳送門唯一ID
    PortalPlane plane_;              // 傳送門平面定義
    PortalId linked_portal_id_;      // 鏈接的傳送門ID
    bool is_active_;                 // 是否啟用
    bool is_recursive_;              // 是否處於遞歸狀態
    PhysicsState physics_state_;     // 傳送門自身的物理狀態（如果可移動）
    int max_recursion_depth_;        // 最大遞歸渲染深度
    
public:
    explicit Portal(PortalId id);
    ~Portal() = default;
    
    // 禁用拷貝（傳送門應該是唯一的）
    Portal(const Portal&) = delete;
    Portal& operator=(const Portal&) = delete;
    
    // === 基本屬性訪問 ===
    
    PortalId get_id() const { return id_; }
    
    const PortalPlane& get_plane() const { return plane_; }
    void set_plane(const PortalPlane& plane) { plane_ = plane; }
    
    // === 鏈接管理 ===
    
    PortalId get_linked_portal() const { return linked_portal_id_; }
    void set_linked_portal(PortalId portal_id) { linked_portal_id_ = portal_id; }
    bool is_linked() const { return linked_portal_id_ != INVALID_PORTAL_ID; }
    
    // === 狀態管理 ===
    
    bool is_active() const { return is_active_; }
    void set_active(bool active) { is_active_ = active; }
    
    bool is_recursive() const { return is_recursive_; }
    void set_recursive(bool recursive) { is_recursive_ = recursive; }
    
    // === 物理狀態（用於移動的傳送門） ===
    
    const PhysicsState& get_physics_state() const { return physics_state_; }
    void set_physics_state(const PhysicsState& state) { physics_state_ = state; }
    
    // === 渲染相關 ===
    
    int get_max_recursion_depth() const { return max_recursion_depth_; }
    void set_max_recursion_depth(int depth) { max_recursion_depth_ = depth; }
    
    // === 實用方法 ===
    
    /**
     * 獲取指定面的法向量
     */
    Vector3 get_face_normal(PortalFace face) const {
        return plane_.get_face_normal(face);
    }
    
    /**
     * 檢查點是否在傳送門的有效區域內
     */
    bool is_point_in_bounds(const Vector3& point) const;
    
    /**
     * 獲取傳送門的四個角點（世界坐標）
     */
    void get_corner_points(Vector3 corners[4]) const;
    
    /**
     * 檢查傳送門是否面向指定位置
     */
    bool is_facing_position(const Vector3& position, PortalFace face = PortalFace::A) const;
};

} // namespace Portal

#endif // PORTAL_H