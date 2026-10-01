# 工作记录：更换应用图标

- **日期：** 2026-10-01
- **分支：** `chore/new-app-icon`（基于 `main` 的 `fd3c109`）
- **关联文档：** [PR 记录](../pull-requests/2026-10-01-new-app-icon.md)（[Sqhh99/links#32](https://github.com/Sqhh99/links/pull/32)）

## 一、用户的请求

> Help me change the software icon to this, and push the PR. Delete the old resource files and put the new icon files in the standard location. @res/appIcon.png @res/appIcon.ico

用户提供了两个新文件：

- `res/appIcon.png`：1254×1254 RGBA 原图；
- `res/appIcon.ico`：包含 16、32、48、64、128、256 六个尺寸的 PNG 帧。

要求用它们替换旧图标，放到项目约定的位置，并提交 PR。

## 二、制定的计划

1. **确认“标准位置”。** 构建系统已经约定了图标路径（`CMakeLists.txt` 第 12–19 行、`main.cpp` 第 121–128 行）：
   - `res/icon/appIcon/linux/icon_<N>x<N>.png`（N = 16/32/48/64/128/256），用于窗口图标（qrc）和 Linux 安装；
   - `res/icon/appIcon/macos/AppIcon.iconset/icon_<N>x<N>.png`（N = 16/32/128/256/512），由 `iconutil` 生成 `.icns`；
   - `res/icon/appIcon/windows/icon.ico`，由 `app_icon.rc.in` 嵌入 exe。

   沿用这些路径，就不需要改任何 CMake 或 C++ 代码。
2. **生成各尺寸 PNG。**
   - 16–256：直接从用户的 `.ico` 中取出对应的 PNG 帧，保证与 Windows 图标逐像素一致；
   - macOS 的 512：`.ico` 里没有，用原图缩放生成。环境里没有 Pillow / ImageMagick / pip，所以写了一个纯 Python 的 PNG 读写和面积平均缩放脚本（预乘 alpha，避免透明边缘发黑），放在会话临时目录，不进仓库。
3. **替换文件。** 新文件覆盖旧文件；原图移到 `res/icon/appIcon/appIcon.png` 作为以后重新生成各尺寸的源文件；`res/` 根目录下的两个临时文件不再保留。
4. 写记录、提交、推送并开 PR。

## 三、具体改了哪些文件

| 文件 | 改动 | 原因 |
| --- | --- | --- |
| `res/icon/appIcon/windows/icon.ico` | 替换为用户提供的 `appIcon.ico`（6 个尺寸，旧的是 4 个） | Windows exe 图标 |
| `res/icon/appIcon/linux/icon_{16,32,48,64,128,256}x*.png` | 替换为新 `.ico` 中对应尺寸的 PNG 帧 | 运行时窗口图标（`main.cpp` 通过 qrc 加载）和 Linux hicolor 安装图标 |
| `res/icon/appIcon/macos/AppIcon.iconset/icon_{16,32,128,256}x*.png` | 同上 | macOS `.icns` |
| `res/icon/appIcon/macos/AppIcon.iconset/icon_512x512.png` | 从 1254×1254 原图缩放生成 | `.ico` 中没有 512 |
| `res/icon/appIcon/appIcon.png`（新增） | 用户提供的原图 | 作为源文件保存，以后换尺寸或补 `@2x` 时用；不注册进 qrc，不影响构建产物 |

没有改动 `CMakeLists.txt`、`main.cpp` 或打包脚本：文件名和路径都没变。

## 四、验证情况

- [x] 用脚本解析了新 `.ico` 的目录项，确认 6 帧都是 8 位 RGBA PNG，尺寸与文件名一致。
- [x] 目视检查了生成的 512×512 和 32×32 图标，边缘和透明区域正常。
- [x] `grep` 确认 CMake、`main.cpp`、`res/linux/links.desktop.in` 引用的图标路径都仍然存在。
- [ ] **没有构建，也没有运行程序。** 本次在 WSL 中进行，没有执行 Windows 构建。需要在 Windows 上 `build.cmd release` 后确认 exe 图标、任务栏和窗口标题栏图标已更新；macOS / Linux 由 PR 的 CI 构建检验。

## 五、遗留事项

- macOS iconset 只有 1x 尺寸（沿用原有结构），没有 `@2x` 文件，Retina 屏上 Dock 图标会用 512 放大。如需要，可从 `appIcon.png` 补生成 `icon_512x512@2x.png`（1024）等文件。
- Windows 资源缓存可能导致资源管理器仍显示旧图标，需要清理图标缓存后再确认。
