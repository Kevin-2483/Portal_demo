#ifndef GAME_CORE_MANAGER_H
#define GAME_CORE_MANAGER_H

#include <godot_cpp/classes/node.hpp>
#include <godot_cpp/classes/engine.hpp>
#include <godot_cpp/classes/packed_scene.hpp>
#include <godot_cpp/classes/resource_loader.hpp>
#include <godot_cpp/classes/dir_access.hpp>
#include <unordered_map>
#include <string>

// 前向聲明
namespace portal_core
{
  class PortalGameWorld;
}

namespace godot
{

  // 聲明遊戲核心管理器，負責管理 C++/Entt 世界
  class GameCoreManager : public Node
  {
    GDCLASS(GameCoreManager, Node)

  private:
    double time_passed_;
    bool core_initialized_;
    bool is_paused_;           // 新增：暂停状态
    
    // 編輯器持久化相關
    bool is_editor_mode_;
    bool editor_persistent_mode_;
    static int reference_count_;
    static GameCoreManager* editor_instance_;
    
    // 延遲銷毀機制
    bool pending_destruction_;
    double destruction_delay_;
    double destruction_timer_;
    
    // 模板管理相关
    static std::unordered_map<std::string, godot::Ref<godot::PackedScene>> templates_;
    static std::unordered_map<std::string, godot::Dictionary> template_properties_;
    static std::unordered_map<godot::Node*, std::string> active_entities_;
    static bool templates_loaded_;
    static const char* TEMPLATES_PATH;
    
    // Schema系统相关成员变量
    std::unordered_map<std::string, godot::Dictionary> schema_properties_;
    std::unordered_map<std::string, godot::Array> schema_presets_;

  protected:
    static void _bind_methods();

  public:
    GameCoreManager();
    ~GameCoreManager();

    void _ready() override;
    void _process(double delta) override;
    void _exit_tree() override;

    // 管理核心系統的方法
    void initialize_core();
    void shutdown_core();
    bool is_core_initialized() const { return core_initialized_; }

    // 編輯器持久化管理
    void add_reference();
    void remove_reference();
    void set_editor_persistent(bool persistent);
    bool is_editor_persistent() const { return editor_persistent_mode_; }
    
    // 延遲銷毀控制
    void request_shutdown();
    void force_shutdown();
    bool is_pending_destruction() const { return pending_destruction_; }
    
    // 暂停/恢复功能
    void pause_updates();
    void resume_updates();
    bool is_paused() const { return is_paused_; }

    // 靜態方法來獲取遊戲世界實例
    static portal_core::PortalGameWorld *get_game_world();
    
    // 編輯器實例管理
    static GameCoreManager* get_editor_instance();
    static void set_editor_instance(GameCoreManager* instance);
    
    // 模板管理方法
    static void load_all_templates();
    static void scan_directory_recursive(const String& path, const String& prefix = "");
    static void analyze_all_templates();
    static Dictionary load_schema_properties(const String& template_name);
    

    
    // 实体生成和管理方法
    static Node* spawn_entity(const String& template_name, Node* parent = nullptr, const Dictionary& overrides = Dictionary());
    static Node* spawn_entity_with_ecs_override(const String& template_name, Node* parent = nullptr, 
                                              const Dictionary& component_overrides = Dictionary(), 
                                              const Dictionary& general_overrides = Dictionary());
    static Array get_available_templates();
    static bool has_template(const String& template_name);
    static void clear_all_entities();
    static int get_active_entity_count();
    static Array get_active_entities();
    static Array get_entities_by_template(const String& template_name);
    static void destroy_entity(Node* entity);
    
    // 属性和模板信息方法
    static Dictionary get_template_properties(const String& template_path);
    static Dictionary get_all_template_properties();
    
    // Schema系统相关方法
    bool load_template_schema(const String& template_name);
    Dictionary get_schema_properties(const String& template_name);
    Array get_schema_presets(const String& template_name);
    Dictionary validate_property_overrides(const String& template_name, const Dictionary& overrides);
    
    // === 兼容性方法已移除，请使用新的实体管理方法 ===
    
  private:
    // 内部辅助方法
    static void apply_overrides(Node* instance, const Dictionary& overrides);
    static void set_property_by_path(Node* root_node, const String& property_path, const Variant& value);
    static Node* find_node_by_type(Node* root, const String& node_type);
    static void set_simple_property(Node* node, const String& property_name, const Variant& value);
    static Node* find_ecs_node(Node* root);
    static void apply_ecs_component_overrides(Node* ecs_node, const Dictionary& component_overrides);
    static void on_entity_destroyed(Node* entity, const String& template_name);
    
    // 信號：當 GameCore 初始化完成時發出
    void emit_core_initialized();
    void emit_core_shutdown();
  };

} // namespace godot

#endif // GAME_CORE_MANAGER_H