# PR：将信令服务仓库加为 Git 子模块

- **日期：** 2026-09-28
- **分支：** `chore/add-sig-server-submodule` → `main`
- **基线提交：** `6ac1f71`
- **关联记录：** [工作记录](../work-logs/2026-09-28-add-sig-server-submodule.md)

## 关联

客户端仓库的说明里约定 `server/` 存放信令服务，但目录此前并不存在。信令服务已单独放在 [Sqhh99/links-sig-server](https://github.com/Sqhh99/links-sig-server)。本 PR 把它加为子模块，克隆本仓库后可以直接拿到对应版本的服务端代码。

## 改了什么

- 新增子模块 `server`，URL 为 `https://github.com/Sqhh99/links-sig-server.git`，跟踪 `main`。
- 当前钉住的提交是 `bf360ec`（Default LiveKit WebSocket URL to 127.0.0.1 and document layout）。
- `README.md` 写明这是独立的 Rust 服务，不参与顶层 CMake 构建，并给出初始化命令：

```bash
git clone --recurse-submodules https://github.com/Sqhh99/links.git
git submodule update --init server
```

- `.gitignore` 增加 `.vs`，忽略 Visual Studio 的本地目录。
- 顶层 CMake、客户端源码和 CI workflow 没有改。客户端构建不依赖 `server/`，现有 checkout 动作也不会去拉子模块。

## 怎么验证

- [ ] `build.cmd release`
- [ ] `build.cmd tests`

**以上两项均未打勾。** 本次没有 C++ / QML / CMake 改动，客户端构建行为预期不变。改动在 WSL 中完成，这里不能跑 Windows 的 `build.cmd`。合并前不需要为这个子模块单独补客户端构建；若要确认子模块能检出，在干净克隆上执行 `git submodule update --init server`，应得到 `server` @ `bf360ec`。

**已执行：**

- `git submodule add https://github.com/Sqhh99/links-sig-server.git server`
- `git submodule set-branch --branch main server`
- `git submodule status` 显示 `bf360ecba77baaadd2e7e8772e9f253821908b9b server (heads/main)`
- `server/Cargo.toml` 存在，说明检出的是信令服务仓库而不是空目录

## 检查项

- [ ] 已阅读 CONTRIBUTING.md（仓库中当前不存在该文件）
- [x] UI / QML 改动附了截图：不适用，没有界面改动
- [x] 若改动了翻译库、i18n、模型或打包资源，已在上文写明：没有改这些内容
- [x] AI 使用披露：是。子模块添加、README、`.gitignore` 的换行，以及本记录由 Grok 完成。`.vs` 这一行是提交前工作区里已有的忽略规则，按要求一并纳入本次提交。子模块检出已用 `git submodule status` 核对，客户端未构建、未跑测试。
