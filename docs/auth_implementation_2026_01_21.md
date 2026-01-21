# 登录和注册功能实现日志

**日期**: 2026-01-21

## 概述

实现了完整的用户认证系统，包括邮箱登录、注册（含验证码）功能，同时清理了模块结构。

---

## 模块清理

### 删除的废弃文件
- `ui/qml/login/LoginWindow.qml` - 已被 HomeWindow 替代
- `ui/qml/login/HeroPanel.qml` - 仅被 LoginWindow 使用
- `ui/qml/login/AvatarPicker.qml` - 从未使用

### 移动的文件
| 原路径 | 新路径 |
|--------|--------|
| `ui/qml/login/JoinForm.qml` | `ui/qml/home/JoinForm.qml` |
| `ui/qml/login/QuickStartForm.qml` | `ui/qml/home/QuickStartForm.qml` |
| `ui/qml/login/ScheduleForm.qml` | `ui/qml/home/ScheduleForm.qml` |

---

## 新增文件

### AuthBackend (ui/backend/)
- `AuthBackend.h` / `AuthBackend.cpp`
- QML 可调用的认证后端
- 属性: `loading`, `isLoggedIn`, `errorMessage`, `userEmail`, `userName`, `codeCooldown`
- 方法: `login()`, `requestCode()`, `registerUser()`, `logout()`, `tryAutoLogin()`

---

## 修改的文件

### NetworkClient (core/)
新增认证 API 方法:
- `login(email, password)` → POST `/api/auth/login`
- `requestVerificationCode(email)` → POST `/api/auth/register/request-code`
- `registerUser(email, password, code)` → POST `/api/auth/register`

新增信号: `loginSuccess`, `registerSuccess`, `codeRequestSuccess`, `authError`

### Settings (utils/)
新增认证数据存储:
- `getAuthToken()` / `setAuthToken()`
- `getUserId()` / `setUserId()`
- `getUserEmail()` / `setUserEmail()`
- `getDisplayName()` / `setDisplayName()`
- `hasAuthData()` / `clearAuthData()`

### QML 组件
- `HomeWindow.qml` - 集成 AuthBackend，登录状态驱动 UI
- `AuthModal.qml` - 传递 authBackend 给 LoginCard
- `LoginCard.qml` - 调用 authBackend 方法，显示错误信息
- `RegisterForm.qml` - 新增验证码冷却倒计时，发送验证码按钮

### CMakeLists.txt
- 添加 AuthBackend 源文件
- 更新 QML 文件路径

### main.cpp
- 注册 AuthBackend 为 QML 类型

---

## 技术说明

### 验证码冷却
发送验证码后，60 秒内禁止再次发送，按钮显示倒计时。

### Token 持久化
登录/注册成功后，token 保存到本地配置文件，支持自动登录。
