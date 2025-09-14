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
var editor_plugin: EditorPlugin  # 用于访问EditorInterface
var snapshot_status_label: Label  # 快照状态标签
var snapshots_list: VBoxContainer  # 快照列表容器
var reload_to_ecs_button: Button  # Reload Scene to ECS 按钮引用
var pause_button: Button  # 暂停/恢复按钮引用

func _init():
	name = "ECS Core Manager"
	_build_ui()

func _build_ui():
	# 设置基本布局 - 占满侧边栏宽度
	size_flags_horizontal = Control.SIZE_EXPAND_FILL
	size_flags_vertical = Control.SIZE_EXPAND_FILL
	# 设置合理的最小宽度确保插件面板占满侧边栏
	set_custom_minimum_size(Vector2(200, 0))
	
	var main_vbox = VBoxContainer.new()
	main_vbox.size_flags_horizontal = Control.SIZE_EXPAND_FILL
	main_vbox.set_anchors_and_offsets_preset(Control.PRESET_FULL_RECT)
	add_child(main_vbox)
	
	# 标题
	var title = Label.new()
	title.text = "ECS Core Manager"
	title.add_theme_font_size_override("font_size", 16)
	title.horizontal_alignment = HORIZONTAL_ALIGNMENT_CENTER
	main_vbox.add_child(title)
	
	# 工具栏
	var toolbar = HBoxContainer.new()
	toolbar.size_flags_horizontal = Control.SIZE_EXPAND_FILL
	toolbar.alignment = BoxContainer.ALIGNMENT_CENTER
	main_vbox.add_child(toolbar)
	
	# 重启按钮
	var restart_button = Button.new()
	restart_button.icon = get_theme_icon("Reload", "EditorIcons")
	restart_button.tooltip_text = "Restart ECS Core"
	restart_button.flat = true
	restart_button.pressed.connect(_on_restart_pressed)
	toolbar.add_child(restart_button)
	
	# 开始/暂停按钮
	var play_pause_button = Button.new()
	play_pause_button.icon = get_theme_icon("Play", "EditorIcons")
	play_pause_button.tooltip_text = "Start/Pause ECS"
	play_pause_button.flat = true
	play_pause_button.toggle_mode = true
	play_pause_button.pressed.connect(_on_play_pause_pressed)
	play_pause_button.name = "PlayPauseButton"
	toolbar.add_child(play_pause_button)
	
	# Reload Scene按钮
	var reload_scene_button = Button.new()
	reload_scene_button.icon = get_theme_icon("FileSystem", "EditorIcons")
	reload_scene_button.tooltip_text = "Reload Current Scene"
	reload_scene_button.flat = true
	reload_scene_button.pressed.connect(_on_reload_scene_pressed)
	toolbar.add_child(reload_scene_button)
	
	# 分隔符
	var separator1 = HSeparator.new()
	main_vbox.add_child(separator1)
	
	# 创建TabContainer - 确保占满插件面板宽度
	var tab_container = TabContainer.new()
	tab_container.size_flags_horizontal = Control.SIZE_EXPAND_FILL
	tab_container.size_flags_vertical = Control.SIZE_EXPAND_FILL
	tab_container.set_custom_minimum_size(Vector2(200, 300))
	tab_container.tab_changed.connect(_on_tab_changed)
	main_vbox.add_child(tab_container)
	
	# 创建各个标签页
	_create_core_control_tab(tab_container)
	_create_entity_testing_tab(tab_container)
	_create_scene_management_tab(tab_container)
	_create_template_schema_tab(tab_container)
	_create_system_info_tab(tab_container)
	_create_config_tab(tab_container)

func _create_core_control_tab(parent: TabContainer):
	"""创建核心控制标签页"""
	var vbox = VBoxContainer.new()
	vbox.name = "Core Control"
	vbox.size_flags_horizontal = Control.SIZE_EXPAND_FILL
	vbox.set_custom_minimum_size(Vector2(200, 0))
	parent.add_child(vbox)
	
	# 状态显示
	var status_container = HBoxContainer.new()
	status_container.size_flags_horizontal = Control.SIZE_EXPAND_FILL
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
	button_container.size_flags_horizontal = Control.SIZE_EXPAND_FILL
	vbox.add_child(button_container)
	
	start_button = Button.new()
	start_button.text = "Start ECS Instance"
	start_button.size_flags_horizontal = Control.SIZE_EXPAND_FILL
	start_button.pressed.connect(_on_start_pressed)
	button_container.add_child(start_button)
	
	stop_button = Button.new()
	stop_button.text = "Stop ECS Instance"
	stop_button.size_flags_horizontal = Control.SIZE_EXPAND_FILL
	stop_button.pressed.connect(_on_stop_pressed)
	stop_button.disabled = true
	button_container.add_child(stop_button)
	
	restart_button = Button.new()
	restart_button.text = "Restart ECS Instance"
	restart_button.size_flags_horizontal = Control.SIZE_EXPAND_FILL
	restart_button.pressed.connect(_on_restart_pressed)
	restart_button.disabled = true
	button_container.add_child(restart_button)
	
	# 暂停/恢复按钮
	pause_button = Button.new()
	pause_button.name = "PauseButton"
	pause_button.text = "Pause ECS"
	pause_button.size_flags_horizontal = Control.SIZE_EXPAND_FILL
	pause_button.pressed.connect(_on_pause_pressed)
	pause_button.disabled = true
	button_container.add_child(pause_button)
	
	# 配置选项
	var separator_options = HSeparator.new()
	vbox.add_child(separator_options)
	
	var options_title = Label.new()
	options_title.text = "Options"
	options_title.add_theme_font_size_override("font_size", 12)
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

