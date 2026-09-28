# 工作记录：去掉邮箱注册与验证码，改为用户名密码登录（服务端 + 客户端）

- **日期：** 2026-09-28
- **分支：**
  - 服务端子模块 `server/`：`feat/username-login`，基于 `main` 的 `bf360ec`
  - 客户端：`feat/username-login`，基于 `main` 的 `d3cda31`
- **关联文档：** [服务端 PR 记录](../pull-requests/2026-09-28-username-login-server.md)（[Sqhh99/links-sig-server#1](https://github.com/Sqhh99/links-sig-server/pull/1)） · [客户端 PR 记录](../pull-requests/2026-09-28-username-login-client.md)

## 一、用户的请求

> Please take a look at the project; I need to modify it. I want to remove the server-side features related to user registration and email verification, as well as the corresponding client-side registration functionality and UI. The design I'm aiming for allows users to log in to the server directly using a username and password, with the server validating password security and username uniqueness. Incorporating email verification and a registration process is too cumbersome; the project is intended to follow a personal open-source model where such features aren't necessary. Please modify the code and submit separate pull requests (PRs) for the server-side and client-side changes.

过程中的追加请求：

> .env.example、.env By the way, please update these two files for me.

实际要做的事：

- 服务端去掉注册接口、邮箱验证码和 SMTP，登录标识由邮箱改为用户名。
- 用户名不存在时由登录直接创建账号，服务端校验密码强度和用户名唯一性。
- 客户端去掉注册界面和相关后端代码。
- 服务端、客户端各开一个 PR。
- 另外清理服务端本地的 `.env` 和 `.env.example`。

## 二、制定的计划

1. **确定语义**：由于不再有注册步骤，「登录」同时承担建号。
   - 用户名存在则校验密码；不存在则按规则校验用户名和密码，再建号。
   - 用户名和密码的规则只在建号时检查，老账号（邮箱作为用户名）不受影响。
2. **服务端**：
   - 新增迁移 006：重命名列，改为不区分大小写的唯一索引，删除验证码表。
   - 重写 `UserAuthService`；删除注册接口、邮件模块、`CODE_*`/`SMTP_*` 配置和相关依赖。
   - JWT 声明改为 `username`，并兼容旧 token 中的 `email`。
   - 更新测试和文档。
   - 用 Windows 上的 cargo 构建，起 Postgres 测试容器跑全部测试，再单独验证迁移能否处理已有数据。
3. **客户端**：
   - 删除注册表单和页签；登录表单改为用户名。
   - 删除 `NetworkClient`/`AuthBackend` 中的注册、验证码代码，`Settings` 键改名。
   - 把错误码转换成中文提示；同步 `docs/server-api/`。
4. 编写记录，两边分别提交并开 PR。

## 三、具体改了哪些文件

### 服务端（`server/` 子模块）

| 文件 | 改动 | 原因 |
| --- | --- | --- |
| `migrations/006_username_login.sql` | 新增：`email` 列改名为 `username`；删除 `users_email_key` 约束和 `idx_users_email` 索引，新增 `LOWER(username)` 唯一索引；删除 `email_verification_codes` 表 | 用户名唯一且不区分大小写；老账号的邮箱直接成为用户名 |
| `src/services/user_auth_service.rs` | 重写：`login` 查到用户就校验密码，查不到就校验规则后 `INSERT ... ON CONFLICT DO NOTHING`；并发建号失败时退回普通登录。删除验证码相关代码，新增三个错误码 | 取消注册步骤，由服务端做密码强度和唯一性校验 |
| `src/handlers/auth.rs`、`src/routes.rs` | 删除两个注册路由和处理函数；登录新建账号时返回 201 | 去掉注册接口 |
| `src/types/requests.rs`、`responses.rs` | 删除注册相关的请求和响应类型；`LoginRequest` 改为 `username`；响应中的 `email` 改为 `username`，并新增 `accountCreated` | 同上 |
| `src/auth/user_jwt.rs` | 声明字段 `email` 改为 `username`，加 `#[serde(alias = "email")]`；新增旧 token 的解码测试 | 升级后已登录的用户不会被强制退出 |
| `src/handlers/meeting.rs`、`meeting_registry_service.rs` | 入会时展示名的回退值由邮箱改为用户名 | 跟随字段改名 |
| `src/config.rs`、`state.rs`、`main.rs`、`lib.rs`、`integrations/mod.rs`、`types/error.rs` | 删除 `CODE_*`/`SMTP_*` 配置、`AppState.email` 和 SMTP 初始化；删除仅供验证码限流使用的 `too_many_requests*` | 邮件功能整体移除 |
| `src/integrations/email.rs`、`examples/send_test_email.rs`、`tests/auth_register.rs` | 删除 | 同上 |
| `Cargo.toml`、`Cargo.lock` | 删除 `lettre`、`hmac`、`sha2`、`hex`、`thiserror` | 这些依赖只被邮件和验证码代码使用 |
| `tests/auth_login.rs` | 重写，共 11 个场景 | 覆盖新的登录语义 |
| `tests/auth_refresh.rs`、`meetings.rs`、`rooms.rs`、`support/*` | 直接插入用户的 SQL 改用 `username`；去掉 `FakeEmailSender`；精简测试构造函数 | 跟随 schema 和 `AppState` 的变化 |
| `docs/api/*`、`docs/DEPLOYMENT.md`、`PROJECT_STRUCTURE.md`、`docker/links-sig-server/.env.deploy.example` | 接口文档改为新语义，删除 `CODE_*`/`SMTP_*` 配置说明 | 文档与代码一致 |
| `.gitignore` | 删除 `examples\send_test_email.rs` 一行 | 文件已删除 |

本地未跟踪的配置文件（按追加请求修改，不进入提交）：

| 文件 | 改动 |
| --- | --- |
| `server/.env` | 删除「邮件配置」和「验证码配置」两段，其余值不变 |
| `server/.env.example` | 删除 `Verification Code Settings` 和 `SMTP Settings` 两段 |

这两个文件都在服务端的 `.gitignore` 里，所以不会出现在 PR 中。

### 客户端

| 文件 | 改动 | 原因 |
| --- | --- | --- |
| `ui/qml/login/RegisterForm.qml`、`AuthTabs.qml` | 删除；`CMakeLists.txt` 的 `QML_FILES` 同步移除 | 去掉注册界面 |
| `ui/qml/login/LoginCard.qml` | 去掉模式切换和注册表单，只保留登录表单，副标题提示首次登录自动建号 | 同上 |
| `ui/qml/login/LoginForm.qml` | 字段由邮箱改为用户名，只检查非空；删除没有功能的「忘记密码？」 | 用户名规则只在建号时由服务端检查，老邮箱账号要能登录 |
| `ui/qml/home/AuthModal.qml`、`HomeWindow.qml` | 去掉 `mode` 和 `openWithMode`，以及 `onRegisterSucceeded`；弹窗高度调为 440 | 只剩一个表单 |
| `ui/qml/home/HomeUserCard.qml`、`GuestPromptDialog.qml`、`LoginPromptCard.qml` | 「登录/注册」改为「登录」 | 不再有注册 |
| `ui/backend/network_client.*` | 删除注册和验证码请求；登录发送 `username`，并读取响应中的 `username`；失败回调多传错误码，经 `loginErrorText` 转成中文提示 | 对接新接口 |
| `ui/backend/AuthBackend.*` | 删除注册、验证码和倒计时；`userEmail` 改为 `accountName`；展示名回退时取用户名 `@` 之前的部分 | 同上；老账号不会在会议里暴露完整邮箱 |
| `ui/backend/LoginBackend.cpp` | 默认参会名改用用户名，规则同上 | 跟随 `Settings` 改名 |
| `utils/settings.*` | `auth/email` 改为 `auth/username`，写入时删除旧键，`clearAuthData` 两个键都清 | 跟随字段改名 |
| `docs/server-api/auth.md`、`README.md`、`overview.md` | 从服务端文档同步 | 保持两份一致 |
| `README.md` | 功能列表改为用户名密码登录 | 同上 |

## 四、验证情况

**服务端**（Windows 上的 Rust 1.98.1，从 WSL 调用；测试库为 postgres:16-alpine 容器）：

- `cargo build --all-targets` 通过，没有新增 warning。
- `cargo test` 全部通过：
  - 单元测试 12 个，1 个需要数据库的被忽略；
  - 集成测试：`auth_login` 14、`auth_refresh` 7、`health` 5、`meetings` 36、`rooms` 17、`token` 11。
- `cargo fmt --check` 通过。
- 迁移兼容性：按 `main` 的 001–005 建库，插入邮箱账号和验证码记录后执行 006。
  - 列改名成功，展示名保留，验证码表已删除；
  - 插入大小写不同的同名用户会被唯一索引拒绝。
- 中途 Docker Desktop 被关闭过一次，恢复后重新跑了全部测试。
- 未做：没有启动服务端做端到端联调，也没有构建 Docker 镜像。

**客户端：**

- Docker Desktop 重启后，WSL 的 Windows 互操作失效（启动 `.exe` 报 `Exec format error`），因此没能构建，也没跑 qmllint。
- 只做了一项检查：用 grep 确认被删除的符号在仓库里已没有引用。

**行尾：** 仓库里有些文件的 CRLF 和 LF 混用。最初的编辑脚本把整个文件统一成了 CRLF，后来用脚本按原文件逐行恢复。最终 diff 中没有纯行尾的改动。

## 五、遗留事项

- **客户端**需要在 Windows 上执行 `build.cmd release`，运行后按客户端 PR 记录中的清单手工检查，并补登录弹窗截图。
- **服务端 PR 合并后**，客户端需要单独提交一次 `server` 子模块指针升级。两端必须一起部署，否则登录会得到 422。
- **SMTP 授权码泄露**：被删除的 `examples/send_test_email.rs` 里写死了 QQ 邮箱的 SMTP 授权码，从 `43597dc` 起就在公开仓库的历史中。需要到 QQ 邮箱吊销该授权码；删除文件并不能把它从历史里清除。
- **本地部署配置**：`server/docker/links-sig-server/.env.deploy`（未跟踪）里仍有 `CODE_*`/`SMTP_*` 配置，这次没有改动。
- **`.env.example` 被忽略**：服务端的 `.gitignore` 忽略了 `.env.example`，模板更新只保留在本地。是否要纳入版本库，由作者决定。
- **没有做的功能**：登录没有限流或暴力破解防护（原来也没有）；没有提供修改密码或修改展示名的入口；没有密码找回（没有邮箱就无法实现）。
