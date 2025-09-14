#include "game_core_manager.h"

#include <godot_cpp/core/class_db.hpp>
#include "game_core_manager.h"
#include <godot_cpp/variant/utility_functions.hpp>
#include <godot_cpp/classes/engine.hpp>
#include <godot_cpp/classes/scene_tree.hpp>
#include <godot_cpp/classes/node.hpp>
#include <godot_cpp/classes/resource_loader.hpp>
#include <godot_cpp/classes/packed_scene.hpp>
#include <godot_cpp/classes/dir_access.hpp>
#include "ecs_entity_link_manager.h"

// 模板路径常量
const char *TEMPLATES_PATH = "res://templates";

// 包含 C++ 核心
#include "core/portal_game_world.h"

using namespace godot;

// 靜態成員初始化
int GameCoreManager::reference_count_ = 0;
GameCoreManager *GameCoreManager::editor_instance_ = nullptr;

// 模板管理静态成员初始化
std::unordered_map<std::string, godot::Ref<godot::PackedScene>> GameCoreManager::templates_;
std::unordered_map<std::string, godot::Dictionary> GameCoreManager::template_properties_;
std::unordered_map<godot::Node *, std::string> GameCoreManager::active_entities_;
bool GameCoreManager::templates_loaded_ = false;
const char *GameCoreManager::TEMPLATES_PATH = "res://templates";

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

  // 模板管理方法
  ClassDB::bind_static_method("GameCoreManager", D_METHOD("load_all_templates"),
                              &GameCoreManager::load_all_templates);

  // 实体生成和管理方法
  ClassDB::bind_static_method("GameCoreManager", D_METHOD("spawn_entity", "template_name", "parent", "overrides"),
                              &GameCoreManager::spawn_entity, DEFVAL(Variant()), DEFVAL(Dictionary()));
  ClassDB::bind_static_method("GameCoreManager", D_METHOD("spawn_entity_with_ecs_override", "template_name", "parent", "component_overrides", "general_overrides"),
                              &GameCoreManager::spawn_entity_with_ecs_override, DEFVAL(Variant()), DEFVAL(Dictionary()), DEFVAL(Dictionary()));
  ClassDB::bind_static_method("GameCoreManager", D_METHOD("get_available_templates"),
                              &GameCoreManager::get_available_templates);
  ClassDB::bind_static_method("GameCoreManager", D_METHOD("has_template", "template_name"),
                              &GameCoreManager::has_template);
  ClassDB::bind_static_method("GameCoreManager", D_METHOD("clear_all_entities"),
                              &GameCoreManager::clear_all_entities);
  ClassDB::bind_static_method("GameCoreManager", D_METHOD("get_active_entity_count"),
                              &GameCoreManager::get_active_entity_count);
  ClassDB::bind_static_method("GameCoreManager", D_METHOD("get_active_entities"),
                              &GameCoreManager::get_active_entities);
  ClassDB::bind_static_method("GameCoreManager", D_METHOD("get_entities_by_template", "template_name"),
                              &GameCoreManager::get_entities_by_template);
  ClassDB::bind_static_method("GameCoreManager", D_METHOD("destroy_entity", "entity"),
                              &GameCoreManager::destroy_entity);

  // 属性和模板信息方法
  ClassDB::bind_static_method("GameCoreManager", D_METHOD("get_template_properties", "template_path"),
                              &GameCoreManager::get_template_properties);
  ClassDB::bind_static_method("GameCoreManager", D_METHOD("get_all_template_properties"),
                              &GameCoreManager::get_all_template_properties);

  // Schema系统方法
  ClassDB::bind_method(D_METHOD("load_template_schema", "template_name"), &GameCoreManager::load_template_schema);
  ClassDB::bind_method(D_METHOD("get_schema_properties", "template_name"), &GameCoreManager::get_schema_properties);
  ClassDB::bind_method(D_METHOD("get_schema_presets", "template_name"), &GameCoreManager::get_schema_presets);
  ClassDB::bind_method(D_METHOD("validate_property_overrides", "template_name", "overrides"), &GameCoreManager::validate_property_overrides);

  // 兼容性桥接方法已移除

  // 註冊信號
  ADD_SIGNAL(MethodInfo("core_initialized"));
  ADD_SIGNAL(MethodInfo("core_shutdown"));
  ADD_SIGNAL(MethodInfo("destruction_cancelled"));
  ADD_SIGNAL(MethodInfo("templates_loaded"));
  ADD_SIGNAL(MethodInfo("entity_spawned", PropertyInfo(Variant::OBJECT, "entity"), PropertyInfo(Variant::STRING, "template_name")));
  ADD_SIGNAL(MethodInfo("entity_destroyed", PropertyInfo(Variant::OBJECT, "entity"), PropertyInfo(Variant::STRING, "template_name")));
  
  // 绑定双向链接管理方法
  ClassDB::bind_static_method("GameCoreManager", D_METHOD("get_link_manager"), &GameCoreManager::get_link_manager);
  ClassDB::bind_static_method("GameCoreManager", D_METHOD("setup_entity_destroy_callbacks"), &GameCoreManager::setup_entity_destroy_callbacks);
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

    // 加载模板
    if (!templates_loaded_)
    {
      load_all_templates();
    }
    
    // 设置ECS实体销毁回调
    setup_entity_destroy_callbacks();

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

