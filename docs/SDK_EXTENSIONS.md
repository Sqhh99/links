# SDK扩展 - 音视频捕获功能实现

## 已完成的工作

### 1. SDK层扩展 (C++ SDK)

#### 新增文件:

**VideoSource** (`include/livekit/video_source.h`, `src/video_source.cpp`)
- 创建视频源用于摄像头和屏幕捕获
- 支持RGBA格式的视频帧捕获
- 自动处理时间戳
- 可创建LocalVideoTrack用于发布

**AudioSource** (`include/livekit/audio_source.h`, `src/audio_source.cpp`)
- 创建音频源用于麦克风捕获
- 支持48kHz采样率,单声道/立体声
- 内置回声消除、噪声抑制、自动增益控制
- 可创建LocalAudioTrack用于发布

#### 关键API:

```cpp
// 创建视频源
auto videoSource = std::make_shared<livekit::VideoSource>(640, 480);

// 捕获视频帧 (RGBA格式)
videoSource->CaptureFrame(rgba_data, width, height);

// 创建track并发布
auto videoTrack = videoSource->CreateTrack("camera");
localParticipant->PublishTrack(videoTrack, options);

// 创建音频源
auto audioSource = std::make_shared<livekit::AudioSource>(48000, 1);

// 捕获音频帧 (int16格式)
audioSource->CaptureFrame(audio_samples, samples_per_channel);

// 创建track并发布
auto audioTrack = audioSource->CreateTrack("microphone");
localParticipant->PublishTrack(audioTrack, options);
```

### 2. Qt应用层实现

#### 新增文件:

**CameraCapturer** (`core/camera_capturer.h`, `core/camera_capturer.cpp`)
- 使用Qt Multimedia捕获摄像头
- 自动转换视频帧为RGBA格式
- 实时推送到LiveKit VideoSource
- 支持设备枚举

**MicrophoneCapturer** (`core/microphone_capturer.h`, `core/microphone_capturer.cpp`)
- 使用Qt Multimedia捕获麦克风
- 自动处理音频格式转换
- 实时推送到LiveKit AudioSource
- 支持设备枚举

**ScreenCapturer** (`core/screen_capturer.h`, `core/screen_capturer.cpp`, `core/desktop_capture/`)
- 基于WebRTC架构的模块化设计
- 支持 Windows Graphics Capture (WGC), DXGI, GDI 多种捕获方式
- 自动优选最佳捕获策略
- 实时推送到LiveKit VideoSource


#### 使用示例:

```cpp
// 创建摄像头捕获器
auto cameraCapturer = new CameraCapturer(this);

// 启动捕获
if (cameraCapturer->start()) {
    // 获取VideoSource
    auto videoSource = cameraCapturer->getVideoSource();
    
    // 创建track
    auto videoTrack = videoSource->CreateTrack("camera");
    
    // 发布track
    proto::TrackPublishOptions options;
    options.set_source(proto::SOURCE_CAMERA);
    localParticipant->PublishTrack(videoTrack, options);
}

// 创建麦克风捕获器
auto micCapturer = new MicrophoneCapturer(this);

// 启动捕获
if (micCapturer->start()) {
    // 获取AudioSource
    auto audioSource = micCapturer->getAudioSource();
    
    // 创建track
    auto audioTrack = audioSource->CreateTrack("microphone");
    
    // 发布track
    proto::TrackPublishOptions options;
    options.set_source(proto::SOURCE_MICROPHONE);
    localParticipant->PublishTrack(audioTrack, options);
    options.set_source(proto::SOURCE_MICROPHONE);
    localParticipant->PublishTrack(audioTrack, options);
}

// 创建屏幕捕获器
auto screenCapturer = new ScreenCapturer(this);

// 启动捕获 (屏幕或窗口)
screenCapturer->setScreen(QGuiApplication::primaryScreen()); // 或 setWindow(id)
if (screenCapturer->start()) {
    auto videoSource = screenCapturer->getVideoSource();
    auto videoTrack = videoSource->CreateTrack("screen_share");
    
    proto::TrackPublishOptions options;
    options.set_source(proto::SOURCE_SCREEN_SHARE);
    localParticipant->PublishTrack(videoTrack, options);
}
```

### 3. 构建配置更新

#### SDK CMakeLists.txt
- 添加了 `video_source.cpp` 和 `audio_source.cpp` 到构建
- 导出了 `video_source.h` 和 `audio_source.h` 头文件
- 更新了 `livekit.h` 主头文件

