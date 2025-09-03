#pragma once

#include <godot_cpp/classes/node.hpp>
#include <godot_cpp/classes/node3d.hpp>
#include <godot_cpp/classes/resource.hpp>
#include <godot_cpp/templates/vector.hpp>
#include <godot_cpp/variant/typed_array.hpp>
#include <godot_cpp/variant/node_path.hpp>
#include "entt/entt.hpp"
#include "game_core_manager.h"
#include "ecs_component_resource.h"

using namespace godot;

/**
 * 通用 ECS 節點 - 連接 Godot 和 C++ ECS 世界的橋樑
 * 這是設計師唯一需要使用的節點類型
 * 重構後：使用多態，無需硬編碼任何特定組件類型！
 * 新架構：繼承自 Node，通過 target_node_path 控制任意 Node3D 的變換
 */
class ECSNode : public Node
{
  GDCLASS(ECSNode, Node)

private:
  entt::entity entity;             // 在 C++ ECS 世界中的實體 ID
  bool entity_created;             // 標記實體是否已創建
  TypedArray<Resource> components; // 組件資源數組（設計師配置）
  NodePath target_node_path;       // 新增：用於指定要控制的目標 Node3D 節點

  // 性能優化：緩存 GameCoreManager 引用
  static GameCoreManager *cached_game_core_manager;
  static bool cache_valid;

protected:
  static void _bind_methods();

public:
  ECSNode();
  ~ECSNode();

  // Godot 生命週期
  void _ready() override;
  void _process(double delta) override;
  void _exit_tree() override;
  void _notification(int p_what);

  // 屬性訪問器
  void set_components(const TypedArray<Resource> &p_components);
  TypedArray<Resource> get_components() const;
  void set_target_node_path(const NodePath &p_path);
  NodePath get_target_node_path() const;

private:
  // 內部方法
  void create_ecs_entity();
  void destroy_ecs_entity();
  void apply_components_to_entity();  // 現在使用多態，無需硬編碼！
  void clear_all_non_basic_components(); // 新增：清除所有非基礎組件
  void clear_components_by_runtime_detection(entt::registry& registry); // 新增：運行時檢測清除
  void connect_resource_signals();
  void disconnect_resource_signals();
  void _on_resource_changed();
  Node* get_effective_target_node();  // 修改：返回通用的 Node* 而非 Node3D*

  // 實時更新支持
  void _update_ecs_components();      // 現在使用多態，無需硬編碼！
  void _update_ecs_components_deferred();

  // GameCoreManager 获取方法（简化版 - 使用 autoload）
  GameCoreManager *get_game_core_manager_efficient();
  static void invalidate_cache();
  
  // 安全初始化支持
  bool is_game_core_ready() const;
  void connect_to_game_core_manager();
  void disconnect_from_game_core_manager();
  void connect_to_event_bus();          // 新增：连接编辑器事件总线
  void disconnect_from_event_bus();     // 新增：断开编辑器事件总线
  void _on_core_initialized();
  void _on_core_shutdown();
  void _on_reset_ecs_nodes();           // 新增：重置状态回调
  void _on_clear_ecs_nodes();           // 新增：清除实体回调
  
  // 公开方法
  bool is_entity_created() const;       // 新增：检查实体是否已创建
};