// === 兼容性桥接函数已移除 ===

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

// 兼容性桥接函数已移除，请直接使用新的实体管理方法

Dictionary GameCoreManager::get_template_properties(const String &template_path)
{
  // 直接使用新的C++实现
  std::string template_key = template_path.utf8().get_data();
  auto it = template_properties_.find(template_key);
  if (it != template_properties_.end())
  {
    return it->second;
  }
  return Dictionary();
}

Dictionary GameCoreManager::get_all_template_properties()
{
  Dictionary result;
  for (const auto &pair : template_properties_)
  {
    result[String(pair.first.c_str())] = pair.second;
  }
  return result;
}

// === 新的模板管理方法实现 ===

void GameCoreManager::load_all_templates()
{
  UtilityFunctions::print("[GameCoreManager] Loading all templates...");

  templates_.clear();
  template_properties_.clear();

  scan_directory_recursive(String(TEMPLATES_PATH));
  analyze_all_templates();

  templates_loaded_ = true;

  UtilityFunctions::print("[GameCoreManager] Templates loaded: ", (int)templates_.size(), " templates");

  // 发出模板加载完成信号
  if (auto *instance = get_editor_instance())
  {
    instance->emit_signal("templates_loaded");
  }
}

void GameCoreManager::scan_directory_recursive(const String &path, const String &prefix)
{
  Ref<DirAccess> dir = DirAccess::open(path);
  if (dir.is_null())
  {
    UtilityFunctions::print("[GameCoreManager] Warning: Cannot access directory: ", path);
    return;
  }

  dir->list_dir_begin();
  String file_name = dir->get_next();

  while (!file_name.is_empty())
  {
    String full_path = path + String("/") + file_name;

    if (dir->current_is_dir())
    {
      // 递归扫描子目录
      String sub_prefix = prefix + file_name + String("/");
      scan_directory_recursive(full_path, sub_prefix);
    }
    else if (file_name.ends_with(".tscn"))
    {
      // 加载.tscn文件作为PackedScene
      Ref<PackedScene> scene_resource = ResourceLoader::get_singleton()->load(full_path);
      if (scene_resource.is_valid())
      {
        String template_name = prefix + file_name.get_basename();
        std::string template_key = template_name.utf8().get_data();
        templates_[template_key] = scene_resource;
        UtilityFunctions::print("[GameCoreManager] Loaded template: ", template_name, " -> ", full_path);
      }
      else
      {
        UtilityFunctions::print("[GameCoreManager] Warning: Failed to load ", full_path);
      }
    }

    file_name = dir->get_next();
  }
}

void GameCoreManager::analyze_all_templates()
{
  UtilityFunctions::print("[GameCoreManager] Loading template properties from schema files...");

  for (const auto &pair : templates_)
  {
    String template_name = String(pair.first.c_str());
    Dictionary properties = load_schema_properties(template_name);
    std::string template_key = template_name.utf8().get_data();
    template_properties_[template_key] = properties;
    UtilityFunctions::print("[GameCoreManager] Template '", template_name, "' loaded schema properties");
  }
}

