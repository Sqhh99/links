# Qt 6 QML Module 最佳实践指南

## 概述

从传统的 `.qrc` 和手动 `qmldir` 迁移到 Qt 6 的 `qt_add_qml_module` 是推荐的现代化方式。这是 Qt 6 官方推荐的 QML 模块组织方式，提供更好的类型安全、编译时检查和开发体验。本文档总结了使用过程中的关键技术点和注意事项。

## 技能要点

### 1. CMake 配置

#### 基础配置
```cmake
qt_add_qml_module(${PROJECT_NAME}
    URI MyApp                         # 模块 URI，用于 QML import
    VERSION 1.0                       # 模块版本
    RESOURCE_PREFIX /                 # 资源前缀（根路径）
    NO_RESOURCE_TARGET_PATH           # 不使用目标路径，简化资源路径
    QML_FILES
        qml/Main.qml
        qml/components/CustomButton.qml
        qml/pages/HomePage.qml
        # ... 其他 QML 文件
    RESOURCES
        resources/icons/app_icon.png
        resources/data/config.json
        resources/translations/app_zh_CN.qm
        # ... 其他资源文件
)
```

#### 单例类型配置
```cmake
# 在 qt_add_qml_module 之前设置单例属性
set_source_files_properties(qml/core/AppSettings.qml PROPERTIES
    QT_QML_SINGLETON_TYPE TRUE
)
```

### 2. 资源路径规则

#### 生成的资源路径结构
使用 `RESOURCE_PREFIX /` 和 `NO_RESOURCE_TARGET_PATH` 后：
- QML 文件：`:/qml/Main.qml`、`:/qml/components/StyledButton.qml`
- 资源文件：`:/resources/icons/app_icon.png`、`:/resources/game_mappings.json`
- qmldir 文件：`:/qmldir`

#### C++ 端访问资源
```cpp
// 直接使用简化的路径
QFile file(":/resources/data/config.json");
m_translator->load(":/resources/translations/app_en_US");
```

#### QML 端访问资源
```qml
// 使用 qrc:/ 前缀
Image {
    source: "qrc:/resources/icons/app_icon.png"
}

// 推荐：封装工具函数
// ThemeProvider.qml
function assetUrl(path) {
    return "qrc:/resources/" + path
}

// 使用
Image {
    source: ThemeProvider.assetUrl("icons/app_icon.png")
}
```

### 3. QML 导入方式

#### 旧方式（相对路径）
```qml
import QtQuick
import "../themes"
import "../components"
```

#### 新方式（模块导入）
```qml
import QtQuick
import MyApp  // 统一模块导入
```

#### 自动注册的组件
`qt_add_qml_module` 会自动生成 `qmldir`，所有 QML_FILES 中的组件都会自动注册：
```
module MyApp
Main 1.0 qml/Main.qml
CustomButton 1.0 qml/components/CustomButton.qml
AppSettings 1.0 qml/core/AppSettings.qml  # 单例
```

### 4. 单例模式

#### QML 端声明
```qml
pragma Singleton
import QtQuick

QtObject {
    id: appSettings
    
    // 单例属性
    readonly property string appName: "MyApp"
    readonly property int version: 1
    
    // 单例方法
    function getSetting(key) {
        // ...
    }
}
```

#### CMake 端配置
```cmake
set_source_files_properties(qml/core/AppSettings.qml PROPERTIES
    QT_QML_SINGLETON_TYPE TRUE
)
```

#### 使用单例
```qml
import MyApp

ApplicationWindow {
    title: AppSettings.appName  // 直接使用，无需实例化
    
    Component.onCompleted: {
        console.log(AppSettings.getSetting("theme"))
    }
}
```

### 5. C++ 端加载 QML

#### 推荐方式：直接加载 QML 文件
```cpp
QQmlApplicationEngine engine;

// 添加模块导入路径
engine.addImportPath("qrc:/");

// 加载主 QML 文件
const QUrl url(QStringLiteral("qrc:/qml/Main.qml"));
engine.load(url);
```

