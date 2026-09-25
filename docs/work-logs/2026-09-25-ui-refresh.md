# 工作记录：界面整体翻新（腾讯会议风格配色、紧凑首页、设置页重做、Lucide 图标）

- **日期：** 2026-09-25
- **分支：** `feat/ui-refresh`（基于 `main` 的 `d2f0a9c`）
- **关联文档：** [PR 记录](../pull-requests/2026-09-25-ui-refresh.md)（无审查归档）

## 一、用户的请求

> Please take a look at this project; I need your help optimizing the UI. The current interface—based on an older AI model's "vibe coding" style—looks outdated and unappealing. It currently uses a black-and-white color scheme. I would like the main interface of the meeting software to be more compact, similar to that of Tencent Meeting. The design should be minimalist yet sophisticated, with a harmonious color palette. The settings interface also needs optimization. Additionally, the current icons were manually downloaded and don't always look cohesive, so please replace all of them; you can search for and download suitable icons from https://allsvgicons.com/pack/lucide/ and remove the old ones.

用户附了当前首页截图和一张腾讯会议首页参考图。规划阶段用户确认了三项取舍：

- **配色**：腾讯会议式品牌蓝
- **会议窗口**：只换图标和配色，不改布局
- **首页尺寸**：约 880×580，采用紧凑布局

## 二、制定的计划

1. **图标管线**：从 `i.allsvgicons.com/lucide/<name>.svg` 下载 Lucide 图标，统一改成 24×24、白色描边，新增可着色的 `Icon.qml`（`MultiEffect` colorization），删除旧 PNG。
2. **`Theme.qml` 换色板**：保留所有旧属性名以兼容，新增 `brand` / `success` / `danger` / `icon*` / `radius*` 等 token。
3. **公共控件统一重绘**：按钮、输入框、下拉框、复选框、Tab 等；新增 `ToggleSwitch`、`BrandSlider`。
4. **首页重做**：
   - 64px 图标导轨，顶部头像弹出账号菜单
   - 2×2 纯色蓝底图标块
   - 右侧大号日期，下接会议列表
5. **设置窗口重做**：
   - 左侧图标导航，右侧页头
   - 设置项按「分组卡片 + 行」组织
   - 开关改用 Switch
6. **会议窗口与共享选择器**：只换图标、硬编码颜色改走 token，底栏略微收紧。
7. **CMake 登记**：新 QML 与 SVG 资源；链接 `Qt6::Svg`，保证部署时带上 `qsvg` 插件。

## 三、具体改了哪些文件

