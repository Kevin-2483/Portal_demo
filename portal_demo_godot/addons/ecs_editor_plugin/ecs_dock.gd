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
	
	# 暂停/恢复按钮
	pause_button = Button.new()
	pause_button.name = "PauseButton"
	pause_button.text = "Pause ECS"
	pause_button.pressed.connect(_on_pause_pressed)
	pause_button.disabled = true
	button_container.add_child(pause_button)
	
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
	
	# 新增：同步场景到ECS按钮
	reload_to_ecs_button = Button.new()
	reload_to_ecs_button.text = "Reload Scene to ECS"
	reload_to_ecs_button.tooltip_text = "Force reload current editor scene state to ECS entities"
	reload_to_ecs_button.pressed.connect(_on_reload_scene_to_ecs_pressed)
	reload_to_ecs_button.disabled = true  # 初始状态为禁用
	button_container.add_child(reload_to_ecs_button)
	
	# Scene Management 分组
	var scene_group = VBoxContainer.new()
	vbox.add_child(scene_group)
	
	var scene_label = Label.new()
	scene_label.text = "Scene Management"
	scene_label.add_theme_font_size_override("font_size", 12)
	scene_group.add_child(scene_label)
	
	# 重新加载场景按钮
	var reload_scene_button = Button.new()
	reload_scene_button.text = "Reload Scene"
	reload_scene_button.tooltip_text = "Reload current scene from file"
	reload_scene_button.pressed.connect(_on_reload_scene_pressed)
	scene_group.add_child(reload_scene_button)
	
	# Snapshot 管理容器
	var snapshot_container = VBoxContainer.new()
	scene_group.add_child(snapshot_container)
	
	# Snapshot 标题和状态
	var snapshot_header = HBoxContainer.new()
	snapshot_container.add_child(snapshot_header)
	
	var snapshot_title = Label.new()
	snapshot_title.text = "Snapshots"
	snapshot_title.add_theme_font_size_override("font_size", 11)
	snapshot_header.add_child(snapshot_title)
	
	snapshot_status_label = Label.new()
	snapshot_status_label.name = "SnapshotStatus"
	snapshot_status_label.text = "(0)"
	snapshot_status_label.add_theme_color_override("font_color", Color.GRAY)
	snapshot_header.add_child(snapshot_status_label)
	
	# 快照按钮
	var snapshot_buttons = HBoxContainer.new()
	snapshot_container.add_child(snapshot_buttons)
	
	var create_snapshot_button = Button.new()
	create_snapshot_button.text = "Create"
	create_snapshot_button.tooltip_text = "Create new snapshot"
	create_snapshot_button.pressed.connect(_on_create_snapshot_pressed)
	snapshot_buttons.add_child(create_snapshot_button)
	
	var clear_all_button = Button.new()
	clear_all_button.text = "Clear All"
	clear_all_button.tooltip_text = "Delete all snapshots"
	clear_all_button.pressed.connect(_on_clear_all_snapshots_pressed)
	snapshot_buttons.add_child(clear_all_button)
	
	# 快照列表容器
	var snapshots_scroll = ScrollContainer.new()
	snapshots_scroll.set_custom_minimum_size(Vector2(0, 150))
	snapshot_container.add_child(snapshots_scroll)
	
	snapshots_list = VBoxContainer.new()
	snapshots_list.name = "SnapshotsList"
	snapshots_scroll.add_child(snapshots_list)
	
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
