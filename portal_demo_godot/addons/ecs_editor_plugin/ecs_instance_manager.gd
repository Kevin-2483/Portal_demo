@tool
extends Node
class_name ECSInstanceManager

signal instance_created(instance)
signal instance_destroyed(instance)
signal status_changed(status)

var current_instance: GameCoreManager = null
var auto_start: bool = false
var event_bus = null # 事件总线引用

enum Status {
	STOPPED,
	STARTING,
	RUNNING,
	PAUSED, # 新增：暂停状态
	STOPPING,
	ERROR
}

var current_status: Status = Status.STOPPED

func _ready():
	name = "ECSInstanceManager"
	print("ECS Instance Manager: Ready")

# 设置事件总线引用
func set_event_bus(bus):
	event_bus = bus
	print("ECS Instance Manager: Event bus linked")

func start_instance() -> bool:
	if current_status == Status.RUNNING:
		print("ECS Instance Manager: Instance already running")
		return true
	
	print("ECS Instance Manager: Starting instance...")
	_set_status(Status.STARTING)
	
	# 启动前验证系统清洁状态
	_verify_clean_state()
	
	# 创建 GameCoreManager 实例
	current_instance = GameCoreManager.new()
	if not current_instance:
		print("ECS Instance Manager: Failed to create GameCoreManager")
		_set_status(Status.ERROR)
		return false
		
	current_instance.name = "EditorGameCoreManager"
	
	# 将实例添加到组中，以便追踪
	current_instance.add_to_group("GameCoreManager")
	
	# 设置为编辑器持久化模式 (如果方法可用)
	if current_instance.has_method("set_editor_persistent"):
		current_instance.set_editor_persistent(true)
	else:
		print("Warning: set_editor_persistent method not available - using basic mode")
	
	# 连接信号
	if current_instance.has_signal("core_initialized"):
		current_instance.connect("core_initialized", _on_core_initialized)
	if current_instance.has_signal("core_shutdown"):
		current_instance.connect("core_shutdown", _on_core_shutdown)
	if current_instance.has_signal("destruction_cancelled"):
		current_instance.connect("destruction_cancelled", _on_destruction_cancelled)
	
	# 添加到场景树
	add_child(current_instance)
	
	# 将新实例注册到事件总线
	if event_bus:
		event_bus.set_current_game_core(current_instance)
		print("ECS Instance Manager: Instance registered to event bus")
	
	# 在编辑器中，节点的 _ready() 会自动被调用
	# 不需要手动调用 _ready()
	
	return true

func stop_instance() -> void:
	if current_status == Status.STOPPED:
		print("ECS Instance Manager: No instance to stop")
		return
	
	print("ECS Instance Manager: Stopping instance...")
	_set_status(Status.STOPPING)
	
	# 第一步：清理所有 ECS 节点和实体
	if event_bus:
		print("ECS Instance Manager: Clearing all ECS nodes before shutdown...")
		event_bus.clear_all_ecs_nodes()
		# 等待清理完成
		await get_tree().process_frame
	
	if current_instance:
		# 第二步：等待一帧以确保所有 ECS 节点完成清理
		print("ECS Instance Manager: Waiting for ECS nodes cleanup...")
		await get_tree().process_frame
		
		# 第三步：禁用持久化模式
		if current_instance.has_method("set_editor_persistent"):
			current_instance.set_editor_persistent(false)
			print("ECS Instance Manager: Persistent mode disabled")
		
		# 第四步：强制关闭核心系统
		print("ECS Instance Manager: Force shutting down core systems...")
		if current_instance.has_method("force_shutdown"):
			current_instance.force_shutdown()
		else:
			# 回退到基本的关闭方法
			if current_instance.has_method("shutdown_core"):
				current_instance.shutdown_core()
		
		# 第五步：再等待一帧确保物理系统完全清理
		await get_tree().process_frame
		
		# 第六步：从场景树移除
		if current_instance.get_parent():
			current_instance.get_parent().remove_child(current_instance)
		
		current_instance.queue_free()
		current_instance = null
		print("ECS Instance Manager: Instance destroyed and freed")
	
	# 第七步：从事件总线注销实例
	if event_bus:
		event_bus.set_current_game_core(null)
		print("ECS Instance Manager: Instance unregistered from event bus")
	
	# 第八步：最终等待确保所有清理完成
	await get_tree().process_frame
	
	_set_status(Status.STOPPED)
	instance_destroyed.emit(null)
	print("ECS Instance Manager: Stop completed - system should be clean")