func _create_entity_testing_tab(parent: TabContainer):
	"""创建实体测试标签页"""
	var vbox = VBoxContainer.new()
	vbox.name = "Entity Test"
	vbox.size_flags_horizontal = Control.SIZE_EXPAND_FILL
	vbox.set_custom_minimum_size(Vector2(200, 0))
	parent.add_child(vbox)
	
	# 实体生成区域
	var spawn_title = Label.new()
	spawn_title.text = "Entity Spawning"
	spawn_title.add_theme_font_size_override("font_size", 12)
	vbox.add_child(spawn_title)
	
	var spawn_random_button = Button.new()
	spawn_random_button.text = "Spawn Random Entity"
	spawn_random_button.size_flags_horizontal = Control.SIZE_EXPAND_FILL
	spawn_random_button.tooltip_text = "Randomly spawn an entity from available templates"
	spawn_random_button.pressed.connect(_on_spawn_random_entity_pressed)
	vbox.add_child(spawn_random_button)
	
	var spawn_multiple_button = Button.new()
	spawn_multiple_button.text = "Spawn 5 Random Entities"
	spawn_multiple_button.size_flags_horizontal = Control.SIZE_EXPAND_FILL
	spawn_multiple_button.tooltip_text = "Spawn 5 random entities for stress testing"
	spawn_multiple_button.pressed.connect(_on_spawn_multiple_entities_pressed)
	vbox.add_child(spawn_multiple_button)
	
	# ECSNode 控制区域
	var separator_nodes = HSeparator.new()
	vbox.add_child(separator_nodes)
	
	var nodes_title = Label.new()
	nodes_title.text = "ECSNode Control"
	nodes_title.add_theme_font_size_override("font_size", 12)
	vbox.add_child(nodes_title)
	
	var reset_button = Button.new()
	reset_button.text = "Reset All ECSNodes"
	reset_button.size_flags_horizontal = Control.SIZE_EXPAND_FILL
	reset_button.pressed.connect(_on_reset_nodes_pressed)
	vbox.add_child(reset_button)
	
	var clear_button = Button.new()
	clear_button.text = "Clear All ECSNodes"
	clear_button.size_flags_horizontal = Control.SIZE_EXPAND_FILL
	clear_button.pressed.connect(_on_clear_nodes_pressed)
	vbox.add_child(clear_button)
	
	var clear_entities_button = Button.new()
	clear_entities_button.text = "Clear All Entities"
	clear_entities_button.size_flags_horizontal = Control.SIZE_EXPAND_FILL
	clear_entities_button.tooltip_text = "Remove all spawned entities from the scene"
	clear_entities_button.pressed.connect(_on_clear_all_entities_pressed)
	vbox.add_child(clear_entities_button)

func _create_scene_management_tab(parent: TabContainer):
	"""创建场景管理标签页"""
	var vbox = VBoxContainer.new()
	vbox.name = "Scene Management"
	vbox.size_flags_horizontal = Control.SIZE_EXPAND_FILL
	vbox.set_custom_minimum_size(Vector2(200, 0))
	parent.add_child(vbox)
	
	# 同步场景到ECS按钮
	reload_to_ecs_button = Button.new()
	reload_to_ecs_button.text = "Reload Scene to ECS"
	reload_to_ecs_button.size_flags_horizontal = Control.SIZE_EXPAND_FILL
	reload_to_ecs_button.tooltip_text = "Force reload current editor scene state to ECS entities"
	reload_to_ecs_button.pressed.connect(_on_reload_scene_to_ecs_pressed)
	reload_to_ecs_button.disabled = true  # 初始状态为禁用
	vbox.add_child(reload_to_ecs_button)
	
	# 重新加载场景按钮
	var reload_scene_button = Button.new()
	reload_scene_button.text = "Reload Scene"
	reload_scene_button.size_flags_horizontal = Control.SIZE_EXPAND_FILL
	reload_scene_button.tooltip_text = "Reload current scene from file"
	reload_scene_button.pressed.connect(_on_reload_scene_pressed)
	vbox.add_child(reload_scene_button)
	
	# Snapshot 管理区域
	var separator_snapshots = HSeparator.new()
	vbox.add_child(separator_snapshots)
	
	# Snapshot 标题和状态
	var snapshot_header = HBoxContainer.new()
	snapshot_header.size_flags_horizontal = Control.SIZE_EXPAND_FILL
	vbox.add_child(snapshot_header)
	
	var snapshot_title = Label.new()
	snapshot_title.text = "Snapshots"
	snapshot_title.add_theme_font_size_override("font_size", 12)
	snapshot_header.add_child(snapshot_title)
	
	snapshot_status_label = Label.new()
	snapshot_status_label.name = "SnapshotStatus"
	snapshot_status_label.text = "(0)"
	snapshot_status_label.add_theme_color_override("font_color", Color.GRAY)
	snapshot_header.add_child(snapshot_status_label)
	
	# 快照按钮
	var snapshot_buttons = HBoxContainer.new()
	snapshot_buttons.size_flags_horizontal = Control.SIZE_EXPAND_FILL
	vbox.add_child(snapshot_buttons)
	
	var create_snapshot_button = Button.new()
	create_snapshot_button.text = "Create"
	create_snapshot_button.size_flags_horizontal = Control.SIZE_EXPAND_FILL
	create_snapshot_button.tooltip_text = "Create new snapshot"
	create_snapshot_button.pressed.connect(_on_create_snapshot_pressed)
	snapshot_buttons.add_child(create_snapshot_button)
	
	var clear_all_button = Button.new()
	clear_all_button.text = "Clear All"
	clear_all_button.size_flags_horizontal = Control.SIZE_EXPAND_FILL
	clear_all_button.tooltip_text = "Delete all snapshots"
	clear_all_button.pressed.connect(_on_clear_all_snapshots_pressed)
	snapshot_buttons.add_child(clear_all_button)
	
	# 快照列表容器
	var snapshots_scroll = ScrollContainer.new()
	snapshots_scroll.size_flags_horizontal = Control.SIZE_EXPAND_FILL
	snapshots_scroll.size_flags_vertical = Control.SIZE_EXPAND_FILL
	snapshots_scroll.set_custom_minimum_size(Vector2(0, 150))
	vbox.add_child(snapshots_scroll)
	
	snapshots_list = VBoxContainer.new()
	snapshots_list.name = "SnapshotsList"
	snapshots_list.size_flags_horizontal = Control.SIZE_EXPAND_FILL
	snapshots_scroll.add_child(snapshots_list)