#### ❌ 不推荐：loadFromModule（需要显式类型）
```cpp
// 仅在 QML 文件显式声明为类型时才能使用
// 对于普通 ApplicationWindow 不适用
engine.loadFromModule("MyApp", "Main");  // 会失败
```

### 6. 迁移步骤总结

1. **更新 CMakeLists.txt**
   - 移除旧的 `qt6_add_resources()`
   - 添加 `qt_add_qml_module()`
   - 配置单例属性

2. **更新 QML 导入**
   - 全局替换相对导入为模块导入
   - 移除所有手动 `qmldir` 文件

3. **更新资源路径**
   - C++ 端：`:/resources/...`
   - QML 端：`qrc:/resources/...`
   - 创建工具函数简化路径拼接

4. **更新 C++ 加载代码**
   - 添加 `engine.addImportPath("qrc:/")`
   - 使用 `qrc:/qml/Main.qml` 加载

5. **清理旧文件**
   - 删除 `.qrc` 文件
   - 删除手动 `qmldir` 文件

6. **验证和测试**
   - 检查资源是否正确加载
   - 验证模块导入是否工作
   - 测试单例是否可访问

## 常见问题

### ❌ Module contains no type named "Main"
**原因**：使用 `loadFromModule()` 但 Main.qml 不是显式类型

**解决**：改用 `engine.load(QUrl("qrc:/qml/Main.qml"))`

### ❌ Type is not a type
**原因**：缺少 `engine.addImportPath("qrc:/")`

**解决**：在加载 QML 前添加导入路径

### ❌ 资源文件找不到
**原因**：资源路径前缀不正确

**解决**：
- 检查 `RESOURCE_PREFIX` 设置
- 使用 `:/resources/...` 而非 `:/MyApp/resources/...`（启用 NO_RESOURCE_TARGET_PATH 时）
- 验证生成的 `.qrc` 文件中的路径（位于 `build/.qt/rcc/` 目录）

### ❌ 单例未生效
**原因**：
1. 缺少 `pragma Singleton`
2. 缺少 `QT_QML_SINGLETON_TYPE` 属性
3. CMake 配置在 `qt_add_qml_module` 之后

**解决**：
```cmake
# 必须在 qt_add_qml_module 之前
set_source_files_properties(qml/themes/ThemeProvider.qml PROPERTIES
    QT_QML_SINGLETON_TYPE TRUE
)

qt_add_qml_module(...)
```

## 最佳实践

### ✅ 使用工具函数封装资源路径
```qml
// ResourceHelper.qml (单例)
function assetUrl(path) {
    return "qrc:/resources/" + path
}

// 使用
Image { source: ResourceHelper.assetUrl("icons/delete.png") }
```

### ✅ 统一模块导入
```qml
// 推荐：单一导入语句
import MyApp

// 避免：相对路径导入
import "../components"
import "../core"
import "../pages"
```

### ✅ 保持资源路径一致性
- C++ 使用 `:/resources/...`
- QML 使用 `qrc:/resources/...`
- 封装工具函数避免重复

### ✅ 使用 NO_RESOURCE_TARGET_PATH
简化资源路径，避免深层嵌套如 `:/qt/qml/MyApp/...`。使用 `NO_RESOURCE_TARGET_PATH` 后资源直接位于 `:/resources/...`，更简洁直观。

### ✅ 清理构建目录
迁移后清理 `.qt` 目录避免缓存问题：
```powershell
Remove-Item -Recurse -Force build\.qt
cmake --build build --config Release
```

## 参考资源

- [Qt 6 QML Modules](https://doc.qt.io/qt-6/qt-add-qml-module.html)
- [Qt CMake Policy QTP0004](https://doc.qt.io/qt-6/qt-cmake-policy-qtp0004.html)
- [QML Import System](https://doc.qt.io/qt-6/qtqml-modules-topic.html)

## 适用版本

- **Qt 版本**: 6.2+（推荐 6.5 或更高）
- **CMake 版本**: 3.16+（推荐 3.21 或更高）
- **文档更新**: 2026-01-10