Dictionary GameCoreManager::load_schema_properties(const String &template_name)
{
  // 构造 schema 文件路径
  String schema_path = String(TEMPLATES_PATH) + "/" + template_name + ".schema.tres";

  // 尝试加载 schema 资源
  if (ResourceLoader::get_singleton()->exists(schema_path))
  {
    Ref<Resource> schema_resource = ResourceLoader::get_singleton()->load(schema_path);
    if (schema_resource.is_valid() && schema_resource->has_method("get_properties"))
    {
      Variant properties = schema_resource->call("get_properties");
      UtilityFunctions::print("[GameCoreManager] Loaded schema for '", template_name, "'");
      return properties;
    }
    else
    {
      UtilityFunctions::print("[GameCoreManager] Warning: Invalid schema resource at ", schema_path);
    }
  }
  else
  {
    UtilityFunctions::print("[GameCoreManager] Warning: Schema file not found: ", schema_path, ", no overridable properties available");
  }

  return Dictionary();
}

Node *GameCoreManager::spawn_entity(const String &template_name, Node *parent, const Dictionary &overrides)
{
  std::string template_key = template_name.utf8().get_data();

  auto it = templates_.find(template_key);
  if (it == templates_.end())
  {
    UtilityFunctions::print("[GameCoreManager] Error: Template '", template_name, "' not found!");
    return nullptr;
  }

  // 实例化场景
  Ref<PackedScene> template_scene = it->second;
  Node *instance = template_scene->instantiate();

  if (!instance)
  {
    UtilityFunctions::print("[GameCoreManager] Error: Failed to instantiate template '", template_name, "'");
    return nullptr;
  }

  // 应用属性覆写
  if (!overrides.is_empty())
  {
    apply_overrides(instance, overrides);
  }

  // 添加到场景树
  if (!parent)
  {
    SceneTree *scene_tree = Object::cast_to<SceneTree>(Engine::get_singleton()->get_main_loop());
    if (scene_tree)
    {
      parent = scene_tree->get_current_scene();
    }
  }

  parent->add_child(instance);

  // 记录活跃实体
  active_entities_[instance] = template_key;

  // 查找ECSNode并建立双向链接
  Node *ecs_node = find_ecs_node(instance);
  if (ecs_node) {
    // 获取ECS实体ID（假设ECSNode有get_entity_id方法）
    Variant entity_id_var = ecs_node->call("get_entity_id");
    if (entity_id_var.get_type() == Variant::INT) {
      uint32_t entity_id = entity_id_var;
      if (entity_id != 0) {
        ECSEntityLinkManager::get_instance()->create_link(entity_id, instance, template_name);
        UtilityFunctions::print("[GameCoreManager] Created bidirectional link for entity ", entity_id, " <-> node ", instance->get_name());
      }
    }
  }

  // 连接销毁信号以便清理
  if (instance->has_signal("tree_exited"))
  {
    // 注意：这里需要使用callable来绑定静态方法
    Callable cleanup_callable = Callable(instance, "queue_free").bind();
    instance->connect("tree_exited", cleanup_callable);
  }

  UtilityFunctions::print("[GameCoreManager] Spawned entity '", template_name, "'");

  // 发出实体生成信号
  if (auto *manager_instance = get_editor_instance())
  {
    manager_instance->emit_signal("entity_spawned", instance, template_name);
  }

  return instance;
}

