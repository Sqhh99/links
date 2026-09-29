# PR：WebRTC Audio Processing 升级到 3.0（m153.8010.0.2）

- **日期：** 2026-09-29
- **分支：** `chore/upgrade-webrtc-apm-3` → `main`
- **基线提交：** `472aaba`
- **关联记录：** [工作记录](../work-logs/2026-09-29-upgrade-webrtc-apm-3.md)

## 关联

[Sqhh99/webrtc-audio-processing](https://github.com/Sqhh99/webrtc-audio-processing/releases) 发布了基于 WebRTC M153 的 3.0 版本。它和目前使用的 2.1（`webrtc-audio-processing-mirror`）在这些方面都不同：

- 发布地址；
- 包名和目录结构；
- 库文件名；
- 部分 API。

## 改了什么

### 下载模块 `cmake/FetchWebRTCAudioProcessing.cmake`

- **来源：** 改从新仓库下载，默认版本 `m153.8010.0.2`，标签没有 `v` 前缀。
- **包名：** 改为 `webrtc-audio-processing.<platform>_<arch>`：
  - Windows：`windows_x86_64` / `windows_arm64`
  - macOS：`macos_x86_64` / `macos_arm64`
  - Linux：`ubuntu-24.04_x86_64` / `ubuntu-24.04_armv8`，与 LiveKit SDK 选用的发行版一致
- **本地目录：** 就是包名，例如 `third_party/webrtc-audio-processing.windows_x86_64`。已经手动解压好的目录会被直接使用。
- **路径：** 头文件和库文件换成 `-3` 的新路径。模块对外输出的变量名不变，所以测试和拷贝 DLL 的 CMake 代码不用改。
- **目录检查：** 缺少头文件或库时直接报错，并提示删除目录重新下载。目录名里没有版本号，不检查的话，旧目录会在编译阶段才以难以理解的方式报错。

### 代码 `core/audio_processing_module.*`

- **创建 APM：** 改用 `BuiltinAudioProcessingBuilder(config).Build(webrtc::CreateEnvironment())`，原来的 `AudioProcessingBuilder` 已被上游删除。
- **删除 `echo_canceller.mobile_mode`：** 上游移除了 AECM。
- **引用计数：** APM 是引用计数对象，改为通过 `Release()` 释放；原来是直接 `delete`。头文件仍然不包含任何 WebRTC 头文件。

### 编译宏

`core/audio_processing_module.cpp` 编译时定义 `WEBRTC_WIN` / `WEBRTC_POSIX`，这是新版发布说明的要求。只有这个文件包含 WebRTC 头文件。

### CI

- **`package.yml`：** 三个平台打包时写死的运行库文件名改为：
  - `webrtc-audio-processing-3-0.dll`
  - `libwebrtc-audio-processing-3.0.dylib`
  - `libwebrtc-audio-processing-3.so*`
- **`build.yml`：** 不需要改，它没有引用任何 APM 文件名。

## 怎么验证

- [x] 实际检查了新包的内容：
  - 各平台压缩包的目录结构（都没有顶层目录）；
  - Windows DLL 导出了 `BuiltinAudioProcessingBuilder::Build` 和 `EnvironmentFactory::Create`；
  - Linux SONAME 为 `libwebrtc-audio-processing-3.so.0`，Ubuntu 24.04 包最高依赖 GLIBC 2.38；
  - macOS install name 仍是绝对路径，所以保留 `install_name_tool`。
- [x] 对照 3.0 头文件核对了用到的每一个接口。
- [x] `grep` 确认没有遗留 2.1 的文件名、仓库名和 `mobile_mode`。
- [ ] `build.cmd test`：没有执行。本次 WSL 会话无法调用 Windows 工具链，需要作者在 Windows 上运行。预期结果：
  - 配置阶段直接使用已解压的目录；
  - `build/bin` 下出现 `webrtc-audio-processing-3-0.dll`；
  - `audio_processing_tests` 和 `microphone_capturer_tests` 通过。
- [ ] Linux 和 macOS：由本 PR 触发的 `build.yml` 检验。
- [ ] 打包：可在本分支手动触发 `package.yml` 检验新文件名，本次没有触发。
- [ ] 通话中回声消除、降噪、自动增益的实际效果：没有测试。

## 检查项

- [ ] 已阅读 CONTRIBUTING.md（仓库中当前不存在该文件）
- [ ] UI / QML 改动附了截图（不涉及 UI）
- [x] 若改动了翻译库、i18n、模型或打包资源，已在上文写明：`package.yml` 中的运行库文件名已更新。新包自带的 `licenses/` 目录还没有打进发布包，留作后续处理。
- [x] AI 使用披露：是。CMake、C++ 和 CI 改动由 Claude Code 编写，只做了静态核对，没有构建或运行。
