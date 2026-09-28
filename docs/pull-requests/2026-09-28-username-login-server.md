# PR：去掉邮箱注册与验证码，改为用户名密码登录（首次登录即创建账号）

- **日期：** 2026-09-28
- **仓库：** [Sqhh99/links-sig-server](https://github.com/Sqhh99/links-sig-server)（本仓库的 `server/` 子模块）
- **分支：** `feat/username-login` → `main`
- **基线提交：** `bf360ec`
- **关联记录：** [工作记录](../work-logs/2026-09-28-username-login.md) · [客户端 PR 记录](2026-09-28-username-login-client.md)

## 关联

项目按个人开源的方式使用，邮箱验证码注册流程过于繁琐：

- 需要配置 SMTP；
- 注册要先发验证码，再提交注册；
- 客户端要维护两套表单。

本 PR 去掉注册和邮箱验证，改为用户名 + 密码直接登录。服务端负责校验密码强度和用户名唯一性。

## 改了什么

### 接口

- 删除 `POST /api/auth/register/request-code` 和 `POST /api/auth/register`，两者现在返回 404。
- `POST /api/auth/login` 的请求体由 `{email, password}` 改为 `{username, password}`：
  - **用户名已存在**：校验密码，成功返回 `200`；密码不匹配返回 `401`，`code` 为 `INVALID_CREDENTIALS`，提示「用户名已被占用或密码错误」。
  - **用户名不存在**：校验规则，通过后创建账号并登录，返回 `201`。
  - 响应里的 `email` 改为 `username`，并新增 `accountCreated`。
- `POST /api/auth/refresh` 的响应里 `email` 改为 `username`。
- 用户 JWT 的 `email` 声明改为 `username`。升级前签发的 token 仍带 `email` 字段，通过 `#[serde(alias = "email")]` 继续有效，直到过期。

### 新账号规则（只在创建时检查）

- **用户名**：2–32 个字符，只能包含字母（任意文字）、数字、`_`、`-`、`.`；违反时返回 `INVALID_USERNAME`。
- **密码**：8–128 个字符，同时包含英文字母和数字，且不能与用户名相同；违反时返回 `WEAK_PASSWORD`。这与客户端原有的密码校验一致。
- **大小写**：用户名匹配不区分大小写，保存时保留创建时的大小写。唯一性由 `LOWER(username)` 唯一索引保证。
- **并发**：两个请求同时创建同名账号时，插入使用 `ON CONFLICT DO NOTHING`，后到的请求按普通登录处理。

### 数据库：迁移 `006_username_login.sql`

- `users.email` 重命名为 `username`。已有账号的邮箱直接成为用户名，老用户仍可用「邮箱 + 原密码」登录；只有新建账号才检查用户名规则，所以不受影响。
- 去掉原有的 `users_email_key` 唯一约束和冗余的 `idx_users_email` 索引，换成 `LOWER(username)` 唯一索引。
- 删除 `email_verification_codes` 表。

### 删除的代码与配置

- `src/integrations/email.rs`（SMTP 发送和测试用的 `FakeEmailSender`）。
- `AppState.email` 字段；`AppState::new` 不再需要邮件发送器，测试构造函数只保留 `with_livekit(config, db, livekit)`。
- `Config` 里的 `CODE_*` 和 `SMTP_*` 配置项，以及 `.env.deploy.example` 中对应的配置段。
- 依赖 `lettre`、`hmac`、`sha2`、`hex`、`thiserror`，`Cargo.lock` 同步瘦身。
- `examples/send_test_email.rs`，以及 `.gitignore` 里对应的那一行。
- `AppError::too_many_requests*`：原来只用于验证码限流。

### 测试与文档

- 删除 `tests/auth_register.rs`。
- 重写 `tests/auth_login.rs`，覆盖以下场景：
  - 老账号登录
  - 密码错误
  - 大小写不敏感
  - 返回展示名
  - 邮箱形式的老账号
  - 空密码
  - 首次登录建号后再登录
  - 弱密码
  - 非法用户名
  - 缺少 `username` 字段
  - 注册接口已 404
- 其余测试里直接插入用户的 SQL 由 `email` 改为 `username`。
- `docs/api/*`、`docs/DEPLOYMENT.md` 和 `PROJECT_STRUCTURE.md` 同步更新。

## 部署注意

- 客户端和服务端要同时升级：
  - 新客户端发送的是 `username`，旧服务端会返回 422；
  - 旧客户端发送的是 `email`，新服务端也会返回 422。
  - 客户端对应改动见 [客户端 PR 记录](2026-09-28-username-login-client.md)。
- 部署环境里的 `CODE_*`、`SMTP_*` 环境变量可以删掉；不删也只是被忽略。
- **请吊销 QQ 邮箱的 SMTP 授权码。** 被删除的 `examples/send_test_email.rs` 里写死了一个授权码，这个文件从 `43597dc` 起就在公开仓库的历史中。删除文件并不能把它从历史里清掉。

## 怎么验证

以下命令均用 Windows 上的 Rust 1.98.1 执行，测试数据库是 `docker/postgres-docker` 里的 `postgres-test`（postgres:16-alpine）：

- [x] `cargo build --all-targets`：通过，没有新增 warning。剩余 warning 都是改动前就有的测试辅助函数未使用等。
- [x] `cargo test`：全部通过。
  - 单元测试 12 个，1 个需要数据库的被忽略。
  - 集成测试：`auth_login` 14、`auth_refresh` 7、`health` 5、`meetings` 36、`rooms` 17、`token` 11。
- [x] `cargo fmt --check`：通过。
- [x] 迁移兼容性：在临时库里按 `main` 的 001–005 建表，插入两个邮箱账号（其中一个有展示名）和一条验证码记录，再执行 006。结果如下：
  - 列已重命名，展示名保留；
  - 验证码表已删除；
  - `LOWER(username)` 唯一索引生效，插入 `ALICE@example.com` 会被拒绝；
  - `ON CONFLICT DO NOTHING` 在表达式索引上按预期不返回行。
- [ ] 未启动服务端和客户端做端到端联调：客户端这次没能构建，见客户端 PR 记录。
- [ ] 未构建 Docker 镜像。

## 检查项

- [ ] 已阅读 CONTRIBUTING.md（服务端仓库有 `docs/CONTRIBUTING.md`，本次未逐条对照）
- [x] 若改动了打包资源，已在上文写明：删除了 `CODE_*` / `SMTP_*` 配置项，以及 `lettre` 等依赖
- [x] AI 使用披露：是。全部改动由 Claude Code 完成，包括接口设计、迁移、测试和文档。服务端已按上文完成构建、测试和迁移验证。
