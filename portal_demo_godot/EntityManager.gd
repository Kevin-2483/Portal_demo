# EntityManager.gd
# 游戏对象生成系统 - 全局单例
# 负责管理所有实体模板并提供动态生成和属性覆写功能

extends Node

# 存储所有加载的模板
var templates: Dictionary = {}

# 存储模板的可覆写属性信息
# Key: 模板路径, Value: Dictionary包含属性信息
var template_properties: Dictionary = {}

# 模板扫描路径
const TEMPLATES_PATH = "res://templates"

# 活跃实体的追踪字典，性能优于数组
# Key: 实体实例 (Node), Value: 模板路径 (String)
var active_entities: Dictionary = {}

# 初始化信号
signal templates_loaded
signal entity_spawned(entity: Node, template_name: String)
signal entity_destroyed(entity: Node, template_name: String)

func _ready():
	print("[EntityManager] Initializing...")
	load_all_templates()
	print("[EntityManager] Ready - ", templates.size(), " templates loaded")

# 扫描并加载所有模板
func load_all_templates():
	templates.clear()
	template_properties.clear()
	_scan_directory_recursive(TEMPLATES_PATH)
	_analyze_all_templates()
	templates_loaded.emit()
	print("[EntityManager] Templates loaded: ", templates.keys())

# 递归扫描目录
func _scan_directory_recursive(path: String, prefix: String = ""):
	var dir = DirAccess.open(path)
	if dir == null:
		print("[EntityManager] Warning: Cannot access directory: ", path)
		return
	
	dir.list_dir_begin()
	var file_name = dir.get_next()
	
	while file_name != "":
		var full_path = path + "/" + file_name
		
		if dir.current_is_dir():
			# 递归扫描子目录
			var sub_prefix = prefix + file_name + "/"
			_scan_directory_recursive(full_path, sub_prefix)
		elif file_name.ends_with(".tscn"):
			# 加载.tscn文件作为PackedScene
			var scene_resource = load(full_path) as PackedScene
			if scene_resource:
				var template_name = prefix + file_name.get_basename()
				templates[template_name] = scene_resource
				print("[EntityManager] Loaded template: ", template_name, " -> ", full_path)
			else:
				print("[EntityManager] Warning: Failed to load ", full_path)
		
		file_name = dir.get_next()

# 获取可用模板列表
func get_available_templates() -> Array[String]:
	var template_names: Array[String] = []
	for key in templates.keys():
		template_names.append(key)
	return template_names

# 检查模板是否存在
func has_template(template_name: String) -> bool:
	return templates.has(template_name)

# 主要功能：生成实体
# template_name: 模板名称
# parent: 父节点（如果为null，则添加到场景树根节点）
# overrides: 属性覆写字典
func spawn_entity(template_name: String, parent: Node = null, overrides: Dictionary = {}) -> Node:
	if not templates.has(template_name):
		print("[EntityManager] Error: Template '", template_name, "' not found!")
		return null
	
	# 实例化场景
	var template: PackedScene = templates[template_name]
	var instance = template.instantiate()
	
	if instance == null:
		print("[EntityManager] Error: Failed to instantiate template '", template_name, "'")
		return null
	
	# 应用属性覆写
	if not overrides.is_empty():
		_apply_overrides(instance, overrides)
	
	# 添加到场景树
	if parent == null:
		parent = get_tree().current_scene
	
	parent.add_child(instance)
	
	# 记录活跃实体到字典中
	active_entities[instance] = template_name
	
	# 连接销毁信号以便清理
	if instance.has_signal("tree_exited"):
		instance.tree_exited.connect(_on_entity_destroyed.bind(instance, template_name))
	
	print("[EntityManager] Spawned entity '", template_name, "' at ", instance.global_position if instance.has_method("get_global_position") else "N/A")
	entity_spawned.emit(instance, template_name)
	
	return instance

# 应用属性覆写 - Schema 驱动版本，支持路径属性
func _apply_overrides(instance: Node, overrides: Dictionary):
	print("[EntityManager] Applying overrides: ", overrides)
	
	for property_path in overrides.keys():
		var value = overrides[property_path]
		_set_property_by_path(instance, property_path, value)

# Schema 驱动的属性设置 - 支持路径形式的属性（如 "MeshInstance3D.material_override"）
func _set_property_by_path(root_node: Node, property_path: String, value):
	var path_parts = property_path.split(".")
	
	# 如果是简单属性（没有节点路径前缀）
	if path_parts.size() == 1:
		_set_simple_property(root_node, property_path, value)
		return
	
	# 处理复合路径属性（如 "MeshInstance3D.material_override"）
	var node_type = path_parts[0]
	var property_name = path_parts[1]
	
	# 递归查找指定类型的节点
	var target_node = _find_node_by_type(root_node, node_type)
	if target_node:
		_set_simple_property(target_node, property_name, value)
	else:
		print("[EntityManager] Warning: Node type '", node_type, "' not found for property '", property_path, "'")

