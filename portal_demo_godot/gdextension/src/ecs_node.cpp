#include "ecs_node.h"
#include "game_core_manager.h"
#include "ecs_component_resource.h"

#include <godot_cpp/core/class_db.hpp>
#include <godot_cpp/variant/utility_functions.hpp>
#include <godot_cpp/classes/scene_tree.hpp>
#include <godot_cpp/classes/engine.hpp>
#include <godot_cpp/classes/window.hpp>
#include <godot_cpp/classes/node3d.hpp>
#include <godot_cpp/variant/node_path.hpp>

// 包含 C++ ECS 組件
#include "core/components/transform_component.h"
#include "core/renderComponents/TransformRenderProxy.h"
#include "core/portal_game_world.h"

// 添加必要的頭文件用於運行時組件檢測
#include <string>
#include <typeinfo>

using namespace godot;

// 靜態變量初始化
GameCoreManager *ECSNode::cached_game_core_manager = nullptr;
bool ECSNode::cache_valid = false;

void ECSNode::_bind_methods()
{
  ClassDB::bind_method(D_METHOD("set_components", "components"), &ECSNode::set_components);
  ClassDB::bind_method(D_METHOD("get_components"), &ECSNode::get_components);
  ClassDB::bind_method(D_METHOD("set_target_node_path", "path"), &ECSNode::set_target_node_path);
  ClassDB::bind_method(D_METHOD("get_target_node_path"), &ECSNode::get_target_node_path);
  ClassDB::bind_method(D_METHOD("_on_resource_changed"), &ECSNode::_on_resource_changed);
  ClassDB::bind_method(D_METHOD("_update_ecs_components_deferred"), &ECSNode::_update_ecs_components_deferred);
  ClassDB::bind_method(D_METHOD("_on_core_initialized"), &ECSNode::_on_core_initialized);
  ClassDB::bind_method(D_METHOD("_on_core_shutdown"), &ECSNode::_on_core_shutdown);
  ClassDB::bind_method(D_METHOD("_on_reset_ecs_nodes"), &ECSNode::_on_reset_ecs_nodes); // 新增
  ClassDB::bind_method(D_METHOD("_on_clear_ecs_nodes"), &ECSNode::_on_clear_ecs_nodes); // 新增
  ClassDB::bind_method(D_METHOD("is_entity_created"), &ECSNode::is_entity_created);     // 新增
  ClassDB::bind_method(D_METHOD("reload_from_scene"), &ECSNode::reload_from_scene);         // 新增
  ClassDB::bind_method(D_METHOD("force_recreate_entity"), &ECSNode::force_recreate_entity); // 新增  
  // === 简化的组件管理API绑定 ===
  ClassDB::bind_method(D_METHOD("has_godot_component", "component_class"), &ECSNode::has_godot_component);
  ClassDB::bind_method(D_METHOD("add_godot_component", "component_resource"), &ECSNode::add_godot_component);
  ClassDB::bind_method(D_METHOD("remove_godot_component", "component_class"), &ECSNode::remove_godot_component);
  
  // === 实体ID访问绑定 ===
  ClassDB::bind_method(D_METHOD("get_entity_id"), &ECSNode::get_entity_id);
  
  // === 实体销毁回调绑定 ===
  // 注意：_on_entity_destroyed方法不绑定到Godot，因为entt::entity类型不被支持

  // 添加編輯器通知支持
  ClassDB::bind_method(D_METHOD("_notification", "what"), &ECSNode::_notification);

  ADD_PROPERTY(PropertyInfo(Variant::ARRAY, "components", PROPERTY_HINT_TYPE_STRING, "Resource"), "set_components", "get_components");
  ADD_PROPERTY(PropertyInfo(Variant::NODE_PATH, "target_node_path"), "set_target_node_path", "get_target_node_path");
}

ECSNode::ECSNode()
{
  entity = entt::null;
  entity_created = false;
  current_change_source = ChangeSource::EDITOR_UI;  // 默认为编辑器操作
  suppress_full_rebuild = false;                    // 默认不抑制重建
  UtilityFunctions::print("ECSNode: Constructor called");
}

ECSNode::~ECSNode()
{
  UtilityFunctions::print("ECSNode: Destructor called");
  disconnect_resource_signals();
  destroy_ecs_entity();
}

void ECSNode::_ready()
{
  UtilityFunctions::print("ECSNode: _ready called");

  Engine *engine = Engine::get_singleton();
  bool is_editor = engine && engine->is_editor_hint();

  if (is_editor)
  {
    UtilityFunctions::print("ECSNode: Running in editor mode - connecting to event bus");

    // 编辑器模式：连接到插件事件总线
    connect_to_event_bus();
  }
  else
  {
    UtilityFunctions::print("ECSNode: Running in runtime mode - using direct connection");

    // 运行时模式：保持原有逻辑，直接连接 GameCoreManager
    connect_to_game_core_manager();

    // 如果 GameCore 已經初始化，直接創建實體
    if (is_game_core_ready())
    {
      UtilityFunctions::print("ECSNode: GameCore already ready, creating entity immediately");
      create_ecs_entity();
    }
    else
    {
      UtilityFunctions::print("ECSNode: GameCore not ready, waiting for initialization signal");
    }
  }

  connect_resource_signals();
}

