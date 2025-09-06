@tool
extends MarginContainer

## 通用预设UI组件
## 
## 这个脚本可以为任何继承自 IPresettableResource 的资源提供预设功能
## 无需为每种资源类型重复编写预设UI代码

@onready var preset_label: Label = $VBoxContainer/HeaderContainer/PresetLabel
@onready var preset_options: OptionButton = $VBoxContainer/HeaderContainer/PresetOptions
@onready var button_container: HBoxContainer = $VBoxContainer/ButtonContainer
@onready var load_button: Button = $VBoxContainer/ButtonContainer/LoadButton
@onready var save_button: Button = $VBoxContainer/ButtonContainer/SaveButton
@onready var delete_button: Button = $VBoxContainer/ButtonContainer/DeleteButton
@onready var reset_button: Button = $VBoxContainer/ButtonContainer/ResetButton
@onready var auto_fill_container: VBoxContainer = $VBoxContainer/AutoFillContainer
@onready var auto_fill_label: Label = $VBoxContainer/AutoFillContainer/AutoFillLabel
@onready var auto_fill_button_container: HBoxContainer = $VBoxContainer/AutoFillContainer/AutoFillButtonContainer
@onready var warning_label: RichTextLabel = $VBoxContainer/WarningContainer/WarningLabel

# 当前处理的资源
var inspected_resource: Resource
var resource_class_name: String
var resource_display_name: String
var preset_dir: String

# 编辑器接口引用 - 用于自动填充功能
var editor_interface: EditorInterface

# 防抖定时器用于约束检查
var constraint_update_timer: Timer

# 预设系统设置
const PRESET_ROOT_DIR = "res://component_presets/"
const PRESET_FILE_EXTENSION = ".tres"

func _ready() -> void:
	# 创建防抖定时器
	constraint_update_timer = Timer.new()
	constraint_update_timer.wait_time = 0.1  # 100ms 防抖延迟
	constraint_update_timer.one_shot = true
	constraint_update_timer.timeout.connect(_update_constraints_delayed)
	add_child(constraint_update_timer)
	
	print("UniversalPresetUI: Initialized successfully")
	
## 设置编辑器接口引用（由插件调用）
func set_editor_interface(p_editor_interface: EditorInterface) -> void:
	editor_interface = p_editor_interface
	
	# 连接所有按钮信号
	if load_button:
		load_button.pressed.connect(_on_load_pressed)
	if save_button:
		save_button.pressed.connect(_on_save_pressed)
	if delete_button:
		delete_button.pressed.connect(_on_delete_pressed)
	if reset_button:
		reset_button.pressed.connect(_on_reset_pressed)
	
	# 监听预设选择变化
	if preset_options:
		preset_options.item_selected.connect(_on_preset_selected)
	
	# 延迟初始化，确保所有组件都已准备好
	call_deferred("_delayed_initialization")

## 延迟初始化，确保组件设置完成后再刷新预设列表
func _delayed_initialization() -> void:
	if inspected_resource and not resource_class_name.is_empty():
		refresh_presets()
		update_constraint_warnings()

## 由 C++ InspectorPlugin 调用，设置当前要处理的资源
func setup_for_resource(resource: Resource, resource_class: String, display_name: String) -> void:
	# 如果之前有资源，断开连接
	if inspected_resource and inspected_resource.changed.is_connected(_on_resource_changed):
		inspected_resource.changed.disconnect(_on_resource_changed)
	
	inspected_resource = resource
	resource_class_name = resource_class
	resource_display_name = display_name
	
	print("Setup preset UI for resource: ", resource, " class: ", resource_class, " display: ", display_name)
	
	# 构建预设目录路径
	preset_dir = PRESET_ROOT_DIR.path_join(resource_class_name) + "/"
	print("Preset directory set to: ", preset_dir)
	
	# 连接资源变化信号以实时更新约束警告
	if inspected_resource and not inspected_resource.changed.is_connected(_on_resource_changed):
		inspected_resource.changed.connect(_on_resource_changed)
		print("Connected to resource changed signal for: ", inspected_resource)
	elif inspected_resource:
		print("Signal already connected for resource: ", inspected_resource)
	else:
		print("Warning: No resource to connect signal to")
	
	# 确保目录存在
	_ensure_preset_directory_exists()
	
	# 更新UI显示
	_update_ui_labels()
	
	# 强制刷新预设列表（确保UI正确显示）
	call_deferred("refresh_presets")
	
	# 检查并显示约束警告
	call_deferred("update_constraint_warnings")
	
	# 设置自动填充功能
	call_deferred("setup_auto_fill_ui")