func restart_instance() -> bool:
	print("ECS Instance Manager: Starting restart sequence...")
	_restart_async()
	return true

func _restart_async():
	# 等待停止完成
	await stop_instance()
	
	# 额外等待确保系统完全清理
	print("ECS Instance Manager: Waiting for complete cleanup before restart...")
	await get_tree().create_timer(0.1).timeout # 100ms 缓冲时间
	
	# 启动新实例
	print("ECS Instance Manager: Starting new instance after restart...")
	start_instance()

func pause_instance() -> bool:
	"""暂停ECS实例 - 停止更新但保持状态"""
	if current_status != Status.RUNNING:
		print("ECS Instance Manager: Cannot pause - instance not running (current status: %s)" % Status.keys()[current_status])
		return false
	
	if not current_instance:
		print("ECS Instance Manager: Cannot pause - no instance available")
		return false
	
	print("ECS Instance Manager: Pausing instance...")
	
	# 如果GameCoreManager支持暂停功能
	if current_instance.has_method("pause_updates"):
		current_instance.pause_updates()
		_set_status(Status.PAUSED)
		print("ECS Instance Manager: Instance paused successfully")
		return true
	else:
		# 备用方案：暂时禁用process
		if current_instance.has_method("set_process_mode"):
			current_instance.set_process_mode(Node.PROCESS_MODE_DISABLED)
			_set_status(Status.PAUSED)
			print("ECS Instance Manager: Instance paused (fallback method)")
			return true
		else:
			print("ECS Instance Manager: Pause not supported by current instance")
			return false

func resume_instance() -> bool:
	"""恢复ECS实例 - 继续更新"""
	if current_status != Status.PAUSED:
		print("ECS Instance Manager: Cannot resume - instance not paused (current status: %s)" % Status.keys()[current_status])
		return false
	
	if not current_instance:
		print("ECS Instance Manager: Cannot resume - no instance available")
		return false
	
	print("ECS Instance Manager: Resuming instance...")
	
	# 如果GameCoreManager支持恢复功能
	if current_instance.has_method("resume_updates"):
		current_instance.resume_updates()
		_set_status(Status.RUNNING)
		print("ECS Instance Manager: Instance resumed successfully")
		return true
	else:
		# 备用方案：重新启用process
		if current_instance.has_method("set_process_mode"):
			current_instance.set_process_mode(Node.PROCESS_MODE_INHERIT)
			_set_status(Status.RUNNING)
			print("ECS Instance Manager: Instance resumed (fallback method)")
			return true
		else:
			print("ECS Instance Manager: Resume not supported by current instance")
			return false

func toggle_pause() -> bool:
	"""切换暂停/恢复状态"""
	if current_status == Status.RUNNING:
		return pause_instance()
	elif current_status == Status.PAUSED:
		return resume_instance()
	else:
		print("ECS Instance Manager: Cannot toggle pause - invalid status: %s" % Status.keys()[current_status])
		return false

func is_paused() -> bool:
	return current_status == Status.PAUSED

func get_instance() -> GameCoreManager:
	return current_instance

func is_running() -> bool:
	return current_status == Status.RUNNING

func is_active() -> bool:
	"""检查实例是否处于活动状态（运行或暂停）"""
	return current_status == Status.RUNNING or current_status == Status.PAUSED

func get_status() -> Status:
	return current_status