# 递归查找指定类型的节点
func _find_node_by_type(root: Node, node_type: String) -> Node:
	if root.get_class() == node_type:
		return root
	
	for child in root.get_children():
		var result = _find_node_by_type(child, node_type)
		if result:
			return result
	
	return null

# 简化的属性设置方法 - 支持常用的Node属性
func _set_simple_property(node: Node, property_name: String, value):
	# 处理常见的Node属性
	match property_name:
		"position":
			if node.has_method("set_position"):
				node.set_position(value)
				print("[EntityManager] Set position = ", value)
			else:
				print("[EntityManager] Warning: Node doesn't have position property")
		"rotation":
			if node.has_method("set_rotation"):
				node.set_rotation(value)
				print("[EntityManager] Set rotation = ", value)
			else:
				print("[EntityManager] Warning: Node doesn't have rotation property")
		"scale":
			if node.has_method("set_scale"):
				node.set_scale(value)
				print("[EntityManager] Set scale = ", value)
			else:
				print("[EntityManager] Warning: Node doesn't have scale property")
		"global_position":
			if node.has_method("set_global_position"):
				node.set_global_position(value)
				print("[EntityManager] Set global_position = ", value)
			else:
				print("[EntityManager] Warning: Node doesn't have global_position property")
		_:
			# 尝试直接属性设置
			if property_name in node:
				node[property_name] = value
				print("[EntityManager] Set property '", property_name, "' = ", value)
			elif node.has_method("set_" + property_name):
				node.call("set_" + property_name, value)
				print("[EntityManager] Called setter for '", property_name, "' = ", value)
			else:
				print("[EntityManager] Warning: Property '", property_name, "' not found or not settable in ", node.get_class())

# 辅助方法：安全销毁实体
func destroy_entity(entity: Node):
	if is_instance_valid(entity) and active_entities.has(entity):
		entity.queue_free()
		print("[EntityManager] Requested destruction of tracked entity")
	else:
		print("[EntityManager] Warning: Attempted to destroy untracked or invalid entity")

# 特殊方法：深度覆写ECS组件资源
func spawn_entity_with_ecs_override(template_name: String, parent: Node = null, component_overrides: Dictionary = {}, general_overrides: Dictionary = {}) -> Node:
	if not templates.has(template_name):
		print("[EntityManager] Error: Template '", template_name, "' not found!")
		return null
	
	var template: PackedScene = templates[template_name]
	var instance = template.instantiate()
	
	if instance == null:
		print("[EntityManager] Error: Failed to instantiate template '", template_name, "'")
		return null
	
	# 查找ECSNode并处理组件覆写
	var ecs_node = _find_ecs_node(instance)
	if ecs_node and not component_overrides.is_empty():
		_apply_ecs_component_overrides(ecs_node, component_overrides)
	
	# 应用常规属性覆写
	if not general_overrides.is_empty():
		_apply_overrides(instance, general_overrides)
	
	# 添加到场景树
	if parent == null:
		parent = get_tree().current_scene
	
	parent.add_child(instance)
	active_entities[instance] = template_name
	
	if instance.has_signal("tree_exited"):
		instance.tree_exited.connect(_on_entity_destroyed.bind(instance, template_name))
	
	print("[EntityManager] Spawned ECS entity '", template_name, "'")
	entity_spawned.emit(instance, template_name)
	
	return instance

# 查找ECSNode
func _find_ecs_node(root: Node) -> Node:
	if root.get_class() == "ECSNode":
		return root
	
	for child in root.get_children():
		var result = _find_ecs_node(child)
		if result:
			return result
	
	return null