func _create_system_info_tab(parent: TabContainer):
	"""创建系统信息标签页"""
	var vbox = VBoxContainer.new()
	vbox.name = "System Info"
	vbox.size_flags_horizontal = Control.SIZE_EXPAND_FILL
	vbox.set_custom_minimum_size(Vector2(200, 0))
	parent.add_child(vbox)
	
	# 信息显示
	var info_title = Label.new()
	info_title.text = "Instance Info"
	info_title.add_theme_font_size_override("font_size", 12)
	vbox.add_child(info_title)
	
	info_text = TextEdit.new()
	info_text.placeholder_text = "No instance information available"
	info_text.editable = false
	info_text.size_flags_horizontal = Control.SIZE_EXPAND_FILL
	info_text.size_flags_vertical = Control.SIZE_EXPAND_FILL
	info_text.custom_minimum_size = Vector2(0, 200)
	vbox.add_child(info_text)
	
	# 刷新按钮
	var refresh_button = Button.new()
	refresh_button.text = "Refresh Info"
	refresh_button.size_flags_horizontal = Control.SIZE_EXPAND_FILL
	refresh_button.pressed.connect(_update_info_display)
	vbox.add_child(refresh_button)

func _create_config_tab(parent: TabContainer):
	"""创建配置选项标签页"""
	var vbox = VBoxContainer.new()
	vbox.name = "Config"
	vbox.size_flags_horizontal = Control.SIZE_EXPAND_FILL
	vbox.set_custom_minimum_size(Vector2(200, 0))
	parent.add_child(vbox)
	
	# 配置选项标题
	var options_title = Label.new()
	options_title.text = "Options"
	options_title.add_theme_font_size_override("font_size", 12)
	vbox.add_child(options_title)
	
	# 分隔符
	var separator = HSeparator.new()
	vbox.add_child(separator)
	
	persistent_checkbox = CheckBox.new()
	persistent_checkbox.text = "Editor Persistent Mode"
	persistent_checkbox.button_pressed = true
	persistent_checkbox.toggled.connect(_on_persistent_toggled)
	vbox.add_child(persistent_checkbox)
	
	auto_start_checkbox = CheckBox.new()
	auto_start_checkbox.text = "Auto Start on Plugin Load"
	auto_start_checkbox.toggled.connect(_on_auto_start_toggled)
	vbox.add_child(auto_start_checkbox)
	
	# 添加一些空间
	var spacer = Control.new()
	spacer.size_flags_vertical = Control.SIZE_EXPAND_FILL
	vbox.add_child(spacer)

func _ready():
	# 连接实例管理器的信号
	if instance_manager:
		instance_manager.status_changed.connect(_on_status_changed)
		print("ECSDock: Connected to instance manager signals")
	
	# 刷新快照列表和状态
	_refresh_snapshots_list()

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