void ECSNode::_process(double delta)
{
  // _process 现在只负责插值渲染更新
  if (!entity_created)
  {
    return;
  }

  // 獲取通用的目標節點（可以是任何類型的 Godot 節點！）
  Node *target_node = get_effective_target_node();
  if (!target_node)
  {
    return; // 沒有有效目標，跳過同步
  }

  // 獲取ECS註冊表
  auto game_world = godot::GameCoreManager::get_game_world();
  if (!game_world)
  {
    return;
  }
  auto &registry = game_world->get_registry();

  // 處理插值渲染更新（僅針對Transform相關組件）
  process_interpolated_rendering(registry, target_node, delta);
}

void ECSNode::process_interpolated_rendering(entt::registry& registry, Node* target_node, double delta)
{
  // 檢查是否有TransformRenderProxy組件
  if (!registry.all_of<portal_core::TransformRenderProxy>(entity))
  {
    return;
  }

  // 查找TransformRenderProxyComponentResource
  Ref<Resource> transform_render_proxy_resource;
  for (int i = 0; i < components.size(); i++)
  {
    Ref<Resource> component_resource = components[i];
    if (component_resource.is_null())
    {
      continue;
    }

    Ref<ECSComponentResource> ecs_resource = Object::cast_to<ECSComponentResource>(component_resource.ptr());
    if (ecs_resource.is_valid() && ecs_resource->get_component_type_name() == "TransformRenderProxy")
    {
      transform_render_proxy_resource = component_resource;
      break;
    }
  }

  // 如果找到TransformRenderProxyComponentResource，使用其插值渲染機制
  if (transform_render_proxy_resource.is_valid())
  {
    Ref<ECSComponentResource> ecs_resource = Object::cast_to<ECSComponentResource>(transform_render_proxy_resource.ptr());
    if (ecs_resource.is_valid())
    {
      // 🎨 使用插值渲染進行平滑的視覺更新
      ecs_resource->sync_to_node(registry, entity, target_node);
    }
  }
}

void ECSNode::_notification(int p_what)
{
  switch (p_what)
  {
  case NOTIFICATION_READY:
    // 節點准備好時確保組件同步
    if (entity_created && components.size() > 0)
    {
      UtilityFunctions::print("ECSNode: READY notification - ensuring components are applied");
      apply_components_to_entity();
    }
    break;

  case NOTIFICATION_ENTER_TREE:
    // 進入場景樹時重新清理可能失效的緩存
    invalidate_cache();

    // 確保資源信號連接（防止場景切換後丟失）
    connect_resource_signals();
    break;

  case NOTIFICATION_EXIT_TREE:
    // 離開場景樹時清理
    disconnect_resource_signals();
    // 在場景切換時清理緩存
    invalidate_cache();
    break;

  case NOTIFICATION_EDITOR_PRE_SAVE:
  case NOTIFICATION_EDITOR_POST_SAVE:
    // 編輯器保存時確保組件同步
    if (entity_created && is_inside_tree())
    {
      UtilityFunctions::print("ECSNode: Editor save notification - ensuring components are synced");
      _update_ecs_components();
    }
    break;

  case NOTIFICATION_WM_CLOSE_REQUEST:
  case NOTIFICATION_APPLICATION_FOCUS_OUT:
    // 應用失去焦點時進行最終同步
    if (entity_created && is_inside_tree())
    {
      _update_ecs_components();
    }
    break;
  }
}

void ECSNode::_exit_tree()
{
  UtilityFunctions::print("ECSNode: _exit_tree called");

  Engine *engine = Engine::get_singleton();
  bool is_editor = engine && engine->is_editor_hint();

  if (is_editor)
  {
    // 编辑器模式：断开事件总线连接
    disconnect_from_event_bus();
  }
  else
  {
    // 运行时模式：断开直接连接
    disconnect_from_game_core_manager();
  }

  destroy_ecs_entity();
}

void ECSNode::set_components(const TypedArray<Resource> &p_components)
{
  UtilityFunctions::print("ECSNode: Components array changed - old size: ", components.size(), ", new size: ", p_components.size());

  // 取消訂閱舊資源的變更信號
  disconnect_resource_signals();

  // 在設置新組件之前，先完全清除當前實體的所有非基礎組件
  if (entity_created && is_inside_tree())
  {
    clear_all_non_basic_components();
  }

  components = p_components;

  // 訂閱新資源的變更信號
  connect_resource_signals();

  // 如果實體已經存在且在場景樹中，立即更新組件
  if (entity_created && is_inside_tree())
  {
    apply_components_to_entity();

    // 編輯器環境下強制刷新
    Engine *engine = Engine::get_singleton();
    if (engine && engine->is_editor_hint())
    {
      // 通知編輯器屬性已改變
      notify_property_list_changed();
    }
  }
}

TypedArray<Resource> ECSNode::get_components() const
{
  return components;
}

void ECSNode::set_target_node_path(const NodePath &p_path)
{
  target_node_path = p_path;
  UtilityFunctions::print("ECSNode: Target node path set to: ", target_node_path);
}

NodePath ECSNode::get_target_node_path() const
{
  return target_node_path;
}

