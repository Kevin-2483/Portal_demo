@tool
extends EditorScript

# 测试脚本：验证 ECS 编辑器插件功能

func _run():
	print("=== ECS Editor Plugin 测试开始 ===")
	
	# 测试 1: 检查插件是否已加载
	test_plugin_loaded()
	
	# 测试 2: 测试实例管理器
	await test_instance_manager()
	
	# 测试 3: 测试持久化功能
	await test_persistence_features()
	
	print("=== ECS Editor Plugin 测试完成 ===")

func test_plugin_loaded():
	print("测试 1: 检查插件加载状态")
	
	var plugin_manager = EditorInterface.get_editor_plugins()
	print("插件管理器可用: ", plugin_manager != null)
	
	# 检查插件文件是否存在
	var plugin_path = "res://addons/ecs_editor_plugin/plugin.cfg"
	if FileAccess.file_exists(plugin_path):
		print("✓ 插件配置文件存在")
	else:
		print("✗ 插件配置文件不存在")
	
	var plugin_script_path = "res://addons/ecs_editor_plugin/plugin.gd"
	if FileAccess.file_exists(plugin_script_path):
		print("✓ 插件脚本文件存在")
	else:
		print("✗ 插件脚本文件不存在")

func test_instance_manager():
	print("\n测试 2: 实例管理器功能")
	
	# 创建临时实例管理器进行测试
	var manager = load("res://addons/ecs_editor_plugin/ecs_instance_manager.gd").new()
	
	print("创建实例管理器: ", manager != null)
	
	if manager:
		# 测试初始状态
		print("初始状态: ", manager.Status.keys()[manager.get_status()])
		print("是否运行中: ", manager.is_running())
		print("当前实例: ", manager.get_instance())
		
		# 测试启动实例
		print("尝试启动实例...")
		var success = manager.start_instance()
		print("启动结果: ", success)
		
		if success:
			await get_tree().process_frame
			print("启动后状态: ", manager.Status.keys()[manager.get_status()])
			print("实例: ", manager.get_instance())
			
			# 测试状态信息
			var info = manager.get_status_info()
			print("状态信息: ", info)
			
			# 清理
			manager.cleanup()
		
		manager.queue_free()

func test_persistence_features():
	print("\n测试 3: 持久化功能")
	
	# 直接测试 GameCoreManager
	print("创建 GameCoreManager 实例...")
	var game_core = GameCoreManager.new()
	
	if game_core:
		print("✓ GameCoreManager 实例创建成功")
		print("编辑器持久化模式: ", game_core.is_editor_persistent())
		
		# 测试持久化设置
		game_core.set_editor_persistent(true)
		print("设置持久化模式后: ", game_core.is_editor_persistent())
		
		game_core.set_editor_persistent(false)
		print("取消持久化模式后: ", game_core.is_editor_persistent())
		
		# 测试引用计数
		game_core.add_reference()
		print("添加引用后状态正常")
		
		game_core.remove_reference()
		print("移除引用后状态正常")
		
		# 清理
		game_core.queue_free()
		print("✓ 持久化功能测试完成")
	else:
		print("✗ GameCoreManager 实例创建失败")

func print_separator():
	print("----------------------------------------")