func _on_pause_pressed():
	"""暂停/恢复按钮处理"""
	if not instance_manager:
		print("ECSDock: No instance manager available")
		return
	
	if instance_manager.is_running():
		# 当前运行 -> 暂停
		if instance_manager.pause_instance():
			print("ECSDock: ECS instance paused")
		else:
			print("ECSDock: Failed to pause ECS instance")
	elif instance_manager.is_paused():
		# 当前暂停 -> 恢复
		if instance_manager.resume_instance():
			print("ECSDock: ECS instance resumed")
		else:
			print("ECSDock: Failed to resume ECS instance")
	else:
		print("ECSDock: Cannot pause/resume - ECS instance not in valid state")

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
			status_label.text = "RUNNING"
			status_label.add_theme_color_override("font_color", Color.GREEN)
		instance_manager.Status.PAUSED:
			status_label.text = "PAUSED"
			status_label.add_theme_color_override("font_color", Color.ORANGE)
		instance_manager.Status.STARTING:
			status_label.add_theme_color_override("font_color", Color.YELLOW)
		instance_manager.Status.STOPPING:
			status_label.add_theme_color_override("font_color", Color.ORANGE)
		instance_manager.Status.ERROR:
			status_label.add_theme_color_override("font_color", Color.PURPLE)
		_:
			status_label.add_theme_color_override("font_color", Color.RED)
	
	# 更新按钮状态
	var is_active = instance_manager.is_active() if instance_manager else false
	var is_running_status = instance_manager.is_running() if instance_manager else false
	var is_paused = instance_manager.is_paused() if instance_manager else false
	
	start_button.disabled = has_instance
	stop_button.disabled = not is_active  # 运行或暂停状态都可以停止
	restart_button.disabled = not is_active
	
	# 更新暂停/恢复按钮
	if pause_button:
		pause_button.disabled = not is_active
		if is_running_status:
			pause_button.text = "Pause ECS"
			pause_button.tooltip_text = "暂停ECS更新，保持状态"
		elif is_paused:
			pause_button.text = "Resume ECS"
			pause_button.tooltip_text = "恢复ECS更新"
		else:
			pause_button.text = "Pause ECS"
			pause_button.tooltip_text = "暂停ECS更新，保持状态"
	
	# 更新"Reload Scene to ECS"按钮状态 - 运行或暂停时都可用
	if reload_to_ecs_button:
		reload_to_ecs_button.disabled = not is_active
	
	# 更新复选框
	if has_instance and instance_manager.current_instance:
		if instance_manager.current_instance.has_method("is_editor_persistent"):
			persistent_checkbox.button_pressed = instance_manager.current_instance.is_editor_persistent()
		else:
			persistent_checkbox.button_pressed = false
	
	# 同步工具栏按钮状态
	_update_toolbar_button_state()

func _update_toolbar_button_state():
	"""更新工具栏按钮状态以与第一个标签页的pause_button保持同步"""
	var play_pause_button = find_child("PlayPauseButton")
	if not play_pause_button or not instance_manager:
		return
	
	var is_running = instance_manager.is_running()
	var is_paused = instance_manager.is_paused()
	var has_instance = instance_manager.current_instance != null
	var is_active = instance_manager.is_active() if instance_manager else false
	
	# 与第一个标签页的pause_button逻辑保持一致
	if not has_instance:
		# 没有实例 -> 显示启动图标
		play_pause_button.icon = get_theme_icon("Play", "EditorIcons")
		play_pause_button.tooltip_text = "Start ECS Instance"
		play_pause_button.disabled = false
	elif is_running:
		# 正在运行 -> 显示暂停图标
		play_pause_button.icon = get_theme_icon("Pause", "EditorIcons")
		play_pause_button.tooltip_text = "Pause ECS"
		play_pause_button.disabled = false
	elif is_paused:
		# 已暂停 -> 显示恢复图标
		play_pause_button.icon = get_theme_icon("Play", "EditorIcons")
		play_pause_button.tooltip_text = "Resume ECS"
		play_pause_button.disabled = false
	else:
		# 其他状态（停止等）-> 禁用
		play_pause_button.icon = get_theme_icon("Play", "EditorIcons")
		play_pause_button.tooltip_text = "Start ECS Instance"
		play_pause_button.disabled = not is_active

func _on_play_pause_pressed():
	"""处理工具栏开始/暂停按钮 - 与第一个标签页的暂停按钮功能一致"""
	if not instance_manager:
		print("ECSDock: No instance manager available")
		return
	
	# 如果没有实例，先启动
	if not instance_manager.current_instance:
		_on_start_pressed()
		return
	
	# 有实例时，直接调用暂停/恢复逻辑（与第一个标签页的pause_button相同）
	_on_pause_pressed()
	
	# 更新UI状态
	_update_ui_state()
	

func _on_reload_scene_pressed():
	"""重新加载当前场景"""
	if not editor_plugin:
		print("ECSDock: Editor plugin not available")
		return
	
	var editor_interface = editor_plugin.get_editor_interface()
	if not editor_interface:
		print("ECSDock: EditorInterface not available")
		return
	
	var scene_root = editor_interface.get_edited_scene_root()
	if not scene_root:
		print("ECSDock: No active scene")
		return
	
	var scene_path = scene_root.scene_file_path
	if scene_path.is_empty():
		print("ECSDock: Scene not saved, cannot reload")
		return
	
	print("ECSDock: Reloading scene from: ", scene_path)
	editor_interface.reload_scene_from_path(scene_path)

func _on_create_snapshot_pressed():
	"""创建新快照"""
	if instance_manager.is_running():
		print("ECSDock: Cannot create snapshot while ECS instance is running")
		return
	
	if instance_manager:
		var snapshot_id = instance_manager.create_and_save_snapshot()
		if not snapshot_id.is_empty():
			_refresh_snapshots_list()
			print("ECSDock: Snapshot created with ID: %s" % snapshot_id)
		else:
			print("ECSDock: Failed to create snapshot")
	else:
		print("ECSDock: Instance manager not available")