| 文件 | 改动 | 原因 |
| --- | --- | --- |
| `res/icon/lucide/*.svg`（43 个）、`res/icon/lucide/LICENSE` | 新增 Lucide 图标（ISC 许可证），`currentColor` 改为 `#FFFFFF`，`1em` 改为 `24` | Qt SVG 不解析 `em`；白色源图经 MultiEffect 着色后正好等于目标色（已对照 Qt `multieffect.frag`：`rgb = gray * colorizationColor`） |
| `res/icon/*.png`（34 个） | 删除 | 旧图标风格不统一，且黑色 PNG 只能靠 opacity 调深浅，无法着色 |
| `ui/qml/components/Icon.qml` | 新增 | 统一的可着色图标组件，所有图标都经它渲染 |
| `ui/qml/components/ToggleSwitch.qml`、`BrandSlider.qml` | 新增 | 设置页用的开关和滑条。命名避开 `Switch` / `Slider`，防止遮蔽 QtQuick.Controls 的同名类型导致递归 |
| `ui/qml/theme/Theme.qml` | 新色板 | 浅色 / 深色各一套，旧属性名全部保留；`iconOpacity` 已无引用，删除 |
| `ui/qml/components/*.qml`（IconButton、PrimaryButton、SecondaryButton、TextField、ComboBox、CheckBox、PillToggle、TabButton、LinkButton、TitleBar） | 重绘 | 统一高度（36/40）、圆角 8、品牌色、hover 态。`IconButton` 改为接收 `iconName`；`SecondaryButton` 新增 `soft` 变体；`TitleBar` 新增 `showTitle` |
| `ui/qml/home/HomeWindow.qml` | 重排 | 窗口 880×580；布局为「导轨 \| 操作区 \| 日期与会议列表」；背景为品牌色淡渐变。所有 backend、模型、对话框逻辑原样保留 |
| `ui/qml/home/HomeSidebar.qml`、`HomeNavButton.qml`、`HomeUserCard.qml` | 重写 | 导轨仅显示图标，选中项左侧有指示条；头像显示首字母，点击弹出账号菜单（游客弹「登录 / 注册」）；设置入口移到导轨底部 |
| `ui/qml/home/QuickActionCard.qml`、`HomeMeetingPage.qml` | 重写 | 64px 蓝色方块加下方文字；游客点「快速会议」「预定会议」时方块带锁角标，点击仍走原有登录提示；操作弹窗改挂到 `Overlay.overlay`，并按窗口高度收缩 |
| `ui/qml/home/MeetingListPanel.qml`、`HostMeetingListPanel.qml`、`MeetingListItem.qml`、新增 `StatusPill.qml` | 重写 | 扁平列表行，第二行为「时间 · 会议号」；状态胶囊按状态着色（进行中为蓝底、待开始为橙色）；空状态加图标。删除了未被调用的 `headerTitle` / `actionText` 等属性 |
| `ui/qml/home/HomeRecordingPage.qml` | 样式 | 空状态和列表行改成新风格 |
| `ui/qml/home/AuthModal.qml`、`GuestPromptDialog.qml`、`CancelMeetingDialog.qml`、`MeetingPasswordDialog.qml`、`ScheduleTimeSelect.qml`、`ui/qml/login/*.qml` | 颜色改 token、加阴影 | 原先写死的浅色值在深色主题下不可读；登录弹窗高度按窗口收缩 |
| `ui/qml/settings/SettingsWindow.qml` | 重写 | 窗口 680×500；左侧导航用 Repeater 生成；右侧页头显示标题和描述；修复侧栏自带 `radius: 16` 在右边缘露出圆角的问题 |
| `ui/qml/settings/SettingsSection.qml`、`SettingsRow.qml`、`SegmentedPicker.qml` | 新增 | 设置页的分组卡片、行、分段选择器 |
| `ui/qml/settings/AudioSettings.qml`、`VideoSettings.qml`、`NetworkSettings.qml`、`AppearanceSettings.qml` | 重写布局 | 所有 `backend.*` 读写保持不变，只把 CheckBox 换成 Switch 行、把自绘分段按钮换成 SegmentedPicker |
| `ui/qml/conference/*.qml`、`ui/qml/screenpicker/*.qml` | 换图标与颜色 | 静音或关摄像头显示为红色（启用了 `ParticipantItem` 里原本注释掉的着色逻辑）；挂断和结束共享改为红底白图标；视频浮层上的图标改为白色。三个文件内的局部 `IconButton` 组件改名，避免与公共 `IconButton` 混淆 |
| `ui/qml/home/HomeContactsPage.qml`、`InviteOptionCard.qml` | 删除 | 均未登记到 CMake，属于死代码，而且仍引用旧 PNG |
| `CMakeLists.txt` | 登记 | `find_package` 增加 `Svg` 并链接 `Qt6::Svg`；在 `QML_FILES` 中加入 7 个新 QML；`RESOURCES` 中的 PNG 列表替换为 SVG 列表 |

## 四、验证情况

- **qmllint**：用 Windows Qt 6.10 的 `qmllint.exe`（经 WSL 调用）检查了全部 QML。改动文件没有语法错误，也没有新增的属性或类型错误。剩余警告都是改动前就存在的：
  - 布局内直接设置 width / height
  - `Popup.window`
  - `VideoRenderer` 属性
  - 由 `qmlRegisterType` 注册的 backend 类型没有 qmltypes 导致的误报
- **资源核对**：用脚本核对了 QML 中引用的每个图标名，都有对应的 SVG 文件，也都登记进了 `RESOURCES`；仓库内已没有对旧 `res/icon/*.png` 的引用（应用图标 `appIcon` 除外）。
- **未构建、未运行**：在 WSL 中执行，没有跑 `build.cmd release`，也没有启动程序看实际效果；`ctest` 也没跑（本次不涉及 C++ 改动）。需要在 Windows 上补跑并重点检查：
  - 首页浅色 / 深色效果
  - 游客态与登录态
  - 头像菜单
  - 四个操作弹窗
  - 设置页的保存
  - 会议内麦克风、摄像头的切换和着色
  - 屏幕共享浮动条
  - 150% DPI 下图标是否清晰
- **CMake 重新配置**：本次新增了 `Svg` 组件，需要重新 configure。本地 Qt 6.10 已带 `qsvg.dll`；CI 的 `install-qt-action` 默认安装包含 qtsvg。

