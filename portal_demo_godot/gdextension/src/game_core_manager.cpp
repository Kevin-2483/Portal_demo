#include "game_core_manager.h"

#include <godot_cpp/core/class_db.hpp>
#include <godot_cpp/variant/utility_functions.hpp>

using namespace godot;

// _bind_methods 用于向 Godot 暴露 C++ 函数或属性，我们暂时留空
void GameCoreManager::_bind_methods() {
}

// 构造函数：当节点被创建时调用
GameCoreManager::GameCoreManager() {
    time_passed = 0.0;
    UtilityFunctions::print("GameCoreManager constructed from C++.");
}

// 析构函数：当节点被销毁时调用
GameCoreManager::~GameCoreManager() {
}

// _process 函数：每一帧都被调用
void GameCoreManager::_process(double delta) {
    time_passed += delta;
    // 为了避免刷屏，我们每秒打印一次
    if (time_passed >= 1.0) {
        UtilityFunctions::print("Hello from C++! Time: ", time_passed);
        time_passed = 0.0;
    }
}