func _set_status(new_status: Status):
	if current_status != new_status:
		current_status = new_status
		status_changed.emit(current_status)
		print("ECS Instance Manager: Status changed to ", Status.keys()[current_status])

func _on_core_initialized():
	print("ECS Instance Manager: Core initialized")
	_set_status(Status.RUNNING)
	instance_created.emit(current_instance)

func _on_core_shutdown():
	print("ECS Instance Manager: Core shutdown")
	if current_status != Status.STOPPING:
		_set_status(Status.STOPPED)

func _on_destruction_cancelled():
	print("ECS Instance Manager: Destruction cancelled - persistent mode active")

func cleanup():
	print("ECS Instance Manager: Cleanup called")
	if current_instance:
		if current_instance.has_method("set_editor_persistent"):
			current_instance.set_editor_persistent(false)
		stop_instance()
	
	# 清理事件总线引用
	if event_bus:
		event_bus = null

func create_and_save_snapshot(snapshot_name: String = "") -> String:
	"""创建并保存场景快照，返回快照ID"""
	if current_status == Status.RUNNING:
		print("ECS Instance Manager: Cannot create snapshot while instance is running")
		return ""
	
	# 获取当前编辑的场景根节点
	var scene_root = EditorInterface.get_edited_scene_root()
	if not scene_root:
		print("ECS Instance Manager: No active scene to snapshot")
		return ""
	
	var scene_path = scene_root.scene_file_path
	if scene_path.is_empty():
		print("ECS Instance Manager: Scene not saved, cannot save snapshot")
		return ""
	
	# 生成快照ID和时间戳
	var timestamp = Time.get_datetime_string_from_system()
	var snapshot_id = "snapshot_%d" % Time.get_unix_time_from_system()
	
	# 如果没有提供名称，使用时间戳作为默认名称
	if snapshot_name.is_empty():
		snapshot_name = "Snapshot %s" % Time.get_datetime_string_from_system().replace("T", " ")
	
	var snapshot_data = {}
	# 递归保存所有节点的状态信息
	_create_node_snapshot(scene_root, snapshot_data)
	
	# 获取现有的快照集合
	var snapshots_collection = {}
	if scene_root.has_meta("ecs_snapshots"):
		snapshots_collection = scene_root.get_meta("ecs_snapshots")
	
	# 添加新快照
	snapshots_collection[snapshot_id] = {
		"name": snapshot_name,
		"timestamp": timestamp,
		"created_time": Time.get_unix_time_from_system(),
		"node_count": snapshot_data.size(),
		"data": snapshot_data
	}
	
	# 保存快照集合
	scene_root.set_meta("ecs_snapshots", snapshots_collection)
	print("ECS Instance Manager: Snapshot '%s' created with ID: %s" % [snapshot_name, snapshot_id])
	
	# 标记场景为已修改，提示用户保存
	var editor_interface = EditorInterface
	if editor_interface and editor_interface.has_method("mark_scene_as_unsaved"):
		editor_interface.mark_scene_as_unsaved()
	
	return snapshot_id

func _create_node_snapshot(node: Node, snapshot: Dictionary):
	"""递归创建节点快照"""
	var node_id = str(node.get_path())
	var node_data = {}
	
	# 保存基本信息
	node_data.name = node.name
	node_data.type = node.get_class()
	
	# 如果是Node3D，保存变换信息
	if node is Node3D:
		var node_3d = node as Node3D
		node_data.transform = {
			"position": node_3d.position,
			"rotation": node_3d.rotation,
			"scale": node_3d.scale
		}
	
	# 如果是Control，保存位置和大小
	elif node is Control:
		var control = node as Control
		node_data.rect = {
			"position": control.position,
			"size": control.size
		}
	
	# 保存自定义属性（可扩展）
	if node.has_method("get_ecs_snapshot_data"):
		node_data.custom = node.get_ecs_snapshot_data()
	
	snapshot[node_id] = node_data
	
	# 递归处理子节点
	for child in node.get_children():
		_create_node_snapshot(child, snapshot)