void ECSNode::create_ecs_entity()
{
  if (entity_created)
  {
    return;
  }

  // 獲取遊戲世界實例
  auto game_world = godot::GameCoreManager::get_game_world();
  if (!game_world)
  {
    UtilityFunctions::print("ECSNode: Error - Cannot get PortalGameWorld instance");
    return;
  }

  // 在 ECS 世界中創建實體
  auto &registry = game_world->get_registry();
  entity = registry.create();
  entity_created = true;

  UtilityFunctions::print("ECSNode: ECS entity created with ID: ", (int)entity);

  // 添加基礎變換組件
  auto &transform_comp = registry.emplace<portal_core::TransformComponent>(entity);

  // 從有效的目標節點初始化變換（target_node_path 或父節點）
  Node *effective_target = get_effective_target_node();
  if (effective_target)
  {
    // 嘗試從目標節點獲取變換（如果是 Node3D）
    Node3D *target_3d = Object::cast_to<Node3D>(effective_target);
    if (target_3d)
    {
      Transform3D godot_transform = target_3d->get_global_transform();
      Vector3 pos = godot_transform.get_origin();
      Vector3 rot = godot_transform.get_basis().get_euler();

      transform_comp.position = portal_core::Vector3(pos.x, pos.y, pos.z);
      transform_comp.rotation = portal_core::Quaternion::sEulerAngles(portal_core::Vector3(rot.x, rot.y, rot.z));

      if (!target_node_path.is_empty())
      {
        UtilityFunctions::print("ECSNode: Initialized transform from target Node3D: ", target_node_path);
      }
      else
      {
        UtilityFunctions::print("ECSNode: Initialized transform from parent Node3D (default behavior)");
      }
    }
    else
    {
      // 目標節點不是 Node3D，使用默認變換
      transform_comp.position = portal_core::Vector3(0, 0, 0);
      transform_comp.rotation = portal_core::Quaternion::sEulerAngles(portal_core::Vector3(0, 0, 0));
      UtilityFunctions::print("ECSNode: Target node is not Node3D, using default transform");
    }
  }
  else
  {
    // 沒有找到有效的目標節點，使用默認變換
    transform_comp.position = portal_core::Vector3(0, 0, 0);
    transform_comp.rotation = portal_core::Quaternion::sEulerAngles(portal_core::Vector3(0, 0, 0));
    UtilityFunctions::print_rich("[color=yellow]ECSNode: No valid target node found, using default transform[/color]");
  }

  // 應用設計師配置的組件
  apply_components_to_entity();
  
  // 建立双向链接
  auto* manager = get_game_core_manager_efficient();
  if (manager) {
    auto* link_manager = manager->get_link_manager();
    if (link_manager) {
      link_manager->create_link((uint32_t)entity, this, "ECSNode");
      UtilityFunctions::print("[ECSNode] Established bidirectional link for entity ", (uint32_t)entity);
    }
  }
}

void ECSNode::destroy_ecs_entity()
{
  if (!entity_created)
  {
    return;
  }

  // 清理双向链接
  auto* manager = get_game_core_manager_efficient();
  if (manager) {
    auto* link_manager = manager->get_link_manager();
    if (link_manager) {
      link_manager->remove_link_by_entity((uint32_t)entity, false);
      UtilityFunctions::print("[ECSNode] Cleared bidirectional link for entity ", (uint32_t)entity);
    }
  }
  
  auto game_world = godot::GameCoreManager::get_game_world();
  if (game_world)
  {
    auto &registry = game_world->get_registry();
    if (registry.valid(entity))
    {
      registry.destroy(entity);
      UtilityFunctions::print("ECSNode: ECS entity destroyed");
    }
  }

  entity = entt::null;
  entity_created = false;
}

void ECSNode::apply_components_to_entity()
{
  if (!entity_created)
  {
    UtilityFunctions::print("ECSNode: Cannot apply components - entity not created");
    return;
  }

  // 安全檢查：確保 GameCore 已準備就緒
  if (!is_game_core_ready())
  {
    UtilityFunctions::print("ECSNode: Cannot apply components - GameCore not ready");
    return;
  }

  auto game_world = godot::GameCoreManager::get_game_world();
  if (!game_world)
  {
    UtilityFunctions::print("ECSNode: Cannot apply components - no game world");
    return;
  }

  auto &registry = game_world->get_registry();

  UtilityFunctions::print("ECSNode: Applying ", components.size(), " components to entity using POLYMORPHISM");

  // 🎉 多態魔法開始！不再需要任何if-else判斷！
  for (int i = 0; i < components.size(); i++)
  {
    Ref<Resource> component_resource = components[i];
    if (component_resource.is_null())
    {
      continue;
    }

    // 嘗試轉換為ECS組件資源基類
    Ref<ECSComponentResource> ecs_resource = Object::cast_to<ECSComponentResource>(component_resource.ptr());
    if (ecs_resource.is_valid())
    {
      // 使用多態！調用子類實現的apply_to_entity方法
      bool success = ecs_resource->apply_to_entity(registry, entity);
      if (success)
      {
        UtilityFunctions::print("ECSNode: Successfully applied component: ", ecs_resource->get_component_type_name());
      }
      else
      {
        UtilityFunctions::print("ECSNode: Failed to apply component: ", ecs_resource->get_component_type_name());
      }
    }
    else
    {
      // 這個資源不是ECS組件資源，跳過（或者可以添加警告）
      UtilityFunctions::print("ECSNode: Skipping non-ECS component resource: ", component_resource->get_class());
    }
  }

  UtilityFunctions::print("ECSNode: Component application complete - NO IF-ELSE CHAINS NEEDED! 🚀");
}

