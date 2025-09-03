@tool
extends Control
class_name ECSDock

var instance_manager: Node  # 改为 Node 类型，避免循环引用
var status_label: Label
var start_button: Button
var stop_button: Button
var restart_button: Button
var persistent_checkbox: CheckBox
var auto_start_checkbox: CheckBox
var info_text: TextEdit

func _init():
	name = "ECS Core Manager"
	_build_ui()

func _build_ui():
	# 设置基本布局
	set_custom_minimum_size(Vector2(250, 400))
	
	var vbox = VBoxContainer.new()
	add_child(vbox)
	
	# 标题
	var title = Label.new()
	title.text = "ECS Core Manager"
	title.add_theme_font_size_override("font_size", 16)
	title.horizontal_alignment = HORIZONTAL_ALIGNMENT_CENTER
	vbox.add_child(title)
	
	# 分隔符
	var separator1 = HSeparator.new()
	vbox.add_child(separator1)
	
	# 状态显示
	var status_container = HBoxContainer.new()
	vbox.add_child(status_container)
	
	var status_title = Label.new()
	status_title.text = "Status: "
	status_container.add_child(status_title)
	
	status_label = Label.new()
	status_label.text = "STOPPED"
	status_label.add_theme_color_override("font_color", Color.RED)
	status_container.add_child(status_label)
	
	# 控制按钮
	var button_container = VBoxContainer.new()
	vbox.add_child(button_container)
	
	start_button = Button.new()
	start_button.text = "Start ECS Instance"
	start_button.pressed.connect(_on_start_pressed)
	button_container.add_child(start_button)
	
	stop_button = Button.new()
	stop_button.text = "Stop ECS Instance"
	stop_button.pressed.connect(_on_stop_pressed)
	stop_button.disabled = true
	button_container.add_child(stop_button)
	
	restart_button = Button.new()
	restart_button.text = "Restart ECS Instance"
	restart_button.pressed.connect(_on_restart_pressed)
	restart_button.disabled = true
	button_container.add_child(restart_button)
	
	# 新增：ECSNode 控制按钮
	var separator_nodes = HSeparator.new()
	button_container.add_child(separator_nodes)
	
	var nodes_title = Label.new()
	nodes_title.text = "ECSNode Control"
	nodes_title.add_theme_font_size_override("font_size", 12)
	button_container.add_child(nodes_title)
	
	var reset_button = Button.new()
	reset_button.text = "Reset All ECSNodes"
	reset_button.pressed.connect(_on_reset_nodes_pressed)
	button_container.add_child(reset_button)
	
	var clear_button = Button.new()
	clear_button.text = "Clear All ECSNodes"
	clear_button.pressed.connect(_on_clear_nodes_pressed)
	button_container.add_child(clear_button)
	
	# 分隔符
	var separator2 = HSeparator.new()
	vbox.add_child(separator2)
	
	# 配置选项
	var options_title = Label.new()
	options_title.text = "Options"
	options_title.add_theme_font_size_override("font_size", 14)
	vbox.add_child(options_title)
	
	persistent_checkbox = CheckBox.new()
	persistent_checkbox.text = "Editor Persistent Mode"
	persistent_checkbox.button_pressed = true
	persistent_checkbox.toggled.connect(_on_persistent_toggled)
	vbox.add_child(persistent_checkbox)
	
	auto_start_checkbox = CheckBox.new()
	auto_start_checkbox.text = "Auto Start on Plugin Load"
	auto_start_checkbox.toggled.connect(_on_auto_start_toggled)
	vbox.add_child(auto_start_checkbox)
	
	# 分隔符
	var separator3 = HSeparator.new()
	vbox.add_child(separator3)
	
	# 信息显示
	var info_title = Label.new()
	info_title.text = "Instance Info"
	info_title.add_theme_font_size_override("font_size", 14)
	vbox.add_child(info_title)
	
	info_text = TextEdit.new()
	info_text.placeholder_text = "No instance information available"
	info_text.editable = false
	info_text.custom_minimum_size = Vector2(0, 150)
	vbox.add_child(info_text)
	
	# 刷新按钮
	var refresh_button = Button.new()
	refresh_button.text = "Refresh Info"
	refresh_button.pressed.connect(_update_info_display)
	vbox.add_child(refresh_button)

func _ready():
	if instance_manager:
		_connect_manager_signals()
	_update_ui_state()
	_update_info_display()