## 确保预设目录存在
func _ensure_preset_directory_exists() -> void:
	if not DirAccess.dir_exists_absolute(PRESET_ROOT_DIR):
		DirAccess.make_dir_recursive_absolute(PRESET_ROOT_DIR)
	
	if not DirAccess.dir_exists_absolute(preset_dir):
		DirAccess.make_dir_recursive_absolute(preset_dir)

## 更新UI标签显示
func _update_ui_labels() -> void:
	if preset_label:
		preset_label.text = "%s Presets:" % resource_display_name

## 刷新预设列表
func refresh_presets() -> void:
	if not preset_options:
		print("Warning: preset_options is not ready")
		return
	
	if resource_class_name.is_empty():
		print("Warning: resource_class_name is empty, cannot refresh presets")
		return
		
	preset_options.clear()
	preset_options.add_item("< Select Preset >", -1)
	
	print("Refreshing presets from directory: ", preset_dir)
	print("Resource class name: ", resource_class_name)
	
	# 确保预设目录存在
	_ensure_preset_directory_exists()
	
	var dir = DirAccess.open(preset_dir)
	if not dir:
		print("Warning: Cannot access preset directory: ", preset_dir)
		print("Trying to create directory...")
		if DirAccess.make_dir_recursive_absolute(preset_dir) == OK:
			print("Directory created successfully")
			dir = DirAccess.open(preset_dir)
		else:
			print("Failed to create directory")
			return
	
	var preset_files: Array[String] = []
	dir.list_dir_begin()
	var file_name = dir.get_next()
	
	while file_name != "":
		if not dir.current_is_dir() and file_name.ends_with(PRESET_FILE_EXTENSION):
			preset_files.append(file_name.get_basename())
			print("Found preset file: ", file_name)
		file_name = dir.get_next()
	
	dir.list_dir_end()
	
	print("Total preset files found: ", preset_files.size())
	
	# 排序预设名称
	preset_files.sort()
	
	# 添加到选项列表
	for preset_name in preset_files:
		preset_options.add_item(preset_name)
		print("Added preset to UI: ", preset_name)
	
	# 更新按钮状态
	_update_button_states()
	
	print("Preset options count: ", preset_options.get_item_count())

## 更新按钮启用状态
func _update_button_states() -> void:
	var has_presets = preset_options.get_item_count() > 1  # 除了 "< Select Preset >" 之外还有其他项
	var has_selection = preset_options.selected > 0
	
	if load_button:
		load_button.disabled = not has_selection
	if delete_button:
		delete_button.disabled = not has_selection

## 预设选择变化回调
func _on_preset_selected(index: int) -> void:
	_update_button_states()

## 加载预设
func _on_load_pressed() -> void:
	if not inspected_resource or preset_options.selected <= 0:
		return
	
	var preset_name = preset_options.get_item_text(preset_options.selected)
	var preset_path = preset_dir.path_join(preset_name + PRESET_FILE_EXTENSION)
	
	var preset_resource = load(preset_path) as Resource
	if not preset_resource:
		push_error("Failed to load preset: " + preset_path)
		return
	
	# 复制所有属性从预设到当前资源
	_copy_resource_properties(preset_resource, inspected_resource)
	
	# 通知编辑器资源已更改
	inspected_resource.emit_changed()
	if Engine.is_editor_hint():
		inspected_resource.notify_property_list_changed()
	
	# 更新约束警告
	update_constraint_warnings()
	
	print("Loaded preset: ", preset_name)

