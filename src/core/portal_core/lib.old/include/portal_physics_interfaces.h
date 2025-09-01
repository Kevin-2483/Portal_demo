#pragma once

#include "portal_types.h"
#include "portal_center_of_mass.h"
#include <vector>

// 前置聲明 - Portal類別（在Portal命名空間中）
namespace Portal {
    class Portal;
}

namespace PortalCore {

// 使用Portal命名空間的類型
using ::Portal::Vector3;
using ::Portal::Transform;
using ::Portal::EntityId;
using ::Portal::PortalId;
using ::Portal::BoundingBoxAnalysis;
using ::Portal::EntityDescription;
using ::Portal::CenterOfMassConfig;
using ::Portal::CenterOfMassResult;
using ::Portal::CenterOfMassType;
using ::Portal::WeightedPoint;

// 定義簡單的包圍盒結構
struct BoundingBox {
    Vector3 min;
    Vector3 max;
    
    BoundingBox() = default;
    BoundingBox(const Vector3& min_val, const Vector3& max_val) 
        : min(min_val), max(max_val) {}
};

/**
 * 物理數據提供者介面 - 引擎必須實現
 * 只提供基礎的物理數據，不涉及複雜的檢測邏輯
 * 現在集成了強大的質心管理系統
 */
class IPhysicsDataProvider : public ::Portal::ICenterOfMassProvider {
public:
    virtual ~IPhysicsDataProvider() = default;
    
    // === 基礎數據獲取（必須實現） ===
    
    /**
     * 獲取實體的變換矩陣
     */
    virtual Transform get_entity_transform(EntityId entity) = 0;
    
    /**
     * 獲取實體的包圍盒（本地坐標系）
     */
    virtual BoundingBox get_entity_bounding_box(EntityId entity) = 0;
    
    /**
     * 獲取實體的速度
     */
    virtual Vector3 get_entity_velocity(EntityId entity) = 0;
    
    // === 質心管理系統集成（繼承自 ICenterOfMassProvider） ===
    
    /**
     * 計算實體的質心（基於配置）
     * 預設實現：使用內建的質心管理器
     */
    virtual CenterOfMassResult calculate_center_of_mass(
        EntityId entity_id, 
        const CenterOfMassConfig& config) override {
        
        // 預設實現：基於配置類型進行簡單計算
        CenterOfMassResult result;
        Transform transform = get_entity_transform(entity_id);
        
        switch (config.type) {
            case CenterOfMassType::GEOMETRIC_CENTER: {
                BoundingBox bbox = get_entity_bounding_box(entity_id);
                result.local_position = (bbox.min + bbox.max) * 0.5f;
                break;
            }
            case CenterOfMassType::CUSTOM_POINT: {
                result.local_position = config.custom_point;
                break;
            }
            default: {
                // 其他類型需要引擎具體實現
                BoundingBox bbox = get_entity_bounding_box(entity_id);
                result.local_position = (bbox.min + bbox.max) * 0.5f;
                break;
            }
        }
        
        result.world_position = transform.transform_point(result.local_position);
        result.is_valid = true;
        return result;
    }
    
    // === 可選的質心系統高級功能（有預設實現） ===
    
    /**
     * 獲取骨骼/節點的世界變換（用於骨骼附著模式）
     * 預設實現：返回實體變換（適用於無骨骼系統）
     */
    virtual Transform get_bone_transform(
        EntityId entity_id, 
        const std::string& bone_name) const override {
        // 注意：這裡需要使用const_cast，因為預設實現中get_entity_transform不是const
        // 引擎實現時應該提供適當的const版本
        return const_cast<IPhysicsDataProvider*>(this)->get_entity_transform(entity_id);
    }
    
    /**
     * 檢查實體的網格是否已更改（用於自動更新）
     * 預設實現：總是返回false（無變化檢測）
     */
    virtual bool has_mesh_changed(EntityId entity_id) const override {
        return false;  // 預設：不支援變化檢測
    }
    
    /**
     * 獲取實體的物理質量分布信息
     * 預設實現：返回空列表（無質量分布數據）
     */
    virtual std::vector<WeightedPoint> get_mass_distribution(EntityId entity_id) const override {
        return {};  // 預設：無質量分布數據
    }
    
    /**
     * 獲取實體的質心位置（世界坐標系）
     * 預設實現：使用AABB中心作為質心（向後兼容）
     * 建議使用新的質心管理系統替代此方法
     */
    virtual Vector3 get_entity_center_of_mass(EntityId entity) {
        BoundingBox bbox = get_entity_bounding_box(entity);
        Transform transform = get_entity_transform(entity);
        Vector3 local_center = (bbox.min + bbox.max) * 0.5f;
        return transform.transform_point(local_center);
    }
    
