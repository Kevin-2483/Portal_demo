#ifndef GAME_CORE_MANAGER_H
#define GAME_CORE_MANAGER_H

#include <godot_cpp/classes/node.hpp>
#include <godot_cpp/classes/engine.hpp>

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
    
    // 編輯器持久化相關
    bool is_editor_mode_;
    bool editor_persistent_mode_;
    static int reference_count_;
    static GameCoreManager* editor_instance_;
    
    // 延遲銷毀機制
    bool pending_destruction_;
    double destruction_delay_;
    double destruction_timer_;

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

    // 靜態方法來獲取遊戲世界實例
    static portal_core::PortalGameWorld *get_game_world();
    
    // 編輯器實例管理
    static GameCoreManager* get_editor_instance();
    static void set_editor_instance(GameCoreManager* instance);
    
    // 信號：當 GameCore 初始化完成時發出
    void emit_core_initialized();
    void emit_core_shutdown();
  };

} // namespace godot

#endif // GAME_CORE_MANAGER_H