## 保存预设
func _on_save_pressed() -> void:
	if not inspected_resource:
		return
	
	var dialog = EditorFileDialog.new()
	dialog.access = EditorFileDialog.ACCESS_RESOURCES
	dialog.file_mode = EditorFileDialog.FILE_MODE_SAVE_FILE
	dialog.add_filter("*" + PRESET_FILE_EXTENSION, "Resource File")
	dialog.current_dir = preset_dir
	dialog.current_file = "new_preset" + PRESET_FILE_EXTENSION
	
	# 连接保存确认信号
	dialog.file_selected.connect(_on_save_file_selected)
	
	# 显示对话框
	get_tree().root.add_child(dialog)
	dialog.popup_centered(Vector2i(800, 600))

## 保存文件选择回调
func _on_save_file_selected(path: String) -> void:
	var error = ResourceSaver.save(inspected_resource, path)
	if error == OK:
		print("Preset saved to: ", path)
		refresh_presets()
		
		# 自动选择刚保存的预设
		var preset_name = path.get_file().get_basename()
		for i in range(preset_options.get_item_count()):
			if preset_options.get_item_text(i) == preset_name:
				preset_options.selected = i
				_update_button_states()
				break
	else:
		push_error("Failed to save preset to: " + path + " (Error: " + str(error) + ")")

## 删除预设
func _on_delete_pressed() -> void:
	if preset_options.selected <= 0:
		return
	
	var preset_name = preset_options.get_item_text(preset_options.selected)
	var preset_path = preset_dir.path_join(preset_name + PRESET_FILE_EXTENSION)
	
	# 显示确认对话框
	var confirm_dialog = ConfirmationDialog.new()
	confirm_dialog.dialog_text = "Are you sure you want to delete the preset '%s'?" % preset_name
	confirm_dialog.title = "Delete Preset"
	
	# 连接确认信号
	confirm_dialog.confirmed.connect(_delete_preset_file.bind(preset_path))
	
	# 显示对话框
	get_tree().root.add_child(confirm_dialog)
	confirm_dialog.popup_centered()

## 删除预设文件
func _delete_preset_file(preset_path: String = "") -> void:
	# 如果没有传入路径，从当前选择获取
	if preset_path.is_empty() and preset_options.selected > 0:
		var preset_name = preset_options.get_item_text(preset_options.selected)
		preset_path = preset_dir.path_join(preset_name + PRESET_FILE_EXTENSION)
	
	if preset_path.is_empty():
		return
		
	var error = DirAccess.remove_absolute(preset_path)
	if error == OK:
		print("Deleted preset: ", preset_path)
		refresh_presets()
	else:
		push_error("Failed to delete preset: " + preset_path + " (Error: " + str(error) + ")")

## 重置为默认值
func _on_reset_pressed() -> void:
	if not inspected_resource:
		return
	
	# 显示确认对话框
	var confirm_dialog = ConfirmationDialog.new()
	confirm_dialog.dialog_text = "Are you sure you want to reset all properties to default values?"
	confirm_dialog.title = "Reset to Defaults"
	
	# 连接确认信号
	confirm_dialog.confirmed.connect(_reset_to_defaults)
	
	# 显示对话框
	get_tree().root.add_child(confirm_dialog)
	confirm_dialog.popup_centered()

## 执行重置操作
func _reset_to_defaults() -> void:
	if not inspected_resource:
		return
	
	# 创建一个同类型的新实例来获取默认值
	var script_class = inspected_resource.get_script() as Script
	if script_class:
		var default_instance = script_class.new()
		_copy_resource_properties(default_instance, inspected_resource)
		default_instance.queue_free()
	else:
		# 如果没有脚本，尝试使用类名创建
		var default_instance = ClassDB.instantiate(inspected_resource.get_class())
		if default_instance:
			_copy_resource_properties(default_instance, inspected_resource)
			default_instance.queue_free()
	
	# 通知编辑器资源已更改
	inspected_resource.emit_changed()
	if Engine.is_editor_hint():
		inspected_resource.notify_property_list_changed()
	
	print("Reset ", resource_display_name, " to default values")

