#ifndef PORTAL_CONSOLE_H
#define PORTAL_CONSOLE_H

#define PORTAL_INCLUDE_EXAMPLES
#include "../lib/include/portal.h"
#include "../lib/include/portal_example.h"
#include <iostream>
#include <string>
#include <vector>
#include <map>
#include <functional>
#include <memory>
#include <sstream>
#include <chrono>
#include <iomanip>

namespace Portal
{
  namespace Console
  {

    /**
     * 控制台传送门系统
     * 提供交互式控制台界面来操作传送门系统
     */
    class PortalConsole
    {
    private:
      struct ConsoleEntity
      {
        EntityId id;
        Transform transform;
        PhysicsState physics_state;
        Vector3 bounds_min;
        Vector3 bounds_max;
        std::string name;
        bool is_valid = true;
      };

      std::unique_ptr<Example::ExamplePhysicsQuery> physics_query_;
      std::unique_ptr<Example::ExamplePhysicsManipulator> physics_manipulator_;
      std::unique_ptr<Example::ExampleRenderQuery> render_query_;
      std::unique_ptr<Example::ExampleRenderManipulator> render_manipulator_;
      std::unique_ptr<Example::ExampleEventHandler> event_handler_;
      std::unique_ptr<PortalManager> portal_manager_;

      // 存储接口结构体
      HostInterfaces interfaces_;

      std::map<std::string, std::function<void(const std::vector<std::string> &)>> commands_;
      std::map<PortalId, std::string> portal_names_;
      std::map<EntityId, std::string> entity_names_;
      EntityId next_entity_id_;
      bool running_;

    public:
      PortalConsole();
      ~PortalConsole() = default;

      /**
       * 初始化控制台系统
       */
      bool initialize();

      /**
       * 运行控制台主循环
       */
      void run();

      /**
       * 关闭控制台系统
       */
      void shutdown();

      /**
       * 执行单个命令
       * @param command 命令字符串
       */
      void execute_command(const std::string &command);

    private:
      // 命令处理函数
      void setup_commands();
      void cmd_help(const std::vector<std::string> &args);
      void cmd_status(const std::vector<std::string> &args);
      void cmd_create_portal(const std::vector<std::string> &args);
      void cmd_link_portals(const std::vector<std::string> &args);
      void cmd_list_portals(const std::vector<std::string> &args);
      void cmd_create_entity(const std::vector<std::string> &args);
      void cmd_list_entities(const std::vector<std::string> &args);
      void cmd_move_entity(const std::vector<std::string> &args);
      void cmd_teleport_entity(const std::vector<std::string> &args);
      void cmd_update(const std::vector<std::string> &args);
      void cmd_set_entity_velocity(const std::vector<std::string> &args);
      void cmd_set_portal_velocity(const std::vector<std::string> &args);
      void cmd_teleport_with_velocity(const std::vector<std::string> &args);
      void cmd_test_moving_portal(const std::vector<std::string> &args);
      void cmd_simulate(const std::vector<std::string> &args);
      void cmd_debug_collision(const std::vector<std::string> &args);
      void cmd_simulate_collision_detection(const std::vector<std::string> &args);
      void cmd_get_entity_info(const std::vector<std::string> &args);
      void cmd_destroy_portal(const std::vector<std::string> &args);
      void cmd_exit(const std::vector<std::string> &args);

      // 輔助函數：模擬引擎的碰撞檢測邏輯
      bool check_entity_portal_crossing(EntityId entity_id, PortalId portal_id);

      // 工具函数
      std::vector<std::string> split_command(const std::string &command);
      PortalId find_portal_by_name(const std::string &name);
      EntityId find_entity_by_name(const std::string &name);
      void print_banner();
      void print_portal_info(const Portal *portal, const std::string &name);
      void print_entity_info(EntityId entity_id, const std::string &name);
      Vector3 parse_vector3(const std::string &str);
      Quaternion parse_quaternion(const std::string &str);

      // 获取扩展的物理查询接口
      Example::ExamplePhysicsQuery *get_physics_query()
      {
        return physics_query_.get();
      }
    };

  } // namespace Console
} // namespace Portal

#endif // PORTAL_CONSOLE_H
