#!/usr/bin/env python
# -*- coding: utf-8 -*-
"""
统一编译脚本 - Portal Demo 项目
支持编译 godot-cpp、core 代码和 gdextension 代码
"""
import os
import sys

# 确保 SCons 和 Python 版本
EnsureSConsVersion(4, 0)
EnsurePythonVersion(3, 8)

print("=== Portal Demo 项目编译系统 ===")
print(f"项目根目录: {Dir('#').abspath}")

# --- 第一步：加载 godot-cpp 环境 ---
# 从 godot-cpp 的 SConstruct 文件继承基础编译环境。
# 这是标准做法，它会根据命令行参数（如 platform, target, arch）自动配置好大部分编译选项。
print("\n=== 第一步：加载 godot-cpp 环境 ===")
env = SConscript("portal_demo_godot/gdextension/godot-cpp/SConstruct")
print("Godot-cpp 环境已加载。")

# --- 针对 macOS 的环境微调 ---
if env["platform"] == "macos":
    print("\n=== 正在为 macOS 环境进行微调 ===")
    
    # 确保链接到正确的 C++ 标准库。虽然通常是默认行为，但显式指定更安全。
    env.Append(LINKFLAGS=['-stdlib=libc++'])
    print("  - 已确保链接 -stdlib=libc++")
    
    # 这是一个在 macOS 上开发 GDExtension 时常用的链接器标志。
    # 它告诉链接器，如果在编译时找不到某些符号（比如 Godot 引擎自身的函数），
    # 不用报错，而是假设在运行时由主程序（Godot）提供。
    env.Append(LINKFLAGS=['-Wl,-undefined,dynamic_lookup'])
    print("  - 已添加 '-undefined,dynamic_lookup' 链接器标志")

# --- 第二步：配置项目编译环境 ---
print("\n=== 第二步：配置项目编译环境 ===")

# 显式设置 custom_api_file，这是 GDExtension 必需的
api_file = "portal_demo_godot/gdextension/extension_api.json"
if os.path.exists(api_file):
    env["custom_api_file"] = api_file
    print(f"使用 API 文件: {api_file}")
else:
    print(f"错误: API 文件不存在: {api_file}")
    Exit(1)

# 设置统一的 build 目录，将编译中间文件与源码分离
build_dir = "build"
env.VariantDir(build_dir, ".", duplicate=0)
print(f"构建目录设置为: {build_dir}")

# 添加项目所需的头文件搜索路径
env.Append(CPPPATH=[
    Dir("portal_demo_godot/gdextension/include"),
    Dir("core/include"),
])
print("项目头文件路径已设置。")

# --- 第三步：收集源文件 ---
print("\n=== 第三步：收集源文件 ===")
all_sources = Glob(f"{build_dir}/portal_demo_godot/gdextension/src/*.cpp") + \
              Glob(f"{build_dir}/core/src/*.cpp")

if not all_sources:
    print("错误: 没有找到任何源文件！请检查路径。")
    Exit(1)
print(f"总共找到 {len(all_sources)} 个源文件。")

# --- 第四步：配置目标库 ---
print("\n=== 第四步：配置目标库 ===")
library_path = ""
library = None

# 根据平台配置输出
if env["platform"] == "macos":
    framework_name = f"libgdextension_bridge.{env['platform']}.{env['target']}.framework"
    library_path = f"portal_demo_godot/bin/{framework_name}/libgdextension_bridge.{env['platform']}.{env['target']}"
    library = env.SharedLibrary(target=library_path, source=all_sources)
    print(f"macOS Framework 目标: {framework_name}")

elif env["platform"] == "ios":
    lib_name_suffix = "simulator.a" if env.get("ios_simulator", False) else "a"
    lib_name = f"libgdextension_bridge.{env['platform']}.{env['target']}.{lib_name_suffix}"
    library_path = f"portal_demo_godot/bin/{lib_name}"
    library = env.StaticLibrary(target=library_path, source=all_sources)
    print(f"iOS 静态库目标: {lib_name}")

else:
    lib_name = f"libgdextension_bridge{env['suffix']}{env['SHLIBSUFFIX']}"
    library_path = f"portal_demo_godot/bin/{lib_name}"
    library = env.SharedLibrary(target=library_path, source=all_sources)
    print(f"共享库目标: {lib_name}")

# --- 第五步：设置默认目标和别名 ---
Default(library)
env.Alias("gdextension", library)
env.Clean(".", ["portal_demo_godot/bin", build_dir])

print("\n=== 编译配置完成 ===")
print("使用示例:")
print("  scons platform=macos arch=x86_64     # 编译 Intel 架构")
print("  scons platform=macos arch=arm64      # 编译 Apple Silicon 架构")
print("  scons platform=macos arch=universal  # 编译通用架构 (推荐)")
print("  scons -c                             # 清理构建文件")