Node *GameCoreManager::spawn_entity_with_ecs_override(const String &template_name, Node *parent,
                                                      const Dictionary &component_overrides,
                                                      const Dictionary &general_overrides)
{
  std::string template_key = template_name.utf8().get_data();

  auto it = templates_.find(template_key);
  if (it == templates_.end())
  {
    UtilityFunctions::print("[GameCoreManager] Error: Template '", template_name, "' not found!");
    return nullptr;
  }

  Ref<PackedScene> template_scene = it->second;
  Node *instance = template_scene->instantiate();

  if (!instance)
  {
    UtilityFunctions::print("[GameCoreManager] Error: Failed to instantiate template '", template_name, "'");
    return nullptr;
  }

  // 查找ECSNode并处理组件覆写
  Node *ecs_node = find_ecs_node(instance);
  if (ecs_node && !component_overrides.is_empty())
  {
    apply_ecs_component_overrides(ecs_node, component_overrides);
  }

  // 应用常规属性覆写
  if (!general_overrides.is_empty())
  {
    apply_overrides(instance, general_overrides);
  }

  // 添加到场景树
  if (!parent)
  {
    SceneTree *scene_tree = Object::cast_to<SceneTree>(Engine::get_singleton()->get_main_loop());
    if (scene_tree)
    {
      parent = scene_tree->get_current_scene();
    }
  }

  parent->add_child(instance);
  active_entities_[instance] = template_key;

  // 建立双向链接（ECSNode已经在前面找到了）
  if (ecs_node) {
    // 获取ECS实体ID
    Variant entity_id_var = ecs_node->call("get_entity_id");
    if (entity_id_var.get_type() == Variant::INT) {
      uint32_t entity_id = entity_id_var;
      if (entity_id != 0) {
        ECSEntityLinkManager::get_instance()->create_link(entity_id, instance, template_name);
        UtilityFunctions::print("[GameCoreManager] Created bidirectional link for ECS entity ", entity_id, " <-> node ", instance->get_name());
      }
    }
  }

  if (instance->has_signal("tree_exited"))
  {
    Callable cleanup_callable = Callable(instance, "queue_free").bind();
    instance->connect("tree_exited", cleanup_callable);
  }

  UtilityFunctions::print("[GameCoreManager] Spawned ECS entity '", template_name, "'");

  if (auto *manager_instance = get_editor_instance())
  {
    manager_instance->emit_signal("entity_spawned", instance, template_name);
  }

  return instance;
}

Array GameCoreManager::get_available_templates()
{
  Array template_names;
  for (const auto &pair : templates_)
  {
    template_names.append(String(pair.first.c_str()));
  }
  return template_names;
}

bool GameCoreManager::has_template(const String &template_name)
{
  std::string template_key = template_name.utf8().get_data();
  return templates_.find(template_key) != templates_.end();
}

void GameCoreManager::clear_all_entities()
{
  for (auto it = active_entities_.begin(); it != active_entities_.end(); ++it)
  {
    Node *entity = it->first;
    if (entity && Object::cast_to<Node>(entity))
    {
      entity->queue_free();
    }
  }
  active_entities_.clear();
  UtilityFunctions::print("[GameCoreManager] All entities cleared");
}

int GameCoreManager::get_active_entity_count()
{
  return static_cast<int>(active_entities_.size());
}

Array GameCoreManager::get_active_entities()
{
  Array entities;
  for (auto it = active_entities_.begin(); it != active_entities_.end(); ++it)
  {
    Node *entity = it->first;
    if (entity && Object::cast_to<Node>(entity))
    {
      entities.append(entity);
    }
  }
  return entities;
}

Array GameCoreManager::get_entities_by_template(const String &template_name)
{
  Array entities;
  std::string template_key = template_name.utf8().get_data();

  for (auto it = active_entities_.begin(); it != active_entities_.end(); ++it)
  {
    Node *entity = it->first;
    const std::string &entity_template = it->second;

    if (entity && Object::cast_to<Node>(entity) && entity_template == template_key)
    {
      entities.append(entity);
    }
  }
  return entities;
}

void GameCoreManager::destroy_entity(Node *entity)
{
  if (entity && active_entities_.find(entity) != active_entities_.end())
  {
    entity->queue_free();
    UtilityFunctions::print("[GameCoreManager] Requested destruction of tracked entity");
  }
  else
  {
    UtilityFunctions::print("[GameCoreManager] Warning: Attempted to destroy untracked or invalid entity");
  }
}

// === 内部辅助方法实现 ===

void GameCoreManager::apply_overrides(Node *instance, const Dictionary &overrides)
{
  UtilityFunctions::print("[GameCoreManager] Applying overrides: ", overrides.size(), " properties");

  Array keys = overrides.keys();
  for (int i = 0; i < keys.size(); ++i)
  {
    String property_path = keys[i];
    Variant value = overrides[property_path];
    set_property_by_path(instance, property_path, value);
  }
}

