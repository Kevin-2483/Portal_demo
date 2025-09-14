# plugin.gd
@tool
extends EditorPlugin

var dock_instance

func _enter_tree():
	# 添加自定义停靠面板
	dock_instance = preload("res://addons/template_schema_generator/template_schema_dock.gd").new()
	add_control_to_dock(DOCK_SLOT_LEFT_BL, dock_instance)
	print("[TemplateSchemaGenerator] Plugin enabled")

func _exit_tree():
	# 移除停靠面板
	if dock_instance:
		remove_control_from_docks(dock_instance)
		dock_instance = null
	print("[TemplateSchemaGenerator] Plugin disabled")
