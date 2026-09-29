# PR：升级 server 子模块，接入一键启动的 Docker 配置

- **日期：** 2026-09-29
- **分支：** `chore/server-docker-stack` → `main`
- **基线提交：** `8d31360`
- **关联记录：** [工作记录](../work-logs/2026-09-29-server-docker-stack.md) · [服务端 PR 记录](2026-09-29-docker-stack.md) · 服务端 PR [Sqhh99/links-sig-server#2](https://github.com/Sqhh99/links-sig-server/pull/2)

## 关联

服务端 PR [Sqhh99/links-sig-server#2](https://github.com/Sqhh99/links-sig-server/pull/2) 做了两件事：

- 把 PostgreSQL、LiveKit 和 links-sig-server 合并到 `server/docker/compose.yaml`，一条命令就能启动；
- 附带使用说明 `server/docker/README.md`。

本 PR 升级 `server` 子模块指针，让客户端仓库用上这套配置，同时加入本次的工作记录和 PR 记录。

## 改了什么

- `server` 子模块指针从 `bf360ec` 升到 `a032234`，包括：
  - 已合并的 [links-sig-server#1](https://github.com/Sqhh99/links-sig-server/pull/1)：用户名登录，与客户端 #26 对应。此前 `main` 上的指针还没有包含它；
  - [links-sig-server#2](https://github.com/Sqhh99/links-sig-server/pull/2)：Docker 一键启动。
- 新增记录：
  - `docs/work-logs/2026-09-29-server-docker-stack.md`
  - `docs/pull-requests/2026-09-29-docker-stack.md`（服务端 PR 记录）
  - `docs/pull-requests/2026-09-29-server-docker-stack.md`（本文件）

客户端代码没有改动。

## 合并顺序

1. 先合并服务端 PR #2，沿用 merge commit，这样 `a032234` 会留在服务端 `main` 的历史里。
2. 再合并本 PR。

如果服务端 PR 改用 squash 合并，或者合并前又追加了提交，需要先把这里的子模块指针改成合并后服务端 `main` 上的提交。

## 怎么验证

服务端部分的验证见[服务端 PR 记录](2026-09-29-docker-stack.md)：用 Docker 构建、启动、做 API 和媒体链路冒烟测试，`cargo test` 全部通过。

本 PR 自身：

- [x] `git submodule status` 显示 `server` 指向 `a032234`，该提交已推送到 `origin/chore/docker-stack`。
- [ ] 没有构建客户端：客户端代码没有改动。
- [ ] 没有用 Links 客户端实际连接这套 Docker 服务。

## 检查项

- [ ] 已阅读 CONTRIBUTING.md（仓库中当前不存在该文件）
- [ ] UI / QML 改动附了截图（不涉及 UI）
- [x] 若改动了翻译库、i18n、模型或打包资源，已在上文写明：不涉及
- [x] AI 使用披露：是。全部改动由 Claude Code 完成。
