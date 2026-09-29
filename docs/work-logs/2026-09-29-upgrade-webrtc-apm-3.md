# 工作记录：WebRTC Audio Processing 从 2.1 升级到 3.0（m153.8010.0.2）

- **日期：** 2026-09-29
- **分支：** `chore/upgrade-webrtc-apm-3`（基于 `main` 的 `472aaba`）
- **关联文档：**
  - [PR 记录](../pull-requests/2026-09-29-upgrade-webrtc-apm-3.md)（[Sqhh99/links#31](https://github.com/Sqhh99/links/pull/31)）
  - 同一请求的另一半：画廊视图去边框，见 [Sqhh99/links#30](https://github.com/Sqhh99/links/pull/30)（记录 `docs/work-logs/2026-09-29-gallery-view-frames.md` 在该分支上）

## 一、用户的请求

> There are two areas in the current project that need optimization: 1. The gallery view in the meeting interface contains numerous borders and frames, which detracts from the visual consistency of the UI. 2. A new version of the `webrtc-audio-processing` project has been released (https://github.com/Sqhh99/webrtc-audio-processing/releases); `FetchWebRTCAudioProcessing.cmake` and the corresponding code need to be updated to support this new version, and the CI pipeline must also be adjusted, as the package structure differs from the previous version. I have already downloaded the package to the `third_party\webrtc-audio-processing.windows_x86_64` directory.

本记录只覆盖第 2 点：

- 把 WebRTC APM 升级到新仓库发布的最新版本；
- 相应修改下载模块、调用代码和 CI。

第 1 点单独开了分支和记录。

## 二、制定的计划

1. **弄清新包和旧包的差别。** 用 `gh release view` 查看发布内容，逐个列出 Windows、Ubuntu、macOS 包的目录，解析二进制的导出符号、SONAME 和 install name。
2. **弄清 API 的变化。** 读新包里的 `NEWS` 和头文件，对照 `core/audio_processing_module.cpp` 用到的每一个接口。
3. **改代码。** 依次修改：
   - `cmake/FetchWebRTCAudioProcessing.cmake`
   - `core/audio_processing_module.*`
   - `CMakeLists.txt`
   - `package.yml`

### 新包和旧包的差别（都已实际核对）

| 项目 | 2.1（旧） | 3.0（新） |
| --- | --- | --- |
| 仓库 | `Sqhh99/webrtc-audio-processing-mirror` | `Sqhh99/webrtc-audio-processing` |
| 标签 | `v2.1.0-mirror.1` | `m153.8010.0.2`（没有 `v` 前缀） |
| 包名 | `webrtc-audio-processing-<platform>-<arch>.<ext>` | `webrtc-audio-processing.<platform>_<arch>.<ext>` |
| Linux | 只有一个 `linux` 包 | 按发行版分：`ubuntu-{22.04,24.04,26.04}`、`raspberry-pi-os` |
| 架构名 | `x64` / `arm64` | `x86_64`；arm64 在 Windows / macOS 叫 `arm64`，在 Linux 叫 `armv8` |
| 头文件目录 | `include/webrtc-audio-processing-2` | `include/webrtc-audio-processing-3` |
| Windows 库 | `webrtc-audio-processing-2.lib`、`-2-1.dll` | `webrtc-audio-processing-3.lib`、`-3-0.dll` |
| Linux 库 | `libwebrtc-audio-processing-2.so.1` | `libwebrtc-audio-processing-3.so.0`（SONAME 就是这个文件名） |
| macOS 库 | `libwebrtc-audio-processing-2.1.dylib` | `libwebrtc-audio-processing-3.0.dylib`，install name 仍是 CI 上的绝对路径 |
| 新增内容 | — | `licenses/`、`NEWS` |

- **压缩包结构：** 和旧包一样，没有顶层目录。
- **Windows DLL 的导出：** 包含 `BuiltinAudioProcessingBuilder::Build` 和 `EnvironmentFactory::Create`，新写法所需的符号都有。
- **Ubuntu 24.04 包：** 最高依赖 GLIBC 2.38 / GLIBCXX 3.4.30。

### API 的变化（来自 `NEWS`，并已对照头文件）

- **`AudioProcessingBuilder` 被移除**，改用 `BuiltinAudioProcessingBuilder(config).Build(CreateEnvironment())`。
- **`Config::EchoCanceller::mobile_mode` 被移除**（上游删除了 AECM）。
- **`rtc::` 命名空间被移除**。我们的代码只用了 `webrtc::`，不受影响。
- **C++20 成为必需**。项目已经是 C++20。
- **不用 pkg-config 时需要定义 `WEBRTC_POSIX` / `WEBRTC_WIN`。** 我们包含的头文件中，只有 `rtc_export.h` 用到它们。
- **其余接口都在：**
  - AGC2 的 `adaptive_digital.max_gain_db` 和 `fixed_digital.gain_db`；
  - NS level、HPF、`enforce_high_pass_filtering`；
  - int16 版本的 `ProcessStream` / `ProcessReverseStream`；
  - `set_stream_delay_ms`、`kNoError`。

## 三、具体改了哪些文件

| 文件 | 改动 | 原因 |
| --- | --- | --- |
| `cmake/FetchWebRTCAudioProcessing.cmake` | 重写：<br>- 新仓库地址、新标签，默认版本 `m153.8010.0.2`；<br>- 新包名规则，本地目录名就是包名，所以作者已解压的 `third_party/webrtc-audio-processing.windows_x86_64` 会被直接使用；<br>- Linux 选 `ubuntu-24.04`，与 `FetchLiveKitSDK.cmake` 一致；<br>- 新的头文件和库文件路径；<br>- 新增 `WEBRTC_APM_DEFINITIONS`；<br>- 新增目录结构检查：缺少头文件或库时直接报错，并提示删除目录重新下载；<br>- 下载失败时删除残留的压缩包。<br>对外输出的变量名不变 | 包的结构全部变了。<br>目录名里没有版本号，旧的或解压不完整的目录，不检查的话会在编译或链接阶段才以难以理解的方式报错 |
| `CMakeLists.txt` | - 更新版本注释；<br>- 只给 `core/audio_processing_module.cpp` 加上 `WEBRTC_WIN` / `WEBRTC_POSIX` | 按发布说明的要求定义宏。只有这一个文件包含 WebRTC 头文件，范围越小越好 |
| `core/audio_processing_module.cpp` | - 改用 `BuiltinAudioProcessingBuilder(config).Build(webrtc::CreateEnvironment())`；<br>- 删除 `mobile_mode`；<br>- 新增 `builtin_audio_processing_builder.h` 和 `environment_factory.h` 两个头文件 | API 破坏性变更 |
| `core/audio_processing_module.h` | `apm_` 改为 `std::unique_ptr<webrtc::AudioProcessing, ApmReleaser>`，其中 `ApmReleaser` 调用 `Release()` | APM 是引用计数对象。原来的写法从 `scoped_refptr` `release()` 出指针后直接 `delete`，绕过了引用计数。<br>头文件仍然只做前置声明，因为它会被包含 Qt 的翻译单元间接包含 |
| `.github/workflows/package.yml` | 三个平台打包时写死的运行库文件名改为 3.0 的名字：<br>- Windows 必需文件列表；<br>- macOS 的查找、架构检查和 `install_name_tool`；<br>- Linux 的拷贝和必需库检查 | 文件名变了，不改打包会失败 |

**`build.yml` 不需要改：** 它没有引用任何 APM 文件名，下载和路径全部由 CMake 模块处理。

## 四、验证情况

- [x] 发布资产、压缩包目录、DLL 导出、Linux SONAME 和 GLIBC 需求、macOS install name 都用脚本实际检查过（见第二节）。
- [x] 对照 3.0 头文件核对了 `audio_processing_module.cpp` 用到的每一个接口。
- [x] `grep` 确认仓库中（`third_party/`、`build/`、`server/` 除外）已没有 `audio-processing-2`、`2-1.dll`、`2.1.dylib`、`processing-mirror`、`mobile_mode` 的引用。
- [ ] **没有构建，也没有运行测试。** 本次 WSL 会话的 Windows 互操作没有开启（`cmd.exe` 报 "Exec format error"），WSL 里也没有 Linux 编译器。需要：
  - 作者在 Windows 上运行 `build.cmd test`。预期：
    - 配置阶段输出 `Found WebRTC Audio Processing at …/webrtc-audio-processing.windows_x86_64`，不会重新下载；
    - `build/bin` 下出现 `webrtc-audio-processing-3-0.dll`；
    - `audio_processing_tests` 和 `microphone_capturer_tests` 通过。
  - PR 触发的 `build.yml` 检验 Linux 和 macOS 的下载、编译和测试。
- [ ] 打包流程没有运行。可以在分支上手动触发 `package.yml`（`workflow_dispatch`）来检验新的文件名。

## 五、遗留事项

- **第三方许可证：** 新包带了 `licenses/`（webrtc、abseil、pffft、rnnoise），目前没有打进发布包。BSD 类许可证要求在二进制分发中附带声明，建议后续补上。
- **本地旧目录：** `third_party/webrtc-audio-processing-windows-x64` 是 2.1 的旧目录，已不再使用，可以删除。
- **macOS arm64：** `install_name_tool` 修改 arm64 dylib 后可能需要重新签名。CI 目前只在 Intel macOS 上构建，这次没有处理。
- **需要在通话中确认：** 回声消除、降噪、自动增益在 3.0 下效果正常。
