#pragma once

#include "system_base.h"
#include <entt/entt.hpp>
#include <memory>
#include <unordered_map>
#include <string>
#include <iostream>
#include <vector>
#include <queue>
#include <algorithm>
#include <unordered_set>
#include <thread>
#include <future>

namespace portal_core
{

  /**
   * 系統管理器
   * 使用 entt::organizer 來管理系統的執行順序和依賴關係，支持並行執行
   */
  class SystemManager
  {
  public:
    SystemManager() = default;
    ~SystemManager() = default;

    /**
     * 初始化系統管理器
     * 自動從 SystemRegistry 載入所有註冊的系統
     */
    void initialize()
    {
      if (initialized_)
      {
        std::cout << "SystemManager: Already initialized, skipping." << std::endl;
        return;
      }

      std::cout << "SystemManager: Initializing..." << std::endl;

      // 清空現有系統
      systems_.clear();

      // 從註冊表載入所有系統
      const auto &registered_systems = SystemRegistry::get_registered_systems();

      std::cout << "SystemManager: Found " << registered_systems.size() << " registered systems." << std::endl;

      // 第一步：創建所有系統實例
      for (const auto &pair : registered_systems)
      {
        const std::string &name = pair.first;
        const SystemRegistry::SystemInfo &info = pair.second;

        auto system = info.factory();
        if (system)
        {
          if (system->initialize())
          {
            systems_[name] = std::move(system);
            std::cout << "SystemManager: Created system '" << name << "'" << std::endl;
          }
          else
          {
            std::cerr << "SystemManager: Failed to initialize system '" << name << "'" << std::endl;
          }
        }
        else
        {
          std::cerr << "SystemManager: Failed to create system '" << name << "'" << std::endl;
        }
      }

      // 第二步：構建任務圖
      build_task_graph_manual(registered_systems);

      initialized_ = true;
      std::cout << "SystemManager: Initialization complete." << std::endl;
    }

    /**
     * 手動添加系統（用於特殊情況）
     */
    void add_system(const std::string &name, std::unique_ptr<ISystem> system)
    {
      if (!system)
      {
        std::cerr << "SystemManager: Cannot add null system '" << name << "'" << std::endl;
        return;
      }

      if (!system->initialize())
      {
        std::cerr << "SystemManager: Failed to initialize system '" << name << "'" << std::endl;
        return;
      }

      systems_[name] = std::move(system);

      // 重新構建任務圖（如果已初始化）
      if (initialized_)
      {
        rebuild_task_graph();
      }
    }

    /**
     * 移除系統
     */
    void remove_system(const std::string &name)
    {
      auto it = systems_.find(name);
      if (it != systems_.end())
      {
        it->second->cleanup();
        systems_.erase(it);

        // 重新構建任務圖
        if (initialized_)
        {
          rebuild_task_graph();
        }
      }
    }

    /**
     * 更新所有系統
     * 根據任務圖進行順序或並行執行
     */
    void update_systems(entt::registry &registry, float delta_time)
    {
      if (!initialized_)
      {
        std::cerr << "SystemManager: Not initialized, call initialize() first." << std::endl;
        return;
      }

      if (enable_parallel_execution_)
      {
        execute_systems_parallel(registry, delta_time);
      }
      else
      {
        execute_systems_sequential(registry, delta_time);
      }
    }

    /**
     * 啟用/禁用並行執行
     */
    void set_parallel_execution(bool enabled)
    {
      enable_parallel_execution_ = enabled;
      std::cout << "SystemManager: Parallel execution "
                << (enabled ? "enabled" : "disabled") << std::endl;
    }

    /**
     * 獲取系統
     */
    ISystem *get_system(const std::string &name)
    {
      auto it = systems_.find(name);
      return (it != systems_.end()) ? it->second.get() : nullptr;
    }

    /**
     * 清理所有系統
     */
    void cleanup()
    {
      for (auto &pair : systems_)
      {
        pair.second->cleanup();
      }
      systems_.clear();
      parallel_layers_.clear();
      initialized_ = false;
    }

    /**
     * 重置系统管理器状态（支持静态系统重新注册）
     * 清理系统实例并重新注册所有静态系统
     */
    void reset()
    {
      // 清理当前系统实例
      cleanup();

      // 重新注册所有静态系统（解决静态注册清除后无法重新注册的问题）
      SystemRegistry::reset_and_re_register();

      std::cout << "SystemManager: Reset completed with static system re-registration." << std::endl;
    }