func _on_clear_all_snapshots_pressed():
	"""删除所有快照"""
	if instance_manager.is_running():
		print("ECSDock: Cannot delete snapshots while ECS instance is running")
		return
	
	if instance_manager:
		if instance_manager.delete_snapshot(""):  # 空字符串表示删除所有
			_refresh_snapshots_list()
			print("ECSDock: All snapshots deleted")
		else:
			print("ECSDock: Failed to delete snapshots")
	else:
		print("ECSDock: Instance manager not available")

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
	
	# 同时刷新快照列表
	_refresh_snapshots_list()

func _update_snapshot_status():
	"""更新快照状态显示"""
	if not snapshot_status_label:
		return
	
	if not instance_manager:
		snapshot_status_label.text = "(0)"
		snapshot_status_label.add_theme_color_override("font_color", Color.GRAY)
		return
	
	var snapshots = instance_manager.get_snapshots_list()
	var count = snapshots.size()
	
	snapshot_status_label.text = "(%d)" % count
	if count > 0:
		snapshot_status_label.add_theme_color_override("font_color", Color.GREEN)
	else:
		snapshot_status_label.add_theme_color_override("font_color", Color.GRAY)

func _refresh_snapshots_list():
	"""刷新快照列表显示"""
	if not snapshots_list or not instance_manager:
		return
	
	# 清空现有列表
	for child in snapshots_list.get_children():
		child.queue_free()
	
	# 获取快照列表
	var snapshots = instance_manager.get_snapshots_list()
	
	if snapshots.is_empty():
		var no_snapshots_label = Label.new()
		no_snapshots_label.text = "No snapshots available"
		no_snapshots_label.add_theme_color_override("font_color", Color.GRAY)
		no_snapshots_label.horizontal_alignment = HORIZONTAL_ALIGNMENT_CENTER
		snapshots_list.add_child(no_snapshots_label)
	else:
		# 为每个快照创建UI元素
		for snapshot in snapshots:
			var item_container = VBoxContainer.new()
			item_container.add_theme_constant_override("separation", 2)
			snapshots_list.add_child(item_container)
			
			# 快照信息行
			var info_container = HBoxContainer.new()
			item_container.add_child(info_container)
			
			var name_label = Label.new()
			name_label.text = snapshot.name
			name_label.add_theme_font_size_override("font_size", 10)
			name_label.clip_contents = true
			info_container.add_child(name_label)
			
			var spacer = Control.new()
			spacer.size_flags_horizontal = Control.SIZE_EXPAND_FILL
			info_container.add_child(spacer)
			
			var node_count_label = Label.new()
			node_count_label.text = "%d nodes" % snapshot.node_count
			node_count_label.add_theme_font_size_override("font_size", 9)
			node_count_label.add_theme_color_override("font_color", Color.GRAY)
			info_container.add_child(node_count_label)
			
			# 时间戳
			var time_label = Label.new()
			time_label.text = snapshot.timestamp.replace("T", " ")
			time_label.add_theme_font_size_override("font_size", 8)
			time_label.add_theme_color_override("font_color", Color.GRAY)
			item_container.add_child(time_label)
			
			# 操作按钮
			var buttons_container = HBoxContainer.new()
			item_container.add_child(buttons_container)
			
			var restore_btn = Button.new()
			restore_btn.text = "Restore"
			restore_btn.add_theme_font_size_override("font_size", 9)
			restore_btn.pressed.connect(_on_restore_snapshot.bind(snapshot.id))
			buttons_container.add_child(restore_btn)
			
			var rename_btn = Button.new()
			rename_btn.text = "Rename"
			rename_btn.add_theme_font_size_override("font_size", 9)
			rename_btn.pressed.connect(_on_rename_snapshot.bind(snapshot.id))
			buttons_container.add_child(rename_btn)
			
			var delete_btn = Button.new()
			delete_btn.text = "Delete"
			delete_btn.add_theme_font_size_override("font_size", 9)
			delete_btn.pressed.connect(_on_delete_snapshot.bind(snapshot.id))
			buttons_container.add_child(delete_btn)
			
			# 分隔线
			var separator = HSeparator.new()
			item_container.add_child(separator)
	
	# 更新状态
	_update_snapshot_status()

func _on_restore_snapshot(snapshot_id: String):
	"""恢复指定快照"""
	if instance_manager.is_running():
		print("ECSDock: Cannot restore snapshot while ECS instance is running")
		return
	
	if instance_manager.restore_snapshot(snapshot_id):
		print("ECSDock: Snapshot restored successfully")
	else:
		print("ECSDock: Failed to restore snapshot")

func _on_rename_snapshot(snapshot_id: String):
	"""重命名快照"""
	# 简单的重命名对话框（可以改进为更好的UI）
	var new_name = "Renamed Snapshot " + Time.get_datetime_string_from_system().replace("T", " ")
	if instance_manager.rename_snapshot(snapshot_id, new_name):
		_refresh_snapshots_list()
		print("ECSDock: Snapshot renamed to '%s'" % new_name)
	else:
		print("ECSDock: Failed to rename snapshot")

