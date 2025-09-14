#include "game_core_manager.h"

#include <godot_cpp/core/class_db.hpp>
#include <godot_cpp/variant/utility_functions.hpp>
#include <godot_cpp/classes/engine.hpp>

// 包含 C++ 核心
#include "core/portal_game_world.h"

using namespace godot;

// 靜態成員初始化
int GameCoreManager::reference_count_ = 0;
GameCoreManager *GameCoreManager::editor_instance_ = nullptr;

// _bind_methods 用於向 Godot 暴露 C++ 函數或屬性
void GameCoreManager::_bind_methods()
{
  // 向 Godot 暴露管理方法
  ClassDB::bind_method(D_METHOD("initialize_core"), &GameCoreManager::initialize_core);
  ClassDB::bind_method(D_METHOD("shutdown_core"), &GameCoreManager::shutdown_core);
  ClassDB::bind_method(D_METHOD("is_core_initialized"), &GameCoreManager::is_core_initialized);

  // 編輯器持久化方法
  ClassDB::bind_method(D_METHOD("add_reference"), &GameCoreManager::add_reference);
  ClassDB::bind_method(D_METHOD("remove_reference"), &GameCoreManager::remove_reference);
  ClassDB::bind_method(D_METHOD("set_editor_persistent", "persistent"), &GameCoreManager::set_editor_persistent);
  ClassDB::bind_method(D_METHOD("is_editor_persistent"), &GameCoreManager::is_editor_persistent);
  ClassDB::bind_method(D_METHOD("request_shutdown"), &GameCoreManager::request_shutdown);
  ClassDB::bind_method(D_METHOD("force_shutdown"), &GameCoreManager::force_shutdown);
  ClassDB::bind_method(D_METHOD("is_pending_destruction"), &GameCoreManager::is_pending_destruction);

  // 暂停/恢复方法
  ClassDB::bind_method(D_METHOD("pause_updates"), &GameCoreManager::pause_updates);
  ClassDB::bind_method(D_METHOD("resume_updates"), &GameCoreManager::resume_updates);
  ClassDB::bind_method(D_METHOD("is_paused"), &GameCoreManager::is_paused);

  // 实体生成桥接方法
  ClassDB::bind_static_method("GameCoreManager", D_METHOD("spawn_godot_entity", "template_name", "parent", "overrides"), 
                             &GameCoreManager::spawn_godot_entity, DEFVAL(Variant()), DEFVAL(Dictionary()));
  ClassDB::bind_static_method("GameCoreManager", D_METHOD("get_available_godot_templates"), 
                             &GameCoreManager::get_available_godot_templates);
  ClassDB::bind_static_method("GameCoreManager", D_METHOD("has_godot_template", "template_name"), 
                             &GameCoreManager::has_godot_template);
  ClassDB::bind_static_method("GameCoreManager", D_METHOD("spawn_godot_entity_with_ecs_override", "template_name", "parent", "component_overrides", "general_overrides"), 
                             &GameCoreManager::spawn_godot_entity_with_ecs_override, DEFVAL(Variant()), DEFVAL(Dictionary()), DEFVAL(Dictionary()));
  ClassDB::bind_static_method("GameCoreManager", D_METHOD("clear_all_godot_entities"), 
                             &GameCoreManager::clear_all_godot_entities);
  ClassDB::bind_static_method("GameCoreManager", D_METHOD("get_godot_entity_count"), 
                             &GameCoreManager::get_godot_entity_count);
  ClassDB::bind_static_method("GameCoreManager", D_METHOD("get_template_properties", "template_path"), 
                             &GameCoreManager::get_template_properties);
  ClassDB::bind_static_method("GameCoreManager", D_METHOD("get_all_template_properties"), 
                             &GameCoreManager::get_all_template_properties);

  // 註冊信號
  ADD_SIGNAL(MethodInfo("core_initialized"));
  ADD_SIGNAL(MethodInfo("core_shutdown"));
  ADD_SIGNAL(MethodInfo("destruction_cancelled"));
}

// 構造函數：當節點被創建時調用
GameCoreManager::GameCoreManager()
    : time_passed_(0.0),
      core_initialized_(false),
      is_paused_(false), // 初始化暂停状态
      is_editor_mode_(false),
      editor_persistent_mode_(false),
      pending_destruction_(false),
      destruction_delay_(5.0), // 5秒延遲
      destruction_timer_(0.0)
{
  UtilityFunctions::print("GameCoreManager constructor called");

  // 檢測編輯器模式
  if (Engine::get_singleton()->is_editor_hint())
  {
    is_editor_mode_ = true;
    editor_persistent_mode_ = true; // 編輯器模式下默認持久化

    // 設置為編輯器實例
    if (!editor_instance_)
    {
      editor_instance_ = this;
      UtilityFunctions::print("GameCoreManager: Set as editor instance");
    }
  }

  add_reference();
}