    /**
     * 獲取系統執行順序（用於調試）
     */
    std::vector<std::string> get_execution_order() const
    {
      std::vector<std::string> order;
      for (const auto &layer : parallel_layers_)
      {
        for (const std::string &system_name : layer)
        {
          order.push_back(system_name);
        }
      }
      return order;
    }

    /**
     * 獲取並行執行層次（用於調試）
     */
    const std::vector<std::vector<std::string>> &get_parallel_layers() const
    {
      return parallel_layers_;
    }

  private:
    std::unordered_map<std::string, std::unique_ptr<ISystem>> systems_;
    std::vector<std::vector<std::string>> parallel_layers_; // 並行執行層次
    bool initialized_ = false;
    bool enable_parallel_execution_ = false;

    /**
     * 手動構建任務圖
     */
    void build_task_graph_manual(const std::vector<std::pair<std::string, SystemRegistry::SystemInfo>> &registered_systems)
    {
      // 構建依賴關係
      std::unordered_map<std::string, std::vector<std::string>> dependencies;
      std::unordered_map<std::string, int> in_degree;
      std::unordered_map<std::string, std::unordered_set<std::string>> dependents;

      // 初始化
      for (const auto &pair : registered_systems)
      {
        const std::string &name = pair.first;
        if (systems_.find(name) != systems_.end())
        {
          in_degree[name] = 0;
          dependencies[name] = {};
          dependents[name] = {};
        }
      }

      // 設置依賴關係
      for (const auto &pair : registered_systems)
      {
        const std::string &name = pair.first;
        const SystemRegistry::SystemInfo &info = pair.second;

        if (systems_.find(name) == systems_.end())
          continue;

        for (const std::string &dependency : info.dependencies)
        {
          if (systems_.find(dependency) != systems_.end())
          {
            dependencies[name].push_back(dependency);
            dependents[dependency].insert(name);
            in_degree[name]++;
            std::cout << "SystemManager: '" << dependency << "' -> '" << name << "' dependency added." << std::endl;
          }
          else
          {
            std::cerr << "SystemManager: Warning - Dependency '" << dependency
                      << "' for system '" << name << "' not found." << std::endl;
          }
        }
      }

      // 檢測並報告循環依賴
      if (!detect_circular_dependencies(in_degree, dependents))
      {
        std::cerr << "SystemManager: Circular dependencies detected! System execution may be incorrect." << std::endl;
      }

      // 分析並行執行層次
      analyze_parallel_layers(in_degree, dependents);

      std::cout << "SystemManager: Task graph analysis complete. "
                << parallel_layers_.size() << " execution layers identified." << std::endl;
    }

    /**
     * 檢測循環依賴
     */
    bool detect_circular_dependencies(
        const std::unordered_map<std::string, int> &in_degree,
        const std::unordered_map<std::string, std::unordered_set<std::string>> &dependents)
    {
      std::unordered_map<std::string, int> temp_in_degree = in_degree;
      std::queue<std::string> zero_in_degree_queue;
      std::unordered_set<std::string> processed;

      // 找到所有入度為0的節點
      for (const auto &pair : temp_in_degree)
      {
        if (pair.second == 0)
        {
          zero_in_degree_queue.push(pair.first);
        }
      }

      // 拓撲排序
      while (!zero_in_degree_queue.empty())
      {
        std::string current = zero_in_degree_queue.front();
        zero_in_degree_queue.pop();
        processed.insert(current);

        // 減少所有依賴節點的入度
        auto it = dependents.find(current);
        if (it != dependents.end())
        {
          for (const std::string &dependent : it->second)
          {
            temp_in_degree[dependent]--;
            if (temp_in_degree[dependent] == 0)
            {
              zero_in_degree_queue.push(dependent);
            }
          }
        }
      }

      // 檢查是否有未處理的節點（說明存在循環）
      std::vector<std::string> circular_systems;
      for (const auto &pair : temp_in_degree)
      {
        if (processed.find(pair.first) == processed.end())
        {
          circular_systems.push_back(pair.first);
        }
      }

      if (!circular_systems.empty())
      {
        std::cerr << "SystemManager: Circular dependency detected among systems: ";
        for (size_t i = 0; i < circular_systems.size(); ++i)
        {
          std::cerr << circular_systems[i];
          if (i < circular_systems.size() - 1)
            std::cerr << ", ";
        }
        std::cerr << std::endl;
        return false;
      }

      return true;
    }