# 应用ECS组件覆写 - 优化版本，只在需要时创建副本
func _apply_ecs_component_overrides(ecs_node: Node, component_overrides: Dictionary):
	var components = ecs_node.get("components")
	if components == null or not (components is Array):
		print("[EntityManager] Warning: ECSNode has no components array")
		return
	
	var new_components: Array[Resource] = []
	var was_modified = false
	
	for i in range(components.size()):
		var component = components[i]
		if component is Resource:
			var component_class = component.get_class()
			
			# 检查是否有针对此组件类型的覆写
			if component_overrides.has(component_class):
				# 只有需要修改时才创建副本
				var component_copy = component.duplicate()
				var overrides = component_overrides[component_class]
				var component_was_modified = false
				
				for property_name in overrides.keys():
					var value = overrides[property_name]
					if property_name in component_copy:
						component_copy[property_name] = value
						component_was_modified = true
						print("[EntityManager] Override ", component_class, ".", property_name, " = ", value)
					else:
						print("[EntityManager] Warning: Property '", property_name, "' not found in ", component_class)
				
				if component_was_modified:
					new_components.append(component_copy)
					was_modified = true
				else:
					# 如果实际上没有修改任何属性，使用原始组件
					new_components.append(component)
			else:
				# 没有针对此组件的覆写，使用原始组件
				new_components.append(component)
		else:
			new_components.append(component)
	
	# 只有在实际发生修改时才更新ECSNode的组件数组
	if was_modified:
		ecs_node.set("components", new_components)
		print("[EntityManager] ECS components updated with overrides")
	else:
		print("[EntityManager] No ECS component modifications applied")

# 实体销毁回调
func _on_entity_destroyed(entity: Node, template_name: String):
	if active_entities.has(entity):
		active_entities.erase(entity)
		entity_destroyed.emit(entity, template_name)
		print("[EntityManager] Entity '", template_name, "' destroyed")
	else:
		print("[EntityManager] Warning: Attempted to remove untracked entity")

# 清理所有活跃实体
func clear_all_entities():
	for entity in active_entities.keys():  # 迭代Dictionary的keys
		if is_instance_valid(entity):
			entity.queue_free()
	active_entities.clear()
	print("[EntityManager] All entities cleared")

# 重新加载模板（开发时有用）
func reload_templates():
	print("[EntityManager] Reloading templates...")
	load_all_templates()

# 获取活跃实体数量
func get_active_entity_count() -> int:
	return active_entities.size()

# 获取所有活跃实体
func get_active_entities() -> Array[Node]:
	var entities: Array[Node] = []
	for entity in active_entities.keys():
		if is_instance_valid(entity):
			entities.append(entity)
	return entities

# 根据模板名称获取实体
func get_entities_by_template(template_name: String) -> Array[Node]:
	var entities: Array[Node] = []
	for entity in active_entities.keys():
		if is_instance_valid(entity) and active_entities[entity] == template_name:
			entities.append(entity)
	return entities

# 获取模板的可覆写属性信息 - Schema 驱动版本
func get_template_properties(template_path: String) -> Dictionary:
	if template_properties.has(template_path):
		return template_properties[template_path]
	return {}

# 获取所有模板及其属性信息 - Schema 驱动版本
func get_all_template_properties() -> Dictionary:
	return template_properties.duplicate()

# 从 schema.tres 文件加载所有模板的属性信息 - Schema 驱动版本
func _analyze_all_templates():
	print("[EntityManager] Loading template properties from schema files...")
	
	for template_path in templates.keys():
		var properties = _load_schema_properties(template_path)
		template_properties[template_path] = properties
		print("[EntityManager] Template '", template_path, "' loaded ", properties.size(), " property groups from schema")

# 从 schema.tres 文件加载单个模板的属性信息 - Schema 驱动版本
func _load_schema_properties(template_name: String) -> Dictionary:
	# 构造 schema 文件路径
	var schema_path = TEMPLATES_PATH + "/" + template_name + ".schema.tres"
	
	# 尝试加载 schema 资源
	if ResourceLoader.exists(schema_path):
		var schema_resource = load(schema_path)
		if schema_resource and schema_resource.has_method("get_properties"):
			var properties = schema_resource.get_properties()
			print("[EntityManager] Loaded schema for '", template_name, "': ", properties.keys())
			return properties
		else:
			print("[EntityManager] Warning: Invalid schema resource at ", schema_path)
	else:
		print("[EntityManager] Warning: Schema file not found: ", schema_path, ", falling back to dynamic analysis")
		# 如果没有 schema 文件，回退到动态分析
		return _analyze_template_properties_fallback(template_name)
	
	return {}

# 回退方法：动态分析模板属性（保留原有逻辑作为兼容性回退）
func _analyze_template_properties_fallback(template_path: String) -> Dictionary:
	var properties = {
		"root_node": {},
		"ecs_components": {}
	}
	
	if not templates.has(template_path):
		return properties
	
	var template: PackedScene = templates[template_path]
	var temp_instance = template.instantiate()
	
	if not temp_instance:
		return properties
	
	# 分析根节点属性
	properties.root_node = _analyze_node_properties(temp_instance)
	
	# 查找并分析ECS组件
	var ecs_node = _find_ecs_node(temp_instance)
	if ecs_node:
		properties.ecs_components = _analyze_ecs_components(ecs_node)
	
	# 清理临时实例
	temp_instance.queue_free()
	
	return properties

