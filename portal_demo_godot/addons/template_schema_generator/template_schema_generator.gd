# template_schema_generator.gd
# 模板模式生成器 - 学习预设系统的属性获取方法，生成.schema.tres到tscn同级目录

@tool
class_name TemplateSchemaGenerator
extends RefCounted

const TemplateSchemaResource = preload("res://addons/template_schema_generator/template_schema_resource.gd")

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
	
	# 分析ECS组件（学习预设的属性获取方法）
	_analyze_ecs_components(temp_instance, schema)
	
	# 分析原生节点属性（增强版本）
	_analyze_native_properties(temp_instance, schema)
	
	# 生成基础预设
	_generate_basic_presets(schema)
	
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

# 学习预设系统的方法：分析ECS组件属性
func _analyze_ecs_components(node: Node, schema: TemplateSchemaResource):
	var ecs_node = _find_ecs_node(node)
	if not ecs_node:
		print("[TemplateSchemaGenerator] No ECS node found")
		return
	
	print("[TemplateSchemaGenerator] Found ECS node, analyzing components using preset-like method...")
	
	var components = ecs_node.get("components")
	if not components or not (components is Array):
		print("[TemplateSchemaGenerator] No components array found")
		return
	
	for component in components:
		if component is Resource:
			var component_class = component.get_class()
			print("[TemplateSchemaGenerator] Analyzing component: ", component_class)
			
			# 使用预设系统的属性获取方法
			var component_properties = _extract_properties_system(component)
			
			if not component_properties.is_empty():
				schema.add_ecs_component(component_class, component_properties)
				print("[TemplateSchemaGenerator] Added ", component_properties.size(), " properties for component: ", component_class)

# 学习预设系统：提取资源的所有可保存属性（模仿universal_preset_ui.gd的_copy_resource_properties逻辑）
func _extract_properties_system(resource: Resource) -> Dictionary:
	var properties = {}
	
	if not resource:
		return properties
	
	# 获取资源的所有属性（这是预设系统的核心方法）
	var property_list = resource.get_property_list()
	
	for prop in property_list:
		var prop_name = prop.name
		var prop_usage = prop.usage
		
		# 跳过不应该被保存的属性（参考预设系统的逻辑）
		if prop_name == "resource_local_to_scene" or prop_name == "resource_path" or prop_name == "resource_name":
			continue
		
		# 跳过脚本相关属性
		if prop_name == "script":
			continue
			
		# 跳过私有属性
		if prop_name.begins_with("_"):
			continue
		
		# 只保存可序列化的属性（这是预设系统的关键判断）
		if prop_usage & PROPERTY_USAGE_STORAGE:
			var prop_value = resource.get(prop_name)
			
			# 构建属性信息（包含完整的元数据）
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
			print("[TemplateSchemaGenerator] Found property: ", prop_name, " = ", prop_value, " (type: ", property_info.type_name, ")")
	
	return properties

# 分析原生节点属性（增强版本）
func _analyze_native_properties(node: Node, schema: TemplateSchemaResource):
	print("[TemplateSchemaGenerator] Analyzing native properties...")
	
	# 分析根节点
	_analyze_node_properties(node, "root", schema)
	
	# 递归分析子节点
	_analyze_children_recursive(node, schema)

func _analyze_node_properties(node: Node, node_path: String, schema: TemplateSchemaResource):
	var node_class = node.get_class()
	print("[TemplateSchemaGenerator] Analyzing node: ", node_path, " (", node_class, ")")
	
	# 获取节点的重要属性
	var important_props = _get_important_properties_for_node(node)
	print("[TemplateSchemaGenerator] Important properties for ", node_class, ": ", important_props)
	
	for prop_name in important_props:
		# 更安全的属性检查
		if node.has_method("get") and node.has_method("set"):
			# 尝试获取属性值
			var prop_value = null
			
			# 使用异常处理来安全获取属性
			if prop_name == "transform" and node is Node3D:
				prop_value = (node as Node3D).transform
			elif prop_name == "position" and node is Node3D:
				prop_value = (node as Node3D).position
			elif prop_name == "rotation" and node is Node3D:
				prop_value = (node as Node3D).rotation
			elif prop_name == "scale" and node is Node3D:
				prop_value = (node as Node3D).scale
			elif prop_name == "visible":
				prop_value = node.visible if "visible" in node else null
			else:
				# 尝试通用获取
				if prop_name in node:
					prop_value = node.get(prop_name)
			
			if prop_value != null:
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
				print("[TemplateSchemaGenerator] Added native property: ", full_prop_name, " = ", prop_value)
			else:
				print("[TemplateSchemaGenerator] Could not get property: ", prop_name, " from ", node_class)