func _on_delete_snapshot(snapshot_id: String):
	"""删除指定快照"""
	if instance_manager.is_running():
		print("ECSDock: Cannot delete snapshot while ECS instance is running")
		return
	
	if instance_manager.delete_snapshot(snapshot_id):
		_refresh_snapshots_list()
		print("ECSDock: Snapshot deleted")
	else:
		print("ECSDock: Failed to delete snapshot")

func _on_reload_scene_to_ecs_pressed():
	"""强制重新加载编辑器场景状态到ECS"""
	if not instance_manager.is_running():
		print("ECSDock: ECS instance is not running - cannot reload scene to ECS")
		return
	
	if not editor_plugin:
		print("ECSDock: Editor plugin not available")
		return
	
	var editor_interface = editor_plugin.get_editor_interface()
	if not editor_interface:
		print("ECSDock: EditorInterface not available")
		return
	
	var scene_root = editor_interface.get_edited_scene_root()
	if not scene_root:
		print("ECSDock: No active scene")
		return
	
	print("ECSDock: Starting scene to ECS reload...")
	
	# 获取事件总线
	var event_bus = get_tree().get_root().find_child("ECSEventBus", true, false)
	if not event_bus:
		print("ECSDock: ECSEventBus not found")
		return
	
	# 调用事件总线的重新加载功能
	if event_bus.has_method("reload_scene_to_ecs"):
		var success = event_bus.call("reload_scene_to_ecs")
		if success:
			print("ECSDock: Scene reloaded to ECS successfully")
		else:
			print("ECSDock: Failed to reload scene to ECS")
	else:
		print("ECSDock: ECSEventBus does not support reload_scene_to_ecs method")

# === 实体测试功能 ===

func _on_spawn_random_entity_pressed():
	"""随机生成一个实体到场景中"""
	if not editor_plugin:
		print("ECSDock: Editor plugin not available")
		return
	
	var editor_interface = editor_plugin.get_editor_interface()
	if not editor_interface:
		print("ECSDock: EditorInterface not available")
		return
	
	var scene_root = editor_interface.get_edited_scene_root()
	if not scene_root:
		print("ECSDock: No active scene")
		return
	
	# 获取可用模板
	var available_templates = GameCoreManager.get_available_templates()
	if available_templates.is_empty():
		print("ECSDock: No templates available")
		return
	
	# 随机选择一个模板
	var random_template = available_templates[randi() % available_templates.size()]
	print("ECSDock: Spawning random entity with template: ", random_template)
	
	# 随机位置（在场景中心附近）
	var random_pos = Vector3(
		randf_range(-5.0, 5.0),
		randf_range(0.0, 3.0),
		randf_range(-5.0, 5.0)
	)
	
	# 生成实体
	var entity = GameCoreManager.spawn_entity(random_template, scene_root, {"position": random_pos})
	if entity:
		print("ECSDock: Successfully spawned entity at position: ", random_pos)
		# 标记场景为已修改
		editor_interface.mark_scene_as_unsaved()
	else:
		print("ECSDock: Failed to spawn entity")

func _on_spawn_multiple_entities_pressed():
	"""生成5个随机实体进行压力测试"""
	print("ECSDock: Spawning 5 random entities...")
	for i in range(5):
		_on_spawn_random_entity_pressed()
		await get_tree().process_frame  # 等待一帧，避免阻塞
	print("ECSDock: Finished spawning 5 entities")

func _on_clear_all_entities_pressed():
	"""清除场景中所有生成的实体"""
	if not editor_plugin:
		print("ECSDock: Editor plugin not available")
		return
	
	var editor_interface = editor_plugin.get_editor_interface()
	if not editor_interface:
		print("ECSDock: EditorInterface not available")
		return
	
	var scene_root = editor_interface.get_edited_scene_root()
	if not scene_root:
		print("ECSDock: No active scene")
		return
	
	# 查找并删除所有通过GameCoreManager生成的实体
	# 这些实体通常会有特定的命名模式或标记
	var entities_removed = 0
	var children_to_remove = []
	
	# 收集需要删除的节点
	for child in scene_root.get_children():
		# 检查是否是通过模板生成的实体（通常名称包含模板名）
		if _is_spawned_entity(child):
			children_to_remove.append(child)
	
	# 删除收集到的节点
	for child in children_to_remove:
		child.queue_free()
		entities_removed += 1
	
	if entities_removed > 0:
		print("ECSDock: Removed ", entities_removed, " spawned entities")
		# 标记场景为已修改
		editor_interface.mark_scene_as_unsaved()
	else:
		print("ECSDock: No spawned entities found to remove")

func _is_spawned_entity(node: Node) -> bool:
	"""检查节点是否是通过GameCoreManager生成的实体"""
	# 检查节点名称是否包含常见的模板名称
	var node_name = node.name.to_lower()
	var template_indicators = ["ball", "cube", "sphere", "box", "entity", "test"]
	
	for indicator in template_indicators:
		if indicator in node_name:
			return true
	
	# 检查是否有ECS相关的组件或标记
	if node.has_method("get_ecs_components") or node.get("is_ecs_entity"):
		return true
	
	# 检查是否是从模板实例化的场景
	if node.scene_file_path != "" and "templates" in node.scene_file_path:
		return true
	
	return false

