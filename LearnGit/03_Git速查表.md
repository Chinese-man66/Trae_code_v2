# Git 速查表

> 一页纸掌握 Git

---

## 创建仓库

```bash
git init                          # 初始化
git clone url                     # 克隆
git clone url folder              # 克隆到目录
```

---

## 基本操作

```bash
git status                        # 状态
git add file                      # 添加
git add .                         # 添加全部
git commit -m "msg"               # 提交
git commit -am "msg"              # 添加并提交
```

---

## 查看信息

```bash
git diff                          # 差异
git diff --staged                 # 暂存区差异
git log                           # 历史
git log --oneline                 # 简洁历史
git show commit                   # 提交详情
git blame file                    # 文件 blame
```

---

## 撤销操作

```bash
git checkout -- file               # 撤销修改
git restore file                  # 撤销修改 (新)
git reset HEAD file               # 取消暂存
git reset --soft HEAD~1           # 撤销提交
git reset --hard HEAD~1           # 撤销提交(丢弃)
git revert HEAD                    # 撤销提交(新建)
```

---

## 分支操作

```bash
git branch                        # 列表
git branch name                   # 创建
git checkout -b name              # 创建并切换
git checkout name                 # 切换
git switch name                   # 切换 (新)
git branch -d name                # 删除
git merge name                    # 合并
git rebase main                   # 变基
```

---

## 远程操作

```bash
git remote -v                     # 远程列表
git remote add name url            # 添加远程
git fetch name                    # 获取
git pull name branch              # 拉取
git push name branch              # 推送
git push -u name branch           # 推送并追踪
git push name --delete branch     # 删除远程分支
```

---

## 储藏操作

```bash
git stash                         # 储藏
git stash save "msg"              # 储藏(说明)
git stash list                    # 列表
git stash pop                     # 恢复并删除
git stash apply                   # 恢复
git stash drop                    # 删除
git stash clear                   # 清空
```

---

## 标签操作

```bash
git tag                           # 列表
git tag name                      # 创建
git tag -a name -m "msg"          # 创建(附注)
git tag -d name                   # 删除本地
git push origin name              # 推送标签
git push origin --tags            # 推送全部
git push origin --delete name    # 删除远程
```

---

## 常用组合

```bash
# 放弃所有本地修改
git checkout -- . && git clean -fd

# 强制覆盖本地
git fetch origin && git reset --hard origin/main

# 同步远程仓库
git fetch origin && git rebase origin/main

# 查看文件历史
git log -p filename

# 查找 bug
git bisect start && git bisect bad && git bisect good v1.0.0
```

---

## 状态图

```
工作目录 → git add → 暂存区 → git commit → 本地仓库
                    ↑              ↓
                    ← git checkout --
```

---

> 打印出来贴墙上！
