# -*- coding: utf-8 -*-
"""
项目代码合并脚本
将分散的源代码文件整合成一个结构化的文本文件，用于上传到 Gemini/AI 平台
"""

import os
from pathlib import Path
from datetime import datetime

# ==================== 配置区域 ====================

# 项目根目录（脚本所在目录）
PROJECT_ROOT = Path(__file__).parent

# 需要合并的文件类型
INCLUDE_EXTENSIONS = {
    '.cpp', '.cc', '.c',           # C/C++ 源文件
    '.h', '.hpp', '.hxx',          # 头文件
    '.conf', '.cfg', '.ini',       # 配置文件
    '.html', '.css', '.js',        # 前端文件
    '.md',                         # 文档（可选）
}

# 需要单独包含的特殊文件名
INCLUDE_FILES = {
    'Makefile',
    'CMakeLists.txt',
    'README.md',
    'PROJECT_PLAN.md',
}

# 需要忽略的目录
IGNORE_DIRS = {
    '.git', '.svn',                # 版本控制
    '.claude', '.vscode', '.idea', # IDE 配置
    'build', 'bin', 'obj',         # 编译产物
    'debug', 'release',            # 构建目录
    'target', 'out',               # 输出目录
    'node_modules',                # 依赖
    'data',                        # 运行时数据（索引文件等）
    '__pycache__',                 # Python 缓存
}

# 需要忽略的文件
IGNORE_FILES = {
    'merge_project.py',            # 本脚本自身
    'project_codebase.txt',        # 输出文件
    '代码总结.txt',                 # 输出文件
    '新建 文本文档.txt',            # 临时文件
}

# 输出文件名
OUTPUT_FILE = '代码总结.txt'

# ==================== 核心逻辑 ====================

def is_text_file(filepath: Path) -> bool:
    """检测是否为文本文件（防止读取二进制文件）"""
    try:
        with open(filepath, 'r', encoding='utf-8') as f:
            f.read(2048)
        return True
    except (UnicodeDecodeError, IOError):
        return False

def should_include_file(filepath: Path) -> bool:
    """判断文件是否应该被包含"""
    filename = filepath.name

    # 检查是否在忽略列表
    if filename in IGNORE_FILES:
        return False

    # 检查是否为特殊文件
    if filename in INCLUDE_FILES:
        return True

    # 检查文件扩展名
    suffix = filepath.suffix.lower()
    if suffix in INCLUDE_EXTENSIONS:
        return True

    return False

def get_file_type(filepath: Path) -> str:
    """获取文件类型描述（用于语法高亮提示）"""
    suffix = filepath.suffix.lower()
    type_map = {
        '.cpp': 'cpp', '.cc': 'cpp', '.c': 'c',
        '.h': 'cpp', '.hpp': 'cpp', '.hxx': 'cpp',
        '.html': 'html', '.css': 'css', '.js': 'javascript',
        '.conf': 'ini', '.cfg': 'ini', '.ini': 'ini',
        '.md': 'markdown', '.json': 'json',
    }
    if filepath.name == 'Makefile':
        return 'makefile'
    if filepath.name == 'CMakeLists.txt':
        return 'cmake'
    return type_map.get(suffix, 'text')

def collect_files(root_path: Path) -> list:
    """收集所有需要合并的文件"""
    files = []

    for path in root_path.rglob('*'):
        # 跳过目录
        if path.is_dir():
            continue

        # 检查是否在忽略的目录中
        relative_parts = path.relative_to(root_path).parts
        if any(part in IGNORE_DIRS for part in relative_parts):
            continue

        # 检查是否应该包含
        if should_include_file(path) and is_text_file(path):
            files.append(path)

    # 按路径排序，保证输出顺序一致
    return sorted(files, key=lambda p: (
        # 排序优先级：头文件 > 源文件 > 其他
        0 if p.suffix in {'.h', '.hpp'} else
        1 if p.suffix in {'.cpp', '.cc', '.c'} else
        2,
        str(p)
    ))

def merge_files(root_path: Path, output_path: Path):
    """合并所有文件到一个输出文件"""
    files = collect_files(root_path)

    if not files:
        print("❌ 没有找到需要合并的文件！")
        return

    with open(output_path, 'w', encoding='utf-8') as outfile:
        # 写入文件头部说明
        outfile.write("=" * 80 + "\n")
        outfile.write("PROJECT CODEBASE - MERGED SOURCE FILES\n")
        outfile.write("=" * 80 + "\n")
        outfile.write(f"Generated: {datetime.now().strftime('%Y-%m-%d %H:%M:%S')}\n")
        outfile.write(f"Project: {root_path.name}\n")
        outfile.write(f"Total Files: {len(files)}\n")
        outfile.write("\n")
        outfile.write("READING GUIDE:\n")
        outfile.write("- Each file section starts with a header containing file name and path\n")
        outfile.write("- Files are separated by '=' lines\n")
        outfile.write("- Path: shows the relative path from project root\n")
        outfile.write("- Type: indicates the file type for syntax reference\n")
        outfile.write("\n")

        # 写入文件列表索引
        outfile.write("-" * 80 + "\n")
        outfile.write("FILE INDEX:\n")
        outfile.write("-" * 80 + "\n")
        for i, filepath in enumerate(files, 1):
            relative_path = filepath.relative_to(root_path)
            outfile.write(f"  {i:2d}. {relative_path}\n")
        outfile.write("\n")

        # 写入每个文件内容
        for filepath in files:
            relative_path = filepath.relative_to(root_path)
            file_type = get_file_type(filepath)

            print(f"  Adding: {relative_path}")

            # 写入文件头
            outfile.write("=" * 80 + "\n")
            outfile.write(f"File: {filepath.name}\n")
            outfile.write(f"Path: {relative_path}\n")
            outfile.write(f"Type: {file_type}\n")
            outfile.write("=" * 80 + "\n")

            # 写入文件内容
            try:
                with open(filepath, 'r', encoding='utf-8') as infile:
                    content = infile.read()
                    outfile.write(content)
                    # 确保文件末尾有换行
                    if not content.endswith('\n'):
                        outfile.write('\n')
                    outfile.write('\n')  # 文件间空行
            except Exception as e:
                outfile.write(f"[ERROR: Could not read file - {e}]\n\n")
                print(f"  ⚠️  Error reading {filepath}: {e}")

    # 统计信息
    output_size = output_path.stat().st_size
    size_kb = output_size / 1024
    size_mb = size_kb / 1024

    print("\n" + "=" * 50)
    print(f"✅ 合并完成！")
    print(f"   输出文件: {output_path.name}")
    print(f"   文件数量: {len(files)} 个")
    print(f"   文件大小: {size_kb:.1f} KB ({size_mb:.2f} MB)")
    print("=" * 50)

    if size_mb > 5:
        print("\n⚠️  警告：文件较大，可能影响 AI 响应速度")
        print("   建议按模块拆分或精简内容")

def main():
    print("=" * 50)
    print("🔧 项目代码合并工具")
    print("=" * 50)
    print(f"项目目录: {PROJECT_ROOT}")
    print(f"输出文件: {OUTPUT_FILE}")
    print("-" * 50)
    print("正在收集文件...\n")

    output_path = PROJECT_ROOT / OUTPUT_FILE
    merge_files(PROJECT_ROOT, output_path)

if __name__ == "__main__":
    main()
