#pragma once

#include "../math_types.h"
#include <vector>
#include <queue>
#include <variant>
#include <functional>

namespace portal_core
{

  /**
   * 物理命令類型枚舉
   */
  enum class PhysicsCommandType
  {
    // 力和衝量命令
    ADD_FORCE,               // 添加力
    ADD_IMPULSE,             // 添加衝量
    ADD_TORQUE,              // 添加扭矩
    ADD_ANGULAR_IMPULSE,     // 添加角衝量
    ADD_FORCE_AT_POSITION,   // 在指定位置添加力
    ADD_IMPULSE_AT_POSITION, // 在指定位置添加衝量

    // 速度設置命令
    SET_LINEAR_VELOCITY,  // 設置線性速度
    SET_ANGULAR_VELOCITY, // 設置角速度
    ADD_LINEAR_VELOCITY,  // 增加線性速度
    ADD_ANGULAR_VELOCITY, // 增加角速度

    // 位置和旋轉命令
    SET_POSITION, // 設置位置
    SET_ROTATION, // 設置旋轉
    TRANSLATE,    // 平移
    ROTATE,       // 旋轉
    TELEPORT,     // 瞬移（位置+旋轉）

    // 狀態控制命令
    ACTIVATE,            // 激活物理體
    DEACTIVATE,          // 停用物理體
    SET_GRAVITY_SCALE,   // 設置重力縮放
    SET_LINEAR_DAMPING,  // 設置線性阻尼
    SET_ANGULAR_DAMPING, // 設置角阻尼

    // 約束和關節命令
    CREATE_CONSTRAINT, // 創建約束
    REMOVE_CONSTRAINT, // 移除約束
    UPDATE_CONSTRAINT, // 更新約束參數

    // 材質和碰撞命令
    SET_FRICTION,         // 設置摩擦力
    SET_RESTITUTION,      // 設置彈性
    SET_COLLISION_FILTER, // 設置碰撞過濾

    // 查詢命令
    RAYCAST,      // 射線檢測
    OVERLAP_TEST, // 重疊測試

    // 自定義命令
    CUSTOM // 自定義命令
  };

  /**
   * 物理命令執行時機
   */
  enum class PhysicsCommandTiming
  {
    IMMEDIATE,           // 立即執行（當前幀）
    BEFORE_PHYSICS_STEP, // 物理步進前執行
    AFTER_PHYSICS_STEP,  // 物理步進後執行
    DELAYED              // 延遲執行（指定延遲時間）
  };

  /**
   * 物理命令優先級
   */
  enum class PhysicsCommandPriority
  {
    LOW = 0,
    NORMAL = 1,
    HIGH = 2,
    CRITICAL = 3
  };

  /**
   * 基礎物理命令結構
   */
  struct PhysicsCommand
  {
    PhysicsCommandType type;
    PhysicsCommandTiming timing = PhysicsCommandTiming::BEFORE_PHYSICS_STEP;
    PhysicsCommandPriority priority = PhysicsCommandPriority::NORMAL;

    float delay = 0.0f;       // 延遲時間（僅當timing為DELAYED時有效）
    uint32_t frame_count = 0; // 執行幀數（0表示只執行一次）
    bool auto_remove = true;  // 執行後是否自動移除

    // 命令ID（用於移除特定命令）
    uint64_t command_id = 0;

    // 命令數據（使用variant存儲不同類型的命令參數）
    std::variant<
        std::monostate,        // 空狀態（無參數命令）
        Vector3,                  // 向量參數（力、衝量、速度、位置等）
        float,                 // 浮點參數（重力縮放、阻尼等）
        std::pair<Vector3, Vector3>, // 向量對（位置+力、起點+終點等）
        std::pair<Vector3, Quaternion>, // 位置+旋轉
        std::function<void()>  // 自定義函數
        >
        data;

    // 構造函數
    PhysicsCommand() = default;

    PhysicsCommand(PhysicsCommandType cmd_type) : type(cmd_type) {}

    template <typename T>
    PhysicsCommand(PhysicsCommandType cmd_type, T &&cmd_data)
        : type(cmd_type), data(std::forward<T>(cmd_data)) {}

