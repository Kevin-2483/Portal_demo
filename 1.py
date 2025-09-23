#!/usr/bin/env python3
"""
查找并替换src/core中所有源文件中的输出代码（cout、cerr等）为调试宏
排除src/core/portal_core和src/core/tests目录
支持多行输出语句的处理
"""

import os
import re
import glob
from pathlib import Path

# 配置
SOURCE_DIR = "src/core"
EXCLUDE_DIRS = ["portal_core", "tests"]
# <--- 修改: 增加了新的配置文件到排除列表
EXCLUDE_FILES = ["debug_config.h", "portal_build_config.h", "portal_debug_logging.h"]
FILE_EXTENSIONS = [".cpp", ".h", ".hpp", ".cc", ".cxx"]

def should_exclude_file(file_path):
    """检查文件是否应该被排除"""
    path_parts = Path(file_path).parts
    file_name = Path(file_path).name
    
    if file_name in EXCLUDE_FILES:
        return True
    
    for exclude_dir in EXCLUDE_DIRS:
        if exclude_dir in path_parts:
            return True
            
    if file_name.endswith('.md'):
        return True
        
    return False

def find_source_files():
    """查找所有需要处理的源文件"""
    source_files = []
    
    for ext in FILE_EXTENSIONS:
        pattern = os.path.join(SOURCE_DIR, "**", f"*{ext}")
        files = glob.glob(pattern, recursive=True)
        
        for file_path in files:
            if not should_exclude_file(file_path):
                source_files.append(file_path)
    
    return sorted(source_files)

def process_file_content(content):
    """处理文件内容，替换输出语句"""
    replacements_made = []
    
    # 模式1: std::cout << ... << std::endl;
    pattern1 = r'std::cout\s*<<\s*([^;]+?)\s*<<\s*std::endl\s*;'
    # 先处理带endl的，避免被不带endl的模式错误匹配
    content, count = re.subn(pattern1, lambda m: f'PORTAL_DEBUG_LOG({m.group(1).strip()});', content, flags=re.DOTALL)
    if count > 0:
        replacements_made.append(('cout_endl', count))
    
    # 模式2: std::cerr << ... << std::endl;
    pattern2 = r'std::cerr\s*<<\s*([^;]+?)\s*<<\s*std::endl\s*;'
    content, count = re.subn(pattern2, lambda m: f'PORTAL_DEBUG_ERROR({m.group(1).strip()});', content, flags=re.DOTALL)
    if count > 0:
        replacements_made.append(('cerr_endl', count))
    
    # <--- 修改: 使用新的 _SIMPLE 宏
    # 模式3: std::cout << ... ; (不带endl)
    pattern3 = r'std::cout\s*<<\s*([^;]+?)\s*;'
    content, count = re.subn(pattern3, lambda m: f'PORTAL_DEBUG_LOG_SIMPLE({m.group(1).strip()});', content, flags=re.DOTALL)
    if count > 0:
        replacements_made.append(('cout_simple', count))

    # <--- 修改: 使用新的 _SIMPLE 宏
    # 模式4: std::cerr << ... ; (不带endl)
    pattern4 = r'std::cerr\s*<<\s*([^;]+?)\s*;'
    content, count = re.subn(pattern4, lambda m: f'PORTAL_DEBUG_ERROR_SIMPLE({m.group(1).strip()});', content, flags=re.DOTALL)
    if count > 0:
        replacements_made.append(('cerr_simple', count))
        
    return content, replacements_made

def process_file(file_path):
    """处理单个文件"""
    try:
        with open(file_path, 'r', encoding='utf-8') as f:
            content = f.read()
        
        original_content = content
        new_content, replacements_made = process_file_content(content)
        
        if new_content != original_content:
            with open(file_path, 'w', encoding='utf-8') as f:
                f.write(new_content)
            return replacements_made
        
        return []
        
    except Exception as e:
        print(f"处理文件 {file_path} 时出错: {e}")
        return []

# <--- 修改: 整个函数逻辑更新，只包含 portal_debug_logging.h
def add_debug_include(file_path):
    """为修改过的文件添加 portal_debug_logging.h 包含"""
    try:
        with open(file_path, 'r', encoding='utf-8') as f:
            content = f.read()
            
        include_to_add = "portal_debug_logging.h"

        # 检查是否已经包含了
        if f'#include "{include_to_add}"' in content:
            return False
            
        # 计算相对路径
        file_dir = os.path.dirname(file_path)
        debug_dir = os.path.join(SOURCE_DIR, "debug")
        
        # 如果文件就在debug目录下，路径就是文件名本身
        if os.path.normpath(file_dir) == os.path.normpath(debug_dir):
            include_path = include_to_add
        else:
            rel_path = os.path.relpath(debug_dir, file_dir)
            include_path = os.path.join(rel_path, include_to_add).replace('\\', '/')
        
        include_line = f'#include "{include_path}"'
        
        lines = content.split('\n')
        insert_pos = 0
        
        # 寻找最后一个#include语句的位置
        for i, line in enumerate(lines):
            if line.strip().startswith('#include'):
                insert_pos = i + 1
        
        lines.insert(insert_pos, include_line)
        
        with open(file_path, 'w', encoding='utf-8') as f:
            f.write('\n'.join(lines))
        
        return True
        
    except Exception as e:
        print(f"添加include到文件 {file_path} 时出错: {e}")
        return False

def main():
    """主函数"""
    print("开始查找和替换输出语句...")
    
    source_files = find_source_files()
    print(f"找到 {len(source_files)} 个源文件需要处理")
    
    total_replacements = 0
    modified_files = []
    
    for file_path in source_files:
        print(f"处理文件: {file_path}")
        replacements = process_file(file_path)
        
        if replacements:
            modified_files.append(file_path)
            file_replacement_count = sum(count for _, count in replacements)
            total_replacements += file_replacement_count
            
            print(f"  - 替换了 {file_replacement_count} 个输出语句")
            summary = {}
            for r_type, count in replacements:
                summary[r_type] = summary.get(r_type, 0) + count
            for r_type, count in summary.items():
                print(f"    * {r_type}: {count} 个")
            
            # 添加debug_config.h包含
            if add_debug_include(file_path):
                print(f"  - 添加了 portal_debug_logging.h 包含")
    
    print(f"\n处理完成!")
    print(f"修改了 {len(modified_files)} 个文件")
    print(f"总共替换了 {total_replacements} 个输出语句")
    
    if modified_files:
        print("\n修改的文件列表:")
        for file_path in sorted(modified_files):
            print(f"  - {file_path}")

if __name__ == "__main__":
    main()