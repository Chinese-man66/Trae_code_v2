# Trae_code

> C++ 学习项目与 Git 教程仓库

本仓库包含 C++ 高级特性学习代码和 Git 完整教程。

---

## 📁 项目结构

```
Trae_code/
├── LearnGit/              # Git 学习资料
│   ├── 01_Git指令详解.md  # 完整 Git 命令参考
│   ├── 02_Git工作流程.md  # 常见场景实战指南
│   ├── 03_Git速查表.md    # 一页纸速查表
│   └── .gitignore        # 各语言 gitignore 模板
│
├── CppMastery/           # C++ 高级特性专题
│   ├── topics/           # 专题代码
│   │   ├── 01_template_metaprogramming.cpp  # 模板元编程
│   │   ├── 02_move_semantics.cpp            # 移动语义
│   │   ├── 03_smart_pointers.cpp            # 智能指针
│   │   ├── 04_virtual_functions.cpp         # 虚函数与多态
│   │   ├── 05_memory_management.cpp         # 内存管理
│   │   └── final_project.cpp                # 综合项目
│   │
│   └── examples/         # 示例代码
│       ├── stl_containers.cpp
│       ├── templates.cpp
│       ├── smart_pointers.cpp
│       ├── concurrency.cpp
│       └── file_io.cpp
│
└── mydb/                # MyDB 简易数据库项目
    ├── stage1_basics/   # 基础阶段 - CRUD
    ├── stage2_intermediate/ # 进阶阶段 - 索引与查询
    └── stage3_advanced/ # 高级阶段 - 事务与并发
```

---

## 🖥️ C++ 高级特性专题

### 专题列表

| 文件 | 专题 | 核心知识点 |
|------|------|------------|
| [01_template_metaprogramming.cpp](CppMastery/topics/01_template_metaprogramming.cpp) | 模板元编程 | 模板特化、类型萃取、SFINAE、编译时计算 |
| [02_move_semantics.cpp](CppMastery/topics/02_move_semantics.cpp) | 移动语义 | 左值/右值、移动构造/赋值、std::move、完美转发 |
| [03_smart_pointers.cpp](CppMastery/topics/03_smart_pointers.cpp) | 智能指针 | unique_ptr、shared_ptr、weak_ptr、循环引用 |
| [04_virtual_functions.cpp](CppMastery/topics/04_virtual_functions.cpp) | 虚函数表 | vtable 原理、多态、override/final、纯虚函数 |
| [05_memory_management.cpp](CppMastery/topics/05_memory_management.cpp) | 内存管理 | new/delete、placement new、内存池、RAII |
| [final_project.cpp](CppMastery/topics/final_project.cpp) | 综合项目 | 事件系统，整合所有知识点 |

### 编译运行

```bash
cd CppMastery/topics
mkdir build && cd build
cmake ..
make

# 运行各专题
./topic_01_template
./topic_02_move
./topic_03_smart_ptr
./topic_04_virtual
./topic_05_memory

# 运行综合项目
./cpp_final_project
```

---

## 🗄️ MyDB 数据库项目

### 项目阶段

| 阶段 | 主题 | 核心概念 |
|------|------|----------|
| **Stage 1** | 基本存储与 CRUD | Cell 变体、Record 序列化、Table 文件存储 |
| **Stage 2** | 索引与查询 | 哈希/B+树索引、游标、执行计划、SQL 解析 |
| **Stage 3** | 事务与并发 | ACID、2PL 锁、缓冲池、WAL、崩溃恢复 |

---

## 📚 Git 学习教程

### 文件列表

| 文件 | 说明 |
|------|------|
| [LearnGit/01_Git指令详解.md](LearnGit/01_Git指令详解.md) | 完整的 Git 命令参考手册 |
| [LearnGit/02_Git工作流程.md](LearnGit/02_Git工作流程.md) | 25+ 常见场景实战指南 |
| [LearnGit/03_Git速查表.md](LearnGit/03_Git速查表.md) | 一页纸速查表 |

### 快速开始

```bash
# 初始化仓库
git init

# 克隆仓库
git clone https://github.com/Chinese-man66/Trae_code_v2.git

# 基本工作流程
git status
git add .
git commit -m "说明"
git push origin main
```

### 常用命令速查

| 命令 | 说明 |
|------|------|
| `git init` | 初始化仓库 |
| `git clone` | 克隆仓库 |
| `git add` | 添加文件到暂存区 |
| `git commit` | 提交更改 |
| `git push` | 推送到远程 |
| `git pull` | 拉取远程更新 |
| `git status` | 查看状态 |
| `git diff` | 查看差异 |
| `git log` | 查看历史 |
| `git branch` | 分支操作 |
| `git checkout` | 切换分支 |
| `git merge` | 合并分支 |
| `git reset` | 撤销操作 |
| `git stash` | 储藏更改 |

---

## 🔗 相关链接

### 学习资源

- [Pro Git 电子书](https://git-scm.com/book/zh/v2)
- [Git 官方文档](https://git-scm.com/docs)
- [Oh Shit, Git!?!](https://ohshitgit.com/)
- [Git 交互式学习](https://learngitbranching.js.org/)
- [C++ Reference](https://en.cppreference.com/)
- [ISO C++](https://isocpp.org/)

### 工具

- [GitHub](https://github.com/)
- [GitLab](https://gitlab.com/)
- [Bitbucket](https://bitbucket.org/)

---

## 📝 许可证

MIT License

---

> 学习编程最好的方式就是动手实践！