## 复制资源属性
## 从源资源复制所有可序列化的属性到目标资源
func _copy_resource_properties(source: Resource, target: Resource) -> void:
	if not source or not target:
		return
	
	# 获取源资源的所有属性
	for prop in source.get_property_list():
		var prop_name = prop.name
		var prop_usage = prop.usage
		
		# 跳过不应该被复制的属性
		if prop_name == "resource_local_to_scene" or prop_name == "resource_path" or prop_name == "resource_name":
			continue
		
		# 只复制可序列化的属性
		if prop_usage & PROPERTY_USAGE_STORAGE:
			var value = source.get(prop_name)
			target.set(prop_name, value)

## 更新约束警告显示
func update_constraint_warnings() -> void:
	if not warning_label or not inspected_resource:
		return
	
	# 检查资源是否有约束验证方法
	if not inspected_resource.has_method("get_constraint_warnings"):
		warning_label.text = ""
		warning_label.visible = false
		return
	
	# 获取警告信息
	var warnings = inspected_resource.call("get_constraint_warnings")
	if warnings.is_empty():
		warning_label.text = ""
		warning_label.visible = false
	else:
		warning_label.text = "[color=orange]" + warnings + "[/color]"
		warning_label.visible = true
		warning_label.custom_minimum_size.y = 60

## 当资源属性改变时调用（可以由外部Inspector触发）
func on_resource_property_changed() -> void:
	update_constraint_warnings()

## 资源变化回调 - 实时更新约束警告
func _on_resource_changed() -> void:
	print("Resource changed signal received!")
	# 使用防抖定时器，避免频繁刷新
	if constraint_update_timer:
		constraint_update_timer.start()
		print("Started constraint update timer")
	else:
		print("Warning: constraint_update_timer is null")

## 延迟更新约束警告，避免频繁刷新
func _update_constraints_delayed() -> void:
	print("Updating constraints (delayed)")
	update_constraint_warnings()

## 清理资源
func _exit_tree() -> void:
	# 断开资源信号连接
	if inspected_resource and inspected_resource.changed.is_connected(_on_resource_changed):
		inspected_resource.changed.disconnect(_on_resource_changed)

# ===== 自动填充功能 =====

## 设置自动填充UI
func setup_auto_fill_ui() -> void:
	if not inspected_resource or not auto_fill_container:
		return
	
	# 清理现有的自动填充按钮
	if auto_fill_button_container:
		for child in auto_fill_button_container.get_children():
			child.queue_free()
	
	# 检查资源是否支持自动填充
	if not inspected_resource.has_method("get_auto_fill_capabilities"):
		auto_fill_container.visible = false
		return
	
	var capabilities = inspected_resource.call("get_auto_fill_capabilities")
	if capabilities.is_empty():
		auto_fill_container.visible = false
		return
	
	# 显示自动填充容器
	auto_fill_container.visible = true
	
	# 创建自动填充按钮
	for cap_dict in capabilities:
		var capability_name = cap_dict.get("capability_name", "")
		var description = cap_dict.get("description", "")
		var source_node_type = cap_dict.get("source_node_type", "")
		
		var button = Button.new()
		button.text = capability_name
		button.tooltip_text = "%s\nSource: %s\n%s" % [capability_name, source_node_type, description]
		button.pressed.connect(_on_auto_fill_button_pressed.bind(capability_name))
		
		auto_fill_button_container.add_child(button)

