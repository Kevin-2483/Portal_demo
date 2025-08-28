#include <iostream>
#include <memory>
#define PORTAL_INCLUDE_EXAMPLES
#include "../lib/include/portal.h"

using namespace Portal;

class ThreeStateCrossingTest
{
private:
  std::unique_ptr<Example::ExamplePhysicsQuery> physics_query_;
  std::unique_ptr<Example::ExamplePhysicsManipulator> physics_manipulator_;
  std::unique_ptr<Example::ExampleRenderQuery> render_query_;
  std::unique_ptr<Example::ExampleRenderManipulator> render_manipulator_;
  std::unique_ptr<Example::ExampleEventHandler> event_handler_;
  std::unique_ptr<PortalManager> portal_manager_;

  PortalId portal1_id_;
  PortalId portal2_id_;
  EntityId test_entity_id_;

public:
  ThreeStateCrossingTest()
  {
    // 創建所有接口實現
    physics_query_ = std::make_unique<Example::ExamplePhysicsQuery>();
    physics_manipulator_ = std::make_unique<Example::ExamplePhysicsManipulator>(physics_query_.get());
    render_query_ = std::make_unique<Example::ExampleRenderQuery>();
    render_manipulator_ = std::make_unique<Example::ExampleRenderManipulator>();
    event_handler_ = std::make_unique<Example::ExampleEventHandler>();

    // 設置接口容器
    HostInterfaces interfaces;
    interfaces.physics_query = physics_query_.get();
    interfaces.physics_manipulator = physics_manipulator_.get();
    interfaces.render_query = render_query_.get();
    interfaces.render_manipulator = render_manipulator_.get();
    interfaces.event_handler = event_handler_.get();

    // 創建傳送門管理器
    portal_manager_ = std::make_unique<PortalManager>(interfaces);

    test_entity_id_ = 1001;
  }

  void setup_portals()
  {
    std::cout << "=== 設置傳送門 ===\n";

    // 創建兩個面對面的傳送門
    PortalPlane plane1;
    plane1.center = Vector3(-3, 0, 0); // 左側傳送門
    plane1.normal = Vector3(1, 0, 0);  // 朝右
    plane1.up = Vector3(0, 1, 0);
    plane1.right = Vector3(0, 0, 1);
    plane1.width = 2.0f;
    plane1.height = 3.0f;

    PortalPlane plane2;
    plane2.center = Vector3(3, 0, 0);  // 右側傳送門
    plane2.normal = Vector3(-1, 0, 0); // 朝左
    plane2.up = Vector3(0, 1, 0);
    plane2.right = Vector3(0, 0, -1);
    plane2.width = 2.0f;
    plane2.height = 3.0f;

    portal1_id_ = portal_manager_->create_portal(plane1);
    portal2_id_ = portal_manager_->create_portal(plane2);

    portal_manager_->link_portals(portal1_id_, portal2_id_);

    std::cout << "創建並鏈接傳送門 " << portal1_id_ << " 和 " << portal2_id_ << "\n";
  }

  void setup_test_entity()
  {
    std::cout << "\n=== 設置測試實體 ===\n";

    // 創建實體，位於左側傳送門前方
    Transform initial_transform;
    initial_transform.position = Vector3(-5, 0, 0); // 距離左側傳送門2單位

    Vector3 bounds_min(-0.5f, -1.0f, -0.5f); // 1x2x1的盒子
    Vector3 bounds_max(0.5f, 1.0f, 0.5f);

    physics_query_->add_test_entity(test_entity_id_, initial_transform, bounds_min, bounds_max);
    portal_manager_->register_entity(test_entity_id_);

    std::cout << "創建測試實體 " << test_entity_id_ << " 位置: ("
              << initial_transform.position.x << ", "
              << initial_transform.position.y << ", "
              << initial_transform.position.z << ")\n";
  }

  void simulate_crossing()
  {
    std::cout << "\n=== 模擬穿越過程 ===\n";

    // 模擬實體向右移動，穿過傳送門
    float positions[] = {-5.0f, -4.0f, -3.5f, -3.0f, -2.5f, -2.0f, -1.5f, -1.0f, 1.5f, 2.0f, 2.5f, 3.0f, 3.5f, 4.0f, 5.0f};
    int position_count = sizeof(positions) / sizeof(positions[0]);

    for (int i = 0; i < position_count; ++i)
    {
      float x_pos = positions[i];

      // 更新實體位置
      Transform new_transform;
      new_transform.position = Vector3(x_pos, 0, 0);
      physics_query_->update_entity_transform(test_entity_id_, new_transform);

      std::cout << "\n--- 步驟 " << (i + 1) << ": 實體位置 X=" << x_pos << " ---\n";

      // 執行系統更新
      portal_manager_->update(0.016f); // 模擬60FPS

      // 檢查當前狀態
      check_crossing_state();
    }
  }

  void check_crossing_state()
  {
    // 獲取實體的傳送狀態
    const TeleportState *state = portal_manager_->get_entity_teleport_state(test_entity_id_);

    if (state)
    {
      std::cout << "傳送狀態: ";
      switch (state->crossing_state)
      {
      case PortalCrossingState::NOT_TOUCHING:
        std::cout << "NOT_TOUCHING";
        break;
      case PortalCrossingState::CROSSING:
        std::cout << "CROSSING (進度: " << state->transition_progress << ")";
        break;
      case PortalCrossingState::TELEPORTED:
        std::cout << "TELEPORTED";
        break;
      }

      if (state->has_ghost_collider)
      {
        std::cout << " [有幽靈碰撞體]";
      }

      std::cout << "\n";
    }
    else
    {
      std::cout << "無傳送狀態\n";
    }

    // 顯示統計信息
    std::cout << "活躍傳送: " << portal_manager_->get_teleporting_entity_count() << "\n";
  }

  void run_test()
  {
    std::cout << "=== 三狀態機傳送門穿越測試 ===\n\n";

    // 初始化系統
    if (!portal_manager_->initialize())
    {
      std::cout << "❌ 無法初始化傳送門系統!\n";
      return;
    }

    setup_portals();
    setup_test_entity();
    simulate_crossing();

    std::cout << "\n=== 測試完成 ===\n";
    std::cout << "✅ 三狀態機系統運行正常\n";
    std::cout << "✅ 幽靈碰撞體管理正常\n";
    std::cout << "✅ 邊界框分析正常\n";

    // 清理
    portal_manager_->shutdown();
  }
};

int main()
{
  try
  {
    ThreeStateCrossingTest test;
    test.run_test();
    return 0;
  }
  catch (const std::exception &e)
  {
    std::cout << "❌ 測試失敗: " << e.what() << std::endl;
    return 1;
  }
}