# 分析节点的可覆写属性
func _analyze_node_properties(node: Node) -> Dictionary:
	var properties = {}
	
	# 常见的可覆写属性
	var common_properties = [
		"position", "global_position", "rotation", "global_rotation", 
		"scale", "transform", "global_transform",
		"visible", "modulate", "name"
	]
	
	for prop_name in common_properties:
		if prop_name in node:
			var prop_value = node.get(prop_name)
			properties[prop_name] = {
				"type": typeof(prop_value),
				"type_name": _get_type_name(typeof(prop_value)),
				"default_value": prop_value,
				"description": _get_property_description(prop_name)
			}
	
	# 检查特定节点类型的属性
	match node.get_class():
		"MeshInstance3D":
			if node.has_method("get_surface_override_material"):
				properties["material_override"] = {
					"type": TYPE_OBJECT,
					"type_name": "Material",
					"default_value": null,
					"description": "Override material for the mesh"
				}
		"CollisionShape3D":
			if "shape" in node:
				properties["shape"] = {
					"type": TYPE_OBJECT,
					"type_name": "Shape3D", 
					"default_value": node.shape,
					"description": "Collision shape resource"
				}
		"RigidBody3D":
			var physics_props = ["mass", "gravity_scale", "linear_damp", "angular_damp"]
			for prop in physics_props:
				if prop in node:
					properties[prop] = {
						"type": typeof(node.get(prop)),
						"type_name": _get_type_name(typeof(node.get(prop))),
						"default_value": node.get(prop),
						"description": "Physics property: " + prop
					}
	
	return properties

# 分析ECS组件的可覆写属性
func _analyze_ecs_components(ecs_node: Node) -> Dictionary:
	var component_properties = {}
	
	var components = ecs_node.get("components")
	if not components or not (components is Array):
		return component_properties
	
	for component in components:
		if component is Resource:
			var component_class = component.get_class()
			var properties = {}
			
			# 获取Resource的所有属性
			var property_list = component.get_property_list()
			for prop_info in property_list:
				var prop_name = prop_info.name
				# 跳过内部属性
				if prop_name.begins_with("_") or prop_name in ["script", "resource_path", "resource_name"]:
					continue
				
				var prop_value = component.get(prop_name)
				properties[prop_name] = {
					"type": prop_info.type,
					"type_name": _get_type_name(prop_info.type),
					"default_value": prop_value,
					"description": _get_component_property_description(component_class, prop_name)
				}
			
			component_properties[component_class] = properties
	
	return component_properties

# 获取类型名称的辅助函数
func _get_type_name(type: int) -> String:
	match type:
		TYPE_BOOL: return "bool"
		TYPE_INT: return "int" 
		TYPE_FLOAT: return "float"
		TYPE_STRING: return "String"
		TYPE_VECTOR2: return "Vector2"
		TYPE_VECTOR3: return "Vector3"
		TYPE_TRANSFORM3D: return "Transform3D"
		TYPE_QUATERNION: return "Quaternion"
		TYPE_OBJECT: return "Object"
		TYPE_ARRAY: return "Array"
		TYPE_DICTIONARY: return "Dictionary"
		_: return "Variant"

# 获取属性描述
func _get_property_description(prop_name: String) -> String:
	match prop_name:
		"position": return "Local position in 3D space"
		"global_position": return "Global position in 3D space"
		"rotation": return "Local rotation in radians"
		"global_rotation": return "Global rotation in radians"
		"scale": return "Local scale multiplier"
		"transform": return "Local transformation matrix"
		"global_transform": return "Global transformation matrix"
		"visible": return "Visibility state"
		"modulate": return "Color modulation"
		"name": return "Node name identifier"
		_: return "Property: " + prop_name

# 获取组件属性描述
func _get_component_property_description(component_class: String, prop_name: String) -> String:
	match component_class:
		"PhysicsBodyComponentResource":
			match prop_name:
				"mass": return "Object mass in kg"
				"friction": return "Surface friction coefficient"
				"restitution": return "Bounce/elasticity factor"
				"density": return "Material density"
				"linear_damping": return "Linear velocity damping"
				"angular_damping": return "Angular velocity damping"
				"shape_type": return "Collision shape type"
				"shape_size": return "Collision shape dimensions"
				"enable_ccd": return "Continuous collision detection"
				_: return component_class + " property: " + prop_name
		_: return component_class + " property: " + prop_name
