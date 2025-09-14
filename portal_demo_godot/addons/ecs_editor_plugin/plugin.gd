@tool
extends EditorPlugin

const ECSDock = preload("res://addons/ecs_editor_plugin/ecs_dock.gd")
const ECSEventBus = preload("res://addons/ecs_editor_plugin/ecs_event_bus.gd")

var dock_instance
var instance_manager
var event_bus
var preset_inspector_plugin  # 新增：预设检查器插件

func _enter_tree():
	print("ECS Editor Plugin: Entering tree")
	
	# 创建 ECS 事件总线 - 编辑器模式下的通信中心
	event_bus = ECSEventBus.new()
	get_tree().get_root().add_child(event_bus)
	print("ECS Editor Plugin: Event bus created")
	
	# 创建实例管理器
	var ECSInstanceManager = load("res://addons/ecs_editor_plugin/ecs_instance_manager.gd")
	instance_manager = ECSInstanceManager.new()
	add_child(instance_manager)
	
	# 将事件总线传递给实例管理器
	instance_manager.set_event_bus(event_bus)
	print("ECS Editor Plugin: Instance manager created and linked to event bus")
	
	# 创建并添加 dock
	dock_instance = ECSDock.new()
	dock_instance.instance_manager = instance_manager
	dock_instance.editor_plugin = self  # 传递插件引用
	add_control_to_dock(DOCK_SLOT_LEFT_UL, dock_instance)
	
	# 🆕 初始化预设功能
	_setup_preset_system()
	
	# 连接编辑器信号
	_connect_editor_signals()
	
	print("ECS Editor Plugin: Setup completed")

func _exit_tree():
	print("ECS Editor Plugin: Exiting tree")
	
	# 🆕 清理预设系统
	_cleanup_preset_system()
	
	# 清理资源
	if dock_instance:
		remove_control_from_docks(dock_instance)
		dock_instance.queue_free()
		dock_instance = null
	
	if instance_manager:
		instance_manager.cleanup()
		instance_manager.queue_free()
		instance_manager = null
	
	# 清理事件总线
	if event_bus:
		event_bus.cleanup()
		event_bus.queue_free()
		event_bus = null
		print("ECS Editor Plugin: Event bus cleaned up")

# 🆕 设置预设系统
func _setup_preset_system():
	# 实例化C++预设检查器插件
	preset_inspector_plugin = UniversalPresetInspectorPlugin.new()
	
	# 正确的解决方案：直接传递EditorInterface给检查器插件
	# 通过插件的set_editor_interface方法传递
	if preset_inspector_plugin.has_method("set_editor_interface"):
		preset_inspector_plugin.call("set_editor_interface", get_editor_interface())
		print("ECS Editor Plugin: EditorInterface passed to inspector plugin")
	
	add_inspector_plugin(preset_inspector_plugin)
	print("ECS Editor Plugin: Preset inspector plugin added")

# 🆕 清理预设系统
func _cleanup_preset_system():
	if preset_inspector_plugin:
		remove_inspector_plugin(preset_inspector_plugin)
		preset_inspector_plugin = null
		print("ECS Editor Plugin: Preset inspector plugin removed")

func _connect_editor_signals():
	# 监听场景变化
	if EditorInterface.get_selection():
		EditorInterface.get_selection().connect("selection_changed", _on_selection_changed)

func _on_selection_changed():
	# 选择变化时的处理
	pass

func get_plugin_name():
	return "ECS Editor Plugin"
