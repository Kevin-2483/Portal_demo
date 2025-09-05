#pragma once

#include <entt/entt.hpp>
#include <functional>
#include <string>
#include <vector>
#include <memory>

namespace portal_core
{

  /**
   * 系統基礎介面
   * 所有系統都應該繼承這個類
   */
  class ISystem
  {
  public:
    virtual ~ISystem() = default;
    
    /**
     * 系統的更新邏輯
     * @param registry ECS 註冊表
     * @param delta_time 幀間時間差
     */
    virtual void update(entt::registry &registry, float delta_time) = 0;
    
    /**
     * 獲取系統名稱（用於調試和依賴管理）
     */
    virtual const char* get_name() const = 0;
    
    /**
     * 獲取系統依賴列表
     * 返回需要在此系統之前執行的系統名稱
     */
    virtual std::vector<std::string> get_dependencies() const { return {}; }
    
    /**
     * 獲取系統排斥列表
     * 返回不能與此系統同時執行的系統名稱
     */
    virtual std::vector<std::string> get_conflicts() const { return {}; }
    
    /**
     * 系統初始化（可選）
     * @return true 如果初始化成功，false 否則
     */
    virtual bool initialize() { return true; }
    
    /**
     * 系統清理（可選）
     */
    virtual void cleanup() {}
  };

  /**
   * 系統註冊器
   * 用於自動註冊系統到 organizer
   */
  class SystemRegistry
  {
  public:
    using SystemFactory = std::function<std::unique_ptr<ISystem>()>;
    
    struct SystemInfo
    {
      SystemFactory factory;
      std::vector<std::string> dependencies;
      std::vector<std::string> conflicts;
      int priority = 0; // 優先級，數字越小越早執行
    };

  private:
    static std::vector<std::pair<std::string, SystemInfo>>& get_systems()
    {
      static std::vector<std::pair<std::string, SystemInfo>> systems;
      return systems;
    }

  public:
    /**
     * 註冊系統
     * @param name 系統名稱
     * @param factory 系統工廠函數
     * @param dependencies 依賴的系統名稱列表
     * @param conflicts 衝突的系統名稱列表
     * @param priority 優先級（數字越小越早執行）
     */
    static void register_system(
      const std::string& name,
      SystemFactory factory,
      const std::vector<std::string>& dependencies = {},
      const std::vector<std::string>& conflicts = {},
      int priority = 0
    )
    {
      SystemInfo info;
      info.factory = std::move(factory);
      info.dependencies = dependencies;
      info.conflicts = conflicts;
      info.priority = priority;
      
      get_systems().emplace_back(name, std::move(info));
    }
    
    /**
     * 獲取所有註冊的系統
     */
    static const std::vector<std::pair<std::string, SystemInfo>>& get_registered_systems()
    {
      return get_systems();
    }
    
    /**
     * 清空所有註冊的系統（主要用於測試）
     */
    static void clear()
    {
      get_systems().clear();
    }
  };

  /**
   * 自動註冊系統的輔助宏
   * 使用方式：
   * REGISTER_SYSTEM(SystemName, {"dependency1", "dependency2"}, {"conflict1"}, priority)
   */
  #define REGISTER_SYSTEM(SystemClass, Dependencies, Conflicts, Priority) \
    namespace { \
      static bool _##SystemClass##_registered = []() { \
        SystemRegistry::register_system( \
          #SystemClass, \
          []() -> std::unique_ptr<ISystem> { \
            return std::make_unique<SystemClass>(); \
          }, \
          Dependencies, \
          Conflicts, \
          Priority \
        ); \
        return true; \
      }(); \
    }

  /**
   * 簡化版註冊宏（無依賴和衝突）
   */
  #define REGISTER_SYSTEM_SIMPLE(SystemClass, Priority) \
    REGISTER_SYSTEM(SystemClass, {}, {}, Priority)

} // namespace portal_core