void GameCoreManager::set_property_by_path(Node *root_node, const String &property_path, const Variant &value)
{
  PackedStringArray path_parts = property_path.split(".");

  // 如果是简单属性（没有节点路径前缀）
  if (path_parts.size() == 1)
  {
    set_simple_property(root_node, property_path, value);
    return;
  }

  // 处理复合路径属性（如 "MeshInstance3D.material_override"）
  String node_type = path_parts[0];
  String property_name = path_parts[1];

  // 递归查找指定类型的节点
  Node *target_node = find_node_by_type(root_node, node_type);
  if (target_node)
  {
    set_simple_property(target_node, property_name, value);
  }
  else
  {
    UtilityFunctions::print("[GameCoreManager] Warning: Node type '", node_type, "' not found for property '", property_path, "'");
  }
}

Node *GameCoreManager::find_node_by_type(Node *root, const String &node_type)
{
  if (root->get_class() == node_type)
  {
    return root;
  }

  for (int i = 0; i < root->get_child_count(); ++i)
  {
    Node *child = root->get_child(i);
    Node *result = find_node_by_type(child, node_type);
    if (result)
    {
      return result;
    }
  }

  return nullptr;
}

void GameCoreManager::set_simple_property(Node *node, const String &property_name, const Variant &value)
{
  // 处理常见的Node属性
  if (property_name == "position")
  {
    if (node->has_method("set_position"))
    {
      node->call("set_position", value);
      UtilityFunctions::print("[GameCoreManager] Set position = ", value);
    }
    else
    {
      UtilityFunctions::print("[GameCoreManager] Warning: Node doesn't have position property");
    }
  }
  else if (property_name == "rotation")
  {
    if (node->has_method("set_rotation"))
    {
      node->call("set_rotation", value);
      UtilityFunctions::print("[GameCoreManager] Set rotation = ", value);
    }
    else
    {
      UtilityFunctions::print("[GameCoreManager] Warning: Node doesn't have rotation property");
    }
  }
  else if (property_name == "scale")
  {
    if (node->has_method("set_scale"))
    {
      node->call("set_scale", value);
      UtilityFunctions::print("[GameCoreManager] Set scale = ", value);
    }
    else
    {
      UtilityFunctions::print("[GameCoreManager] Warning: Node doesn't have scale property");
    }
  }
  else if (property_name == "global_position")
  {
    if (node->has_method("set_global_position"))
    {
      node->call("set_global_position", value);
      UtilityFunctions::print("[GameCoreManager] Set global_position = ", value);
    }
    else
    {
      UtilityFunctions::print("[GameCoreManager] Warning: Node doesn't have global_position property");
    }
  }
  else
  {
    // 尝试直接属性设置
    if (node->has_method("set") && node->call("has_method", "get_" + property_name))
    {
      node->set(property_name, value);
      UtilityFunctions::print("[GameCoreManager] Set property '", property_name, "' = ", value);
    }
    else if (node->has_method("set_" + property_name))
    {
      node->call("set_" + property_name, value);
      UtilityFunctions::print("[GameCoreManager] Called setter for '", property_name, "' = ", value);
    }
    else
    {
      UtilityFunctions::print("[GameCoreManager] Warning: Property '", property_name, "' not found or not settable in ", node->get_class());
    }
  }
}

Node *GameCoreManager::find_ecs_node(Node *root)
{
  if (root->get_class() == "ECSNode")
  {
    return root;
  }

  for (int i = 0; i < root->get_child_count(); ++i)
  {
    Node *child = root->get_child(i);
    Node *result = find_ecs_node(child);
    if (result)
    {
      return result;
    }
  }

  return nullptr;
}

