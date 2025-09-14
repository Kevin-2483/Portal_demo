# template_schema_generator.gd
# 模板模式生成器 - 专注于分析并记录TSCN中的所有可编辑属性

@tool
class_name TemplateSchemaGenerator
extends RefCounted

const TemplateSchemaResource = preload("res://addons/ecs_editor_plugin/template_schema_resource.gd")

# 生成模板的schema文件
func generate_schema(template_path: String) -> bool:
	print("[TemplateSchemaGenerator] Analyzing template: ", template_path)
	
	# 加载模板文件
	var template_scene = load(template_path) as PackedScene
	if not template_scene:
		printerr("[TemplateSchemaGenerator] Failed to load template: ", template_path)
		return false
	
	# 创建临时实例进行分析
	var temp_instance = template_scene.instantiate()
	if not temp_instance:
		printerr("[TemplateSchemaGenerator] Failed to instantiate template: ", template_path)
		return false
	
	# 创建schema资源
	var schema = TemplateSchemaResource.new()
	schema.template_path = template_path
	schema.template_name = template_path.get_file().get_basename()
	schema.description = "Auto-generated schema for " + schema.template_name
	
	# 分析ECS组件
	_analyze_ecs_components(temp_instance, schema)
	
	# 分析原生节点属性（已升级为完全分析）
	_analyze_native_properties(temp_instance, schema)
	
	# -- 预设生成逻辑已被移除 --
	# _generate_basic_presets(schema)
	
	# 清理临时实例
	temp_instance.queue_free()
	
	# 保存schema文件到tscn同级目录
	var schema_path = template_path.get_basename() + ".schema.tres"
	var result = ResourceSaver.save(schema, schema_path)
	
	if result == OK:
		print("[TemplateSchemaGenerator] Schema saved to: ", schema_path)
		return true
	else:
		printerr("[TemplateSchemaGenerator] Failed to save schema to: ", schema_path)
		return false

# 分析ECS组件属性
func _analyze_ecs_components(node: Node, schema: TemplateSchemaResource):
	var ecs_node = _find_ecs_node(node)
	if not ecs_node:
		print("[TemplateSchemaGenerator] No ECS node found")
		return
	
	print("[TemplateSchemaGenerator] Found ECS node, analyzing components...")
	
	var components = ecs_node.get("components")
	if not components or not (components is Array):
		print("[TemplateSchemaGenerator] No components array found")
		return
	
	for component in components:
		if component is Resource:
			var component_class = component.get_class()
			print("[TemplateSchemaGenerator] Analyzing component: ", component_class)
			
			var component_properties = _extract_properties_from_resource(component)
			
			if not component_properties.is_empty():
				schema.add_ecs_component(component_class, component_properties)
				print("[TemplateSchemaGenerator] Added ", component_properties.size(), " properties for component: ", component_class)

# 通用方法：从一个资源(Resource)中提取所有可存储的属性
func _extract_properties_from_resource(resource: Resource) -> Dictionary:
	var properties = {}
	if not resource:
		return properties
	
	var property_list = resource.get_property_list()
	
	for prop in property_list:
		var prop_name = prop.name
		var prop_usage = prop.usage
		
		# 跳过不应该被保存的属性
		if prop_name in ["resource_local_to_scene", "resource_path", "resource_name", "script"]:
			continue
		
		# 跳过私有属性
		if prop_name.begins_with("_"):
			continue
		
		# 只保存可序列化的属性
		if prop_usage & PROPERTY_USAGE_STORAGE:
			var prop_value = resource.get(prop_name)
			
			var property_info = {
				"type": prop.type,
				"type_name": _get_type_name(prop.type),
				"default_value": prop_value,
				"usage": prop_usage,
				"hint": prop.hint,
				"hint_string": prop.hint_string,
				"description": "Auto-detected from component"
			}
			
			properties[prop_name] = property_info
	
	return properties

# 分析原生节点属性（已升级为完全分析）
func _analyze_native_properties(node: Node, schema: TemplateSchemaResource):
	print("[TemplateSchemaGenerator] Analyzing native properties...")
	# 分析根节点
	_analyze_node_properties(node, "root", schema)
	# 递归分析子节点
	_analyze_children_recursive(node, schema)

# 新的核心方法：分析单个节点的所有可编辑属性
func _analyze_node_properties(node: Node, node_path: String, schema: TemplateSchemaResource):
	var node_class = node.get_class()
	print("[TemplateSchemaGenerator] Analyzing node: ", node_path, " (", node_class, ")")
	
	var property_list = node.get_property_list()
	
	for prop in property_list:
		var prop_name = prop.name
		var prop_usage = prop.usage
		
		# 跳过不应该被保存的属性
		if prop_name in ["script", "owner"]:
			continue
			
		# 跳过私有属性
		if prop_name.begins_with("_"):
			continue
		
		# 只保存可序列化的属性，并且不是分类(GROUP)或子类(SUBGROUP)的标题
		if (prop_usage & PROPERTY_USAGE_STORAGE) and not (prop_usage & PROPERTY_USAGE_GROUP or prop_usage & PROPERTY_USAGE_SUBGROUP):
			var prop_value = node.get(prop_name)
			var full_prop_name = node_path + "." + prop_name if node_path != "root" else prop_name
			
			var property_info = {
				"type": typeof(prop_value),
				"type_name": _get_type_name(typeof(prop_value)),
				"default_value": prop_value,
				"description": "Native property of " + node_class,
				"node_class": node_class,
				"node_path": node_path,
				"category": "Native"
			}
			
			schema.add_root_property(full_prop_name, property_info)
			# print("[TemplateSchemaGenerator] Added native property: ", full_prop_name, " = ", prop_value) # 可以取消注释以获得更详细的日志

func _analyze_children_recursive(parent: Node, schema: TemplateSchemaResource):
	for child in parent.get_children():
		# 跳过ECSNode（已经单独处理）
		if not _is_ecs_node(child):
			var child_path = parent.name + "/" + child.name if parent.name != "root" else child.name
			_analyze_node_properties(child, child_path, schema)
			_analyze_children_recursive(child, schema)

# 查找ECS节点
func _find_ecs_node(root: Node) -> Node:
	if _is_ecs_node(root):
		return root
	for child in root.get_children():
		if _is_ecs_node(child):
			return child
		var result = _find_ecs_node(child)
		if result:
			return result
	return null

# 检测节点是否为ECSNode
func _is_ecs_node(node: Node) -> bool:
	if node.get_class() == "ECSNode":
		return true
	if "components" in node:
		return true
	var script = node.get_script()
	if script and script.get_path().contains("ecs_node"):
		return true
	return false

# 辅助函数
func _get_type_name(type: int) -> String:
	match type:
		TYPE_BOOL: return "bool"
		TYPE_INT: return "int"
		TYPE_FLOAT: return "float"
		TYPE_STRING: return "String"
		TYPE_VECTOR2: return "Vector2"
		TYPE_VECTOR3: return "Vector3"
		TYPE_TRANSFORM2D: return "Transform2D"
		TYPE_TRANSFORM3D: return "Transform3D"
		TYPE_QUATERNION: return "Quaternion"
		TYPE_COLOR: return "Color"
		TYPE_OBJECT: return "Object"
		TYPE_ARRAY: return "Array"
		TYPE_DICTIONARY: return "Dictionary"
		_: return "Variant"