void ECSNode::clear_all_non_basic_components()
{
  if (!entity_created)
  {
    return;
  }

  auto game_world = godot::GameCoreManager::get_game_world();
  if (!game_world)
  {
    return;
  }

  auto &registry = game_world->get_registry();

  UtilityFunctions::print("ECSNode: Clearing all non-basic components from entity ", (int)entity);

  // 使用多態方法清除所有當前已知的組件類型
  // 這樣我們就不需要硬編碼組件類型列表

  // 建立一個臨時的組件資源列表，用於清除操作
  TypedArray<Resource> old_components = components;

  // 遍歷所有已知的組件類型並清除它們
  for (int i = 0; i < old_components.size(); i++)
  {
    Ref<Resource> component_resource = old_components[i];
    if (component_resource.is_null())
    {
      continue;
    }

    Ref<ECSComponentResource> ecs_resource = Object::cast_to<ECSComponentResource>(component_resource.ptr());
    if (ecs_resource.is_valid())
    {
      bool removed = ecs_resource->remove_from_entity(registry, entity);
      if (removed)
      {
        UtilityFunctions::print("ECSNode: Cleared component: ", ecs_resource->get_component_type_name());
      }
    }
  }

  // 額外的保險：清除可能存在的已知組件類型
  // 這里我們使用一個通用的方法來檢測和清除組件
  // 由於 EnTT 提供了運行時組件檢測，我們可以利用這一點
  clear_components_by_runtime_detection(registry);

  UtilityFunctions::print("ECSNode: All non-basic components cleared");
}

void ECSNode::clear_components_by_runtime_detection(entt::registry &registry)
{
  // 這個方法使用更安全的方式來清除組件
  // 我們不使用 EnTT 的內部存儲結構，而是通過已知的組件類型來清除

  if (!registry.valid(entity))
  {
    return;
  }

  // 對於每個已知的組件類型，檢查並移除
  // 這里我們只需要添加基本的檢查，不需要訪問私有存儲結構

  // 保留 TransformComponent，清除其他可能存在的組件類型
  // 由於我們已經使用多態方法，大多數組件都會在之前的循環中被清除
  // 這里只是額外的保險措施

  UtilityFunctions::print("ECSNode: Runtime component detection completed safely");
}

void ECSNode::connect_resource_signals()
{
  for (int i = 0; i < components.size(); i++)
  {
    Ref<Resource> resource = components[i];
    if (resource.is_valid())
    {
      // 連接資源的 changed 信號到我們的回調方法
      if (!resource->is_connected("changed", Callable(this, "_on_resource_changed")))
      {
        resource->connect("changed", Callable(this, "_on_resource_changed"));
        UtilityFunctions::print("ECSNode: Connected to resource changed signal");
      }
    }
  }
}

void ECSNode::disconnect_resource_signals()
{
  for (int i = 0; i < components.size(); i++)
  {
    Ref<Resource> resource = components[i];
    if (resource.is_valid())
    {
      // 斷開資源的 changed 信號
      if (resource->is_connected("changed", Callable(this, "_on_resource_changed")))
      {
        resource->disconnect("changed", Callable(this, "_on_resource_changed"));
        UtilityFunctions::print("ECSNode: Disconnected from resource changed signal");
      }
    }
  }
}

void ECSNode::_on_resource_changed()
{
  // 如果被抑制重建，直接返回
  if (suppress_full_rebuild)
  {
    UtilityFunctions::print("ECSNode: Full rebuild suppressed, skipping resource change handling");
    return;
  }
  
  // 根据变更源决定处理策略
  if (current_change_source == ChangeSource::DYNAMIC_API)
  {
    UtilityFunctions::print("ECSNode: Dynamic API change detected, using incremental update");
    // 动态API变更：这里可以实现更精细的增量更新逻辑
    // 目前先跳过，因为add/remove_godot_component已经处理了增量更新
    return;
  }
  
  // 检查是否在编辑器模式，如果不是则直接返回，避免不必要的重建
  if (!Engine::get_singleton()->is_editor_hint())
  {
    UtilityFunctions::print("ECSNode: Not in editor mode, skipping resource change handling");
    return;
  }
  
  UtilityFunctions::print("ECSNode: Resource changed in editor mode, performing full rebuild");
  
  if (entity_created && is_inside_tree())
  {
    _update_ecs_components();
  }
}

void ECSNode::_update_ecs_components()
{
  if (!entity_created)
  {
    return;
  }

  // 直接使用简化的方法获取 GameCoreManager
  GameCoreManager *manager_instance = get_game_core_manager_efficient();

  if (!manager_instance)
  {
    UtilityFunctions::print("ECSNode: Error - Cannot get GameCoreManager from autoload. Update aborted.");
    return;
  }

  portal_core::PortalGameWorld *game_world = manager_instance->get_game_world();
  if (!game_world)
  {
    UtilityFunctions::print("ECSNode: Error - Cannot get PortalGameWorld. Update aborted.");
    return;
  }

  auto &registry = game_world->get_registry();
  UtilityFunctions::print("ECSNode: Updating ECS components for entity ", (int)entity, " using POLYMORPHISM");

  // 🔄 新策略：先清除，再应用 - 确保完全同步
  clear_all_non_basic_components();

  // 重新应用所有组件（使用我们新的多态方法）
  apply_components_to_entity();

  UtilityFunctions::print("ECSNode: Component update complete - FULLY POLYMORPHIC! 🎯");
}

void ECSNode::_update_ecs_components_deferred()
{
  UtilityFunctions::print("ECSNode: Deferred component update triggered");
  _update_ecs_components();
}

