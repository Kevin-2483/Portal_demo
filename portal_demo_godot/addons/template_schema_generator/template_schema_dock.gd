# template_schema_dock.gd
# 模板模式生成器的停靠面板

@tool
extends Control

const TemplateSchemaGenerator = preload("res://addons/template_schema_generator/template_schema_generator.gd")
const TemplateSchemaResource = preload("res://addons/template_schema_generator/template_schema_resource.gd")

var file_tree: Tree
var info_panel: VBoxContainer
var generate_button: Button
var refresh_button: Button
var selected_template_path: String = ""

const TEMPLATES_PATH = "res://templates"

func _init():
	name = "Template Schema"
	setup_ui()
	refresh_template_list()

func setup_ui():
	set_custom_minimum_size(Vector2(300, 400))
	
	var main_vbox = VBoxContainer.new()
	add_child(main_vbox)
	
	# 标题
	var title_label = Label.new()
	title_label.text = "Template Schema Generator"
	title_label.add_theme_font_size_override("font_size", 14)
	main_vbox.add_child(title_label)
	
	# 按钮容器
	var button_hbox = HBoxContainer.new()
	main_vbox.add_child(button_hbox)
	
	# 刷新按钮
	refresh_button = Button.new()
	refresh_button.text = "Refresh"
	refresh_button.pressed.connect(_on_refresh_pressed)
	button_hbox.add_child(refresh_button)
	
	# 生成按钮
	generate_button = Button.new()
	generate_button.text = "Generate Schema"
	generate_button.pressed.connect(_on_generate_pressed)
	generate_button.disabled = true
	button_hbox.add_child(generate_button)
	
	# 分隔线
	var separator = HSeparator.new()
	main_vbox.add_child(separator)
	
	# 文件树
	var tree_label = Label.new()
	tree_label.text = "Template Files:"
	main_vbox.add_child(tree_label)
	
	file_tree = Tree.new()
	file_tree.set_custom_minimum_size(Vector2(280, 200))
	file_tree.item_selected.connect(_on_tree_item_selected)
	main_vbox.add_child(file_tree)
	
	# 信息面板
	var info_label = Label.new()
	info_label.text = "Template Info:"
	main_vbox.add_child(info_label)
	
	info_panel = VBoxContainer.new()
	var scroll = ScrollContainer.new()
	scroll.set_custom_minimum_size(Vector2(280, 150))
	scroll.add_child(info_panel)
	main_vbox.add_child(scroll)

func refresh_template_list():
	file_tree.clear()
	var root = file_tree.create_item()
	root.set_text(0, "templates")
	root.set_icon(0, get_theme_icon("Folder", "EditorIcons"))
	
	_scan_templates_recursive(TEMPLATES_PATH, root)

func _scan_templates_recursive(path: String, parent_item: TreeItem):
	var dir = DirAccess.open(path)
	if not dir:
		return
	
	var directories = []
	var files = []
	
	# 收集目录和文件
	for item in dir.get_files():
		if item.ends_with(".tscn"):
			files.append(item)
	
	for item in dir.get_directories():
		directories.append(item)
	
	# 先添加目录
	for dir_name in directories:
		var dir_item = parent_item.create_child()
		dir_item.set_text(0, dir_name)
		dir_item.set_icon(0, get_theme_icon("Folder", "EditorIcons"))
		dir_item.set_metadata(0, {"type": "directory", "path": path + "/" + dir_name})
		
		_scan_templates_recursive(path + "/" + dir_name, dir_item)
	
	# 再添加文件
	for file_name in files:
		var file_item = parent_item.create_child()
		var display_name = file_name.get_basename()
		file_item.set_text(0, display_name)
		file_item.set_icon(0, get_theme_icon("PackedScene", "EditorIcons"))
		
		var full_path = path + "/" + file_name
		file_item.set_metadata(0, {"type": "template", "path": full_path})
		
		# 检查是否已有schema文件
		var schema_path = path + "/" + file_name.get_basename() + ".schema.tres"
		if FileAccess.file_exists(schema_path):
			file_item.set_custom_color(0, Color.GREEN)
			file_item.set_tooltip_text(0, "Schema exists: " + schema_path)
		else:
			file_item.set_custom_color(0, Color.YELLOW)
			file_item.set_tooltip_text(0, "No schema file")

func _on_refresh_pressed():
	refresh_template_list()
	_clear_info_panel()
	generate_button.disabled = true
	selected_template_path = ""

func _on_tree_item_selected():
	var selected_item = file_tree.get_selected()
	if not selected_item:
		return
	
	var metadata = selected_item.get_metadata(0)
	if metadata.type == "template":
		selected_template_path = metadata.path
		generate_button.disabled = false
		_update_info_panel(metadata.path)
	else:
		generate_button.disabled = true
		selected_template_path = ""
		_clear_info_panel()

func _update_info_panel(template_path: String):
	_clear_info_panel()
	
	# 显示模板信息
	var path_label = Label.new()
	path_label.text = "Path: " + template_path
	path_label.autowrap_mode = TextServer.AUTOWRAP_WORD_SMART
	info_panel.add_child(path_label)
	
	# 检查schema文件状态
	var schema_path = template_path.get_basename() + ".schema.tres"
	var schema_status = Label.new()
	if FileAccess.file_exists(schema_path):
		schema_status.text = "✓ Schema exists"
		schema_status.modulate = Color.GREEN
		
		# 显示schema信息
		var schema = load(schema_path) as TemplateSchemaResource
		if schema:
			var info_label = Label.new()
			info_label.text = "Properties: " + str(schema.get_all_property_names().size())
			info_panel.add_child(info_label)
			
			var presets_label = Label.new()
			presets_label.text = "Presets: " + str(schema.presets.size())
			info_panel.add_child(presets_label)
			
			if not schema.description.is_empty():
				var desc_label = Label.new()
				desc_label.text = "Description: " + schema.description
				desc_label.autowrap_mode = TextServer.AUTOWRAP_WORD_SMART
				info_panel.add_child(desc_label)
	else:
		schema_status.text = "⚠ No schema file"
		schema_status.modulate = Color.YELLOW
	
	info_panel.add_child(schema_status)

func _clear_info_panel():
	for child in info_panel.get_children():
		child.queue_free()

func _on_generate_pressed():
	if selected_template_path.is_empty():
		return
	
	var generator = TemplateSchemaGenerator.new()
	var success = generator.generate_schema(selected_template_path)
	
	if success:
		print("[TemplateSchemaGenerator] Schema generated for: ", selected_template_path)
		refresh_template_list()
		_update_info_panel(selected_template_path)
	else:
		print("[TemplateSchemaGenerator] Failed to generate schema for: ", selected_template_path)