func _connect_manager_signals():
	if not instance_manager:
		return
		
	instance_manager.status_changed.connect(_on_status_changed)
	instance_manager.instance_created.connect(_on_instance_created)
	instance_manager.instance_destroyed.connect(_on_instance_destroyed)

func _on_start_pressed():
	if instance_manager:
		instance_manager.start_instance()

func _on_stop_pressed():
	if instance_manager:
		instance_manager.stop_instance()

func _on_restart_pressed():
	if instance_manager:
		instance_manager.restart_instance()

func _on_reset_nodes_pressed():
	# 通过事件总线重置所有 ECSNode 状态
	var event_bus = get_tree().get_root().find_child("ECSEventBus", true, false)
	if event_bus:
		event_bus.call("reset_all_ecs_nodes")
		print("ECSDock: Reset signal sent to all ECSNodes")
	else:
		print("ECSDock: Warning - ECSEventBus not found")

func _on_clear_nodes_pressed():
	# 通过事件总线清除所有 ECSNode 实体
	var event_bus = get_tree().get_root().find_child("ECSEventBus", true, false)
	if event_bus:
		event_bus.call("clear_all_ecs_nodes")
		print("ECSDock: Clear signal sent to all ECSNodes")
	else:
		print("ECSDock: Warning - ECSEventBus not found")

func _on_persistent_toggled(pressed: bool):
	if instance_manager and instance_manager.current_instance:
		if instance_manager.current_instance.has_method("set_editor_persistent"):
			instance_manager.current_instance.set_editor_persistent(pressed)
		else:
			print("Warning: Persistent mode not supported in current build")

func _on_auto_start_toggled(pressed: bool):
	if instance_manager:
		instance_manager.auto_start = pressed

func _on_status_changed(status):
	_update_ui_state()
	_update_info_display()

func _on_instance_created(instance):
	_update_ui_state()
	_update_info_display()

func _on_instance_destroyed(instance):
	_update_ui_state()
	_update_info_display()

func _update_ui_state():
	if not instance_manager:
		return
	
	var is_running = instance_manager.is_running()
	var has_instance = instance_manager.current_instance != null
	
	# 更新状态标签
	var status_text = instance_manager.Status.keys()[instance_manager.get_status()]
	status_label.text = status_text
	
	# 设置状态颜色
	match instance_manager.get_status():
		instance_manager.Status.RUNNING:
			status_label.add_theme_color_override("font_color", Color.GREEN)
		instance_manager.Status.STARTING:
			status_label.add_theme_color_override("font_color", Color.YELLOW)
		instance_manager.Status.STOPPING:
			status_label.add_theme_color_override("font_color", Color.ORANGE)
		instance_manager.Status.ERROR:
			status_label.add_theme_color_override("font_color", Color.PURPLE)
		_:
			status_label.add_theme_color_override("font_color", Color.RED)
	
	# 更新按钮状态
	start_button.disabled = has_instance
	stop_button.disabled = not has_instance
	restart_button.disabled = not has_instance
	
	# 更新复选框
	if has_instance and instance_manager.current_instance:
		if instance_manager.current_instance.has_method("is_editor_persistent"):
			persistent_checkbox.button_pressed = instance_manager.current_instance.is_editor_persistent()
		else:
			persistent_checkbox.button_pressed = false

func _update_info_display():
	if not instance_manager:
		info_text.text = "No instance manager available"
		return
	
	var info = instance_manager.get_status_info()
	var info_lines = []
	
	for key in info.keys():
		info_lines.append("%s: %s" % [key, str(info[key])])
	
	# 添加额外信息
	if instance_manager.current_instance:
		info_lines.append("Node Name: " + instance_manager.current_instance.name)
		info_lines.append("Parent: " + str(instance_manager.current_instance.get_parent().name if instance_manager.current_instance.get_parent() else "None"))
	
	# 新增：ECSNode 统计信息
	var event_bus = get_tree().get_root().find_child("ECSEventBus", true, false)
	if event_bus and event_bus.has_method("get_ecs_nodes_info"):
		info_lines.append("--- ECSNode Statistics ---")
		var nodes_info = event_bus.call("get_ecs_nodes_info")
		for key in nodes_info.keys():
			info_lines.append("ECS %s: %s" % [key.replace("_", " ").capitalize(), str(nodes_info[key])])
	
	info_text.text = "\n".join(info_lines)