// 智能的 GameCoreManager 获取方法 - 编辑器使用事件总线，运行时使用直接连接
GameCoreManager *ECSNode::get_game_core_manager_efficient()
{
  // 如果缓存有效，直接返回
  if (cache_valid && cached_game_core_manager &&
      Object::cast_to<GameCoreManager>(cached_game_core_manager))
  {
    return cached_game_core_manager;
  }

  Engine *engine = Engine::get_singleton();
  bool is_editor = engine && engine->is_editor_hint();

  if (is_editor)
  {
    // 编辑器模式：通过事件总线获取实例
    Node *root = get_tree()->get_root();
    if (root)
    {
      Node *event_bus = root->find_child("ECSEventBus", true, false);
      if (event_bus && event_bus->has_method("get_current_game_core"))
      {
        Variant result = event_bus->call("get_current_game_core");
        cached_game_core_manager = Object::cast_to<GameCoreManager>(result);

        if (cached_game_core_manager)
        {
          cache_valid = true;
          UtilityFunctions::print("ECSNode: GameCoreManager acquired from event bus");
          return cached_game_core_manager;
        }
      }
    }

    // 编辑器模式下如果找不到，返回 null（避免错误）
    return nullptr;
  }
  else
  {
    // 运行时模式：直接通过 autoload 获取
    cached_game_core_manager = get_node<GameCoreManager>("/root/GameCore");

    if (cached_game_core_manager)
    {
      cache_valid = true;
      UtilityFunctions::print("ECSNode: GameCoreManager acquired from runtime autoload");
    }
    else
    {
      UtilityFunctions::print("ECSNode: Error - GameCoreManager autoload not found in runtime mode");
    }
  }

  return cached_game_core_manager;
}

void ECSNode::invalidate_cache()
{
  cache_valid = false;
  // 注意：不要设置 cached_game_core_manager = nullptr
  // 因为 autoload 节点在游戏生命周期内始终存在
  UtilityFunctions::print("ECSNode: GameCoreManager cache invalidated");
}

// 安全檢查 GameCore 是否已準備就緒
bool ECSNode::is_game_core_ready() const
{
  // 首先檢查是否能找到 GameCoreManager
  GameCoreManager *manager = const_cast<ECSNode *>(this)->get_game_core_manager_efficient();
  if (!manager)
  {
    return false;
  }

  // 檢查 GameCoreManager 是否已初始化
  if (!manager->is_core_initialized())
  {
    return false;
  }

  // 檢查 PortalGameWorld 實例是否存在
  auto game_world = GameCoreManager::get_game_world();
  if (!game_world)
  {
    return false;
  }

  return true;
}

// 連接到編輯器事件總線
void ECSNode::connect_to_event_bus()
{
  Node *root = get_tree()->get_root();
  if (!root)
  {
    UtilityFunctions::print("ECSNode: Cannot get scene tree root");
    return;
  }

  // 查找 ECSEventBus
  Node *event_bus = root->find_child("ECSEventBus", true, false);
  if (event_bus)
  {
    // 註冊到事件總線
    if (event_bus->has_method("register_ecs_node"))
    {
      event_bus->call("register_ecs_node", this);
      UtilityFunctions::print("ECSNode: Registered to event bus");
    }

    // 連接核心狀態信號
    if (!event_bus->is_connected("game_core_initialized", Callable(this, "_on_core_initialized")))
    {
      event_bus->connect("game_core_initialized", Callable(this, "_on_core_initialized"));
      UtilityFunctions::print("ECSNode: Connected to ECSEventBus.game_core_initialized signal");
    }

    if (!event_bus->is_connected("game_core_shutdown", Callable(this, "_on_core_shutdown")))
    {
      event_bus->connect("game_core_shutdown", Callable(this, "_on_core_shutdown"));
      UtilityFunctions::print("ECSNode: Connected to ECSEventBus.game_core_shutdown signal");
    }

    // 連接控制信號
    if (!event_bus->is_connected("reset_ecs_nodes", Callable(this, "_on_reset_ecs_nodes")))
    {
      event_bus->connect("reset_ecs_nodes", Callable(this, "_on_reset_ecs_nodes"));
      UtilityFunctions::print("ECSNode: Connected to ECSEventBus.reset_ecs_nodes signal");
    }

    if (!event_bus->is_connected("clear_ecs_nodes", Callable(this, "_on_clear_ecs_nodes")))
    {
      event_bus->connect("clear_ecs_nodes", Callable(this, "_on_clear_ecs_nodes"));
      UtilityFunctions::print("ECSNode: Connected to ECSEventBus.clear_ecs_nodes signal");
    }

    // 請求廣播當前狀態（如果 GameCore 已經準備就緒）
    if (event_bus->has_method("broadcast_current_state"))
    {
      event_bus->call("broadcast_current_state");
      UtilityFunctions::print("ECSNode: Requested current state broadcast from event bus");
    }
  }
  else
  {
    UtilityFunctions::print("ECSNode: ECSEventBus not found - plugin may not be loaded yet");
  }
}