    // 輔助方法獲取參數
    template <typename T>
    T get_data() const
    {
      return std::get<T>(data);
    }

    bool has_data() const
    {
      return !std::holds_alternative<std::monostate>(data);
    }
  };

  /**
   * 物理命令組件
   * 存儲待執行的物理命令隊列
   */
  struct PhysicsCommandComponent
  {
    // 命令隊列（按優先級和時機組織）
    std::vector<PhysicsCommand> immediate_commands;
    std::vector<PhysicsCommand> before_physics_commands;
    std::vector<PhysicsCommand> after_physics_commands;
    std::vector<PhysicsCommand> delayed_commands;

    // 重複命令（每幀執行）
    std::vector<PhysicsCommand> recurring_commands;

    // 命令ID計數器
    uint64_t next_command_id = 1;

    // 執行狀態
    bool enabled = true;
    bool clear_after_execution = false; // 執行後是否清空所有命令

    PhysicsCommandComponent() = default;

    // 添加命令的便捷方法

    /**
     * 添加力命令
     */
    void add_force(const Vector3 &force, PhysicsCommandTiming timing = PhysicsCommandTiming::BEFORE_PHYSICS_STEP)
    {
      PhysicsCommand cmd(PhysicsCommandType::ADD_FORCE, force);
      cmd.timing = timing;
      cmd.command_id = next_command_id++;
      add_command(std::move(cmd));
    }

    /**
     * 添加衝量命令
     */
    void add_impulse(const Vector3 &impulse, PhysicsCommandTiming timing = PhysicsCommandTiming::BEFORE_PHYSICS_STEP)
    {
      PhysicsCommand cmd(PhysicsCommandType::ADD_IMPULSE, impulse);
      cmd.timing = timing;
      cmd.command_id = next_command_id++;
      add_command(std::move(cmd));
    }

    /**
     * 添加扭矩命令
     */
    void add_torque(const Vector3 &torque, PhysicsCommandTiming timing = PhysicsCommandTiming::BEFORE_PHYSICS_STEP)
    {
      PhysicsCommand cmd(PhysicsCommandType::ADD_TORQUE, torque);
      cmd.timing = timing;
      cmd.command_id = next_command_id++;
      add_command(std::move(cmd));
    }

    /**
     * 設置線性速度命令
     */
    void set_linear_velocity(const Vector3 &velocity, PhysicsCommandTiming timing = PhysicsCommandTiming::BEFORE_PHYSICS_STEP)
    {
      PhysicsCommand cmd(PhysicsCommandType::SET_LINEAR_VELOCITY, velocity);
      cmd.timing = timing;
      cmd.command_id = next_command_id++;
      add_command(std::move(cmd));
    }

    /**
     * 設置角速度命令
     */
    void set_angular_velocity(const Vector3 &velocity, PhysicsCommandTiming timing = PhysicsCommandTiming::BEFORE_PHYSICS_STEP)
    {
      PhysicsCommand cmd(PhysicsCommandType::SET_ANGULAR_VELOCITY, velocity);
      cmd.timing = timing;
      cmd.command_id = next_command_id++;
      add_command(std::move(cmd));
    }

    /**
     * 設置位置命令
     */
    void set_position(const Vector3 &position, PhysicsCommandTiming timing = PhysicsCommandTiming::BEFORE_PHYSICS_STEP)
    {
      PhysicsCommand cmd(PhysicsCommandType::SET_POSITION, position);
      cmd.timing = timing;
      cmd.command_id = next_command_id++;
      add_command(std::move(cmd));
    }

    /**
     * 設置旋轉命令
     */
    void set_rotation(const Quaternion &rotation, PhysicsCommandTiming timing = PhysicsCommandTiming::BEFORE_PHYSICS_STEP)
    {
      // 注意：這裡我們需要將Quat轉換為Vector3存儲，或擴展variant類型
      // 暫時使用Vector3存儲歐拉角
      Vector3 euler_angles = rotation.GetEulerAngles();
      PhysicsCommand cmd(PhysicsCommandType::SET_ROTATION, euler_angles);
      cmd.timing = timing;
      cmd.command_id = next_command_id++;
      add_command(std::move(cmd));
    }

