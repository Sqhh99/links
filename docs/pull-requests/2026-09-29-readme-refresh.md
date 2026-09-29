# PR：重写客户端 README，并忽略 `__cmake_systeminformation`

- **日期：** 2026-09-29
- **分支：** `docs/readme-refresh` → `main`
- **基线提交：** `8d31360`
- **关联记录：** [工作记录](../work-logs/2026-09-29-readme-refresh.md) · [服务端 PR 记录](2026-09-29-server-readme.md)（[Sqhh99/links-sig-server#3](https://github.com/Sqhh99/links-sig-server/pull/3)）

## 关联

作者要求改进客户端 README，并用英文编写。原来的 README 有几个问题：

- 目录链接写成了本机路径 `/mnt/d/workspace/cpp-workspace/links/...`，在 GitHub 上全部失效；
- 没有说明怎样启动后端，也没有说明客户端怎样连接后端；
- 缺少架构说明；
- Qt 模块列表漏了 `Svg`，这是 UI 改版后新增的依赖。

另外，作者本地对 `.gitignore` 加了一行，要求随本 PR 一起提交。

## 改了什么

### `README.md`（英文重写）

- **项目简介**：技术栈、支持的平台、界面语言；`server/` 子模块的定位；自托管方式。
- **功能**：按账号、会议、会中、音频处理、本地录制、外观分组，与当前代码一致。
- **快速开始**，分四步：
  1. 带子模块克隆；
  2. 用 `server/docker` 启动后端；
  3. 构建客户端；
  4. 在「设置 → 网络 → 信令地址」里填 `http://127.0.0.1:8081`，并说明客户端默认连接的是托管服务器。
- **构建**：
  - 依赖列表：Qt 版本下限来自 `qt_standard_project_setup(REQUIRES 6.8)`，模块列表与 `find_package` 一致，vcpkg 依赖来自 `vcpkg.json`；
  - 各平台额外需要的工具；
  - 首次配置会下载 SDK；
  - Windows 上写死的 Qt 路径；
  - 构建命令和环境变量。
- **测试**：覆盖范围，以及运行单个测试的命令。
- **架构**：从 `CLAUDE.md` 提炼的分层、core 不含 Qt、`ConferenceManager` 的组成、线程规则、各平台的屏幕采集实现。
- **目录**：用表格列出，所有链接改为相对路径。
- **贡献**：文件注册规则、提交风格，以及 CI 和打包的触发条件。
- **许可**：Apache 2.0。

### `.gitignore`

- 新增 `__cmake_systeminformation`：作者本地的改动，按要求一并提交。

### 记录

- 新增：
  - 工作记录；
  - 本 PR 记录；
  - 服务端 PR（links-sig-server#3）记录。
- `server` 子模块指针不在本 PR 里改动，由 #27 负责。

## 合并注意

README 的快速开始依赖服务端的 `docker/` 目录：

- 该目录来自 [links-sig-server#2](https://github.com/Sqhh99/links-sig-server/pull/2)；
- 客户端要等 #27 升级子模块指针后才能用上。

README 中指向服务端仓库的链接，也要等服务端 #2、#3 合并后才能打开。

建议的合并顺序：服务端 #2 → 服务端 #3 → 客户端 #27 → 本 PR。

## 怎么验证

- [x] README 中已没有 `/mnt/` 开头的本机路径。所有相对链接都用脚本检查过，指向的文件或目录都存在。
- [x] 对照代码核对 README 中的事实：
  - Qt 模块和版本来自 `CMakeLists.txt`；
  - `LINKS_SDK_ARCH` 默认为 `x64`，见 `build.sh` 和 `cmake/FetchLiveKitSDK.cmake`；
  - 客户端的服务器地址对应设置项 `server/signaling_url`：`SettingsBackend::apiUrl` 读写的就是它，设置界面为 `NetworkSettings.qml` 的「信令地址」；
  - CI 的触发分支是 `main` 和 `develop`，打包由 `v*` 标签触发。
- [ ] 只改了文档和 `.gitignore`，没有构建或运行测试。

## 检查项

- [ ] 已阅读 CONTRIBUTING.md（仓库中当前不存在该文件）
- [ ] UI / QML 改动附了截图（不涉及 UI）
- [x] 若改动了翻译库、i18n、模型或打包资源，已在上文写明：不涉及
- [x] AI 使用披露：是。README 由 Claude Code 编写，并已对照代码核对。