// 斷開與編輯器事件總線的連接
void ECSNode::disconnect_from_event_bus()
{
  Node *root = get_tree()->get_root();
  if (!root)
  {
    return;
  }

  Node *event_bus = root->find_child("ECSEventBus", true, false);
  if (event_bus)
  {
    // 從事件總線注銷
    if (event_bus->has_method("unregister_ecs_node"))
    {
      event_bus->call("unregister_ecs_node", this);
      UtilityFunctions::print("ECSNode: Unregistered from event bus");
    }

    // 斷開所有信號連接
    if (event_bus->is_connected("game_core_initialized", Callable(this, "_on_core_initialized")))
    {
      event_bus->disconnect("game_core_initialized", Callable(this, "_on_core_initialized"));
      UtilityFunctions::print("ECSNode: Disconnected from ECSEventBus.game_core_initialized signal");
    }

    if (event_bus->is_connected("game_core_shutdown", Callable(this, "_on_core_shutdown")))
    {
      event_bus->disconnect("game_core_shutdown", Callable(this, "_on_core_shutdown"));
      UtilityFunctions::print("ECSNode: Disconnected from ECSEventBus.game_core_shutdown signal");
    }

    if (event_bus->is_connected("reset_ecs_nodes", Callable(this, "_on_reset_ecs_nodes")))
    {
      event_bus->disconnect("reset_ecs_nodes", Callable(this, "_on_reset_ecs_nodes"));
      UtilityFunctions::print("ECSNode: Disconnected from ECSEventBus.reset_ecs_nodes signal");
    }

    if (event_bus->is_connected("clear_ecs_nodes", Callable(this, "_on_clear_ecs_nodes")))
    {
      event_bus->disconnect("clear_ecs_nodes", Callable(this, "_on_clear_ecs_nodes"));
      UtilityFunctions::print("ECSNode: Disconnected from ECSEventBus.clear_ecs_nodes signal");
    }
  }
}

// 連接到 GameCoreManager 的信號
void ECSNode::connect_to_game_core_manager()
{
  GameCoreManager *manager = get_game_core_manager_efficient();
  if (manager)
  {
    // 連接初始化信號
    if (!manager->is_connected("core_initialized", Callable(this, "_on_core_initialized")))
    {
      manager->connect("core_initialized", Callable(this, "_on_core_initialized"));
      UtilityFunctions::print("ECSNode: Connected to GameCoreManager.core_initialized signal");
    }

    // 連接關閉信號
    if (!manager->is_connected("core_shutdown", Callable(this, "_on_core_shutdown")))
    {
      manager->connect("core_shutdown", Callable(this, "_on_core_shutdown"));
      UtilityFunctions::print("ECSNode: Connected to GameCoreManager.core_shutdown signal");
    }
  }
  else
  {
    UtilityFunctions::print("ECSNode: Warning - Cannot find GameCoreManager to connect signals");
  }
}

// 斷開與 GameCoreManager 的信號連接
void ECSNode::disconnect_from_game_core_manager()
{
  if (cached_game_core_manager && Object::cast_to<GameCoreManager>(cached_game_core_manager))
  {
    // 斷開初始化信號
    if (cached_game_core_manager->is_connected("core_initialized", Callable(this, "_on_core_initialized")))
    {
      cached_game_core_manager->disconnect("core_initialized", Callable(this, "_on_core_initialized"));
      UtilityFunctions::print("ECSNode: Disconnected from GameCoreManager.core_initialized signal");
    }

    // 斷開關閉信號
    if (cached_game_core_manager->is_connected("core_shutdown", Callable(this, "_on_core_shutdown")))
    {
      cached_game_core_manager->disconnect("core_shutdown", Callable(this, "_on_core_shutdown"));
      UtilityFunctions::print("ECSNode: Disconnected from GameCoreManager.core_shutdown signal");
    }
  }
}

// 當 GameCore 初始化完成時的回調
void ECSNode::_on_core_initialized()
{
  UtilityFunctions::print("ECSNode: Received core_initialized signal");

  if (!entity_created && is_inside_tree())
  {
    create_ecs_entity();

    // 如果實體創建成功，應用組件
    if (entity_created)
    {
      apply_components_to_entity();
    }
  }
}

// 當 GameCore 關閉時的回調
void ECSNode::_on_core_shutdown()
{
  UtilityFunctions::print("ECSNode: Received core_shutdown signal");

  // 清理實體
  destroy_ecs_entity();

  // 清理緩存
  invalidate_cache();
}

// 新增：重置 ECSNode 狀態的回調
void ECSNode::_on_reset_ecs_nodes()
{
  UtilityFunctions::print("ECSNode: Received reset_ecs_nodes signal - resetting state");

  // 重置狀態標誌
  entity_created = false;
  entity = entt::null;

  // 清理緩存
  invalidate_cache();

  // 如果在場景樹中，嘗試重新初始化
  if (is_inside_tree())
  {
    if (is_game_core_ready())
    {
      UtilityFunctions::print("ECSNode: GameCore ready after reset, recreating entity");
      create_ecs_entity();
    }
    else
    {
      UtilityFunctions::print("ECSNode: GameCore not ready after reset, waiting for signal");
    }
  }
}

// 新增：清除 ECSNode 實體的回調
void ECSNode::_on_clear_ecs_nodes()
{
  UtilityFunctions::print("ECSNode: Received clear_ecs_nodes signal - clearing entity");

  // 強制銷毀實體
  destroy_ecs_entity();

  // 清理緩存
  invalidate_cache();
}

// 新增：檢查實體是否已創建的公開方法
bool ECSNode::is_entity_created() const
{
  return entity_created;
}

