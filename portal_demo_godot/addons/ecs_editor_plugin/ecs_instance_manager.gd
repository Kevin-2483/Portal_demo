@tool
extends Node
class_name ECSInstanceManager

signal instance_created(instance)
signal instance_destroyed(instance)  
signal status_changed(status)

var current_instance: GameCoreManager = null
var auto_start: bool = false
var event_bus = null  # 事件总线引用

enum Status {
	STOPPED,
	STARTING,
	RUNNING,
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
	
	# 创建 GameCoreManager 实例
	current_instance = GameCoreManager.new()
	if not current_instance:
		print("ECS Instance Manager: Failed to create GameCoreManager")
		_set_status(Status.ERROR)
		return false
		
	current_instance.name = "EditorGameCoreManager"
	
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
	
	# 从事件总线注销实例
	if event_bus:
		event_bus.set_current_game_core(null)
		print("ECS Instance Manager: Instance unregistered from event bus")
	
	if current_instance:
		# 先禁用持久化模式 (如果方法可用)
		if current_instance.has_method("set_editor_persistent"):
			current_instance.set_editor_persistent(false)
		
		# 强制关闭 (如果方法可用)
		if current_instance.has_method("force_shutdown"):
			current_instance.force_shutdown()
		else:
			# 回退到基本的关闭方法
			if current_instance.has_method("shutdown_core"):
				current_instance.shutdown_core()
		
		# 从场景树移除
		if current_instance.get_parent():
			current_instance.get_parent().remove_child(current_instance)
		
		current_instance.queue_free()
		current_instance = null
	
	_set_status(Status.STOPPED)
	instance_destroyed.emit(null)

func restart_instance() -> bool:
	stop_instance()
	# 简单等待，不使用 await
	call_deferred("_do_restart")
	return true

func _do_restart():
	start_instance()

func get_instance() -> GameCoreManager:
	return current_instance

func is_running() -> bool:
	return current_status == Status.RUNNING

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
