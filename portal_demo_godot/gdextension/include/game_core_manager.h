#ifndef GAME_CORE_MANAGER_H
#define GAME_CORE_MANAGER_H

#include <godot_cpp/classes/node.hpp>

// 声明我们的自定义节点类，它继承自 Godot 的 Node
class GameCoreManager : public godot::Node {
    GDCLASS(GameCoreManager, godot::Node)

private:
    double time_passed;

protected:
    // Godot 内部需要这个静态函数来绑定方法（我们暂时不用，但必须有）
    static void _bind_methods();

public:
    GameCoreManager();
    ~GameCoreManager();

    // Godot 会在每一帧调用这个函数，相当于 GDScript 的 _process
    void _process(double delta) override;
};

#endif // GAME_CORE_MANAGER_H