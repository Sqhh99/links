# PR：更换应用图标

- **日期：** 2026-10-01
- **分支：** `chore/new-app-icon` → `main`
- **基线提交：** `fd3c109`
- **PR：** [Sqhh99/links#32](https://github.com/Sqhh99/links/pull/32)
- **关联记录：** [工作记录](../work-logs/2026-10-01-new-app-icon.md)

## 关联

把应用图标换成新设计（橙色圆角底、白色小狗头像）。

## 改了什么

- **Windows：** `res/icon/appIcon/windows/icon.ico` 换成新图标，包含 16/32/48/64/128/256 六个尺寸（原来是 4 个）。
- **窗口图标和 Linux：** `res/icon/appIcon/linux/` 下 6 个 PNG 换成新图标，直接取自新 `.ico` 的对应帧。
- **macOS：** `AppIcon.iconset` 下 5 个 PNG 换成新图标；`.ico` 中没有的 512×512 由原图缩放生成。
- **原图：** 1254×1254 的 `appIcon.png` 保存在 `res/icon/appIcon/appIcon.png`，以后重新生成尺寸时使用，不打进程序。

所有文件名和路径不变，所以 CMake、`main.cpp`、打包脚本都不需要修改。

## 怎么验证

- [x] 解析新 `.ico`，确认 6 帧都是 RGBA PNG，尺寸与文件名一致。
- [x] 目视检查生成的 512×512 和 32×32 图标。
- [ ] `build.cmd release`：没有执行，本次在 WSL 中完成。需要在 Windows 上构建后确认 exe、任务栏和窗口图标已更新。
- [ ] Linux / macOS：由本 PR 触发的 CI 构建检验。

## 检查项

- [ ] 已阅读 CONTRIBUTING.md（仓库中当前不存在该文件）
- [ ] UI / QML 改动附了截图（只改了图标资源，没有截图）
- [x] 若改动了翻译库、i18n、模型或打包资源，已在上文写明：替换了三个平台的图标资源，路径不变。
- [x] AI 使用披露：是。尺寸生成、文件替换和记录由 Claude Code 完成；图标原图和 `.ico` 由作者提供。没有构建或运行。