## 自动填充按钮点击处理
func _on_auto_fill_button_pressed(capability_name: String) -> void:
	if not inspected_resource:
		_show_auto_fill_error("No resource selected")
		return
	
	# 获取目标节点路径
	var target_node = _get_target_node()
	if not target_node:
		_show_auto_fill_error("No target node found. Make sure the resource has a valid target_node_path")
		return
	
	# 检查节点是否支持此能力
	if not inspected_resource.has_method("can_auto_fill_from_node"):
		_show_auto_fill_error("Resource does not support auto-fill functionality")
		return
	
	var can_fill = inspected_resource.call("can_auto_fill_from_node", target_node, capability_name)
	if not can_fill:
		_show_auto_fill_error("Target node '%s' does not support capability '%s'" % [target_node.get_class(), capability_name])
		return
	
	# 执行自动填充
	if not inspected_resource.has_method("auto_fill_from_node"):
		_show_auto_fill_error("Resource does not implement auto_fill_from_node method")
		return
	
	var result_dict = inspected_resource.call("auto_fill_from_node", target_node, capability_name)
	
	# 处理结果
	if result_dict.get("success", false):
		var applied_capability = result_dict.get("applied_capability", capability_name)
		var property_count = result_dict.get("property_values", {}).size()
		_show_auto_fill_success("Successfully applied '%s' (%d properties updated)" % [applied_capability, property_count])
		
		# 刷新约束检查
		update_constraint_warnings()
	else:
		var error_message = result_dict.get("error_message", "Unknown error")
		_show_auto_fill_error("Auto-fill failed: " + error_message)

## 获取目标节点
func _get_target_node() -> Node:
	# 从资源获取目标节点路径
	if not inspected_resource.has_method("get") or not inspected_resource.has_method("get_property_list"):
		return null
	
	# 查找 target_node_path 属性
	var property_list = inspected_resource.call("get_property_list")
	var has_target_path = false
	
	for prop in property_list:
		if prop.get("name", "") == "target_node_path":
			has_target_path = true
			break
	
	var target_path = ""
	if has_target_path:
		target_path = inspected_resource.call("get", "target_node_path")
	else:
		print("Resource does not have target_node_path property, will try to use selected node")
	
	# 使用EditorInterface获取当前编辑的场景
	if not editor_interface:
		print("EditorInterface not available for auto-fill")
		return null
	
	var current_scene = editor_interface.get_edited_scene_root()
	if not current_scene:
		print("No current scene being edited")
		return null
	
	var target_node: Node = null
	
	# 如果指定了target_node_path，优先使用它
	if target_path and not target_path.is_empty():
		target_node = current_scene.get_node_or_null(target_path)
		if target_node:
			print("Found target node at specified path: ", target_node.name, " (", target_node.get_class(), ")")
			return target_node
		else:
			print("Target node not found at path: ", target_path, ", trying to find a suitable fallback...")
	
	# 如果没有指定target_node_path或指定的路径无效，尝试智能查找
	# 在编辑器中，使用当前选中的节点作为目标
	var editor_selection = editor_interface.get_selection()
	var selected_nodes = editor_selection.get_selected_nodes()
	
	if selected_nodes.size() > 0:
		var selected_node = selected_nodes[0]
		
		# 如果选中的是ECSNode，使用其父节点作为目标（符合ECSNode的设计逻辑）
		if selected_node.get_class() == "ECSNode":
			var parent_node = selected_node.get_parent()
			if parent_node:
				print("Selected ECSNode, using parent as target: ", parent_node.name, " (", parent_node.get_class(), ")")
				return parent_node
			else:
				print("Selected ECSNode has no parent")
				return null
		else:
			# 使用当前选中的节点作为目标
			print("Using selected node as target: ", selected_node.name, " (", selected_node.get_class(), ")")
			return selected_node
	
	print("No suitable target node found - please specify target_node_path or select a node in the scene")
	return null

## 显示自动填充成功消息
func _show_auto_fill_success(message: String) -> void:
	print("Auto-fill success: ", message)
	# 可以在这里添加更多的UI反馈，比如临时显示成功提示

## 显示自动填充错误消息
func _show_auto_fill_error(message: String) -> void:
	print("Auto-fill error: ", message)
	# 可以在这里添加更多的UI反馈，比如弹出错误对话框
	
	# 清理定时器
	if constraint_update_timer:
		constraint_update_timer.queue_free()