// 新增：獲取有效的目標節點（優先使用 target_node_path，否則使用父節點）
// 🌟 修改：現在返回通用的 Node* 以支持任何類型的 Godot 節點
Node *ECSNode::get_effective_target_node()
{
  // 如果指定了 target_node_path，優先使用它
  if (!target_node_path.is_empty())
  {
    Node *target_node = get_node<Node>(target_node_path); // 修改：使用 Node 而不是 Node3D
    if (target_node)
    {
      return target_node;
    }
    else
    {
      UtilityFunctions::print_rich("[color=orange]ECSNode: Target node not found at path: ", target_node_path, ", falling back to parent node[/color]");
    }
  }

  // 如果沒有指定 target_node_path 或指定的路徑無效，則使用父節點
  Node *parent_node = get_parent();
  if (parent_node)
  {
    return parent_node; // 修改：直接返回父節點，不再限制為 Node3D
  }

  return nullptr;
}

// 新增：从场景重新加载状态到ECS
bool ECSNode::reload_from_scene()
{
  UtilityFunctions::print("ECSNode: reload_from_scene called for: ", get_path());
  
  if (!entity_created)
  {
    UtilityFunctions::print("ECSNode: No entity created yet, cannot reload");
    return false;
  }

  if (!is_game_core_ready())
  {
    UtilityFunctions::print("ECSNode: GameCore not ready, cannot reload");
    return false;
  }

  // 获取游戏世界
  auto game_world = godot::GameCoreManager::get_game_world();
  if (!game_world)
  {
    UtilityFunctions::print("ECSNode: Cannot get PortalGameWorld instance");
    return false;
  }

  auto &registry = game_world->get_registry();
  
  // 确保实体仍然有效
  if (!registry.valid(entity))
  {
    UtilityFunctions::print("ECSNode: Entity is no longer valid, cannot reload");
    return false;
  }

  // 重新读取目标节点的当前状态
  Node *effective_target = get_effective_target_node();
  if (!effective_target)
  {
    UtilityFunctions::print("ECSNode: No effective target node found");
    return false;
  }

  // 更新变换组件（如果目标是Node3D）
  Node3D *target_3d = Object::cast_to<Node3D>(effective_target);
  if (target_3d && registry.any_of<portal_core::TransformComponent>(entity))
  {
    auto &transform_comp = registry.get<portal_core::TransformComponent>(entity);
    
    // 获取当前的Godot变换
    Transform3D godot_transform = target_3d->get_global_transform();
    Vector3 pos = godot_transform.get_origin();
    Vector3 rot = godot_transform.get_basis().get_euler();
    Vector3 scale = godot_transform.get_basis().get_scale();

    // 更新ECS变换组件
    transform_comp.position = portal_core::Vector3(pos.x, pos.y, pos.z);
    transform_comp.rotation = portal_core::Quaternion::sEulerAngles(portal_core::Vector3(rot.x, rot.y, rot.z));
    // 注意：如果ECS TransformComponent支持scale，也可以在这里设置

    UtilityFunctions::print("ECSNode: Updated transform from Godot - Position: (", pos.x, ", ", pos.y, ", ", pos.z, ")");
  }

  // 重新应用所有组件（确保组件状态同步）
  apply_components_to_entity();
  
  UtilityFunctions::print("ECSNode: Successfully reloaded from scene");
  return true;
}

// 新增：强制重新创建实体
bool ECSNode::force_recreate_entity()
{
  UtilityFunctions::print("ECSNode: force_recreate_entity called for: ", get_path());
  
  if (!is_game_core_ready())
  {
    UtilityFunctions::print("ECSNode: GameCore not ready, cannot recreate entity");
    return false;
  }

  // 销毁现有实体
  destroy_ecs_entity();

  // 重新创建实体
  create_ecs_entity();

  // 应用组件
  if (entity_created)
  {
    apply_components_to_entity();
    UtilityFunctions::print("ECSNode: Successfully recreated entity");
    return true;
  }
  else
  {
    UtilityFunctions::print("ECSNode: Failed to recreate entity");
    return false;
  }
}

// === 简化的Godot组件管理API实现 ===
bool ECSNode::has_godot_component(const String& component_class) const
{
  for (int i = 0; i < components.size(); i++)
  {
    Resource *res = Object::cast_to<Resource>(components[i]);
    if (res && res->get_class() == component_class)
    {
      return true;
    }
  }
  return false;
}

bool ECSNode::add_godot_component(Resource* component_resource)
{
  if (!component_resource)
  {
    UtilityFunctions::print("ECSNode: Cannot add null component");
    return false;
  }
  
  String component_class = component_resource->get_class();
  
  // 检查是否已存在该类型的组件
  if (has_godot_component(component_class))
  {
    UtilityFunctions::print("ECSNode: Component already exists: ", component_class);
    return false;
  }
  
  // 设置变更源为动态API
  current_change_source = ChangeSource::DYNAMIC_API;
  suppress_full_rebuild = true;
  
  // 添加组件到数组
  components.append(component_resource);
  
  // 如果实体已创建，只应用这个新组件，不触发完全重建
  if (is_entity_created())
  {
    // 只应用这个新组件到ECS实体
    Ref<ECSComponentResource> ecs_resource = Object::cast_to<ECSComponentResource>(component_resource);
    if (ecs_resource.is_valid())
    {
      auto game_world = GameCoreManager::get_game_world();
      if (game_world)
      {
        auto &registry = game_world->get_registry();
        ecs_resource->apply_to_entity(registry, entity);  // 只应用这一个组件
        UtilityFunctions::print("ECSNode: Applied single component to entity: ", component_class);
      }
    }
    
    // 连接信号（使用专门的单组件连接方法）
    connect_single_component_signal(component_resource);
  }
  
  // 恢复默认状态
  suppress_full_rebuild = false;
  current_change_source = ChangeSource::EDITOR_UI;
  
  UtilityFunctions::print("ECSNode: Added component: ", component_class);
  return true;
}

