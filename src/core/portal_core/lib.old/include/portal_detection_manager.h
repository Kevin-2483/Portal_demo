#pragma once

#include "portal_physics_interfaces.h"
#include "portal_math.h"

namespace PortalCore {

/**
 * Portal檢測管理器
 * 負責協調Portal庫內建檢測和引擎重載檢測
 * 實現混合檢測架構的核心邏輯
 */
class PortalDetectionManager {
private:
    IPhysicsDataProvider* data_provider_;      // 必須的數據提供者
    IPortalDetectionOverride* override_;       // 可選的重載實現
    
public:
    /**
     * 構造函數
     * @param provider 物理數據提供者（必須）
     * @param override 檢測重載實現（可選）
     */
    PortalDetectionManager(IPhysicsDataProvider* provider, 
                          IPortalDetectionOverride* override = nullptr);
    
    ~PortalDetectionManager() = default;
    
    // === 統一的檢測介面 ===
    
    /**
     * 質心穿越檢測（支援重載）
     * 優先使用引擎重載實現，否則使用Portal庫內建邏輯
     */
    bool check_center_crossing(EntityId entity, const ::Portal::Portal& portal);
    
    /**
     * 包圍盒分析（支援重載）
     * 分析實體包圍盒與傳送門的相交情況
     */
    ::Portal::BoundingBoxAnalysis analyze_entity_bounding_box(EntityId entity, const ::Portal::Portal& portal);
    
    /**
     * 相交實體查詢（支援重載）
     * 查詢與傳送門相交的所有實體
     */
    std::vector<EntityId> get_intersecting_entities(const ::Portal::Portal& portal);
    
    /**
     * 質心穿越進度計算（支援重載）
     * 計算質心穿越傳送門的進度 (0.0-1.0)
     */
    float calculate_crossing_progress(EntityId entity, const ::Portal::Portal& portal);
    
    // === 數據提供者管理 ===
    
    /**
     * 設定物理數據提供者
     */
    void set_data_provider(IPhysicsDataProvider* provider);
    
    /**
     * 設定檢測重載實現
     */
    void set_detection_override(IPortalDetectionOverride* override);
    
    /**
     * 獲取物理數據提供者
     */
    IPhysicsDataProvider* get_data_provider() const { return data_provider_; }
    
    /**
     * 檢查管理器是否已正確初始化
     */
    bool is_initialized() const;

private:
    // === Portal庫內建檢測方法 ===
    
    /**
     * Portal庫內建的質心穿越檢測
     */
    bool default_center_crossing_check(EntityId entity, const ::Portal::Portal& portal);
    
    /**
     * Portal庫內建的包圍盒分析
     */
    ::Portal::BoundingBoxAnalysis default_bounding_box_analysis(EntityId entity, const ::Portal::Portal& portal);
    
    /**
     * Portal庫內建的相交查詢（效率較低，建議引擎重載）
     */
    std::vector<EntityId> default_intersection_query(const ::Portal::Portal& portal);
    
    /**
     * Portal庫內建的穿越進度計算
     */
    float default_crossing_progress_calculation(EntityId entity, const ::Portal::Portal& portal);
};

} // namespace PortalCore
