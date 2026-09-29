# PR：新增英文 README，删除 PROJECT_STRUCTURE.md

- **日期：** 2026-09-29
- **仓库：** [Sqhh99/links-sig-server](https://github.com/Sqhh99/links-sig-server)（本仓库的 `server/` 子模块）
- **分支：** `docs/server-readme` → `chore/docker-stack`
- **基线提交：** `a032234`（[links-sig-server#2](https://github.com/Sqhh99/links-sig-server/pull/2) 的分支头）
- **PR：** [Sqhh99/links-sig-server#3](https://github.com/Sqhh99/links-sig-server/pull/3)
- **关联记录：** [工作记录](../work-logs/2026-09-29-readme-refresh.md) · [客户端 PR 记录](2026-09-29-readme-refresh.md)

## 关联

服务端仓库原来没有 README，项目说明只能在中文的 `PROJECT_STRUCTURE.md` 里找，而这份文件的内容已经不全。按作者要求：

- 新增一份英文 README；
- 删除 `PROJECT_STRUCTURE.md`。

README 里的快速开始用到了 #2 新增的 `docker/`，而 #2 也改动了 `PROJECT_STRUCTURE.md`。所以本 PR 以 #2 的分支为基线，免得产生「修改/删除」冲突。#2 合并并删除分支后，GitHub 会把本 PR 的目标分支自动改为 `main`。

## 改了什么

- **新增 `README.md`（英文）**，包含以下几部分：
  - **项目简介**：说明它与 Links 客户端、LiveKit 的关系。
  - **功能**：
    - 用户名登录：首次登录即创建账号，并写明用户名和密码规则；
    - 9 位会议号、预定会议、主持人先入会、会议密码、游客入会；
    - 会议生命周期和会议记录；
    - LiveKit 房间和参与者管理；
    - `/join` 网页观看页。
  - **Docker 快速开始**：链接到中文的 `docker/README.md`。
  - **从源码运行**：环境变量表，内容与 `src/config.rs` 的默认值一致。
  - **测试**：`postgres-test` 加 `cargo test`，以及 `TEST_DATABASE_URL`。
  - **API 一览**：内容与 `src/routes.rs` 一致，并链接到中文的 `docs/api/`。
  - **目录结构**：取代原来 `PROJECT_STRUCTURE.md` 的作用。
- **删除 `PROJECT_STRUCTURE.md`**：有用的内容已经并入 README。仓库里没有其他文件引用它。

## 怎么验证

- [x] 对照源码核对 README 中的事实：
  - 环境变量及其默认值来自 `src/config.rs`；
  - 路由来自 `src/routes.rs`；
  - 用户名和密码规则来自 `src/services/user_auth_service.rs`；
  - 游客只能观看的说明来自 `static/js/app.js`。
- [x] README 里的相对链接（`docker/`、`docs/`、`migrations/` 等）都指向实际存在的文件。
- [x] `grep` 确认仓库中已没有对 `PROJECT_STRUCTURE.md` 的引用。
- [ ] 只改了文档，没有重新构建或运行测试。

## 检查项

- [ ] 已阅读 CONTRIBUTING.md（服务端仓库有 `docs/CONTRIBUTING.md`，但这次只改文档）
- [x] 若改动了打包资源，已在上文写明：不涉及
- [x] AI 使用披露：是。README 由 Claude Code 编写，并已对照源码核对。
