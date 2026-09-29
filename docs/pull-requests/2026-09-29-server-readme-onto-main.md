# PR：把英文 README 重新合入 main，并删除 PROJECT_STRUCTURE.md

- **日期：** 2026-09-29
- **仓库：** [Sqhh99/links-sig-server](https://github.com/Sqhh99/links-sig-server)（本仓库的 `server/` 子模块）
- **分支：** `docs/readme-onto-main` → `main`
- **基线提交：** `4688730`（当时的 `main`，含 [#4](https://github.com/Sqhh99/links-sig-server/pull/4) 的撤回）
- **本次提交：** `ecaba19`
- **合并提交：** `aaa75e9`
- **PR：** [Sqhh99/links-sig-server#5](https://github.com/Sqhh99/links-sig-server/pull/5)
- **关联记录：** [工作记录](https://github.com/Sqhh99/links/blob/docs/server-readme-record/docs/work-logs/2026-09-29-readme-refresh.md) · [原先的 PR 记录](https://github.com/Sqhh99/links/blob/main/docs/pull-requests/2026-09-29-server-readme.md)

## 关联

英文 README 和删除 `PROJECT_STRUCTURE.md` 已经在 [#3](https://github.com/Sqhh99/links-sig-server/pull/3) 里做过，但那次 PR 的目标分支是 `chore/docker-stack`，不是 `main`。随后 [#4](https://github.com/Sqhh99/links-sig-server/pull/4) 把这次改动从 `main` 撤回。

合并前打开 `main` 会看到：

- 没有 `README.md`，仓库首页是 “Add a README”；
- `PROJECT_STRUCTURE.md` 还在。

这两次提交都已经在 `main` 的历史里，直接再把 `docs/server-readme` 或 `chore/docker-stack` 合进 `main` 不会改任何文件。本 PR 在当时的 `main` 上重新应用同一份文件改动，并已合入。

## 改了什么

与 [#3](https://github.com/Sqhh99/links-sig-server/pull/3) 的文件内容相同，基线换成已经包含 Docker 配置的 `main`：

- **新增 `README.md`（英文）**：项目简介、用户名登录、会议功能、Docker 快速开始、从源码运行时的环境变量、测试、API 一览、目录结构。
- **删除 `PROJECT_STRUCTURE.md`**：目录结构已写进 README。当前 `main` 上没有任何文件再引用它。

没有改 Rust 代码、迁移或 Docker 配置。

## 怎么验证

- [x] `git diff origin/main...HEAD` 只有 `README.md`（新增）和 `PROJECT_STRUCTURE.md`（删除）。
- [x] README 里的相对链接都指向当前 `main` 上存在的路径：`docker/compose.yaml`、`docker/README.md`、`migrations/`、`docs/api/README.md`、`docs/CONTRIBUTING.md`、`docs/TROUBLESHOOTING.md`。
- [x] 在重新应用后的树上搜索，已没有 `PROJECT_STRUCTURE` 引用。
- [x] 当前 `origin/main` 的文件树与写这份 README 时的基线 `a032234` 一致，文中的环境变量、路由和登录规则没有过期。
- [x] 已合入 `main`（`aaa75e9`）。合并后的根目录有 `README.md`，没有 `PROJECT_STRUCTURE.md`。
- [ ] 只改了文档，没有重新构建，也没有运行测试。

## 检查项

- [ ] 已阅读 CONTRIBUTING.md（服务端仓库有 `docs/CONTRIBUTING.md`，这次只改文档）
- [x] 若改动了打包资源，已在上文写明：不涉及
- [x] AI 使用披露：是。README 正文沿用 [#3](https://github.com/Sqhh99/links-sig-server/pull/3)，本次由 Grok 把该改动重新应用到 `main` 并开 PR。对照过链接和 `PROJECT_STRUCTURE` 引用，没有重新构建或跑测试。