// 析構函數：當節點被銷毀時調用
GameCoreManager::~GameCoreManager()
{
  UtilityFunctions::print("GameCoreManager destructor called");

  // 清理編輯器實例引用
  if (editor_instance_ == this)
  {
    editor_instance_ = nullptr;
  }

  remove_reference();
  force_shutdown(); // 強制關閉，不考慮延遲
}

void GameCoreManager::_ready()
{
  UtilityFunctions::print("GameCoreManager: _ready() called");
  initialize_core();
}

// _process 函數：每一幀都被調用
void GameCoreManager::_process(double delta)
{
  if (!core_initialized_)
  {
    return;
  }

  // 如果暂停，跳过更新（但仍处理延迟销毁）
  if (is_paused_)
  {
    // 仍然处理延迟销毁逻辑，即使在暂停状态
    if (pending_destruction_)
    {
      destruction_timer_ += delta;
      if (destruction_timer_ >= destruction_delay_)
      {
        if (is_editor_mode_ && editor_persistent_mode_)
        {
          pending_destruction_ = false;
          destruction_timer_ = 0.0;
          emit_signal("destruction_cancelled");
          return;
        }
        else
        {
          force_shutdown();
          return;
        }
      }
    }
    return; // 暂停时不更新游戏逻辑
  }

  time_passed_ += delta;

  // 處理延遲銷毀邏輯
  if (pending_destruction_)
  {
    destruction_timer_ += delta;
    if (destruction_timer_ >= destruction_delay_)
    {
      if (is_editor_mode_ && editor_persistent_mode_)
      {
        // 編輯器持久化模式：取消銷毀
        UtilityFunctions::print("GameCoreManager: Destruction cancelled due to editor persistent mode");
        pending_destruction_ = false;
        destruction_timer_ = 0.0;
        emit_signal("destruction_cancelled");
        return;
      }
      else
      {
        // 執行延遲銷毀
        force_shutdown();
        return;
      }
    }
  }

  // 更新 C++/Entt 核心系統
  auto *game_world = portal_core::PortalGameWorld::get_instance();
  if (game_world)
  {
    game_world->update_systems(static_cast<float>(delta));
  }
}

void GameCoreManager::_exit_tree()
{
  UtilityFunctions::print("GameCoreManager: _exit_tree() called");

  if (is_editor_mode_ && editor_persistent_mode_)
  {
    UtilityFunctions::print("GameCoreManager: Editor persistent mode - skipping shutdown");
    return;
  }

  request_shutdown();
}

void GameCoreManager::initialize_core()
{
  if (core_initialized_)
  {
    UtilityFunctions::print("GameCore already initialized");
    return;
  }

  UtilityFunctions::print("Initializing game core...");

  // 初始化 Portal Game World
  portal_core::PortalGameWorld::create_instance();

  auto *game_world = portal_core::PortalGameWorld::get_instance();
  if (game_world)
  {
    UtilityFunctions::print("Game core initialized successfully!");
    core_initialized_ = true;

    // 發出初始化完成信號
    emit_signal("core_initialized");
  }
  else
  {
    UtilityFunctions::print("ERROR: Game core initialization failed!");
  }
}

void GameCoreManager::shutdown_core()
{
  if (!core_initialized_)
  {
    return;
  }

  UtilityFunctions::print("Shutting down game core...");

  // 發出關閉信號（在實際關閉之前）
  emit_signal("core_shutdown");

  // 获取游戏世界实例进行彻底清理
  auto *game_world = portal_core::PortalGameWorld::get_instance();
  if (game_world)
  {
    // 清理所有 ECS 实体（包括物理实体）
    auto &registry = game_world->get_registry();
    UtilityFunctions::print("GameCoreManager: Clearing all ECS entities before shutdown...");
    registry.clear();

    // 获取并清理物理系统
    auto &system_manager = game_world->get_system_manager();
    UtilityFunctions::print("GameCoreManager: Forcing physics system cleanup...");
    system_manager.force_cleanup_physics();

    UtilityFunctions::print("GameCoreManager: Pre-shutdown cleanup completed");
  }

  // 銷毀 Portal Game World
  portal_core::PortalGameWorld::destroy_instance();

  core_initialized_ = false;
  UtilityFunctions::print("Game core shut down");
}

