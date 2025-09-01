#ifndef PORTAL_CORE_V2_H
#define PORTAL_CORE_V2_H

/**
 * Portal Core Library V2 - 事件驅動架構
 *
 * 這是傳送門庫的重構版本，主要特點：
 *
 * 1. 事件驅動架構：
 *    - 庫不再主動進行物理檢測
 *    - 所有檢測由外部物理引擎負責
 *    - 庫通過事件介面響應檢測結果
 *
 * 2. 模塊化設計：
 *    - Portal: 傳送門本體管理
 *    - TeleportManager: 傳送狀態和幽靈實體管理
 *    - CenterOfMassManager: 質心系統管理
 *    - PortalManager: 主控制器和事件分發
 *
 * 3. 簡化介面：
 *    - 移除所有檢測相關介面
 *    - 保留完整的無縫傳送功能
 *    - 支援自定義質心和A/B面
 *
 * 4. 高度重用：
 *    - 所有數學計算100%重用
 *    - 質心管理系統完整保留
 *    - 渲染系統完整保留
 *
 * 使用方式：
 * ```cpp
 * // 1. 準備介面
 * PortalInterfaces interfaces;
 * interfaces.physics_data = your_physics_provider;
 * interfaces.physics_manipulator = your_physics_manipulator;
 *
 * // 2. 創建管理器
 * PortalManager manager(interfaces);
 * manager.initialize();
 *
 * // 3. 創建傳送門和設置實體
 * PortalId portal1 = manager.create_portal(plane1);
 * PortalId portal2 = manager.create_portal(plane2);
 * manager.link_portals(portal1, portal2);
 *
 * // 4. 配置實體的質心（讓物理引擎知道質心位置）
 * CenterOfMassConfig config;
 * config.type = CenterOfMassType::CUSTOM_POINT;
 * config.custom_point = Vector3(0, 0.5f, 0);  // 質心在實體上方
 * manager.set_entity_center_of_mass_config(entity_id, config);
 *
 * // 5. 物理引擎檢測到質心穿越時調用事件：
 * // manager.on_entity_intersect_portal_start(entity_id, portal_id);
 * // manager.on_entity_center_crossed_portal(entity_id, portal_id, PortalFace::A);
 * // 庫會自動響應事件，創建幽靈實體並執行身份互換
 *
 * // 6. 每幀更新
 * manager.update(delta_time);
 * ```
 */

// 核心類型定義
#include "portal_types.h"

// 數學庫（100%重用）
#include "math/portal_math.h"

// 事件介面（新架構核心）
#include "interfaces/portal_event_interfaces.h"

// 核心管理類別
#include "core/portal.h"
#include "core/portal_center_of_mass.h"
#include "core/portal_teleport_manager.h"
#include "core/portal_manager.h"

/**
 * 主要命名空間
 * 所有公開API都在Portal命名空間中
 */
namespace Portal
{

  /**
   * 庫版本信息
   */
  constexpr int PORTAL_CORE_VERSION_MAJOR = 2;
  constexpr int PORTAL_CORE_VERSION_MINOR = 0;
  constexpr int PORTAL_CORE_VERSION_PATCH = 0;

  /**
   * 獲取版本字符串
   */
  inline const char *get_version_string()
  {
    return "2.0.0";
  }

  /**
   * 獲取架構描述
   */
  inline const char *get_architecture_info()
  {
    return "Event-Driven Architecture - External Physics Detection";
  }

  /**
   * 便利函數：創建標準的傳送門系統
   * @param physics_data 物理數據提供者
   * @param physics_manipulator 物理操作介面
   * @param render_query 渲染查詢介面（可選）
   * @param render_manipulator 渲染操作介面（可選）
   * @param event_handler 事件處理器（可選）
   * @return 配置完成的傳送門管理器指針，需要手動delete
   */
  inline PortalManager *create_portal_system(
      IPhysicsDataProvider *physics_data,
      IPhysicsManipulator *physics_manipulator,
      IRenderQuery *render_query = nullptr,
      IRenderManipulator *render_manipulator = nullptr,
      IPortalEventHandler *event_handler = nullptr)
  {

    PortalInterfaces interfaces;
    interfaces.physics_data = physics_data;
    interfaces.physics_manipulator = physics_manipulator;
    interfaces.render_query = render_query;
    interfaces.render_manipulator = render_manipulator;
    interfaces.event_handler = event_handler;

    if (!interfaces.is_valid())
    {
      return nullptr;
    }

    auto *manager = new PortalManager(interfaces);
    if (!manager->initialize())
    {
      delete manager;
      return nullptr;
    }

    return manager;
  }

  /**
   * 便利函數：銷毀傳送門系統
   */
  inline void destroy_portal_system(PortalManager *manager)
  {
    if (manager)
    {
      manager->shutdown();
      delete manager;
    }
  }

} // namespace Portal

#endif // PORTAL_CORE_V2_H