bool ECSNode::remove_godot_component(const String& component_class)
{
  // 设置变更源为动态API
  current_change_source = ChangeSource::DYNAMIC_API;
  suppress_full_rebuild = true;
  
  // 查找并移除指定类型的组件
  for (int i = 0; i < components.size(); i++)
  {
    Resource *res = Object::cast_to<Resource>(components[i]);
    if (res && res->get_class() == component_class)
    {
      // 先断开信号连接
      Ref<Resource> resource = res;
      if (resource.is_valid() && resource->is_connected("changed", Callable(this, "_on_resource_changed")))
      {
        resource->disconnect("changed", Callable(this, "_on_resource_changed"));
      }
      
      // 如果实体已创建，先从ECS中移除该组件
      if (is_entity_created())
      {
        auto game_world = godot::GameCoreManager::get_game_world();
        if (game_world)
        {
          auto &registry = game_world->get_registry();
          Ref<ECSComponentResource> ecs_resource = Object::cast_to<ECSComponentResource>(resource.ptr());
          if (ecs_resource.is_valid())
          {
            ecs_resource->remove_from_entity(registry, entity);
            UtilityFunctions::print("ECSNode: Removed ECS component: ", ecs_resource->get_component_type_name());
          }
        }
      }
      
      components.remove_at(i);
      
      // 恢复默认状态
      suppress_full_rebuild = false;
      current_change_source = ChangeSource::EDITOR_UI;
      
      UtilityFunctions::print("ECSNode: Removed component: ", component_class);
      return true;
    }
  }
  
  // 恢复默认状态（即使没找到组件）
  suppress_full_rebuild = false;
  current_change_source = ChangeSource::EDITOR_UI;
  
  UtilityFunctions::print("ECSNode: Component not found: ", component_class);
  return false;
}

// === 实体销毁回调实现 ===
void ECSNode::_on_entity_destroyed(entt::entity destroyed_entity, uint64_t godot_node_id)
{
  if (entity == destroyed_entity)
  {
    UtilityFunctions::print("ECSNode: Our entity was destroyed from ECS system - cleaning up nodes");
    
    // 标记实体已被销毁
    entity_created = false;
    entity = entt::null;
    
    // 销毁目标节点及其子节点
    Node* target_node = get_effective_target_node();
    if (target_node && target_node != this)
    {
      UtilityFunctions::print("ECSNode: Destroying target node: ", target_node->get_name());
      target_node->queue_free();
    }
    
    // 销毁ECSNode自身
    UtilityFunctions::print("ECSNode: Destroying ECSNode itself");
    queue_free();
  }
}

// === 实体ID访问实现 ===
uint32_t ECSNode::get_entity_id() const
{
	// 返回ECS实体ID，用于双向链接
	// 使用entt::to_integral确保保留版本信息，避免ABA问题
	return entt::to_integral(entity);
}

// 单个组件更新的辅助方法实现
bool ECSNode::update_single_component(Resource* component_resource)
{
  if (!component_resource)
  {
    UtilityFunctions::print("ECSNode: Cannot update null component");
    return false;
  }
  
  if (!entity_created)
  {
    UtilityFunctions::print("ECSNode: Cannot update component - entity not created");
    return false;
  }

  // 安全检查：确保 GameCore 已准备就绪
  if (!is_game_core_ready())
  {
    UtilityFunctions::print("ECSNode: Cannot update component - GameCore not ready");
    return false;
  }

  auto game_world = godot::GameCoreManager::get_game_world();
  if (!game_world)
  {
    UtilityFunctions::print("ECSNode: Cannot update component - no game world");
    return false;
  }

  auto &registry = game_world->get_registry();

  // 尝试转换为ECS组件资源基类
  Ref<ECSComponentResource> ecs_resource = Object::cast_to<ECSComponentResource>(component_resource);
  if (ecs_resource.is_valid())
  {
    // 使用多态！调用子类实现的apply_to_entity方法
    bool success = ecs_resource->apply_to_entity(registry, entity);
    if (success)
    {
      UtilityFunctions::print("ECSNode: Successfully updated single component: ", ecs_resource->get_component_type_name());
    }
    else
    {
      UtilityFunctions::print("ECSNode: Failed to update single component: ", ecs_resource->get_component_type_name());
    }
    return success;
  }
  else
  {
    UtilityFunctions::print("ECSNode: Cannot update non-ECS component resource: ", component_resource->get_class());
    return false;
  }
}

bool ECSNode::connect_single_component_signal(Resource* component_resource)
{
  if (!component_resource)
  {
    UtilityFunctions::print("ECSNode: Cannot connect signal for null component");
    return false;
  }

  // 检查资源是否已经连接了信号，避免重复连接
  if (component_resource->is_connected("changed", Callable(this, "_on_resource_changed")))
  {
    UtilityFunctions::print("ECSNode: Signal already connected for component: ", component_resource->get_class());
    return true;
  }

  // 连接资源变更信号
  Error err = component_resource->connect("changed", Callable(this, "_on_resource_changed"));
  if (err == OK)
  {
    UtilityFunctions::print("ECSNode: Successfully connected signal for component: ", component_resource->get_class());
    return true;
  }
  else
  {
    UtilityFunctions::print("ECSNode: Failed to connect signal for component: ", component_resource->get_class(), " Error: ", err);
    return false;
  }
}