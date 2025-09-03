# ECS Editor Plugin 使用指南

## 功能概述

这个插件提供了在 Godot 编辑器中管理 ECS GameCore 实例的能力，支持：

1. **编辑器持久化模式**: 实例在脚本重新编译时不会被意外销毁
2. **可视化控制界面**: 通过 Dock 面板控制实例的启动和停止
3. **状态监控**: 实时显示实例状态和信息
4. **引用计数保护**: 防止意外的实例销毁

## 安装和使用

### 1. 启用插件

1. 打开 `项目 -> 项目设置 -> 插件`
2. 找到 "ECS Editor Plugin" 并启用它
3. 插件启用后会在左侧出现 "ECS Core Manager" 面板

### 2. 使用 Dock 面板

- **Start ECS Instance**: 创建并启动一个 GameCoreManager 实例
- **Stop ECS Instance**: 停止并销毁当前实例
- **Restart ECS Instance**: 重启实例
- **Editor Persistent Mode**: 启用后实例在编辑器中更加稳定
- **Auto Start on Plugin Load**: 插件加载时自动启动实例

### 3. 状态说明

- **STOPPED**: 没有活动实例
- **STARTING**: 实例正在启动
- **RUNNING**: 实例运行正常
- **STOPPING**: 实例正在停止
- **ERROR**: 发生错误

### 4. 编程接口

也可以通过代码控制：

```gdscript
@tool
extends EditorScript

func _run():
    # 获取插件实例
    var plugin = EditorInterface.get_editor_main_screen().get_child(0).get_plugin("ECS Editor Plugin")
    if plugin and plugin.instance_manager:
        # 启动实例
        plugin.instance_manager.start_instance()
        
        # 获取实例
        var game_core = plugin.instance_manager.get_instance()
        if game_core:
            print("GameCore instance obtained: ", game_core)
```

## 注意事项

1. 实例在编辑器持久化模式下非常稳定，只有以下情况会被销毁：
   - 手动点击停止
   - 禁用持久化模式
   - 退出 Godot 编辑器

2. 多个实例管理：同时只能有一个活动实例

3. 性能考虑：实例会持续运行 ECS 系统，注意监控性能

## 故障排除

如果实例出现问题：
1. 点击 "Stop ECS Instance" 停止当前实例
2. 等待状态变为 "STOPPED"
3. 点击 "Start ECS Instance" 重新启动

如果插件无响应：
1. 禁用并重新启用插件
2. 重启 Godot 编辑器