    /**
     * 獲取所有活動實體列表
     * 用於Portal庫的預設相交查詢實現
     */
    virtual std::vector<EntityId> get_all_active_entities() = 0;
};

/**
 * Portal檢測重載介面 - 引擎可選實現
 * 允許引擎重載關鍵檢測方法以優化性能
 */
class IPortalDetectionOverride {
public:
    virtual ~IPortalDetectionOverride() = default;
    
    /**
     * 重載質心穿越檢測
     * @return true表示已處理（引擎實現），false表示使用Portal庫預設實現
     */
    virtual bool override_center_crossing_check(EntityId entity, const ::Portal::Portal& portal, bool& out_crossed) {
        return false;  // 預設不重載，使用Portal庫實現
    }
    
    /**
     * 重載包圍盒分析
     * @return true表示已處理（引擎實現），false表示使用Portal庫預設實現
     */
    virtual bool override_bounding_box_analysis(EntityId entity, const ::Portal::Portal& portal, BoundingBoxAnalysis& out_analysis) {
        return false;  // 預設不重載，使用Portal庫實現
    }
    
    /**
     * 重載相交實體查詢
     * @return true表示已處理（引擎實現），false表示使用Portal庫預設實現
     */
    virtual bool override_intersection_query(const ::Portal::Portal& portal, std::vector<EntityId>& out_entities) {
        return false;  // 預設不重載，使用Portal庫實現
    }
    
    /**
     * 重載質心穿越進度計算
     * @return true表示已處理（引擎實現），false表示使用Portal庫預設實現
     */
    virtual bool override_crossing_progress_calculation(EntityId entity, const ::Portal::Portal& portal, float& out_progress) {
        return false;  // 預設不重載，使用Portal庫實現
    }
};

/**
 * 物理操作介面 - 保持不變
 * 負責實體的創建、銷毀、變換設定等操作
 */
class IPhysicsManipulator {
public:
    virtual ~IPhysicsManipulator() = default;
    
    // Ghost實體管理
    virtual EntityId create_ghost_entity(const EntityDescription& description) = 0;
    virtual void destroy_entity(EntityId entity) = 0;
    virtual void set_entity_transform(EntityId entity, const Transform& transform) = 0;
    virtual void set_entity_collision_enabled(EntityId entity, bool enabled) = 0;
    virtual void set_entity_visible(EntityId entity, bool visible) = 0;
    
    // 物理屬性設定
    virtual void set_entity_velocity(EntityId entity, const Vector3& velocity) = 0;
    virtual void set_entity_angular_velocity(EntityId entity, const Vector3& angular_velocity) = 0;
};

/**
 * 渲染介面 - 保持不變
 */
class IRenderQuery {
public:
    virtual ~IRenderQuery() = default;
    virtual bool is_entity_visible_through_portal(EntityId entity, const ::Portal::Portal& portal) = 0;
    virtual float calculate_entity_visibility_ratio(EntityId entity, const ::Portal::Portal& portal) = 0;
};

class IRenderManipulator {
public:
    virtual ~IRenderManipulator() = default;
    virtual void render_entity_clipped(EntityId entity, const ::Portal::Portal& portal) = 0;
    virtual void set_entity_render_layers(EntityId entity, uint32_t layers) = 0;
};

/**
 * 事件處理介面 - 保持不變
 */
class IPortalEventHandler {
public:
    virtual ~IPortalEventHandler() = default;
    virtual void on_entity_teleport_begin(EntityId entity, PortalId from_portal, PortalId to_portal) = 0;
    virtual void on_entity_teleport_complete(EntityId entity, PortalId from_portal, PortalId to_portal) = 0;
    virtual void on_ghost_entity_created(EntityId main_entity, EntityId ghost_entity, PortalId portal) = 0;
    virtual void on_ghost_entity_destroyed(EntityId main_entity, EntityId ghost_entity, PortalId portal) = 0;
};

/**
 * 統一的介面集合結構
 */
struct PortalInterfaces {
    IPhysicsDataProvider* physics_data;          // 必須提供
    IPhysicsManipulator* physics_manipulator;    // 必須提供
    IRenderQuery* render_query;                  // 必須提供
    IRenderManipulator* render_manipulator;      // 必須提供
    IPortalEventHandler* event_handler;          // 可選
    IPortalDetectionOverride* detection_override;// 可選，用於性能優化
    
    PortalInterfaces() 
        : physics_data(nullptr)
        , physics_manipulator(nullptr)
        , render_query(nullptr)
        , render_manipulator(nullptr)
        , event_handler(nullptr)
        , detection_override(nullptr) {}
    
    bool is_valid() const {
        return physics_data && physics_manipulator && 
               render_query && render_manipulator;
    }
};

} // namespace PortalCore
