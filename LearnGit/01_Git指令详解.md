# Git 完整学习指南

> 本文件详细记录 Git 常用指令和使用场景

---

## 目录

1. [基础配置](#1-基础配置)
2. [创建仓库](#2-创建仓库)
3. [基本操作](#3-基本操作)
4. [分支管理](#4-分支管理)
5. [远程仓库](#5-远程仓库)
6. [撤销操作](#6-撤销操作)
7. [日志与历史](#7-日志与历史)
8. [标签管理](#8-标签管理)
9. [储藏操作](#9-储藏操作)
10. [高级技巧](#10-高级技巧)
11. [常见问题](#11-常见问题)

---

## 1. 基础配置

### 设置用户信息
```bash
# 全局配置（所有仓库）
git config --global user.name "Your Name"
git config --global user.email "your@email.com"

# 当前仓库配置（仅当前仓库）
git config user.name "Your Name"
git config user.email "your@email.com"
```

### 查看配置
```bash
git config --list
git config user.name
git config --global --list
```

### 配置别名
```bash
git config --global alias.st status
git config --global alias.co checkout
git config --global alias.br branch
git config --global alias.ci commit
git config --global alias.df diff
git config --global alias.lg "log --color --graph --pretty=format:'%Cred%h%Creset -%C(yellow)%d%Creset %s %Cgreen(%cr) %C(bold blue)<%an>%Creset' --abbrev-commit"
```

### SSH 密钥配置
```bash
ssh-keygen -t ed25519 -C "your@email.com"
cat ~/.ssh/id_ed25519.pub
ssh-add ~/.ssh/id_ed25519
ssh -T git@github.com
```

---

## 2. 创建仓库

### 初始化本地仓库
```bash
git init
git init my-project
git init --bare my-project.git
```

### 克隆远程仓库
```bash
git clone https://github.com/user/repo.git
git clone https://github.com/user/repo.git my-folder
git clone git@github.com:user/repo.git
git clone --depth 1 https://github.com/user/repo.git
git clone -b develop https://github.com/user/repo.git
```

### 从已有代码创建仓库
```bash
git init
git add .
git commit -m "Initial commit"
git remote add origin https://github.com/user/repo.git
git push -u origin main
```

---

## 3. 基本操作

### 文件状态
```
工作目录 → git add → 暂存区 → git commit → 本地仓库
```

### 添加文件
```bash
git add filename.txt
git add .
git add *.txt
git add docs/
git add -i
git add -u
```

### 提交文件
```bash
git commit -m "提交说明"
git commit -am "提交说明"    # 只对已跟踪文件
git commit --amend -m "新说明"
git commit --amend --no-edit
```

### 查看状态
```bash
git status
git status -s
git status --short
```

### 查看差异
```bash
git diff                    # 工作区 vs 暂存区
git diff --cached          # 暂存区 vs 最后提交
git diff HEAD              # 工作区 vs 最后提交
git diff branch1 branch2   # 分支差异
git diff commit1 commit2   # 提交差异
git diff --stat           # 统计差异
```

---

## 4. 分支管理

### 基本分支操作
```bash
git branch              # 本地分支
git branch -r           # 远程分支
git branch -a           # 所有分支
git branch -v           # 显示最后一次提交
git branch -vv          # 显示上游分支

git branch feature-login           # 创建分支
git checkout -b feature-login     # 创建并切换
git switch -c feature-login      # 创建并切换 (Git 2.23+)

git checkout feature-login
git switch feature-login

git branch -d feature-login      # 删除已合并分支
git branch -D feature-login       # 强制删除
git push origin --delete feature-login
```

### 合并分支
```bash
git merge feature-login
git rebase main
git merge --squash feature-login
git rebase --abort
git rebase --continue
```

---

## 5. 远程仓库

### 远程仓库操作
```bash
git remote -v
git remote add origin https://github.com/user/repo.git
git remote rename origin old-origin
git remote set-url origin new-url
git remote show origin
git remote remove origin
```

### 拉取与推送
```bash
git pull origin main
git fetch origin
git push origin main
git push -u origin feature-login
git push origin --delete feature-login
git push origin --tags
git push -f origin main    # 强制推送（谨慎！）
```

---

## 6. 撤销操作

### 撤销工作区修改
```bash
git checkout -- filename.txt
git restore filename.txt           # Git 2.23+
git checkout -- .
git restore .
```

### 撤销暂存
```bash
git reset HEAD filename.txt
git restore --staged filename.txt
git reset HEAD
git restore --staged .
```

### 撤销提交
```bash
git reset --soft HEAD~1    # 保留修改在暂存区
git reset --mixed HEAD~1   # 保留修改在工作区（默认）
git reset --hard HEAD~1    # 丢弃所有修改
git revert HEAD            # 创建撤销提交
```

---

## 7. 日志与历史

### 查看日志
```bash
git log
git log --oneline
git log -p
git log --stat
git log -n 5
git log --author="John"
git log --since="2024-01-01"
git log --grep="bug"
git log -- filename.txt
git log --graph --oneline --all
```

### 查看特定提交
```bash
git show commit-id
git show commit-id:filename.txt
git log -p filename.txt
git log --follow filename.txt
git blame filename.txt
```

---

## 8. 标签管理

```bash
git tag v1.0.0
git tag -a v1.0.0 -m "版本 1.0.0"
git tag -a v0.9.0 commit-id -m "早期版本"
git tag
git tag -l "v1.*"
git show v1.0.0
git tag -d v1.0.0
git push origin v1.0.0
git push origin --tags
```

---

## 9. 储藏操作

```bash
git stash
git stash save "修改说明"
git stash push -m "修改说明"
git stash -u              # 包括未跟踪文件
git stash list
git stash pop
git stash pop stash@{0}
git stash apply
git stash drop
git stash clear
git stash branch new-branch stash@{0}
```

---

## 10. 高级技巧

### 子模块
```bash
git submodule add https://github.com/user/repo.git path/to/folder
git clone --recurse-submodules https://github.com/user/repo.git
git submodule init
git submodule update
git submodule update --recursive
```

### Bisect（二分查找 bug）
```bash
git bisect start
git bisect bad
git bisect good v1.0.0
git bisect good   # 或 git bisect bad
git bisect reset
```

### 工作区
```bash
git worktree add ../my-project-feature feature-branch
git worktree list
git worktree remove ../my-project-feature
```

---

## 11. 常见问题

### 撤销 merge
```bash
git reset --hard HEAD~1
git merge --abort
```

### push 被拒绝
```bash
git pull origin main
# 或
git pull --rebase origin main
```

### 强制覆盖本地
```bash
git fetch origin
git reset --hard origin/main
```

### 误删分支恢复
```bash
git reflog
git checkout -b branch-name HEAD@{N}
```

---

## 常用命令速查表

| 操作 | 命令 |
|------|------|
| 创建仓库 | `git init`, `git clone` |
| 添加文件 | `git add .`, `git add file` |
| 提交 | `git commit -m "msg"` |
| 查看状态 | `git status`, `git status -s` |
| 查看差异 | `git diff`, `git diff --staged` |
| 查看日志 | `git log`, `git log --oneline` |
| 创建分支 | `git branch name`, `git checkout -b name` |
| 切换分支 | `git checkout name`, `git switch name` |
| 合并分支 | `git merge name`, `git rebase name` |
| 拉取 | `git pull`, `git fetch` |
| 推送 | `git push`, `git push -u origin name` |
| 撤销修改 | `git checkout -- file`, `git reset HEAD file` |
| 撤销提交 | `git reset --soft HEAD~1`, `git revert HEAD` |
| 储藏 | `git stash`, `git stash pop` |
| 标签 | `git tag name`, `git tag -a name -m "msg"` |

---

## 参考资源

- [Pro Git Book](https://git-scm.com/book/zh/v2)
- [Git 官网](https://git-scm.com/)
- [Oh Shit, Git!?!](https://ohshitgit.com/)