func restore_snapshot(snapshot_id: String = "") -> bool:
	"""恢复指定快照，如果未指定则恢复最新快照"""
	var scene_root = EditorInterface.get_edited_scene_root()
	if not scene_root:
		print("ECS Instance Manager: No active scene to restore")
		return false
	
	if not scene_root.has_method("get_meta") or not scene_root.has_meta("ecs_snapshots"):
		print("ECS Instance Manager: No snapshots found in scene")
		return false
	
	var snapshots_collection = scene_root.get_meta("ecs_snapshots")
	if snapshots_collection.is_empty():
		print("ECS Instance Manager: No snapshots available")
		return false
	
	var target_snapshot = null
	var target_id = ""
	
	# 如果指定了快照ID，使用指定的快照
	if not snapshot_id.is_empty() and snapshots_collection.has(snapshot_id):
		target_snapshot = snapshots_collection[snapshot_id]
		target_id = snapshot_id
	else:
		# 否则找到最新的快照
		var latest_time = 0
		for id in snapshots_collection.keys():
			var snapshot = snapshots_collection[id]
			if snapshot.created_time > latest_time:
				latest_time = snapshot.created_time
				target_snapshot = snapshot
				target_id = id
	
	if not target_snapshot:
		print("ECS Instance Manager: No valid snapshot found")
		return false
	
	_restore_node_snapshot(scene_root, target_snapshot.data)
	print("ECS Instance Manager: Snapshot '%s' restored" % target_snapshot.name)
	return true

func _restore_node_snapshot(root: Node, snapshot: Dictionary):
	"""递归恢复节点快照"""
	for child in root.get_children():
		var node_id = str(child.get_path())
		if snapshot.has(node_id):
			var node_data = snapshot[node_id]
			
			# 恢复变换信息
			if child is Node3D and node_data.has("transform"):
				var node_3d = child as Node3D
				var transform_data = node_data.transform
				node_3d.position = transform_data.position
				node_3d.rotation = transform_data.rotation
				node_3d.scale = transform_data.scale
			
			# 恢复Control位置和大小
			elif child is Control and node_data.has("rect"):
				var control = child as Control
				var rect_data = node_data.rect
				control.position = rect_data.position
				control.size = rect_data.size
			
			# 恢复自定义数据
			if child.has_method("restore_ecs_snapshot_data") and node_data.has("custom"):
				child.restore_ecs_snapshot_data(node_data.custom)
		
		# 递归处理子节点
		_restore_node_snapshot(child, snapshot)

# 获取实例状态信息
func get_status_info() -> Dictionary:
	var info = {
		"status": Status.keys()[current_status],
		"has_instance": current_instance != null,
		"core_initialized": false,
		"persistent_mode": false,
		"reference_count": 0
	}
	
	if current_instance:
		if current_instance.has_method("is_core_initialized"):
			info.core_initialized = current_instance.is_core_initialized()
		if current_instance.has_method("is_editor_persistent"):
			info.persistent_mode = current_instance.is_editor_persistent()
		# Note: reference_count 需要在 C++ 中暴露才能访问
	
	return info