void GameCoreManager::apply_ecs_component_overrides(Node *ecs_node, const Dictionary &component_overrides)
{
  Variant components_var = ecs_node->get("components");
  if (components_var.get_type() != Variant::ARRAY)
  {
    UtilityFunctions::print("[GameCoreManager] Warning: ECSNode has no components array");
    return;
  }

  Array components = components_var;
  Array new_components;
  bool was_modified = false;

  for (int i = 0; i < components.size(); ++i)
  {
    Variant component_var = components[i];
    Ref<Resource> component = component_var;

    if (component.is_valid())
    {
      String component_class = component->get_class();

      // 检查是否有针对此组件类型的覆写
      if (component_overrides.has(component_class))
      {
        // 只有需要修改时才创建副本
        Ref<Resource> component_copy = component->duplicate();
        Dictionary overrides = component_overrides[component_class];
        bool component_was_modified = false;

        Array override_keys = overrides.keys();
        for (int j = 0; j < override_keys.size(); ++j)
        {
          String property_name = override_keys[j];
          Variant value = overrides[property_name];

          if (component_copy->has_method("set") && component_copy->call("has_method", "get_" + property_name))
          {
            component_copy->set(property_name, value);
            component_was_modified = true;
            UtilityFunctions::print("[GameCoreManager] Override ", component_class, ".", property_name, " = ", value);
          }
          else
          {
            UtilityFunctions::print("[GameCoreManager] Warning: Property '", property_name, "' not found in ", component_class);
          }
        }

        if (component_was_modified)
        {
          new_components.append(component_copy);
          was_modified = true;
        }
        else
        {
          // 如果实际上没有修改任何属性，使用原始组件
          new_components.append(component);
        }
      }
      else
      {
        // 没有针对此组件的覆写，使用原始组件
        new_components.append(component);
      }
    }
    else
    {
      new_components.append(component_var);
    }
  }

  // 只有在实际发生修改时才更新ECSNode的组件数组
  if (was_modified)
  {
    ecs_node->set("components", new_components);
    UtilityFunctions::print("[GameCoreManager] ECS components updated with overrides");
  }
  else
  {
    UtilityFunctions::print("[GameCoreManager] No ECS component modifications applied");
  }
}

void GameCoreManager::on_entity_destroyed(Node *entity, const String &template_name)
{
  auto it = active_entities_.find(entity);
  if (it != active_entities_.end())
  {
    active_entities_.erase(it);
    UtilityFunctions::print("[GameCoreManager] Entity '", template_name, "' destroyed");

    if (auto *manager_instance = get_editor_instance())
    {
      manager_instance->emit_signal("entity_destroyed", entity, template_name);
    }
  }
  else
  {
    UtilityFunctions::print("[GameCoreManager] Warning: Attempted to remove untracked entity");
  }
}

// === Schema系统方法实现 ===

bool GameCoreManager::load_template_schema(const String &template_name)
{
  String schema_path = String(TEMPLATES_PATH) + "/" + template_name + ".schema.tres";

  if (ResourceLoader::get_singleton()->exists(schema_path))
  {
    Ref<Resource> schema_resource = ResourceLoader::get_singleton()->load(schema_path);
    if (schema_resource.is_valid())
    {
      std::string template_key = template_name.utf8().get_data();

      // 加载根节点属性和ECS组件属性
      if (schema_resource->has_method("get"))
      {
        Dictionary root_props = schema_resource->call("get", "root_node_properties");
        Dictionary ecs_props = schema_resource->call("get", "ecs_component_properties");
        Array presets = schema_resource->call("get", "presets");

        // 合并所有属性到一个字典中
        Dictionary all_properties;

        // 添加根节点属性
        Array root_keys = root_props.keys();
        for (int i = 0; i < root_keys.size(); ++i)
        {
          String key = root_keys[i];
          all_properties[key] = root_props[key];
        }

        // 添加ECS组件属性（使用组件类名.属性名格式）
        Array component_keys = ecs_props.keys();
        for (int i = 0; i < component_keys.size(); ++i)
        {
          String component_class = component_keys[i];
          Dictionary component_props = ecs_props[component_class];
          Array prop_keys = component_props.keys();

          for (int j = 0; j < prop_keys.size(); ++j)
          {
            String prop_name = prop_keys[j];
            String full_key = component_class + "." + prop_name;
            all_properties[full_key] = component_props[prop_name];
          }
        }

        schema_properties_[template_key] = all_properties;
        schema_presets_[template_key] = presets;

        UtilityFunctions::print("[GameCoreManager] Loaded schema for '", template_name, "' with ", all_properties.size(), " properties");
        return true;
      }
    }
    else
    {
      UtilityFunctions::print("[GameCoreManager] Warning: Invalid schema resource at ", schema_path);
    }
  }
  else
  {
    UtilityFunctions::print("[GameCoreManager] Warning: Schema file not found: ", schema_path);
  }

  return false;
}

Dictionary GameCoreManager::get_schema_properties(const String &template_name)
{
  std::string template_key = template_name.utf8().get_data();
  auto it = schema_properties_.find(template_key);
  if (it != schema_properties_.end())
  {
    return it->second;
  }

  // 如果缓存中没有，尝试动态加载
  if (load_template_schema(template_name))
  {
    auto it2 = schema_properties_.find(template_key);
    if (it2 != schema_properties_.end())
    {
      return it2->second;
    }
  }

  return Dictionary();
}