func _analyze_children_recursive(parent: Node, schema: TemplateSchemaResource):
	for child in parent.get_children():
		# 跳过ECSNode（已经单独处理）
		if not _is_ecs_node(child):
			var child_path = parent.name + "/" + child.name if parent.name != "root" else child.name
			_analyze_node_properties(child, child_path, schema)
			_analyze_children_recursive(child, schema)

func _get_important_properties_for_node(node: Node) -> Array:
	var node_class = node.get_class()
	var base_props = ["position", "rotation", "scale", "visible", "modulate"]
	
	var class_specific = {
		"MeshInstance3D": ["mesh", "cast_shadow", "visibility_range_begin", "visibility_range_end"],
		"DirectionalLight3D": ["light_energy", "light_color", "shadow_enabled"],
		"SpotLight3D": ["light_energy", "light_color", "shadow_enabled", "spot_range", "spot_angle"],
		"Camera3D": ["fov", "near", "far", "projection", "current"],
		"RigidBody3D": ["mass", "friction", "bounce", "gravity_scale"],
		"StaticBody3D": ["friction", "bounce"],
		"CollisionShape3D": ["shape", "disabled"],
		"Area3D": ["monitoring", "monitorable", "collision_layer", "collision_mask"],
		"AudioStreamPlayer3D": ["stream", "volume_db", "pitch_scale", "max_distance"]
	}
	
	var result = base_props.duplicate()
	if class_specific.has(node_class):
		result.append_array(class_specific[node_class])
	
	return result

# 查找ECS节点
func _find_ecs_node(root: Node) -> Node:
	# 检查根节点是否就是ECSNode（多种检测方式）
	if _is_ecs_node(root):
		return root
	
	# 递归查找子节点
	for child in root.get_children():
		if _is_ecs_node(child):
			return child
		
		# 继续递归查找
		var result = _find_ecs_node(child)
		if result:
			return result
	
	return null

# 检测节点是否为ECSNode
func _is_ecs_node(node: Node) -> bool:
	# 检测方式1：类名
	if node.get_class() == "ECSNode":
		return true
	
	# 检测方式2：节点名称包含ECS
	if node.name.contains("ECS"):
		# 进一步检查是否有components属性
		if "components" in node:
			return true
	
	# 检测方式3：脚本路径检测
	var script = node.get_script()
	if script and script.get_path().contains("ecs_node"):
		return true
	
	return false

# 生成基础预设
func _generate_basic_presets(schema: TemplateSchemaResource):
	# 默认预设
	schema.add_preset("Default", {}, {}, "Default template configuration")
	
	# 基于根节点属性生成预设
	if schema.root_node_properties.has("position"):
		schema.add_preset("Elevated", {
			"position": Vector3(0, 5, 0)
		}, {}, "Elevated position")
		
		schema.add_preset("Ground Level", {
			"position": Vector3(0, 0, 0)
		}, {}, "Ground level position")
	
	if schema.root_node_properties.has("scale"):
		schema.add_preset("Large", {
			"scale": Vector3(2, 2, 2)
		}, {}, "Large scale (2x)")
		
		schema.add_preset("Small", {
			"scale": Vector3(0.5, 0.5, 0.5)
		}, {}, "Small scale (0.5x)")
	
	print("[TemplateSchemaGenerator] Generated ", schema.presets.size(), " presets")

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