    /**
     * 分析並行執行層次
     */
    void analyze_parallel_layers(
        const std::unordered_map<std::string, int> &in_degree,
        const std::unordered_map<std::string, std::unordered_set<std::string>> &dependents)
    {
      parallel_layers_.clear();

      std::unordered_map<std::string, int> current_in_degree = in_degree;
      std::unordered_set<std::string> remaining_systems;

      for (const auto &pair : current_in_degree)
      {
        remaining_systems.insert(pair.first);
      }

      int layer = 0;
      while (!remaining_systems.empty())
      {
        std::vector<std::string> current_layer;

        // 找到當前層可以執行的系統（入度為0）
        for (const auto &system : remaining_systems)
        {
          if (current_in_degree[system] == 0)
          {
            current_layer.push_back(system);
          }
        }

        if (current_layer.empty())
        {
          // 檢測到循環依賴，跳出避免無限循環
          std::cerr << "SystemManager: Cannot resolve dependencies for remaining "
                    << remaining_systems.size() << " systems. Skipping them." << std::endl;
          break;
        }

        parallel_layers_.push_back(current_layer);

        // 移除當前層的系統並更新依賴
        for (const std::string &completed_system : current_layer)
        {
          remaining_systems.erase(completed_system);

          // 更新依賴該系統的其他系統的入度
          auto it = dependents.find(completed_system);
          if (it != dependents.end())
          {
            for (const std::string &dependent : it->second)
            {
              if (remaining_systems.count(dependent))
              {
                current_in_degree[dependent]--;
              }
            }
          }
        }

        std::cout << "SystemManager: Layer " << layer++ << " (" << current_layer.size()
                  << " systems): ";
        for (const auto &sys : current_layer)
        {
          std::cout << sys << " ";
        }
        std::cout << std::endl;
      }
    }

    /**
     * 重新構建任務圖（當動態添加/移除系統時）
     */
    void rebuild_task_graph()
    {
      const auto &registered_systems = SystemRegistry::get_registered_systems();
      build_task_graph_manual(registered_systems);
    }

    /**
     * 順序執行系統
     */
    void execute_systems_sequential(entt::registry &registry, float delta_time)
    {
      // 按層次順序執行所有系統
      for (const auto &layer : parallel_layers_)
      {
        for (const std::string &system_name : layer)
        {
          auto it = systems_.find(system_name);
          if (it != systems_.end())
          {
            // 直接調用系統更新，不使用異常處理
            it->second->update(registry, delta_time);
          }
        }
      }
    }

    /**
     * 並行執行系統
     * 每一層內的系統可以並行執行，層與層之間需要同步
     */
    void execute_systems_parallel(entt::registry &registry, float delta_time)
    {
      const size_t PARALLEL_THRESHOLD = 4; // 小於此數量的系統仍然順序執行

      for (size_t layer_idx = 0; layer_idx < parallel_layers_.size(); ++layer_idx)
      {
        const auto &layer = parallel_layers_[layer_idx];

        if (layer.size() == 1 || layer.size() < PARALLEL_THRESHOLD)
        {
          // 單個系統或系統數量較少，直接順序執行避免線程開銷
          for (const std::string &system_name : layer)
          {
            auto it = systems_.find(system_name);
            if (it != systems_.end())
            {
              it->second->update(registry, delta_time);
            }
          }
        }
        else
        {
          // 多個系統且數量足夠，並行執行
          std::vector<std::future<void>> futures;

          for (const std::string &system_name : layer)
          {
            auto it = systems_.find(system_name);
            if (it != systems_.end())
            {
              // 直接捕獲必要的變數，避免使用靜態成員
              futures.emplace_back(std::async(std::launch::async,
                                              [this, &registry, delta_time, system_name]()
                                              {
                                                auto sys_it = this->systems_.find(system_name);
                                                if (sys_it != this->systems_.end())
                                                {
                                                  sys_it->second->update(registry, delta_time);
                                                }
                                              }));
            }
          }

          // 等待當前層所有系統完成
          for (auto &future : futures)
          {
            future.wait();
          }

          std::cout << "SystemManager: Layer " << layer_idx << " completed ("
                    << layer.size() << " systems in parallel)" << std::endl;
        }
      }
    }
  };

} // namespace portal_core