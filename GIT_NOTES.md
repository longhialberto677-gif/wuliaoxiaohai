# Git 速查笔记

> 这份笔记是给"正在学 git"的自己看的，配合本工程的真实操作写成。
> 每一步都对应本仓库里实际发生过的事，不是抽象教程。

---

## 一、先理解一件事：三个"区域"

刚学 git 最容易懵的是"为什么改完文件还要 add 一次"。因为 git 把内容分成三个区域：

```
   工作区                    暂存区                  本地仓库              远端(GitHub)
  (你的文件夹)              (staging)               (.git 里)             (origin)
      │                        │                       │                     │
      │   git add              │     git commit        │    git push         │
      ├───────────────────────►├──────────────────────►├────────────────────►│
      │                        │                       │                     │
      │◄───────────────────────┴───────────────────────┤                     │
      │         git checkout / git restore             │                     │
      │                                                │◄────────────────────┤
      │                                                │    git fetch/pull   │
```

- **工作区**：你拿 Keil 编辑的那些 `.c` / `.h` 文件，改了就变了
- **暂存区**：一个"待提交清单"。`git add` 就是往这张清单上写名字
- **本地仓库**：`git commit` 把清单固化成一个历史节点，有哈希、有说明、不可篡改

> **为什么要有暂存区？** 因为它让你能**只提交一部分改动**。
> 比如你同时改了 FOC 算法和 LCD 显示，可以先把算法 `add` 进去提交一次，
> 再 `add` 显示代码提交第二次，历史就清清楚楚。没有暂存区就做不到。

---

## 二、日常最常用的 6 条命令

```bash
git status              # 看现在是什么状态（最常用，不知道干啥时先敲它）
git diff                # 看工作区具体改了哪几行（还没 add 的）
git diff --cached       # 看暂存区里准备了什么（已 add 的）
git add <文件>          # 把改动放进暂存区；git add -A 表示全部
git commit -m "说明"    # 把暂存区固化成一次提交
git log --oneline       # 看提交历史（一行一条，清爽）
```

**改完代码的标准流程**，就四步：

```bash
git status                       # 1. 看看改了啥
git add -A                       # 2. 全部加入暂存区
git commit -m "加入电流环PI"      # 3. 提交
git push                         # 4. 推到 GitHub
```

注意 `git push` 后面不用再写 `origin main` 了 —— 因为首次推送时用了 `-u`
（`git push -u origin main`），git 已经记住"本地 main 对应 origin 的 main"。

---

## 三、`.gitignore` 的坑（本工程真实踩过）

本工程的 `.gitignore` 排除了 Keil 编译产物。这里有两个我实际踩到的坑：

### 坑 1：`#` 只有写在行首才是注释

```gitignore
# 这样是对的
MDK-ARM/*.uvoptx

MDK-ARM/*.uvoptx   # 窗口布局文件      ← 错的！
```

第二行**不会**忽略 `.uvoptx`。因为 git 把 `#` 后面整串（含空格）当成了
**文件名的一部分**，实际在找"以 `# 窗口布局文件` 结尾的文件"，永远匹配不上。

结果就是：我一开始漏掉了一个 177 KB 的 `myself_foc_g431.uvguix.18236`
（窗口布局文件，还绑着本机用户名）。

### 坑 2：`gitignore` 对**已经 add 过**的文件无效

这也是我踩到的：`startup_stm32g431xx.lst` 我先 `add` 了，后来才往
`.gitignore` 里加规则，结果它**照样被提交**。

解决办法：先从索引里撤下来（**不会删你磁盘上的文件**）：

```bash
git rm --cached 文件名
git commit -m "移除误提交的编译产物"
```

### 验收习惯

改完 `.gitignore`，一定要用这条命令检查"到底哪些被忽略了"：

```bash
git status --ignored
```

输出里 `!!` 开头的就是被忽略的。**看一眼这个列表**，就能确认
该忽略的都忽略了、不该忽略的没被误伤。

### 判断标准：什么该提交？

| 提交 | 不提交 |
| --- | --- |
| 你手写的源码（`myapp/`、`Core/`） | 编译产物（`.o` `.axf` `.hex` `.lst`） |
| CubeMX 配置 `myself_foc_g431.ioc` | `.mxproject`（CubeMX 自动生成） |
| Keil 工程文件 `.uvprojx` | 窗口布局 `.uvguix.*`、`.uvoptx` |
| `README.md`、`.gitignore` | 大体积第三方参考资料、本机路径相关配置 |

一句话判断法：**"这个文件删掉后，能不能靠其他文件重新生成出来？"**
能重新生成的（编译产物、CubeMX 生成的中间文件），就不该进版本库。

---

## 四、本工程的推送是怎么成功的（认证机制）

这次推送踩了一个真实的坑，值得记下来：

1. 你这台机器开着系统代理（Clash Verge，端口 `127.0.0.1:7897`），
   但 **git 默认不走系统代理**，它会直连。直连 GitHub 被重置 →
   报 `Recv failure: Connection was reset`
2. 解决办法是显式告诉 git 走代理（这条已写进本仓库的 `.git/config`）：

   ```bash
   git config --local http.proxy  http://127.0.0.1:7897
   git config --local https.proxy http://127.0.0.1:7897
   ```

   想取消：`git config --local --unset http.proxy`（https 同理）

3. 认证由 **Git Credential Manager**（git 自带）负责，
   首次推送会弹浏览器让你登录授权。凭据存在 Windows 凭据管理器里，
   **不会**写进仓库文件，所以不用担心泄漏。

> ⚠️ 如果哪天换了代理端口或关了代理，推送报连接错误，
> 记得把上面两条 `http.proxy` 改掉或删掉。

---

## 五、提交信息怎么写

现在看 `git log --oneline` 能看到：

```
5809181 初始提交：STM32G431 FOC 学习工程源码
```

**一个提交只做一件事**，说明写"做了什么"而不是"改了文件"：

| 好 | 差 |
| --- | --- |
| `加入电流环 PI 控制器` | `更新` |
| `修复 SVPWM 扇区边界溢出` | `改了一下` |
| `把 FOC 频率从 10k 提到 20k` | `修改代码` |

对 FOC 这种要反复调参的工程，好的提交信息特别值钱 ——
当你发现"改了 PI 参数后电机开始抖"，能靠 `git log` 找到是哪个提交引入的，
甚至用 `git diff HEAD~1` 直接看出那次改了哪几行。

---

## 六、后悔药（早晚用得上）

```bash
git restore <文件>          # 丢弃工作区改动，恢复成上次提交的样子（危险！改动会没）
git restore --staged <文件> # 只把文件从暂存区拿出来，改动保留
git log --oneline --graph   # 图形化看历史
git show <哈希>             # 看某次提交具体改了什么
git diff <哈希1> <哈希2>    # 比较两次提交
```

> `git restore` 丢弃的改动**找不回来**（没提交过就不在任何历史里）。
> 拿不准的时候，先 `git stash`（把改动临时收起来）而不是直接 restore。

---

## 七、推荐的练习顺序

既然本工程已经托管在 GitHub 上了，可以拿真实工程练手：

1. 改一个安全的值（比如 `myapp/Motor_Config.h` 里的 `FOC_HZ` 注释）
2. `git status` → 看 git 怎么描述这个改动
3. `git diff` → 看具体的行级差异
4. `git add -A` → `git commit -m "..."` → `git push`
5. 去 GitHub 网页刷新，看你的改动真的出现在上面了 ——
   这个"本地→云端"的闭环走通一次，git 就基本不神秘了
6. 然后 `git log --oneline` 看历史慢慢变长
