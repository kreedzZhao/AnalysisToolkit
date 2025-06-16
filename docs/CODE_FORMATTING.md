# 代码格式化指南

## 概述

本项目使用 `clang-format` 进行 C++ 代码格式化，通过 `pre-commit` hooks 实现自动化格式化。

## 工具配置

### 1. clang-format 配置

项目根目录的 `.clang-format` 文件定义了代码格式化规则：
- 基于 Google 风格，进行了适当定制
- 4 空格缩进
- 最大行长度：100 字符
- 指针和引用左对齐
- 自动排序 #include 语句

### 2. pre-commit hooks

`.pre-commit-config.yaml` 配置了以下自动化检查：
- 移除行尾空白
- 确保文件以换行符结尾
- 检查 YAML/JSON 语法
- 检查合并冲突标记
- C++ 代码格式化
- CMake 文件格式化

## 使用方法

### 自动格式化（推荐）

每次 `git commit` 时，pre-commit hooks 会自动：
1. 检查并修复代码格式
2. 如果有修改，commit 会被阻止
3. 你需要重新 `git add` 修改后的文件并再次提交

```bash
# 正常的提交流程
git add .
git commit -m "feat: add new feature"
# 如果格式化被修改，会显示错误，然后：
git add .
git commit -m "feat: add new feature"  # 再次提交
```

### 手动格式化

#### 格式化所有文件
```bash
# 使用提供的脚本
./scripts/format-code.sh

# 或者使用 pre-commit
pre-commit run --all-files
```

#### 格式化单个文件
```bash
clang-format -i path/to/your/file.cpp
```

#### 格式化特定目录
```bash
find src/ -name "*.cpp" -o -name "*.h" | xargs clang-format -i
```

### 检查代码格式（不修改）
```bash
# 检查单个文件
clang-format --dry-run --Werror path/to/your/file.cpp

# 检查所有文件
find . -name "*.cpp" -o -name "*.h" | xargs clang-format --dry-run --Werror
```

## IDE 集成

### VS Code
安装 `C/C++` 插件，并在设置中启用：
```json
{
    "C_Cpp.clang_format_style": "file",
    "editor.formatOnSave": true
}
```

### CLion/IntelliJ
1. 打开 Settings → Editor → Code Style → C/C++
2. 选择 "Set from..." → "clang-format"
3. 启用 "Format code on save"

### Vim/Neovim
安装 `vim-clang-format` 插件：
```vim
" 在保存时自动格式化
autocmd BufWritePre *.cpp,*.h,*.hpp ClangFormat
```

## 常见问题

### Q: pre-commit hooks 失败了怎么办？
A: 查看错误信息，通常是格式化修改了文件。重新 `git add` 修改后的文件并再次提交。

### Q: 如何跳过 pre-commit 检查？
A: 使用 `--no-verify` 参数（不推荐）：
```bash
git commit --no-verify -m "commit message"
```

### Q: 如何更新 pre-commit hooks？
A: 运行更新命令：
```bash
pre-commit autoupdate
```

### Q: 某些文件不想被格式化怎么办？
A: 在文件开头添加注释：
```cpp
// clang-format off
// your code here
// clang-format on
```

### Q: 如何自定义格式化规则？
A: 修改根目录的 `.clang-format` 文件。参考 [clang-format 文档](https://clang.llvm.org/docs/ClangFormatStyleOptions.html)。

## 团队协作

1. **新成员入职**：
   ```bash
   git clone <repository>
   cd <project>
   pip install pre-commit
   pre-commit install
   ```

2. **格式化历史代码**：
   ```bash
   ./scripts/format-code.sh
   git add .
   git commit -m "style: format existing code"
   ```

3. **保持一致性**：所有团队成员应该使用相同的 `.clang-format` 配置，避免格式化冲突。

## 相关链接

- [clang-format 文档](https://clang.llvm.org/docs/ClangFormat.html)
- [pre-commit 文档](https://pre-commit.com/)
- [Google C++ 风格指南](https://google.github.io/styleguide/cppguide.html)