func _create_template_schema_tab(parent: TabContainer):
	"""创建Template Schema标签页"""
	var vbox = VBoxContainer.new()
	vbox.name = "Template Schema"
	vbox.size_flags_horizontal = Control.SIZE_EXPAND_FILL
	vbox.set_custom_minimum_size(Vector2(200, 0))
	parent.add_child(vbox)
	
	# 按钮容器
	var button_hbox = HBoxContainer.new()
	button_hbox.size_flags_horizontal = Control.SIZE_EXPAND_FILL
	vbox.add_child(button_hbox)
	
	# 刷新按钮
	var refresh_button = Button.new()
	refresh_button.text = "Refresh"
	refresh_button.size_flags_horizontal = Control.SIZE_EXPAND_FILL
	refresh_button.pressed.connect(_on_template_refresh_pressed)
	button_hbox.add_child(refresh_button)
	
	# 生成按钮
	var generate_button = Button.new()
	generate_button.text = "Generate Schema"
	generate_button.size_flags_horizontal = Control.SIZE_EXPAND_FILL
	generate_button.pressed.connect(_on_template_generate_pressed)
	generate_button.disabled = true
	generate_button.name = "GenerateButton"
	button_hbox.add_child(generate_button)
	
	# 分隔线
	var separator = HSeparator.new()
	vbox.add_child(separator)
	
	# 文件树标签
	var tree_label = Label.new()
	tree_label.text = "Template Files:"
	vbox.add_child(tree_label)
	
	# 文件树
	template_tree = Tree.new()
	template_tree.set_custom_minimum_size(Vector2(180, 200))
	template_tree.size_flags_horizontal = Control.SIZE_EXPAND_FILL
	template_tree.item_selected.connect(_on_template_tree_item_selected)
	template_tree.name = "TemplateTree"
	vbox.add_child(template_tree)
	
	# 信息面板标签
	var info_label = Label.new()
	info_label.text = "Template Info:"
	vbox.add_child(info_label)
	
	# 信息面板
	var scroll = ScrollContainer.new()
	scroll.set_custom_minimum_size(Vector2(180, 100))
	scroll.size_flags_horizontal = Control.SIZE_EXPAND_FILL
	scroll.size_flags_vertical = Control.SIZE_EXPAND_FILL
	var info_panel = VBoxContainer.new()
	info_panel.name = "TemplateInfoPanel"
	scroll.add_child(info_panel)
	vbox.add_child(scroll)
	
	# 延迟初始化模板列表，确保UI完全构建后再加载
	call_deferred("_refresh_template_list")

# Template Schema相关的变量
var selected_template_path: String = ""
var template_tree: Tree = null
const TEMPLATES_PATH = "res://templates"

# Template Schema相关的函数
func _on_tab_changed(tab_index: int):
	# Template Schema标签页的索引是3（Core Control=0, Entity Testing=1, Scene Management=2, Template Schema=3）
	if tab_index == 3:
		print("Switched to Template Schema tab, auto-refreshing...")
		_refresh_template_list()

func _on_template_refresh_pressed():
	_refresh_template_list()
	_clear_template_info_panel()
	var generate_button = find_child("GenerateButton")
	if generate_button:
		generate_button.disabled = true
	selected_template_path = ""

func _on_template_generate_pressed():
	if selected_template_path.is_empty():
		print("[TemplateSchema] No template selected")
		return
	
	# 使用Template Schema Generator生成schema
	var generator = load("res://addons/ecs_editor_plugin/template_schema_generator.gd").new()
	var success = generator.generate_schema(selected_template_path)
	
	if success:
		print("[TemplateSchema] Schema generated successfully for: ", selected_template_path)
		_refresh_template_list()  # 刷新列表以显示新生成的schema
	else:
		print("[TemplateSchema] Failed to generate schema for: ", selected_template_path)

func _find_button_recursive(node: Node, button_name: String) -> Button:
	"""递归搜索指定名称的按钮"""
	if node.name == button_name and node is Button:
		return node as Button
	
	for child in node.get_children():
		var result = _find_button_recursive(child, button_name)
		if result:
			return result
	
	return null

func _on_template_tree_item_selected():
	print("[TemplateSchema] Tree item selected callback triggered")
	
	if not template_tree:
		print("[TemplateSchema] ERROR: template_tree is null!")
		return
	
	var selected_item = template_tree.get_selected()
	if not selected_item:
		print("[TemplateSchema] No item selected")
		return
	
	var item_text = selected_item.get_text(0)
	print("[TemplateSchema] Selected item text: ", item_text)
	
	var metadata = selected_item.get_metadata(0)
	print("[TemplateSchema] Item metadata: ", metadata)
	
	if metadata and metadata.has("type") and metadata.type == "template":
		selected_template_path = metadata.path
		print("[TemplateSchema] Template selected: ", selected_template_path)
		
		var generate_button = find_child("GenerateButton", true, false)
		if generate_button:
			generate_button.disabled = false
			print("[TemplateSchema] Generate button enabled")
		else:
			print("[TemplateSchema] ERROR: GenerateButton not found!")
			print("[TemplateSchema] Searching in all children...")
			# 尝试递归搜索
			generate_button = _find_button_recursive(self, "GenerateButton")
			if generate_button:
				generate_button.disabled = false
				print("[TemplateSchema] Generate button found and enabled via recursive search")
			
		_update_template_info_panel(metadata.path)
	else:
		print("[TemplateSchema] Non-template item selected or invalid metadata")
		var generate_button = find_child("GenerateButton", true, false)
		if not generate_button:
			generate_button = _find_button_recursive(self, "GenerateButton")
		if generate_button:
			generate_button.disabled = true
			print("[TemplateSchema] Generate button disabled")
		selected_template_path = ""
		_clear_template_info_panel()