## 五、遗留事项

- 实际渲染效果尚未确认，间距、字号可能需要按截图再微调。
- 头像目前只显示用户名首字母，没有接入真实头像。
- 腾讯会议参考图中「快速会议」「预定会议」旁的下拉菜单没有实现，因为后端没有对应功能。
- 会议窗口只换了配色和图标，没有重新设计布局，这是按用户的选择执行的。
- `ui/qml/home/LoginPromptCard.qml`、`screenpicker/GhostButton.qml`、`ShareButton.qml` 已登记进 CMake，但没有任何地方使用。本次保留未删：GhostButton 和 ShareButton 的颜色改成了 token，LoginPromptCard 原本就在用 Theme，没有改动。

## 六、后续修复：首次运行时 `Icon.qml` 加载失败（2026-09-25）

用户在 Windows 上执行 `build.cmd release` 构建成功，但启动时报错：

> qrc:/ui/qml/components/Icon.qml:16:21: Invalid property assignment: "implicitHeight" is a read-only property

原因是 `Image` 的 `implicitWidth` / `implicitHeight` 由图片源决定，是只读属性，qmllint 没有检查出来。修复方法：`Icon.qml` 的根元素改为 `Item`，由 `Item` 设置隐式尺寸，内部的 `Image` 填满父元素并负责着色。组件对外的接口（`name` / `size` / `color`）不变。修复后重新跑了 qmllint，没有新的报错。尚未重新构建和运行。

## 七、后续修复：首页操作区被挤到导轨边缘（2026-09-25）

用户运行后截图显示：右侧「日期 + 会议记录」栏占满了整个内容区，2×2 操作块被挤成约 0 宽，堆在左侧导轨边缘，一半被导轨遮住。

原因：Qt Quick Layouts 中，嵌套在布局里的子布局（`ColumnLayout` / `RowLayout` 等）的 `Layout.fillWidth` 默认为 `true`。右侧栏是 `ColumnLayout`，只设置了 `Layout.preferredWidth: 300`，于是和设置了 `fillWidth` 的 `StackLayout` 一起分配剩余空间，结果把后者挤没了。

修复：

- `HomeWindow.qml` 右侧栏：显式设置 `Layout.fillWidth: false`，并用 `Layout.maximumWidth: 300` 固定宽度。
- `AudioSettings.qml` 中「高级音频设置」的折叠行（`RowLayout`）：同样设置 `Layout.fillWidth: false`，避免整行宽度都能点击。

尚未重新构建和运行。

## 八、后续修复：设置页关闭按钮位置、hover 闪烁（2026-09-25）

用户反馈两个问题（附深色主题设置页截图）：

1. 设置页右上角的关闭按钮紧跟在标题文字后面，没有靠右。
2. 按钮 hover 后，鼠标移出的瞬间背景会闪一下。

**原因与修复：**

| 问题 | 原因 | 修复 |
| --- | --- | --- |
| 关闭按钮位置 | 页头标题所在的 `ColumnLayout` 只包含不填充的 `Text`，而布局的最大宽度由子项决定，所以即使设置了 `Layout.fillWidth: true` 也无法伸展，关闭按钮就排在了文字后面 | `SettingsWindow.qml` 中两个 `Text` 设置 `Layout.fillWidth: true` 并加 `elide` |
| hover 闪烁 | hover 背景使用了 `Behavior on color`，而静止状态写的是 `"transparent"`，即 `#00000000`（透明**黑**）。`ColorAnimation` 按 RGBA 逐分量插值，过程中会经过半透明的深色帧，鼠标移出时就闪一下深色；关闭按钮是从红色过渡，会闪暗红 | `Theme.qml` 新增 `clearOf(c)`（同色相、alpha 为 0）和 `hoverClear` token；受影响的 8 处 hover 背景（`IconButton`、`HomeNavButton`、`MeetingListItem`、`HostMeetingListPanel`、`SegmentedPicker`、`SettingsWindow` 导航、`ControlBar`、`FloatingControlBar`）的静止色改为它们，动画过程只改变透明度 |

**顺带修复：** 同类布局问题还出现在 `HostMeetingListPanel.qml` 的「暂无预定会议」空状态：内容会靠左而不是居中。修复方法是让上下占位 `Item` 同时设置 `Layout.fillWidth`。

**验证情况：** 修复后重新跑了 qmllint，没有新的报错。尚未重新构建和运行。
