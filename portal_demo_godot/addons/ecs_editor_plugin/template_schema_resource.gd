# template_schema_resource.gd
# 模板模式资源 - 存储模板的属性信息和预设值

@tool
class_name TemplateSchemaResource
extends Resource

# 模板基本信息
@export var template_path: String = ""
@export var template_name: String = ""
@export var description: String = ""
@export var version: String = "1.0"
@export var created_time: String = ""

# 根节点属性信息
@export var root_node_properties: Dictionary = {}

# ECS组件属性信息  
@export var ecs_component_properties: Dictionary = {}

# 预设配置组合
@export var presets: Array[Dictionary] = []

# 标签和分类
@export var tags: Array[String] = []
@export var category: String = ""

func _init():
	created_time = Time.get_datetime_string_from_system()

# 添加根节点属性
func add_root_property(property_name: String, property_info: Dictionary):
	root_node_properties[property_name] = property_info

# 添加ECS组件属性
func add_ecs_component(component_class: String, properties: Dictionary):
	ecs_component_properties[component_class] = properties

# 添加预设配置
func add_preset(preset_name: String, root_overrides: Dictionary = {}, component_overrides: Dictionary = {}, description: String = ""):
	var preset = {
		"name": preset_name,
		"description": description,
		"root_overrides": root_overrides,
		"component_overrides": component_overrides,
		"created_time": Time.get_datetime_string_from_system()
	}
	presets.append(preset)

# 获取所有可覆写的属性名称
func get_all_property_names() -> Array[String]:
	var names: Array[String] = []
	names.append_array(root_node_properties.keys())
	
	for component_class in ecs_component_properties.keys():
		var component_props = ecs_component_properties[component_class]
		for prop_name in component_props.keys():
			names.append(component_class + "." + prop_name)
	
	return names

# 获取属性信息
func get_property_info(property_path: String) -> Dictionary:
	if "." in property_path:
		# ECS组件属性
		var parts = property_path.split(".", false, 1)
		var component_class = parts[0]
		var prop_name = parts[1]
		
		if ecs_component_properties.has(component_class):
			var component_props = ecs_component_properties[component_class]
			if component_props.has(prop_name):
				return component_props[prop_name]
	else:
		# 根节点属性
		if root_node_properties.has(property_path):
			return root_node_properties[property_path]
	
	return {}

# 验证预设配置
func validate_preset(preset: Dictionary) -> Array[String]:
	var errors: Array[String] = []
	
	# 验证根节点覆写
	for prop_name in preset.get("root_overrides", {}).keys():
		if not root_node_properties.has(prop_name):
			errors.append("Unknown root property: " + prop_name)
	
	# 验证组件覆写
	for component_class in preset.get("component_overrides", {}).keys():
		if not ecs_component_properties.has(component_class):
			errors.append("Unknown component: " + component_class)
			continue
		
		var component_overrides = preset.component_overrides[component_class]
		var component_props = ecs_component_properties[component_class]
		
		for prop_name in component_overrides.keys():
			if not component_props.has(prop_name):
				errors.append("Unknown property in " + component_class + ": " + prop_name)
	
	return errors
