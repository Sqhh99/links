# 编译指南

## 修复的问题

✅ **CMake配置错误已修复**:
- 移除了不存在的 `include/livekit/handle.h` 引用
- 修复了 `FfiHandle` 结构的比较运算符类型

## 编译步骤

### Windows (使用CMake + Visual Studio)

```bash
# 1. 配置项目
cd d:\workspace\lib-workspace\client-sdk-cpp
cmake -B build -G "Visual Studio 17 2022" -A x64

# 2. 编译 (Release模式)
cmake --build build --config Release

# 3. 运行Qt Conference应用
.\build\bin\Release\QtConference.exe
```

### 或者使用build.sh脚本

```bash
./build.sh release
```

## 预期输出

编译成功后,您应该看到:
- `build/bin/Release/QtConference.exe` - Qt会议应用程序
- `build/bin/Release/livekit.dll` - LiveKit SDK动态库

## 运行前准备

1. **启动LiveKit服务器**:
   ```bash
   # 在另一个终端
   cd webrtc-signaling-server
   go run main.go
   ```
   - LiveKit服务器: `ws://localhost:7880`
   - API服务器: `http://localhost:8081`

2. **运行Qt应用**:
   ```bash
   .\build\bin\Release\QtConference.exe
   ```

3. **测试流程**:
   - 输入用户名和房间名
   - 点击"Join Meeting"
   - 点击摄像头按钮启动视频
   - 点击麦克风按钮启动音频
   - 查看日志确认捕获正常

## 预期日志

```
[INFO] Logger initialized
[INFO] Application started
[INFO] API URL set to: http://localhost:8081
[INFO] LoginWindow created
[INFO] Requesting token for room 'test', user 'user1'
[INFO] Token received successfully
[INFO] ConferenceManager created
[INFO] ConferenceWindow created for room: test
[INFO] Connecting to room: ws://localhost:7880
[INFO] Connection initiated
[INFO] Connected to room: test
[INFO] Local participant: user1

# 启动摄像头后:
[INFO] VideoSource created with handle: 123
[INFO] Using camera: Integrated Camera
[INFO] Camera started
[DEBUG] Captured 30 frames (640x480)
[DEBUG] Captured 60 frames (640x480)

# 启动麦克风后:
[INFO] AudioSource created with handle: 124 (rate: 48000, channels: 1)
[INFO] Using microphone: Default Microphone
[INFO] Microphone started (rate: 48000, channels: 1)
[DEBUG] Captured 48000 audio samples
```

## 常见问题

### 1. Qt找不到
**错误**: `Could not find Qt6`
**解决**: 确保CMakeLists.txt中的Qt路径正确:
```cmake
set(CMAKE_PREFIX_PATH "D:/Qt/6.10.0/msvc2022_64")
```

### 2. 摄像头无法启动
**错误**: `No cameras available`
**解决**: 
- 检查摄像头是否被其他应用占用
- 确保Qt Multimedia模块正确安装
- 检查摄像头权限

### 3. 麦克风无法启动
**错误**: `No microphone available`
**解决**:
- 检查系统音频设备
- 确保麦克风权限已授予
- 检查Qt Multimedia配置

### 4. 视频无法传输
**可能原因**:
- LiveKit服务器未运行
- 网络连接问题
- Track未正确发布

**调试**:
- 查看日志中的错误信息
- 使用Web客户端测试服务器
- 检查防火墙设置

## 下一步

编译成功后,您需要:

1. **集成到ConferenceManager** - 将CameraCapturer和MicrophoneCapturer集成到toggleCamera/toggleMicrophone方法中
2. **测试音视频传输** - 使用Web客户端验证能否看到视频/听到音频
3. **实现屏幕共享** - 添加屏幕捕获功能
4. **优化性能** - 调整视频分辨率和帧率

详细说明请参考 `SDK_EXTENSIONS.md`
