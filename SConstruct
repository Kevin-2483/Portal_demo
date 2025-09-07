#!/usr/bin/env python
# -*- coding: utf-8 -*-
"""
统一编译脚本 - Portal Demo 项目 (优化版)
将 godot-cpp 和 GDExtension 分离编译，避免不必要的重编译。
"""
import os
import sys
from SCons.Script import (
    ARGUMENTS,
    Dir,
    Glob,
    SConscript,
    Default,
    Alias,
    Clean,
    Exit,
    GetOption,
    File,
)
from SCons.Util import flatten
from SCons.Environment import Base as SConsEnvironmentBase
import sys

# ==============================================================================
# 自定义工具函数
# ==============================================================================
import collections


def check_include_collisions(cpp_path_nodes, ignored_files=None):
    """
    检查 SCons CPPPATH 中的头文件重名冲突。

    :param cpp_path_nodes: 从 SCons 环境中获取的 CPPPATH 列表 (通常是 Dir 节点)。
    :param ignored_files: 一个可选的 set，包含要忽略检查的文件名。
    :return: 返回一个字典，键是重名的文件名，值是找到该文件的路径列表。
    """
    if ignored_files is None:
        ignored_files = {".DS_Store"}

    print("\n--- 开始检查头文件路径冲突 ---")

    header_extensions = {".h", ".hpp", ".hxx", ".hh"}

    # --- [修正 V2]：使用 set 来自动处理重复的路径 ---
    # defaultdict(set) 会为每个文件名创建一个空的集合 (set)
    seen_headers = collections.defaultdict(set)

    unique_paths = []
    seen_paths = set()
    for path_node in cpp_path_nodes:
        abs_path = path_node.abspath
        if abs_path not in seen_paths:
            unique_paths.append(path_node)
            seen_paths.add(abs_path)

    for path_node in unique_paths:
        dir_path = path_node.abspath
        if not os.path.isdir(dir_path):
            print(f"  - 警告: 包含路径不存在，跳过检查: {dir_path}")
            continue

        for root, _, files in os.walk(dir_path):
            for filename in files:
                if filename in ignored_files:
                    continue

                _, ext = os.path.splitext(filename)
                if ext in header_extensions:
                    full_path = os.path.join(root, filename)
                    # --- [修正 V2]：使用 .add() 方法 ---
                    # 如果 full_path 已经存在于 set 中，这次 add 操作不会产生任何效果。
                    # 只有当一个新的、不同的 full_path 出现时，set 的大小才会增加。
                    seen_headers[filename].add(full_path)

    # 现在的逻辑是：只有当同一个文件名在多个 *不同的* 物理路径被找到时，
    # 对应的 set 的长度才会大于1，这才是真正的冲突。
    collisions = {
        name: sorted(list(paths))
        for name, paths in seen_headers.items()
        if len(paths) > 1
    }

    if not collisions:
        print("--- 检查完成：未发现头文件冲突。 ---")

    return collisions


# 确保 SCons 和 Python 版本
EnsureSConsVersion(4, 0)
EnsurePythonVersion(3, 8)

print("=== Portal Demo 项目编译系统 (优化版) ===")
print(f"项目根目录: {Dir('#').abspath}")

# --- 设置默认编译参数 ---
if sys.platform == "win32":
    detected_platform = "windows"
    default_arch = "x86_64"
elif sys.platform == "darwin":
    detected_platform = "macos"
    default_arch = "universal"
else:
    detected_platform = "linux"
    # 假设 Linux 默认为 64 位
    default_arch = "x86_64"

# 直接赋值，强制覆盖命令行参数，实现全自动
ARGUMENTS["platform"] = detected_platform
print(f"--- 已自动设置平台为: {detected_platform} ---")


# setdefault 仍然用于其他参数，因为它们可能需要手动更改
ARGUMENTS.setdefault("arch", default_arch)
ARGUMENTS.setdefault("target", "template_debug")

# ==============================================================================
# 阶段一：调用 godot-cpp 构建脚本
# ==============================================================================
print("\n=== 阶段一：配置 godot-cpp 库 ===")