    /**
     * 瞬移命令（同時設置位置和旋轉）
     */
    void teleport(const Vector3 &position, const Quaternion &rotation, PhysicsCommandTiming timing = PhysicsCommandTiming::BEFORE_PHYSICS_STEP)
    {
      PhysicsCommand cmd(PhysicsCommandType::TELEPORT, std::make_pair(position, rotation));
      cmd.timing = timing;
      cmd.command_id = next_command_id++;
      add_command(std::move(cmd));
    }

    /**
     * 在指定位置添加力
     */
    void add_force_at_position(const Vector3 &force, const Vector3 &position, PhysicsCommandTiming timing = PhysicsCommandTiming::BEFORE_PHYSICS_STEP)
    {
      PhysicsCommand cmd(PhysicsCommandType::ADD_FORCE_AT_POSITION, std::make_pair(force, position));
      cmd.timing = timing;
      cmd.command_id = next_command_id++;
      add_command(std::move(cmd));
    }

    /**
     * 設置重力縮放
     */
    void set_gravity_scale(float scale, PhysicsCommandTiming timing = PhysicsCommandTiming::BEFORE_PHYSICS_STEP)
    {
      PhysicsCommand cmd(PhysicsCommandType::SET_GRAVITY_SCALE, scale);
      cmd.timing = timing;
      cmd.command_id = next_command_id++;
      add_command(std::move(cmd));
    }

    /**
     * 激活物理體
     */
    void activate(PhysicsCommandTiming timing = PhysicsCommandTiming::BEFORE_PHYSICS_STEP)
    {
      PhysicsCommand cmd(PhysicsCommandType::ACTIVATE);
      cmd.timing = timing;
      cmd.command_id = next_command_id++;
      add_command(std::move(cmd));
    }

    /**
     * 停用物理體
     */
    void deactivate(PhysicsCommandTiming timing = PhysicsCommandTiming::BEFORE_PHYSICS_STEP)
    {
      PhysicsCommand cmd(PhysicsCommandType::DEACTIVATE);
      cmd.timing = timing;
      cmd.command_id = next_command_id++;
      add_command(std::move(cmd));
    }

    /**
     * 添加自定義命令
     */
    void add_custom_command(std::function<void()> func, PhysicsCommandTiming timing = PhysicsCommandTiming::BEFORE_PHYSICS_STEP)
    {
      PhysicsCommand cmd(PhysicsCommandType::CUSTOM, std::move(func));
      cmd.timing = timing;
      cmd.command_id = next_command_id++;
      add_command(std::move(cmd));
    }

    /**
     * 添加延遲命令
     */
    void add_delayed_command(PhysicsCommand cmd, float delay)
    {
      cmd.timing = PhysicsCommandTiming::DELAYED;
      cmd.delay = delay;
      cmd.command_id = next_command_id++;
      add_command(std::move(cmd));
    }

    /**
     * 添加重複命令（每幀執行）
     */
    void add_recurring_command(PhysicsCommand cmd)
    {
      cmd.frame_count = 0; // 0表示無限重複
      cmd.auto_remove = false;
      cmd.command_id = next_command_id++;
      recurring_commands.push_back(std::move(cmd));
    }

    /**
     * 移除指定ID的命令
     */
    void remove_command(uint64_t command_id)
    {
      auto remove_from_vector = [command_id](std::vector<PhysicsCommand> &commands)
      {
        commands.erase(
            std::remove_if(commands.begin(), commands.end(),
                           [command_id](const PhysicsCommand &cmd)
                           { return cmd.command_id == command_id; }),
            commands.end());
      };

      remove_from_vector(immediate_commands);
      remove_from_vector(before_physics_commands);
      remove_from_vector(after_physics_commands);
      remove_from_vector(delayed_commands);
      remove_from_vector(recurring_commands);
    }

    /**
     * 清空所有命令
     */
    void clear_all_commands()
    {
      immediate_commands.clear();
      before_physics_commands.clear();
      after_physics_commands.clear();
      delayed_commands.clear();
      recurring_commands.clear();
    }