func _refresh_template_list():
	print("=== TEMPLATE SCHEMA DEBUG: _refresh_template_list called ===")
	if not template_tree:
		print("ERROR: template_tree is null!")
		# 尝试重新获取template_tree引用
		template_tree = find_child("TemplateTree")
		if not template_tree:
			print("ERROR: Still cannot find TemplateTree after retry!")
			return
		else:
			print("SUCCESS: Found TemplateTree after retry")
	else:
		print("template_tree found successfully")
	
	# 检查模板目录是否存在
	if not DirAccess.dir_exists_absolute(TEMPLATES_PATH):
		print("ERROR: Templates directory does not exist: ", TEMPLATES_PATH)
		return
	else:
		print("Templates directory exists: ", TEMPLATES_PATH)
	
	template_tree.clear()
	var root = template_tree.create_item()
	root.set_text(0, "templates")
	root.set_icon(0, get_theme_icon("Folder", "EditorIcons"))
	root.collapsed = false  # 默认展开根节点
	
	print("Starting recursive scan of templates...")
	_scan_templates_recursive(TEMPLATES_PATH, root)
	print("Template scan completed")
	
	# 设置根节点展开
	if template_tree.get_root():
		template_tree.get_root().collapsed = false
		print("Root node expanded")

func _scan_templates_recursive(path: String, parent_item: TreeItem):
	var dir = DirAccess.open(path)
	if not dir:
		print("Failed to open directory: ", path)
		return
	
	print("Scanning directory: ", path)
	
	var directories = []
	var files = []
	
	# 收集目录和文件
	dir.list_dir_begin()
	var current_item = dir.get_next()
	while current_item != "":
		if dir.current_is_dir():
			directories.append(current_item)
			print("Found directory: ", current_item)
		elif current_item.ends_with(".tscn"):
			files.append(current_item)
			print("Found .tscn file: ", current_item)
		current_item = dir.get_next()
	dir.list_dir_end()
	
	print("Found ", files.size(), " .tscn files and ", directories.size(), " directories in ", path)
	
	# 先添加目录
	for dir_name in directories:
		var dir_item = parent_item.create_child()
		dir_item.set_text(0, dir_name)
		dir_item.set_icon(0, get_theme_icon("Folder", "EditorIcons"))
		dir_item.set_metadata(0, {"type": "directory", "path": path + "/" + dir_name})
		dir_item.collapsed = false  # 默认展开子目录
		
		print("Processing subdirectory: ", path + "/" + dir_name)
		_scan_templates_recursive(path + "/" + dir_name, dir_item)
	
	# 再添加文件
	for template_file in files:
		var file_item = parent_item.create_child()
		var display_name = template_file.get_basename()
		file_item.set_text(0, display_name)
		file_item.set_icon(0, get_theme_icon("PackedScene", "EditorIcons"))
		
		var full_path = path + "/" + template_file
		var metadata = {"type": "template", "path": full_path}
		file_item.set_metadata(0, metadata)
		
		print("Added template file: ", display_name, " with path: ", full_path)
		print("Metadata set: ", metadata)
		
		# 检查是否已有schema文件
		var schema_path = path + "/" + template_file.get_basename() + ".schema.tres"
		if FileAccess.file_exists(schema_path):
			file_item.set_custom_color(0, Color.GREEN)
			file_item.set_tooltip_text(0, "Schema exists: " + schema_path)
			print("Schema exists for: ", display_name)
		else:
			file_item.set_custom_color(0, Color.YELLOW)
			file_item.set_tooltip_text(0, "No schema file")
			print("No schema for: ", display_name)

func _update_template_info_panel(template_path: String):
	var info_panel = find_child("TemplateInfoPanel")
	if not info_panel:
		return
	
	_clear_template_info_panel()
	
	# 显示模板基本信息
	var path_label = Label.new()
	path_label.text = "Path: " + template_path
	path_label.autowrap_mode = TextServer.AUTOWRAP_WORD_SMART
	info_panel.add_child(path_label)
	
	# 检查schema文件
	var schema_path = template_path.get_basename() + ".schema.tres"
	var schema_label = Label.new()
	if FileAccess.file_exists(schema_path):
		schema_label.text = "Schema: EXISTS"
		schema_label.add_theme_color_override("font_color", Color.GREEN)
	else:
		schema_label.text = "Schema: NOT FOUND"
		schema_label.add_theme_color_override("font_color", Color.YELLOW)
	info_panel.add_child(schema_label)

func _clear_template_info_panel():
	var info_panel = find_child("TemplateInfoPanel")
	if not info_panel:
		return
	
	for child in info_panel.get_children():
		child.queue_free()