// 編輯器持久化管理方法
void GameCoreManager::add_reference()
{
  reference_count_++;
  UtilityFunctions::print("GameCoreManager: Reference count increased to ", reference_count_);
}

void GameCoreManager::remove_reference()
{
  reference_count_--;
  UtilityFunctions::print("GameCoreManager: Reference count decreased to ", reference_count_);

  if (reference_count_ <= 0 && !is_editor_mode_)
  {
    // 非編輯器模式下，引用計數為0時自動關閉
    shutdown_core();
  }
}

void GameCoreManager::set_editor_persistent(bool persistent)
{
  editor_persistent_mode_ = persistent;
  UtilityFunctions::print("GameCoreManager: Editor persistent mode set to ", persistent);

  if (!persistent && pending_destruction_)
  {
    // 如果取消持久化且有待處理的銷毀，立即執行
    force_shutdown();
  }
}

void GameCoreManager::request_shutdown()
{
  if (is_editor_mode_ && editor_persistent_mode_)
  {
    UtilityFunctions::print("GameCoreManager: Shutdown requested but editor persistent mode is active");
    pending_destruction_ = true;
    destruction_timer_ = 0.0;
    return;
  }

  shutdown_core();
}

void GameCoreManager::force_shutdown()
{
  pending_destruction_ = false;
  destruction_timer_ = 0.0;
  shutdown_core();
}

// 靜態編輯器實例管理
GameCoreManager *GameCoreManager::get_editor_instance()
{
  return editor_instance_;
}

void GameCoreManager::set_editor_instance(GameCoreManager *instance)
{
  editor_instance_ = instance;
}

// 靜態方法來獲取遊戲世界實例
portal_core::PortalGameWorld *GameCoreManager::get_game_world()
{
  return portal_core::PortalGameWorld::get_instance();
}

// 便利方法來發出信號
void GameCoreManager::emit_core_initialized()
{
  emit_signal("core_initialized");
}

void GameCoreManager::emit_core_shutdown()
{
  emit_signal("core_shutdown");
}

// === 实体生成桥接函数实现 ===

Node* GameCoreManager::spawn_godot_entity(const String& template_name, Node* parent, const Dictionary& overrides)
{
  // 获取EntityManager单例
  Object* entity_manager_obj = Engine::get_singleton()->get_singleton("EntityManager");
  Node* entity_manager = Object::cast_to<Node>(entity_manager_obj);
  if (!entity_manager)
  {
    UtilityFunctions::print("GameCoreManager: EntityManager singleton not found!");
    return nullptr;
  }

  // 构造调用参数
  Array args;
  args.append(template_name);
  args.append(parent ? Variant(parent) : Variant());
  args.append(overrides);

  // 调用GDScript的spawn_entity方法
  Variant result = entity_manager->call("spawn_entity", args[0], args[1], args[2]);
  
  // 转换返回值
  Node* spawned_entity = Object::cast_to<Node>(result);
  if (spawned_entity)
  {
    UtilityFunctions::print("GameCoreManager: Successfully spawned entity from C++: ", template_name);
  }
  else
  {
    UtilityFunctions::print("GameCoreManager: Failed to spawn entity from C++: ", template_name);
  }

  return spawned_entity;
}

Array GameCoreManager::get_available_godot_templates()
{
  // 获取EntityManager单例
  Object* entity_manager_obj = Engine::get_singleton()->get_singleton("EntityManager");
  Node* entity_manager = Object::cast_to<Node>(entity_manager_obj);
  if (!entity_manager)
  {
    UtilityFunctions::print("GameCoreManager: EntityManager singleton not found!");
    return Array();
  }

  // 调用GDScript的get_available_templates方法
  Variant result = entity_manager->call("get_available_templates");
  
  Array templates = result;
  UtilityFunctions::print("GameCoreManager: Retrieved ", templates.size(), " available templates from GDScript");
  
  return templates;
}

bool GameCoreManager::has_godot_template(const String& template_name)
{
  // 获取EntityManager单例
  Object* entity_manager_obj = Engine::get_singleton()->get_singleton("EntityManager");
  Node* entity_manager = Object::cast_to<Node>(entity_manager_obj);
  if (!entity_manager)
  {
    UtilityFunctions::print("GameCoreManager: EntityManager singleton not found!");
    return false;
  }

  // 调用GDScript的has_template方法
  Variant result = entity_manager->call("has_template", template_name);
  return result;
}