    /**
     * 清空指定時機的命令
     */
    void clear_commands_by_timing(PhysicsCommandTiming timing)
    {
      switch (timing)
      {
      case PhysicsCommandTiming::IMMEDIATE:
        immediate_commands.clear();
        break;
      case PhysicsCommandTiming::BEFORE_PHYSICS_STEP:
        before_physics_commands.clear();
        break;
      case PhysicsCommandTiming::AFTER_PHYSICS_STEP:
        after_physics_commands.clear();
        break;
      case PhysicsCommandTiming::DELAYED:
        delayed_commands.clear();
        break;
      }
    }

    /**
     * 獲取指定時機的命令數量
     */
    size_t get_command_count(PhysicsCommandTiming timing) const
    {
      switch (timing)
      {
      case PhysicsCommandTiming::IMMEDIATE:
        return immediate_commands.size();
      case PhysicsCommandTiming::BEFORE_PHYSICS_STEP:
        return before_physics_commands.size();
      case PhysicsCommandTiming::AFTER_PHYSICS_STEP:
        return after_physics_commands.size();
      case PhysicsCommandTiming::DELAYED:
        return delayed_commands.size();
      default:
        return 0;
      }
    }

    /**
     * 獲取總命令數量
     */
    size_t get_total_command_count() const
    {
      return immediate_commands.size() + before_physics_commands.size() +
             after_physics_commands.size() + delayed_commands.size() +
             recurring_commands.size();
    }

    /**
     * 檢查是否有待執行的命令
     */
    bool has_pending_commands() const
    {
      return get_total_command_count() > 0;
    }

    /**
     * 更新延遲命令的計時器
     */
    void update_delayed_commands(float delta_time)
    {
      for (auto &cmd : delayed_commands)
      {
        cmd.delay -= delta_time;
      }
    }

    /**
     * 獲取準備執行的延遲命令
     */
    std::vector<PhysicsCommand> get_ready_delayed_commands()
    {
      std::vector<PhysicsCommand> ready_commands;
      auto it = delayed_commands.begin();
      while (it != delayed_commands.end())
      {
        if (it->delay <= 0.0f)
        {
          ready_commands.push_back(std::move(*it));
          it = delayed_commands.erase(it);
        }
        else
        {
          ++it;
        }
      }
      return ready_commands;
    }

  private:
    /**
     * 內部添加命令方法
     */
    void add_command(PhysicsCommand &&cmd)
    {
      if (!enabled)
        return;

      switch (cmd.timing)
      {
      case PhysicsCommandTiming::IMMEDIATE:
        immediate_commands.push_back(std::move(cmd));
        break;
      case PhysicsCommandTiming::BEFORE_PHYSICS_STEP:
        before_physics_commands.push_back(std::move(cmd));
        break;
      case PhysicsCommandTiming::AFTER_PHYSICS_STEP:
        after_physics_commands.push_back(std::move(cmd));
        break;
      case PhysicsCommandTiming::DELAYED:
        delayed_commands.push_back(std::move(cmd));
        break;
      }

      // 按優先級排序
      auto sort_by_priority = [](std::vector<PhysicsCommand> &commands)
      {
        std::sort(commands.begin(), commands.end(),
                  [](const PhysicsCommand &a, const PhysicsCommand &b)
                  {
                    return static_cast<int>(a.priority) > static_cast<int>(b.priority);
                  });
      };

      switch (cmd.timing)
      {
      case PhysicsCommandTiming::IMMEDIATE:
        sort_by_priority(immediate_commands);
        break;
      case PhysicsCommandTiming::BEFORE_PHYSICS_STEP:
        sort_by_priority(before_physics_commands);
        break;
      case PhysicsCommandTiming::AFTER_PHYSICS_STEP:
        sort_by_priority(after_physics_commands);
        break;
      case PhysicsCommandTiming::DELAYED:
        sort_by_priority(delayed_commands);
        break;
      }
    }
  };

  /**
   * 物理查詢組件
   * 存儲物理查詢請求和結果
   */
  struct PhysicsQueryComponent
  {
    // 射線檢測查詢
    struct RaycastQuery
    {
      Vector3 origin;
      Vector3 direction;
      float max_distance = 1000.0f;
      uint32_t layer_mask = 0xFFFFFFFF;
      bool hit = false;
      Vector3 hit_point;
      Vector3 hit_normal;
      float hit_distance = 0.0f;
      entt::entity hit_entity = entt::null;
    };

