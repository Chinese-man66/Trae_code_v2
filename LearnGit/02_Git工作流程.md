# Git 常用场景实战指南

> 本文件通过实际场景讲解 Git 的使用

---

## 场景 1：修改了文件，突然不想改了

```bash
git checkout -- filename.txt
git restore filename.txt          # Git 2.23+

git checkout -- .
git restore .
```

---

## 场景 2：文件 add 到暂存区了，想撤销

```bash
git reset HEAD filename.txt
git restore --staged filename.txt

git reset HEAD
git restore --staged .
```

---

## 场景 3：已经 commit 了，想撤销

```bash
git reset --soft HEAD~1     # 保留修改在暂存区
git reset --mixed HEAD~1    # 保留修改在工作区
git reset --hard HEAD~1     # 丢弃所有修改
```

---

## 场景 4：已经 push 了，想撤销

```bash
git revert HEAD
git revert abc1234
git push origin main
```

---

## 场景 5：想修改最后一次提交的说明

```bash
git commit --amend -m "新的提交说明"
git commit --amend --no-edit
```

---

## 场景 6：想合并多个提交为一个

```bash
git rebase -i HEAD~3
# 把要合并的提交改为 squash 或 s
```

---

## 场景 7：分支合并冲突

```bash
git status
# 手动解决冲突后
git add filename.txt
git commit

# 或放弃合并
git merge --abort

# 使用对方/自己的版本
git checkout --theirs filename.txt
git checkout --ours filename.txt
```

---

## 场景 8：想删除某个文件的历史

```bash
git filter-repo --path password.txt --invert-paths
git push --force --all
```

---

## 场景 9：误删了分支，想恢复

```bash
git reflog
git checkout -b branch-name HEAD@{N}
```

---

## 场景 10：想看某个文件的历史

```bash
git log filename.txt
git log -p filename.txt
git log --diff-filter=D -- filename.txt
```

---

## 场景 11：想找是哪个提交引入的 bug

```bash
git bisect start
git bisect bad
git bisect good v1.0.0
# 测试...
git bisect good  # 或 git bisect bad
git bisect reset
```

---

## 场景 12：想保存当前工作进度

```bash
git stash
git stash push -m "working on feature"
git stash list
git stash pop
```

---

## 场景 13：想查看两个分支的差异

```bash
git diff main..feature
git diff main..feature --stat
git diff main..feature -- filename.txt
```

---

## 场景 14：想回到某个历史版本

```bash
git checkout abc1234
git switch --detach abc1234
git checkout -b new-branch abc1234
```

---

## 场景 15：想强制覆盖远程仓库

```bash
git push --force origin main
git push -f origin main
```

---

## 场景 16：想从远程仓库获取最新代码，但不合并

```bash
git fetch origin
git branch -r
git diff main origin/main
```

---

## 场景 17：想清理未跟踪的文件

```bash
git clean -n        # 预览
git clean -f        # 删除文件
git clean -fd      # 包括目录
git clean -fdx     # 包括忽略的文件
```

---

## 场景 18：想给提交打标签

```bash
git tag v1.0.0
git tag -a v1.0.0 -m "版本 1.0.0"
git push origin v1.0.0
git push origin --tags
```

---

## 场景 19：多人协作开发流程

```bash
# 1. 克隆
git clone https://github.com/user/repo.git

# 2. 创建功能分支
git checkout -b feature-login

# 3. 开发并提交
git add .
git commit -m "add login feature"

# 4. 保持与主分支同步
git fetch origin
git rebase origin/main

# 5. 推送
git push -u origin feature-login

# 6. 创建 Pull Request

# 7. 合并后切换回主分支
git checkout main
git pull origin main
git branch -d feature-login
git push origin --delete feature-login
```

---

## 场景 20：修复 bug 流程

```bash
git checkout -b fix-login-bug
# 修复 bug...
git add .
git commit -m "fix: 修复登录页面崩溃问题"
git checkout main
git merge fix-login-bug
git push origin main
git branch -d fix-login-bug
```

---

## 场景 21：使用 cherry-pick 拣选提交

```bash
git cherry-pick abc1234
git cherry-pick abc1234 def5678
git cherry-pick abc1234..def9999
git cherry-pick -n abc1234   # 拣选但不提交
```

---

## 常见错误解决

### error: refusing to merge unrelated histories
```bash
git pull origin main --allow-unrelated-histories
```

### error: failed to push some refs
```bash
git pull --rebase origin main
git push origin main
```

### Merge conflict
```bash
# 打开冲突文件，手动解决
git add filename.txt
git commit
```

### 警告：LF will be replaced by CRLF
```bash
git config --global core.autocrlf true
```

---

## 一句话命令

| 场景 | 命令 |
|------|------|
| 放弃所有本地修改 | `git checkout -- .` |
| 撤销最近一次 commit | `git reset --soft HEAD~1` |
| 撤销最近一次 push | `git revert HEAD && git push` |
| 查看某个文件的修改历史 | `git log -p filename` |
| 查看两个分支的差异 | `git diff main..feature` |
| 查找 bug 引入的提交 | `git bisect start && git bisect bad && git bisect good v1.0.0` |
| 临时保存修改 | `git stash` |
| 强制覆盖本地 | `git fetch origin && git reset --hard origin/main` |
| 清理未跟踪文件 | `git clean -fd` |
| 删除远程分支 | `git push origin --delete branch-name` |
