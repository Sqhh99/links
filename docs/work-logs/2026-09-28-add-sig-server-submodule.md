# 工作记录：把信令服务仓库加为 Git 子模块

- **日期：** 2026-09-28
- **分支：** `chore/add-sig-server-submodule`（基于 `main` 的 `6ac1f71`）
- **关联文档：** [PR 记录](../pull-requests/2026-09-28-add-sig-server-submodule.md)

## 一、用户的请求

> 帮我给这个项目添加一下服务端的仓库的子模块， https://github.com/Sqhh99/links-sig-server.git，

把 `links-sig-server` 作为本仓库的 Git 子模块引入。仓库说明里 `server/` 就是信令服务的位置，远端仓库本身就是这套 Rust 信令服务，因此路径用 `server`。

## 二、制定的计划

1. 确认本仓库还没有 `.gitmodules`，也没有 `server/` 目录。
2. 确认远端仓库是独立的 Rust 信令服务（`Cargo.toml` 在仓库根目录）。
3. 执行 `git submodule add`，路径为 `server`，并让子模块跟踪 `main`。
4. 在 `README.md` 里写明子模块地址和初始化命令。

## 三、具体改了哪些文件

| 文件 | 改动 |
| --- | --- |
| `.gitmodules` | 新增子模块 `server`，URL 为 `https://github.com/Sqhh99/links-sig-server.git`，`branch = main` |
| `server` | 子模块检出，当前提交 `bf360ec`（`Default LiveKit WebSocket URL to 127.0.0.1 and document layout`） |
| `README.md` | 说明 `server/` 是子模块，并给出 `git clone --recurse-submodules` 与 `git submodule update --init server` |
| `.gitignore` | 增加 `.vs`，并补上文件末尾换行 |
| `docs/work-logs/2026-09-28-add-sig-server-submodule.md` | 本记录 |
| `docs/pull-requests/2026-09-28-add-sig-server-submodule.md` | PR 记录，同时作为 PR 正文 |

## 四、验证情况

- 已执行 `git submodule add` 与 `git submodule set-branch --branch main server`。
- `git submodule status` 显示 `bf360ecba77baaadd2e7e8772e9f253821908b9b server (heads/main)`。
- 客户端没有改构建，**not built, not tested**。WSL 会话本来也不能跑 Windows 的 `build.cmd`。

## 五、遗留事项

- 父仓库 `.gitignore` 里仍有旧路径 `server/livekit-signaling-server/`。子模块自己的 `.gitignore` 已忽略 `.env` 和 `target/`，那些旧规则目前不起作用。
- CI 的 checkout 没有打开 `submodules`。客户端构建不依赖这个目录，所以没有改 workflow。

## 六、后续请求：新建分支、提交并开 PR

> 新建分支提交commit，提交pr，commit把.gitignore所做的修改也包含进去

1. 分支名为 `chore/add-sig-server-submodule`。
2. 一次提交包含子模块、`.gitmodules`、`README.md`、`.gitignore`（`.vs` 以及文件末尾换行）、工作记录和 PR 记录。
3. PR 正文使用 `docs/pull-requests/2026-09-28-add-sig-server-submodule.md`。