func delete_snapshot(snapshot_id: String = "") -> bool:
	"""删除指定快照，如果未指定则删除所有快照"""
	var scene_root = EditorInterface.get_edited_scene_root()
	if not scene_root:
		print("ECS Instance Manager: No active scene")
		return false
	
	if not scene_root.has_meta("ecs_snapshots"):
		print("ECS Instance Manager: No snapshots to delete")
		return false
	
	var snapshots_collection = scene_root.get_meta("ecs_snapshots")
	
	if snapshot_id.is_empty():
		# 删除所有快照
		scene_root.remove_meta("ecs_snapshots")
		print("ECS Instance Manager: All snapshots deleted")
	else:
		# 删除指定快照
		if snapshots_collection.has(snapshot_id):
			var snapshot_name = snapshots_collection[snapshot_id].name
			snapshots_collection.erase(snapshot_id)
			
			if snapshots_collection.is_empty():
				scene_root.remove_meta("ecs_snapshots")
			else:
				scene_root.set_meta("ecs_snapshots", snapshots_collection)
			
			print("ECS Instance Manager: Snapshot '%s' deleted" % snapshot_name)
		else:
			print("ECS Instance Manager: Snapshot ID '%s' not found" % snapshot_id)
			return false
	
	# 标记场景为已修改
	var editor_interface = EditorInterface
	if editor_interface and editor_interface.has_method("mark_scene_as_unsaved"):
		editor_interface.mark_scene_as_unsaved()
	
	return true

func has_snapshots() -> bool:
	"""检查是否有可用的快照"""
	var scene_root = EditorInterface.get_edited_scene_root()
	if not scene_root:
		return false
	
	if not scene_root.has_meta("ecs_snapshots"):
		return false
	
	var snapshots = scene_root.get_meta("ecs_snapshots")
	return not snapshots.is_empty()

func get_snapshots_list() -> Array:
	"""获取快照列表"""
	var scene_root = EditorInterface.get_edited_scene_root()
	if not scene_root or not scene_root.has_meta("ecs_snapshots"):
		return []
	
	var snapshots_collection = scene_root.get_meta("ecs_snapshots")
	var snapshots_list = []
	
	# 转换为数组并按时间排序（最新的在前）
	for id in snapshots_collection.keys():
		var snapshot = snapshots_collection[id]
		snapshots_list.append({
			"id": id,
			"name": snapshot.name,
			"timestamp": snapshot.timestamp,
			"created_time": snapshot.created_time,
			"node_count": snapshot.node_count
		})
	
	# 按创建时间排序（最新的在前）
	snapshots_list.sort_custom(func(a, b): return a.created_time > b.created_time)
	
	return snapshots_list

func rename_snapshot(snapshot_id: String, new_name: String) -> bool:
	"""重命名快照"""
	var scene_root = EditorInterface.get_edited_scene_root()
	if not scene_root or not scene_root.has_meta("ecs_snapshots"):
		return false
	
	var snapshots_collection = scene_root.get_meta("ecs_snapshots")
	if not snapshots_collection.has(snapshot_id):
		return false
	
	snapshots_collection[snapshot_id].name = new_name
	scene_root.set_meta("ecs_snapshots", snapshots_collection)
	
	# 标记场景为已修改
	var editor_interface = EditorInterface
	if editor_interface and editor_interface.has_method("mark_scene_as_unsaved"):
		editor_interface.mark_scene_as_unsaved()
	
	print("ECS Instance Manager: Snapshot renamed to '%s'" % new_name)
	return true

# 私有方法：验证系统启动前的清洁状态
func _verify_clean_state():
	"""验证系统启动前没有残留的物理实体或其他资源"""
	print("ECS Instance Manager: Verifying clean state before startup...")
	
	# 检查是否有残留的 GameCoreManager 实例
	var existing_cores = get_tree().get_nodes_in_group("GameCoreManager")
	if existing_cores.size() > 0:
		print("ECS Instance Manager: Warning - Found %d existing GameCoreManager instances, cleaning up..." % existing_cores.size())
		for core in existing_cores:
			if is_instance_valid(core):
				if core.has_method("force_shutdown"):
					core.force_shutdown()
				core.queue_free()
		# 等待清理完成
		await get_tree().process_frame
	
	# 检查事件总线状态
	if event_bus:
		var nodes_info = event_bus.get_ecs_nodes_info()
		if nodes_info.entities_created > 0:
			print("ECS Instance Manager: Warning - Found %d active entities, clearing..." % nodes_info.entities_created)
			event_bus.clear_all_ecs_nodes()
			await get_tree().process_frame
	
	print("ECS Instance Manager: Clean state verification completed")
