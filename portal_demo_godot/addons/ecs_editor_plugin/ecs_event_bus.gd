# ECS 事件总线 - 专门用于编辑器模式下的 ECS 系统通信
# 只在编辑器模式下工作，运行时不使用

extends Node

signal game_core_initialized()
signal game_core_shutdown()
signal instance_changed(new_instance)
signal reset_ecs_nodes()         # 新增：重置所有 ECSNode 状态
signal clear_ecs_nodes()         # 新增：清除所有 ECSNode 实体

var _current_game_core: GameCoreManager = null
var _is_editor_mode: bool = false
var _registered_ecs_nodes: Array[Node] = []  # 新增：注册的 ECSNode 列表

func _ready():
	name = "ECSEventBus"
	_is_editor_mode = Engine.is_editor_hint()
	
	if _is_editor_mode:
		print("ECSEventBus: Initialized for editor mode")
		# 设置为持久节点，避免场景切换时被销毁
		process_mode = Node.PROCESS_MODE_ALWAYS
	else:
		print("ECSEventBus: Skipping initialization - runtime mode detected")
		queue_free()
		return

# 设置当前的 GameCore 实例
func set_current_game_core(game_core: GameCoreManager):
	if not _is_editor_mode:
		return
		
	# 断开旧实例的信号
	if _current_game_core and is_instance_valid(_current_game_core):
		if _current_game_core.is_connected("core_initialized", _on_game_core_initialized):
			_current_game_core.disconnect("core_initialized", _on_game_core_initialized)
		if _current_game_core.is_connected("core_shutdown", _on_game_core_shutdown):
			_current_game_core.disconnect("core_shutdown", _on_game_core_shutdown)
	
	# 如果切换到新实例，先清理所有 ECSNode
	if _current_game_core != game_core:
		print("ECSEventBus: GameCore instance changing, clearing all ECSNodes...")
		clear_all_ecs_nodes()
	
	_current_game_core = game_core
	
	# 连接新实例的信号
	if _current_game_core:
		print("ECSEventBus: Connecting to new GameCore instance")
		
		if not _current_game_core.is_connected("core_initialized", _on_game_core_initialized):
			_current_game_core.connect("core_initialized", _on_game_core_initialized)
		if not _current_game_core.is_connected("core_shutdown", _on_game_core_shutdown):
			_current_game_core.connect("core_shutdown", _on_game_core_shutdown)
		
		# 通知实例变更
		instance_changed.emit(_current_game_core)
		
		# 如果新实例已经初始化，立即广播状态
		if _current_game_core.is_core_initialized():
			print("ECSEventBus: New GameCore already initialized, broadcasting...")
			call_deferred("_on_game_core_initialized")
	else:
		print("ECSEventBus: GameCore instance set to null")
		instance_changed.emit(null)

# 获取当前的 GameCore 实例
func get_current_game_core() -> GameCoreManager:
	return _current_game_core

# 检查 GameCore 是否准备就绪
func is_game_core_ready() -> bool:
	return _current_game_core != null and _current_game_core.is_core_initialized()

# 私有回调方法
func _on_game_core_initialized():
	print("ECSEventBus: Broadcasting game_core_initialized signal")
	game_core_initialized.emit()

func _on_game_core_shutdown():
	print("ECSEventBus: Broadcasting game_core_shutdown signal")
	game_core_shutdown.emit()

# 强制广播当前状态 - 用于晚加入的 ECSNode
func broadcast_current_state():
	if not _is_editor_mode:
		return
		
	if is_game_core_ready():
		print("ECSEventBus: Force broadcasting current initialized state")
		call_deferred("_on_game_core_initialized")
	else:
		print("ECSEventBus: GameCore not ready, no state to broadcast")

# === 新增：ECSNode 管理功能 ===

# 注册 ECSNode 到事件总线
func register_ecs_node(ecs_node: Node):
	if not _is_editor_mode:
		return
		
	if ecs_node and not ecs_node in _registered_ecs_nodes:
		_registered_ecs_nodes.append(ecs_node)
		print("ECSEventBus: ECSNode registered. Total: ", _registered_ecs_nodes.size())

# 注销 ECSNode
func unregister_ecs_node(ecs_node: Node):
	if not _is_editor_mode:
		return
		
	if ecs_node in _registered_ecs_nodes:
		_registered_ecs_nodes.erase(ecs_node)
		print("ECSEventBus: ECSNode unregistered. Total: ", _registered_ecs_nodes.size())

# 重置所有 ECSNode 状态 - 用于重启后的状态修复
func reset_all_ecs_nodes():
	if not _is_editor_mode:
		return
		
	print("ECSEventBus: Resetting all ECSNode states...")
	
	# 清理无效的节点引用
	_cleanup_invalid_nodes()
	
	# 发出重置信号
	reset_ecs_nodes.emit()
	
	print("ECSEventBus: Reset signal sent to ", _registered_ecs_nodes.size(), " ECSNodes")

# 清除所有 ECSNode 实体 - 用于实例切换时的清理
func clear_all_ecs_nodes():
	if not _is_editor_mode:
		return
		
	print("ECSEventBus: Clearing all ECSNode entities...")
	
	# 清理无效的节点引用
	_cleanup_invalid_nodes()
	
	# 发出清除信号
	clear_ecs_nodes.emit()
	
	print("ECSEventBus: Clear signal sent to ", _registered_ecs_nodes.size(), " ECSNodes")

# 获取注册的 ECSNode 统计信息
func get_ecs_nodes_info() -> Dictionary:
	if not _is_editor_mode:
		return {}
		
	_cleanup_invalid_nodes()
	
	var info = {
		"total_nodes": _registered_ecs_nodes.size(),
		"valid_nodes": 0,
		"entities_created": 0
	}
	
	for node in _registered_ecs_nodes:
		if is_instance_valid(node):
			info.valid_nodes += 1
			if node.has_method("is_entity_created") and node.call("is_entity_created"):
				info.entities_created += 1
	
	return info

# 私有方法：清理无效的节点引用
func _cleanup_invalid_nodes():
	var valid_nodes: Array[Node] = []
	for node in _registered_ecs_nodes:
		if is_instance_valid(node):
			valid_nodes.append(node)
	
	var removed_count = _registered_ecs_nodes.size() - valid_nodes.size()
	if removed_count > 0:
		print("ECSEventBus: Cleaned up ", removed_count, " invalid node references")
	
	_registered_ecs_nodes.clear()
	_registered_ecs_nodes.append_array(valid_nodes)

# 清理方法
func cleanup():
	if _current_game_core and is_instance_valid(_current_game_core):
		if _current_game_core.is_connected("core_initialized", _on_game_core_initialized):
			_current_game_core.disconnect("core_initialized", _on_game_core_initialized)
		if _current_game_core.is_connected("core_shutdown", _on_game_core_shutdown):
			_current_game_core.disconnect("core_shutdown", _on_game_core_shutdown)
	_current_game_core = null
	_registered_ecs_nodes.clear()
	print("ECSEventBus: Cleaned up")