    // 重疊測試查詢
    struct OverlapQuery
    {
      enum Shape
      {
        SPHERE,
        BOX,
        CAPSULE
      };
      Shape shape = SPHERE;
      Vector3 center;
      Vector3 size; // 半徑（球體）或半尺寸（盒子）
      Quaternion rotation = Quaternion::sIdentity();
      uint32_t layer_mask = 0xFFFFFFFF;
      std::vector<entt::entity> overlapping_entities;
    };

    // 距離查詢
    struct DistanceQuery
    {
      Vector3 point;
      float max_distance = 100.0f;
      uint32_t layer_mask = 0xFFFFFFFF;
      entt::entity closest_entity = entt::null;
      float closest_distance = std::numeric_limits<float>::max();
      Vector3 closest_point;
    };

    // 查詢隊列
    std::vector<RaycastQuery> raycast_queries;
    std::vector<OverlapQuery> overlap_queries;
    std::vector<DistanceQuery> distance_queries;

    // 查詢結果有效性
    bool raycast_results_valid = false;
    bool overlap_results_valid = false;
    bool distance_results_valid = false;

    PhysicsQueryComponent() = default;

    /**
     * 添加射線檢測查詢
     */
    void add_raycast(const Vector3 &origin, const Vector3 &direction, float max_distance = 1000.0f, uint32_t layer_mask = 0xFFFFFFFF)
    {
      RaycastQuery query;
      query.origin = origin;
      query.direction = direction.Normalized();
      query.max_distance = max_distance;
      query.layer_mask = layer_mask;
      raycast_queries.push_back(query);
      raycast_results_valid = false;
    }

    /**
     * 添加球體重疊查詢
     */
    void add_sphere_overlap(const Vec3 &center, float radius, uint32_t layer_mask = 0xFFFFFFFF)
    {
      OverlapQuery query;
      query.shape = OverlapQuery::SPHERE;
      query.center = center;
      query.size = Vec3(radius, radius, radius);
      query.layer_mask = layer_mask;
      overlap_queries.push_back(query);
      overlap_results_valid = false;
    }

    /**
     * 添加盒子重疊查詢
     */
    void add_box_overlap(const Vector3 &center, const Vector3 &half_extents, const Quaternion &rotation = Quaternion::sIdentity(), uint32_t layer_mask = 0xFFFFFFFF)
    {
      OverlapQuery query;
      query.shape = OverlapQuery::BOX;
      query.center = center;
      query.size = half_extents;
      query.rotation = rotation;
      query.layer_mask = layer_mask;
      overlap_queries.push_back(query);
      overlap_results_valid = false;
    }

    /**
     * 添加距離查詢
     */
    void add_distance_query(const Vec3 &point, float max_distance = 100.0f, uint32_t layer_mask = 0xFFFFFFFF)
    {
      DistanceQuery query;
      query.point = point;
      query.max_distance = max_distance;
      query.layer_mask = layer_mask;
      distance_queries.push_back(query);
      distance_results_valid = false;
    }

    /**
     * 清空所有查詢
     */
    void clear_all_queries()
    {
      raycast_queries.clear();
      overlap_queries.clear();
      distance_queries.clear();
      raycast_results_valid = false;
      overlap_results_valid = false;
      distance_results_valid = false;
    }

    /**
     * 獲取最近的射線檢測結果
     */
    const RaycastQuery *get_closest_raycast_hit() const
    {
      if (!raycast_results_valid || raycast_queries.empty())
        return nullptr;

      const RaycastQuery *closest = nullptr;
      float min_distance = std::numeric_limits<float>::max();

      for (const auto &query : raycast_queries)
      {
        if (query.hit && query.hit_distance < min_distance)
        {
          min_distance = query.hit_distance;
          closest = &query;
        }
      }

      return closest;
    }

    /**
     * 獲取重疊實體總數
     */
    size_t get_total_overlapping_entities() const
    {
      if (!overlap_results_valid)
        return 0;

      size_t count = 0;
      for (const auto &query : overlap_queries)
      {
        count += query.overlapping_entities.size();
      }
      return count;
    }
  };

} // namespace portal_core