# **-- [逻辑修正 V9 - 最终版] --**
# 根据主人的指正，custom_api_file 是 godot-cpp 编译时进行代码生成所必需的。
# 因此，必须在调用 SConscript 之前就将其设置好。
api_file = "portal_demo_godot/gdextension/extension_api.json"
if os.path.exists(api_file):
    ARGUMENTS["custom_api_file"] = api_file
    print(f"  - 发现 API 文件，将传递给 godot-cpp: {api_file}")
else:
    print(f"错误: API 文件不存在: {api_file}")
    Exit(1)

godot_cpp_sconstruct_path = "portal_demo_godot/gdextension/godot-cpp/SConstruct"
library_nodes = SConscript(godot_cpp_sconstruct_path)

if library_nodes and not GetOption("clean"):
    # --- BUILD PATH ---
    flat_nodes = flatten(library_nodes)
    if not flat_nodes:
        print("\n错误: godot-cpp SConscript 在构建模式下没有返回有效的构建目标。")
        Exit(1)

    # 新策略：
    # 1. 从返回值中只获取唯一可靠的信息：编译环境对象。
    # 2. 根据环境中的变量，精确地、确定地计算出 godot-cpp 库的目标路径。
    env_base = None
    # 尝试在返回值中直接找到环境对象
    for node in flat_nodes:
        if isinstance(node, SConsEnvironmentBase):
            env_base = node
            break
    # 如果找不到，则假设返回的第一个元素是节点，并从节点获取环境
    if not env_base:
        if hasattr(flat_nodes[0], "get_env"):
            env_base = flat_nodes[0].get_env()

    if not env_base:
        print(f"\n致命错误: 无法从 godot-cpp SConscript 的返回值中推断出编译环境。")
        Exit(1)

    print("  - 已从 godot-cpp 获取基础编译环境。")

    # 根据环境，手动构建我们期望的 godot-cpp 库的文件节点
    godot_cpp_lib_name_stem = ""
    if env_base["platform"] == "windows":
        # 兼容某些版本在 Windows 上错误地添加了 "lib" 前缀的 godot-cpp SConstruct
        godot_cpp_lib_name_stem = f"libgodot-cpp.{env_base['platform']}.{env_base['target']}.{env_base['arch']}"
    else:
        # 正常平台的计算方式 (Linux, macOS 等)
        godot_cpp_lib_name_stem = f"godot-cpp.{env_base['platform']}.{env_base['target']}.{env_base['arch']}"

    # 后面的代码保持不变
    godot_cpp_full_lib_name = (
        f"{env_base['LIBPREFIX']}{godot_cpp_lib_name_stem}{env_base['LIBSUFFIX']}"
    )
    godot_cpp_library = File(
        f"#portal_demo_godot/gdextension/godot-cpp/bin/{godot_cpp_full_lib_name}"
    )

    print("Godot-cpp 库已配置编译。")
    print(f"  - 平台: {env_base['platform']}")
    print(f"  - 架构: {env_base['arch']}")
    print(f"  - 目标: {env_base['target']}")
    print(f"  - 预期库文件: {godot_cpp_library.path}")

    # ==============================================================================
    # 阶段二：编译 GDExtension 插件
    # ==============================================================================
    print("\n=== 阶段二：配置 GDExtension 插件编译环境 ===")

    # --- 2.1：为插件创建一个独立、干净的编译环境 ---
    env_plugin = env_base.Clone()
    print("已为插件创建独立的编译环境。")

    # --- 2.2：配置插件环境 ---
    build_dir = "build"
    env_plugin.VariantDir(build_dir, ".", duplicate=1)
    print(f"  - 构建目录设置为: {build_dir}")

    env_plugin.Append(
        CPPPATH=[
            Dir("#portal_demo_godot/gdextension/godot-cpp/gen/include"),
            Dir("#portal_demo_godot/gdextension/godot-cpp/include"),
            Dir("#portal_demo_godot/gdextension/include"),
            Dir("#portal_demo_godot/gdextension/ecs-components/include"),
            Dir("#src/core"),
            Dir("#src/core/components"),
            Dir("#src/core/systems"),
            Dir("#src/vendor/entt/single_include"),
            Dir("#src/vendor/jolt"),
        ]
    )
    print("  - 插件头文件路径已设置。")
    # --- [新增步骤]：检查头文件包含路径冲突 ---
    header_collisions = check_include_collisions(env_plugin["CPPPATH"])
    if header_collisions:
        print("\n!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!")
        print("!!! 错误：检测到头文件重名冲突！")
        print("!!! 以下文件在多个包含路径中被发现，这可能导致非预期的编译行为。")
        print("!!! 请重命名文件或调整项目结构以消除歧义。")
        print("!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!\n")

        for filename, paths in header_collisions.items():
            print(f"  [冲突文件]: {filename}")
            for path in paths:
                print(f"    - 位于: {path}")
            print("")  # 加一个空行分隔

    ### [修改] 使用平台判断来设置正确的 C++ 标准标志
    if env_plugin["platform"] == "windows":
        env_plugin.Append(CXXFLAGS=["/std:c++17", "/EHsc", "/utf-8"]) # /EHsc 是 Windows 上常用的异常处理模型
        print("  - C++ 标准设置为 C++17 (MSVC)。")
    else:
        env_plugin.Append(CXXFLAGS=["-std=c++17"])
        print("  - C++ 标准设置为 C++17。")


    # --- 2.3 (关键步骤): 配置链接器以使用 godot-cpp 库 ---
    godot_cpp_bin_path = os.path.join(
        Dir("#portal_demo_godot/gdextension/godot-cpp/bin").abspath
    )
    env_plugin.Append(LIBPATH=[godot_cpp_bin_path])
    # 从我们自己构建的节点获取库名
    godot_cpp_lib_name = godot_cpp_library.name.replace(
        env_plugin["LIBPREFIX"], ""
    ).replace(env_plugin["LIBSUFFIX"], "")
    env_plugin.Append(LIBS=[godot_cpp_lib_name])
    print(f"  - 配置链接器以使用库: {godot_cpp_lib_name}")

    ### [修改] 针对不同平台的环境微调
    if env_plugin["platform"] == "macos":
        env_plugin.Append(LINKFLAGS=["-stdlib=libc++", "-Wl,-undefined,dynamic_lookup"])
        print("  - 已为 macOS 添加链接器标志。")
    elif env_plugin["platform"] == "windows":
        # MSVC 通常需要定义这个宏来正确链接 Windows API
        env_plugin.Append(CPPDEFINES=["WINDOWS_ENABLED", "TYPED_METHOD_BIND"])
        # 如果需要调试信息
        if env_plugin.get("target") == "template_debug":
            env_plugin.Append(LINKFLAGS=["/DEBUG"])
        print("  - 已为 Windows 添加编译和链接设置。")


    # --- 2.5：收集 *只属于插件* 的源文件 ---
    # Glob 必须在 variant_dir (build_dir) 中执行，以确保中间文件被正确放置。

    # 收集 Portal 項目源文件
    plugin_sources = (
        Glob(f"{build_dir}/portal_demo_godot/gdextension/src/*.cpp")
        + Glob(f"{build_dir}/portal_demo_godot/gdextension/ecs-components/src/*.cpp")
        + Glob(f"{build_dir}/src/core/*.cpp")
        + Glob(f"{build_dir}/src/core/systems/*.cpp")
    )

    # 收集所有 Jolt Physics 源文件（遞歸搜尋所有子目錄）
    import os

    jolt_base_dir = "src/vendor/jolt/Jolt"
    jolt_sources = []

    # 要排除的目錄（真正的可選組件）
    excluded_dirs = {
        # 'Renderer',  # 調試渲染器可能被其他組件依賴，暫時保留
        # 'Skeleton',  # 骨骼系統被其他組件依賴，需要保留
    }

    # 遍歷所有 Jolt 子目錄並收集 .cpp 文件
    for root, dirs, files in os.walk(jolt_base_dir):
        # 檢查是否在排除目錄中
        rel_path = os.path.relpath(root, jolt_base_dir)
        if any(excluded_dir in rel_path for excluded_dir in excluded_dirs):
            continue

        # 轉換為相對於 build_dir 的路徑
        rel_root = os.path.relpath(root, ".")
        build_root = os.path.join(build_dir, rel_root)

        # 添加該目錄下的所有 .cpp 文件
        cpp_files = [f for f in files if f.endswith(".cpp")]
        for cpp_file in cpp_files:
            jolt_sources.append(File(os.path.join(build_root, cpp_file)))

    # 合併所有源文件
    plugin_sources += jolt_sources

    print(f"  - 收集到 {len(jolt_sources)} 個 Jolt Physics 源文件")
    print(f"  - Portal 項目源文件數量: {len(plugin_sources) - len(jolt_sources)}")
    print(f"  - 總源文件數量: {len(plugin_sources)}")

    # ==============================================================================
    # 阶段 2.5.2：编译测试程序
    # ==============================================================================
    print("\n=== 配置测试程序编译 ===")
    
    # 检查测试源文件是否存在
    test_file_exists = os.path.exists("src/core/tests/test_ecs_physics_core_fixed.cpp")
    if test_file_exists:
        # 测试程序源文件
        test_sources = [
            f"{build_dir}/src/core/tests/test_ecs_physics_core_fixed.cpp",
            # 需要的核心源文件
            f"{build_dir}/src/core/physics_world_manager.cpp",
            f"{build_dir}/src/core/portal_game_world.cpp",
            f"{build_dir}/src/core/event_manager.cpp",
            f"{build_dir}/src/core/systems/physics_system.cpp",
            f"{build_dir}/src/core/systems/physics_command_system.cpp",
        ]
        
        # 使用和主程序完全相同的编译环境（不要Clone，直接使用）
        env_test = env_plugin
        
        # 添加 Jolt 源文件到测试
        test_sources += [str(src) for src in jolt_sources]
        
        # 编译测试程序 - 使用和主程序相同的设置
        test_program = env_test.Program(
            target=f"{build_dir}/test_ecs_physics_core", # SCons 会在 Windows 上自动添加 .exe 后缀
            source=test_sources
        )
        
        print(f"  - 测试程序目标: {build_dir}/test_ecs_physics_core")
        print(f"  - 测试源文件数量: {len(test_sources)}")
        print("  - 使用和主程序相同的编译环境")
        
        print("  - 测试程序编译配置完成")
    else:
        print("  - 警告: 测试文件不存在，跳过测试编译")

    # 检查并编译事件管理器测试
    event_test_file = "src/core/tests/test_event_manager_fixed.cpp"
    if os.path.exists(event_test_file):
        event_test_sources = [
            f"{build_dir}/src/core/tests/test_event_manager_fixed.cpp",
            f"{build_dir}/src/core/event_manager.cpp",
        ]
        
        event_test_program = env_plugin.Program(
            target=f"{build_dir}/test_event_manager",
            source=event_test_sources
        )
        
        print(f"  - 事件管理器测试程序目标: {build_dir}/test_event_manager")
        print(f"  - 事件测试源文件数量: {len(event_test_sources)}")
    else:
        print("  - 警告: 事件管理器测试文件不存在")

    # 检查并编译事件池测试
    pool_test_file = "src/core/tests/test_event_pool_and_concurrency_simple.cpp"
    if os.path.exists(pool_test_file):
        pool_test_sources = [
            f"{build_dir}/src/core/tests/test_event_pool_and_concurrency_simple.cpp",
            f"{build_dir}/src/core/event_manager.cpp",
        ]
        
        pool_test_program = env_plugin.Program(
            target=f"{build_dir}/test_event_pool",
            source=pool_test_sources
        )
        
        print(f"  - 事件池测试程序目标: {build_dir}/test_event_pool")
        print(f"  - 事件池测试源文件数量: {len(pool_test_sources)}")
    else:
        print("  - 警告: 事件池测试文件不存在")

    # --- 2.5.1：Jolt Physics 編譯設定 ---
    # 添加 Jolt 特定的編譯器定義
    env_plugin.Append(
        CPPDEFINES=[
            "JPH_OBJECT_STREAM",  # 啟用對象序列化
            "JPH_DISABLE_TEMP_ALLOCATOR",  # 禁用臨時分配器（與 Godot 兼容）
            "JPH_DISABLE_CUSTOM_ALLOCATOR",  # 禁用自定義分配器（與 Godot 兼容）
        ]
    )

    # 根據編譯目標添加適當的定義
    if env_plugin.get("target") == "template_debug":
        env_plugin.Append(
            CPPDEFINES=[
                "JPH_ENABLE_ASSERTS",  # 調試版本啟用斷言
                "JPH_FLOATING_POINT_EXCEPTIONS_ENABLED",
            ]
        )
    
    ### [修改] 平台特定的 Jolt 优化设置
    if env_plugin["platform"] == "windows":
        # 为 Jolt 添加 Windows 特定的优化
        # /arch:SSE2 是一个非常安全的基础选项，几乎所有 x64 CPU 都支持
        # 如果主人的 CPU 支持，可以改为 /arch:AVX 或 /arch:AVX2 以获得更好性能
        env_plugin.Append(CXXFLAGS=["/arch:AVX2"])
        print("  - 已为 Windows 配置 Jolt SIMD 优化 (/arch:AVX2)。")
    elif env_plugin["platform"] == "macos":
        # 为 Jolt 添加 macOS 特定的优化（如果支援）
        try:
            env_plugin.Append(CXXFLAGS=["-msse4.1"])  # SIMD 優化
            print("  - 已为 macOS 配置 Jolt SIMD 优化 (-msse4.1)。")
        except:
            pass  # 如果不支援則跳過

    print("  - 已配置 Jolt Physics 編譯設定")

    if not plugin_sources:
        print("错误: 没有找到任何插件源文件！请检查路径。")
        Exit(1)
    print(f"  - 总共找到 {len(plugin_sources)} 个插件源文件。")

    # --- 2.6：定义最终的插件库目标 ---
    plugin_library = None
    if env_plugin["platform"] == "macos":
        framework_name = f"libgdextension_bridge.{env_plugin['platform']}.{env_plugin['target']}.framework"
        library_path = f"portal_demo_godot/bin/{framework_name}/libgdextension_bridge.{env_plugin['platform']}.{env_plugin['target']}"
        plugin_library = env_plugin.SharedLibrary(
            target=library_path, source=plugin_sources
        )
        print(f"  - macOS Framework 目标: {framework_name}")
    else:
        # 这个 else 分支对 Windows 和 Linux 都适用
        lib_name = (
            f"libgdextension_bridge{env_plugin['suffix']}{env_plugin['SHLIBSUFFIX']}"
        )
        library_path = f"portal_demo_godot/bin/{lib_name}"
        plugin_library = env_plugin.SharedLibrary(
            target=library_path, source=plugin_sources
        )
        print(f"  - 共享库目标: {library_path}")

    # ==============================================================================
    # 阶段三：定义最终构建目标和清理规则
    # ==============================================================================
    print("\n=== 阶段三：设置最终目标 ===")
    env_plugin.Depends(plugin_library, godot_cpp_library)
    print("  - 已设置插件对 godot-cpp 库的依赖。")

    # 將測試程序加入預設編譯目標
    default_targets = [plugin_library]
    if test_file_exists and 'test_program' in locals():
        default_targets.append(test_program)
        print("  - 测试程序已加入默认编译目标")
    
    # 添加其他测试程序到默认目标
    if 'event_test_program' in locals():
        default_targets.append(event_test_program)
        print("  - 事件管理器测试程序已加入默认编译目标")
    
    if 'pool_test_program' in locals():
        default_targets.append(pool_test_program)
        print("  - 事件池测试程序已加入默认编译目标")
    
    Default(default_targets)
    Alias("gdextension", plugin_library)
    # **-- [逻辑修正 V10] --**
    # 将要清理的目录用 Dir() 包装，使其成为 SCons 节点，确保被正确识别和删除。
    Clean(
        [plugin_library, godot_cpp_library],
        [Dir("portal_demo_godot/bin"), Dir(build_dir)],
    )
    print("\n=== 编译配置完成 ===")

elif not GetOption("clean"):
    # --- BUILD ERROR PATH ---
    print("\n错误: godot-cpp SConscript 没有返回任何构建目标。请检查子模块是否正确。")
    Exit(1)
else:
    # --- CLEAN PATH ---
    print("\n清理模式: 跳过编译配置，仅注册项目清理目标。")
    # **-- [逻辑修正 V10] --**
    # 同样，在清理模式下也要使用 Dir() 来确保目录被正确识别。
    Clean("gdextension", [Dir("portal_demo_godot/bin"), Dir("build")])
    print("\n=== 清理配置完成 ===")