#### Qt Conference CMakeLists.txt
- 添加了 `camera_capturer.cpp`, `microphone_capturer.cpp` 和 `screen_capturer.cpp`
- 添加了 `core/desktop_capture/` 模块源文件
- 添加了对应的头文件

## 技术细节

### 视频流程

1. **Qt捕获**: QCamera → QVideoSink → QVideoFrame
2. **格式转换**: QVideoFrame → QImage (RGBA8888)
3. **推送到SDK**: VideoSource::CaptureFrame()
4. **FFI调用**: CaptureVideoFrameRequest → Rust SDK
5. **WebRTC编码**: Rust → WebRTC → 网络传输

### 音频流程

1. **Qt捕获**: QAudioSource → QIODevice → QByteArray
2. **格式转换**: QByteArray → int16_t array
3. **推送到SDK**: AudioSource::CaptureFrame()
4. **FFI调用**: CaptureAudioFrameRequest → Rust SDK
5. **WebRTC编码**: Rust → WebRTC → 网络传输

### 性能优化

- **视频**: 每30帧记录一次日志,避免日志过多
- **音频**: 每秒记录一次日志
- **零拷贝**: 使用指针传递,避免数据复制
- **异步处理**: Qt信号槽机制确保非阻塞

## 下一步集成

### ConferenceManager需要添加:

```cpp
class ConferenceManager {
private:
    CameraCapturer* cameraCapturer_;
    MicrophoneCapturer* micCapturer_;
    std::shared_ptr<LocalVideoTrack> localVideoTrack_;
    std::shared_ptr<LocalAudioTrack> localAudioTrack_;
    
public:
    void toggleCamera() {
        if (cameraEnabled_) {
            // 启动摄像头
            if (cameraCapturer_->start()) {
                auto videoSource = cameraCapturer_->getVideoSource();
                localVideoTrack_ = videoSource->CreateTrack("camera");
                
                proto::TrackPublishOptions options;
                options.set_source(proto::SOURCE_CAMERA);
                
                auto localParticipant = room_->GetLocalParticipant();
                localParticipant->PublishTrack(localVideoTrack_, options);
            }
        } else {
            // 停止摄像头
            cameraCapturer_->stop();
            if (localVideoTrack_) {
                auto localParticipant = room_->GetLocalParticipant();
                localParticipant->UnpublishTrack(localVideoTrack_->GetSid());
                localVideoTrack_ = nullptr;
            }
        }
    }
    
    void toggleMicrophone() {
        // 类似的实现...
    }
};
```

## 编译和测试

### 编译命令:
```bash
cd client-sdk-cpp
./build.sh release
```

### 测试步骤:
1. 启动LiveKit服务器 (localhost:7880)
2. 启动API服务器 (localhost:8081)
3. 运行Qt Conference应用
4. 加入会议
5. 点击摄像头/麦克风按钮
6. 查看日志确认帧捕获
7. 在Web端查看是否能看到视频/听到音频

### 预期日志:
```
[INFO] VideoSource created with handle: 123
[INFO] Using camera: Integrated Camera
[INFO] Camera started
[DEBUG] Captured 30 frames (640x480)
[DEBUG] Captured 60 frames (640x480)

[INFO] AudioSource created with handle: 124 (rate: 48000, channels: 1)
[INFO] Using microphone: Default Microphone
[INFO] Microphone started (rate: 48000, channels: 1)
[DEBUG] Captured 48000 audio samples
```

## 已知限制

1. **设备选择**: 目前使用默认设备,需要添加设备选择UI
2. **视频质量**: 固定640x480 (摄像头), 屏幕共享取决于源分辨率
3. **音频质量**: 固定48kHz单声道,可能需要立体声支持

## 文件清单

### SDK层 (7个文件)
- `include/livekit/video_source.h`
- `src/video_source.cpp`
- `include/livekit/audio_source.h`
- `src/audio_source.cpp`
- `include/livekit/livekit.h` (已更新)
- `CMakeLists.txt` (已更新)

### Qt应用层 (主要文件)
- `core/camera_capturer.h/cpp`
- `core/microphone_capturer.h/cpp`
- `core/screen_capturer.h/cpp`
- `core/desktop_capture/` (模块目录)
- `CMakeLists.txt` (已更新)

## 总结

✅ **完成**:
- SDK视频/音频源API
- Qt摄像头捕获
- Qt麦克风捕获
- 屏幕/窗口捕获
- 构建配置更新
- 完整的数据流管道

⏳ **待完成**:
- 集成到ConferenceManager
- 添加到UI控制
- 设备选择UI
- 测试和调试

现在可以编译并测试基本的音视频捕获功能了!