Array GameCoreManager::get_schema_presets(const String &template_name)
{
  std::string template_key = template_name.utf8().get_data();
  auto it = schema_presets_.find(template_key);
  if (it != schema_presets_.end())
  {
    return it->second;
  }

  // 如果缓存中没有，尝试动态加载
  if (load_template_schema(template_name))
  {
    auto it2 = schema_presets_.find(template_key);
    if (it2 != schema_presets_.end())
    {
      return it2->second;
    }
  }

  return Array();
}

Dictionary GameCoreManager::validate_property_overrides(const String &template_name, const Dictionary &overrides)
{
  Dictionary result;
  result["valid"] = true;
  Array errors;

  Dictionary schema_properties = get_schema_properties(template_name);

  if (schema_properties.is_empty())
  {
    UtilityFunctions::print("[GameCoreManager] Warning: No schema found for template '", template_name, "', validation skipped");
    result["errors"] = errors;
    return result; // 没有schema时允许所有覆写
  }

  Array override_keys = overrides.keys();
  bool all_valid = true;

  for (int i = 0; i < override_keys.size(); ++i)
  {
    String property_path = override_keys[i];

    if (!schema_properties.has(property_path))
    {
      String error_msg = "Property '" + property_path + "' not found in schema for template '" + template_name + "'";
      errors.append(error_msg);
      UtilityFunctions::print("[GameCoreManager] Validation error: ", error_msg);
      all_valid = false;
      continue;
    }

    // 类型验证
    Dictionary property_info = schema_properties[property_path];
    Variant override_value = overrides[property_path];

    if (property_info.has("type"))
    {
      Variant::Type expected_type = static_cast<Variant::Type>(int(property_info["type"]));
      if (override_value.get_type() != expected_type)
      {
        String error_msg = "Property '" + property_path + "' type mismatch. Expected: " + String::num_int64(expected_type) + ", Got: " + String::num_int64(override_value.get_type());
        errors.append(error_msg);
        UtilityFunctions::print("[GameCoreManager] Validation error: ", error_msg);
        all_valid = false;
      }
    }
  }

  result["valid"] = all_valid;
  result["errors"] = errors;
  return result;
}

// === 双向链接管理方法实现 ===

ECSEntityLinkManager* GameCoreManager::get_link_manager() {
    return ECSEntityLinkManager::get_instance();
}

void GameCoreManager::setup_entity_destroy_callbacks() {
    auto* link_manager = get_link_manager();
    if (!link_manager) {
        UtilityFunctions::print("[GameCoreManager] Warning: Link manager not available");
        return;
    }
    
    // 注册ECS实体销毁回调
    link_manager->register_entity_destroy_callback([](uint32_t entity_id, Node* linked_node) {
        UtilityFunctions::print("[GameCoreManager] ECS entity ", entity_id, " destroyed, processing linked node");
        
        // 如果有链接的Godot节点，从active_entities_中移除
        if (linked_node) {
            auto it = active_entities_.find(linked_node);
            if (it != active_entities_.end()) {
                String template_name = String(it->second.c_str());
                active_entities_.erase(it);
                
                // 发出实体销毁信号
                if (auto* manager_instance = get_editor_instance()) {
                    manager_instance->emit_signal("entity_destroyed", linked_node, template_name);
                }
                
                UtilityFunctions::print("[GameCoreManager] Removed entity from active list: ", template_name);
            }
        }
    });
    
    // 注册Godot节点销毁回调
    link_manager->register_node_destroy_callback([](uint32_t entity_id, Node* destroyed_node) {
        UtilityFunctions::print("[GameCoreManager] Godot node destroyed, processing linked ECS entity ", entity_id);
        
        // 从active_entities_中移除
        if (destroyed_node) {
            auto it = active_entities_.find(destroyed_node);
            if (it != active_entities_.end()) {
                String template_name = String(it->second.c_str());
                active_entities_.erase(it);
                
                UtilityFunctions::print("[GameCoreManager] Removed destroyed node from active list: ", template_name);
            }
        }
    });
    
    UtilityFunctions::print("[GameCoreManager] Entity destroy callbacks setup completed");
}