Node* GameCoreManager::spawn_godot_entity_with_ecs_override(const String& template_name, Node* parent, 
                                                          const Dictionary& component_overrides, 
                                                          const Dictionary& general_overrides)
{
  // 获取EntityManager单例
  Object* entity_manager_obj = Engine::get_singleton()->get_singleton("EntityManager");
  Node* entity_manager = Object::cast_to<Node>(entity_manager_obj);
  if (!entity_manager)
  {
    UtilityFunctions::print("GameCoreManager: EntityManager singleton not found!");
    return nullptr;
  }

  // 构造调用参数
  Array args;
  args.append(template_name);
  args.append(parent ? Variant(parent) : Variant());
  args.append(component_overrides);
  args.append(general_overrides);

  // 调用GDScript的spawn_entity_with_ecs_override方法
  Variant result = entity_manager->call("spawn_entity_with_ecs_override", args[0], args[1], args[2], args[3]);
  
  // 转换返回值
  Node* spawned_entity = Object::cast_to<Node>(result);
  if (spawned_entity)
  {
    UtilityFunctions::print("GameCoreManager: Successfully spawned ECS entity from C++: ", template_name);
  }
  else
  {
    UtilityFunctions::print("GameCoreManager: Failed to spawn ECS entity from C++: ", template_name);
  }

  return spawned_entity;
}

// 暂停更新
void GameCoreManager::pause_updates()
{
  if (!core_initialized_)
  {
    UtilityFunctions::print("GameCoreManager: Cannot pause - core not initialized");
    return;
  }

  if (is_paused_)
  {
    UtilityFunctions::print("GameCoreManager: Already paused");
    return;
  }

  is_paused_ = true;
  UtilityFunctions::print("GameCoreManager: Updates paused");
}

// 恢复更新
void GameCoreManager::resume_updates()
{
  if (!core_initialized_)
  {
    UtilityFunctions::print("GameCoreManager: Cannot resume - core not initialized");
    return;
  }

  if (!is_paused_)
  {
    UtilityFunctions::print("GameCoreManager: Not paused");
    return;
  }

  is_paused_ = false;
  UtilityFunctions::print("GameCoreManager: Updates resumed");
}

void GameCoreManager::clear_all_godot_entities()
{
  // 获取EntityManager单例
  Object* entity_manager_obj = Engine::get_singleton()->get_singleton("EntityManager");
  Node* entity_manager = Object::cast_to<Node>(entity_manager_obj);
  if (!entity_manager)
  {
    UtilityFunctions::print("GameCoreManager: EntityManager singleton not found!");
    return;
  }

  // 调用GDScript的clear_all_entities方法
  entity_manager->call("clear_all_entities");
  UtilityFunctions::print("GameCoreManager: Cleared all entities via C++ bridge");
}

int GameCoreManager::get_godot_entity_count()
{
  // 获取EntityManager单例
  Object* entity_manager_obj = Engine::get_singleton()->get_singleton("EntityManager");
  Node* entity_manager = Object::cast_to<Node>(entity_manager_obj);
  if (!entity_manager)
  {
    UtilityFunctions::print("GameCoreManager: EntityManager singleton not found!");
    return 0;
  }

  // 调用GDScript的get_active_entity_count方法
  Variant result = entity_manager->call("get_active_entity_count");
  return result;
}

Dictionary GameCoreManager::get_template_properties(const String& template_path)
{
  // 获取EntityManager单例
  Object* entity_manager_obj = Engine::get_singleton()->get_singleton("EntityManager");
  Node* entity_manager = Object::cast_to<Node>(entity_manager_obj);
  if (!entity_manager)
  {
    UtilityFunctions::print("GameCoreManager: EntityManager singleton not found!");
    return Dictionary();
  }

  // 调用GDScript的get_template_properties方法
  Variant result = entity_manager->call("get_template_properties", template_path);
  return result;
}

Dictionary GameCoreManager::get_all_template_properties()
{
  // 获取EntityManager单例
  Object* entity_manager_obj = Engine::get_singleton()->get_singleton("EntityManager");
  Node* entity_manager = Object::cast_to<Node>(entity_manager_obj);
  if (!entity_manager)
  {
    UtilityFunctions::print("GameCoreManager: EntityManager singleton not found!");
    return Dictionary();
  }

  // 调用GDScript的get_all_template_properties方法
  Variant result = entity_manager->call("get_all_template_properties");
  return result;
}