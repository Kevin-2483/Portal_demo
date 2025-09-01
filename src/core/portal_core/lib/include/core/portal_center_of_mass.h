#ifndef PORTAL_CENTER_OF_MASS_H
#define PORTAL_CENTER_OF_MASS_H

#include "../portal_types.h"
#include <unordered_map>

namespace Portal
{

  /**
   * 質心管理器接口
   * 允許引擎實現具體的質心計算邏輯
   */
  class ICenterOfMassProvider
  {
  public:
    virtual ~ICenterOfMassProvider() = default;

    /**
     * 計算實體的質心（基於配置）
     */
    virtual CenterOfMassResult calculate_center_of_mass(
        EntityId entity_id,
        const CenterOfMassConfig &config) = 0;

    /**
     * 獲取骨骼/節點的世界變換（用於骨骼附著模式）
     */
    virtual Transform get_bone_transform(
        EntityId entity_id,
        const std::string &bone_name) const = 0;

    /**
     * 檢查實體的網格是否已更改（用於自動更新）
     */
    virtual bool has_mesh_changed(EntityId entity_id) const = 0;

    /**
     * 獲取實體的物理質量分布信息
     */
    virtual std::vector<WeightedPoint> get_mass_distribution(EntityId entity_id) const = 0;

    /**
     * 獲取當前時間戳（毫秒）
     * 用於質心計算結果的時間戳記錄
     */
    virtual uint64_t get_current_timestamp() const = 0;
  };

  /**
   * 內置的質心管理器
   * 提供標準的質心計算功能
   */
  class CenterOfMassManager
  {
  private:
    std::unordered_map<EntityId, CenterOfMassConfig> entity_configs_;
    std::unordered_map<EntityId, CenterOfMassResult> cached_results_;
    ICenterOfMassProvider *provider_;

  public:
    explicit CenterOfMassManager(ICenterOfMassProvider *provider = nullptr)
        : provider_(provider) {}

    /**
     * 設置質心提供者
     */
    void set_provider(ICenterOfMassProvider *provider)
    {
      provider_ = provider;
    }

    /**
     * 為實體設置質心配置
     */
    void set_entity_center_of_mass_config(EntityId entity_id, const CenterOfMassConfig &config)
    {
      entity_configs_[entity_id] = config;
      // 清除緩存以強制重新計算
      cached_results_.erase(entity_id);
    }

    /**
     * 獲取實體的質心配置
     */
    const CenterOfMassConfig *get_entity_center_of_mass_config(EntityId entity_id) const
    {
      auto it = entity_configs_.find(entity_id);
      return (it != entity_configs_.end()) ? &it->second : nullptr;
    }

    /**
     * 計算並獲取質心位置（世界坐標）
     */
    Vector3 get_entity_center_of_mass_world(EntityId entity_id, const Transform &entity_transform);

    /**
     * 計算並獲取質心位置（本地坐標）
     */
    Vector3 get_entity_center_of_mass_local(EntityId entity_id);

    /**
     * 更新所有配置為自動更新的實體
     */
    void update_auto_update_entities(float delta_time);

    /**
     * 強制重新計算指定實體的質心
     */
    void force_recalculate(EntityId entity_id);

    /**
     * 清除實體的質心配置和緩存
     */
    void remove_entity(EntityId entity_id)
    {
      entity_configs_.erase(entity_id);
      cached_results_.erase(entity_id);
    }

    /**
     * 獲取緩存的計算結果
     */
    const CenterOfMassResult *get_cached_result(EntityId entity_id) const
    {
      auto it = cached_results_.find(entity_id);
      return (it != cached_results_.end()) ? &it->second : nullptr;
    }

  private:
    /**
     * 內部計算方法
     */
    CenterOfMassResult calculate_center_of_mass_internal(
        EntityId entity_id,
        const CenterOfMassConfig &config,
        const Transform &entity_transform);

    /**
     * 基於幾何中心的計算（預設實現）
     */
    Vector3 calculate_geometric_center(EntityId entity_id, const Transform &entity_transform);

    /**
     * 基於多點加權的計算
     */
    Vector3 calculate_weighted_average(
        const std::vector<WeightedPoint> &points,
        const Transform &entity_transform);
  };

  // 便利函數：創建常用的質心配置

  /**
   * 創建自定義點質心配置
   */
  inline CenterOfMassConfig create_custom_point_config(const Vector3 &custom_point)
  {
    CenterOfMassConfig config;
    config.type = CenterOfMassType::CUSTOM_POINT;
    config.custom_point = custom_point;
    return config;
  }

  /**
   * 創建骨骼附著質心配置
   */
  inline CenterOfMassConfig create_bone_attachment_config(
      const std::string &bone_name,
      const Vector3 &offset = Vector3(0, 0, 0))
  {
    CenterOfMassConfig config;
    config.type = CenterOfMassType::BONE_ATTACHMENT;
    config.bone_attachment = BoneAttachment(bone_name, offset);
    return config;
  }

  /**
   * 創建多點加權質心配置
   */
  inline CenterOfMassConfig create_weighted_points_config(
      const std::vector<WeightedPoint> &points)
  {
    CenterOfMassConfig config;
    config.type = CenterOfMassType::WEIGHTED_AVERAGE;
    config.weighted_points = points;
    return config;
  }

  /**
   * 創建物理質心配置（考慮物理質量分布）
   */
  inline CenterOfMassConfig create_physics_center_config(bool auto_update = true)
  {
    CenterOfMassConfig config;
    config.type = CenterOfMassType::PHYSICS_CENTER;
    config.consider_physics_mass = true;
    config.auto_update_on_mesh_change = auto_update;
    return config;
  }

} // namespace Portal

#endif // PORTAL_CENTER_OF_MASS_H
