# PR：客户端去掉注册与邮箱验证，改为用户名密码登录

- **日期：** 2026-09-28
- **分支：** `feat/username-login` → `main`
- **基线提交：** `d3cda31`
- **关联记录：** [工作记录](../work-logs/2026-09-28-username-login.md) · [服务端 PR 记录](2026-09-28-username-login-server.md) · 服务端 PR [Sqhh99/links-sig-server#1](https://github.com/Sqhh99/links-sig-server/pull/1)

## 关联

服务端改为用户名 + 密码登录，用户名不存在时自动创建账号，不再有注册和邮箱验证码接口（见[服务端 PR 记录](2026-09-28-username-login-server.md)）。客户端需要随之去掉注册界面，并把登录改为用户名。

## 改了什么

### 界面

- 登录弹窗只保留登录表单，删除了「登录 / 注册」页签和注册表单（邮箱、验证码、倒计时、展示名）。
- 登录表单：
  - 字段由「邮箱」改为「用户名」，副标题提示「首次登录将自动创建账号」。
  - 用户名只检查是否为空。用户名规则只在建号时由服务端检查，这样邮箱形式的老账号仍能登录。
  - 密码仍在本地要求至少 8 位且同时包含字母和数字。
  - 去掉了原本没有功能的「忘记密码？」链接：没有邮箱就无法找回密码。
- 「登录/注册」按钮文案统一改为「登录」，涉及头像菜单、游客提示弹窗和提示卡片。
- 登录弹窗高度由 540 调整为 440。
- 服务端返回的错误码会显示为中文提示：

| 错误码 | 提示 |
| --- | --- |
| `INVALID_CREDENTIALS` | 用户名已被占用或密码错误 |
| `INVALID_USERNAME` | 用户名规则说明 |
| `WEAK_PASSWORD` | 密码规则说明 |

### 后端

- `NetworkClient`：
  - 删除 `requestVerificationCode`、`registerUser`，以及 `registerSuccess`、`codeRequestSuccess` 两个信号。
  - `login` 发送 `username`，并从登录和刷新的响应里读取 `username`。
  - `postAuthJsonWithFallback` 的失败回调多传一个错误码，用来转换成上面的提示。
- `AuthBackend`：
  - 删除 `requestCode`、`registerUser`、验证码倒计时（`codeCooldown` 和定时器），以及 `registerSucceeded`、`codeRequestSucceeded` 信号。
  - `userEmail` 属性改为 `accountName`，表示登录用户名；QML 此前没有使用 `userEmail`。
- `Settings`：
  - `getUserEmail`/`setUserEmail` 改为 `getUsername`/`setUsername`，键名为 `auth/username`。
  - 写入时顺带删掉旧的 `auth/email`，`clearAuthData` 两个键都清。
  - 老用户第一次启动会自动刷新 token，新键随之写入。
- 展示名回退：
  - 服务端没有返回展示名时，用用户名作为展示名。
  - 如果用户名是邮箱（迁移过来的老账号），只取 `@` 之前的部分，避免在会议里暴露完整邮箱。
  - `AuthBackend` 和 `LoginBackend::defaultAuthDisplayName` 都按这个规则处理。

### 文件与文档

- 删除 `ui/qml/login/RegisterForm.qml` 和 `ui/qml/login/AuthTabs.qml`，并从 `CMakeLists.txt` 的 `QML_FILES` 中移除。
- `docs/server-api/` 下的 `auth.md`、`README.md`、`overview.md` 与服务端文档同步。`overview.md` 顺带跟上了服务端已有的 `LIVEKIT_WS_URL` 默认值。
- `README.md` 的功能列表改为用户名密码登录。

### 没有改的

- `server` 子模块指针没有动。需要等服务端 PR 合并后，再单独提交子模块升级。
- 客户端和服务端需要一起升级：新客户端连旧服务端、旧客户端连新服务端，登录都会得到 422。

## 怎么验证

- [ ] `build.cmd release`
- [ ] `build.cmd tests`
- [ ] qmllint 检查改动过的 QML
- [ ] 手工检查：
  - 新用户名首次登录会建号，再次登录正常；
  - 密码错误、弱密码、非法用户名时显示中文提示；
  - 老邮箱账号登录后，展示名为 `@` 前的部分；
  - 重启后自动登录；
  - 切换账号、退出登录正常。

**以上各项均未完成，原因如下：**

- 改动在 WSL 中完成。本次会话中，Docker Desktop 重启后 WSL 的 Windows 互操作失效，`.exe` 无法启动（`Exec format error`），所以没能调用 Windows 上的 CMake / MSVC / qmllint。
- C++ 部分只做了人工核对：`registerSucceeded`、`codeCooldown`、`userEmail`、`getUserEmail` 等被删除的符号在仓库里已没有引用。
- 需要作者在合并前于 Windows 上执行 `build.cmd release`，并连同已合并的服务端一起完成上面的手工检查。

## 检查项

- [ ] 已阅读 CONTRIBUTING.md（仓库中当前不存在该文件）
- [ ] UI / QML 改动附了截图（登录弹窗有改动，尚未截图，需作者补充）
- [x] 若改动了翻译库、i18n、模型或打包资源，已在上文写明：删除了两个 QML 文件，并更新了 `QML_FILES`
- [x] AI 使用披露：是。全部改动由 Claude Code 完成。客户端部分未经构建验证，已在上文如实说明。
