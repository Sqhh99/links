# LiveKit Qt Conference Application

A full-featured video conferencing application built with Qt and LiveKit C++ SDK.

## Features

- ✅ **Video Conferencing**: Real-time audio and video communication
- ✅ **Modern UI**: Dark theme with responsive layout
- ✅ **Participant Management**: View and manage meeting participants
- ✅ **Chat**: Send and receive text messages during meetings
- ✅ **Media Controls**: Toggle microphone, camera, and screen sharing
- ✅ **Connection Management**: Automatic reconnection and status monitoring
- ✅ **Multi-platform**: Works on Windows, macOS, and Linux

## Screenshots

### Login Window
Modern login interface with quick join and room creation options.

### Conference Window
- **Left Sidebar**: Local video preview and remote participant thumbnails
- **Center**: Main video display with control bar
- **Right Sidebar**: Participant list and chat panel

## Requirements

- Qt 6.x
- CMake 3.16+
- LiveKit C++ SDK (included in parent project)
- LiveKit Server running (default: localhost:7880)
- API Server running (default: localhost:8081)

## Building

The project is built as part of the main LiveKit C++ SDK build:

```bash
cd client-sdk-cpp
./build.sh release
```

The executable will be located at:
```
build/bin/QtConference
```

## Running

### 1. Start LiveKit Server

Follow the [LiveKit Server documentation](https://docs.livekit.io/oss/deployment/) to set up and run a LiveKit server.

### 2. Start the API Server

```bash
cd webrtc-signaling-server
go run main.go
```

The API server will start on `http://localhost:8081` by default.

### 3. Run the Application

```bash
./build/bin/QtConference
```

## Usage

### Joining a Meeting

1. Enter your name
2. Enter a room name (or use "Quick Join" for a random room)
3. Click "Join Meeting"

The application will:
- Request an access token from the API server
- Connect to the LiveKit server
- Join the specified room
- Enable your camera and microphone (if available)

### During the Meeting

**Control Bar** (bottom center):
- 🎤 **Microphone**: Toggle your microphone on/off
- 📹 **Camera**: Toggle your camera on/off
- 🖥️ **Screen Share**: Start/stop screen sharing
- 💬 **Chat**: Open/close the chat panel
- 👥 **Participants**: Show/hide the participants list
- 📞 **Leave**: Exit the meeting

**Video Layout**:
- Your local video appears in the left sidebar
- Remote participants appear as thumbnails below your video
- The main area shows the active speaker or selected participant

**Chat**:
- Click the chat button to open the chat panel
- Type your message and press Enter or click Send
- Messages are delivered to all participants in real-time

## Configuration

Settings are stored in your system's application data directory:

- **Windows**: `%APPDATA%/LiveKit/Conference/`
- **macOS**: `~/Library/Application Support/LiveKit/Conference/`
- **Linux**: `~/.config/LiveKit/Conference/`

You can modify:
- Server URL (default: `ws://localhost:7880`)
- API URL (default: `http://localhost:8081`)
- Default media settings (microphone/camera enabled on join)

## Architecture

### Core Components

- **NetworkClient**: HTTP client for API communication (token requests, room management)
- **ConferenceManager**: LiveKit room management and event handling
- **MediaManager**: Media device enumeration and control (placeholder for future features)

### UI Components

- **LoginWindow**: Entry point for joining/creating meetings
- **ConferenceWindow**: Main meeting interface
- **VideoWidget**: Video rendering component with participant info overlay
- **ParticipantItem**: Participant list item with status indicators
- **ChatPanel**: Chat message display and input

### Utilities

- **Logger**: Application logging to file and console
- **Settings**: Persistent configuration storage

## Known Limitations

### Video Rendering
The current implementation includes placeholder video rendering. The LiveKit C++ SDK needs to be extended to provide:
- Video frame callbacks
- Frame format conversion
- Hardware acceleration support

### Media Device Control
Basic media control is implemented, but the following features require SDK extensions:
- Device enumeration (list available cameras/microphones)
- Device selection
- Audio level monitoring
- Video resolution/quality control



## Future Enhancements

- [ ] Implement actual video rendering using Qt Multimedia or OpenGL
- [ ] Add device selection dialogs
- [ ] Implement audio level indicators
- [ ] Add recording functionality
- [ ] Support for virtual backgrounds
- [ ] Picture-in-picture mode
- [ ] Keyboard shortcuts
- [ ] Settings dialog
- [ ] Multi-language support

## Troubleshooting

### Cannot connect to server
- Ensure LiveKit server is running on the configured URL
- Check that the API server is running
- Verify firewall settings allow connections

### No video/audio
- Check camera and microphone permissions
- Verify devices are not in use by another application
- Check browser console for WebRTC errors

### Build errors
- Ensure Qt 6 is properly installed
- Verify CMake can find Qt (`CMAKE_PREFIX_PATH`)
- Check that the LiveKit SDK built successfully

## License

This project follows the same license as the LiveKit C++ SDK (Apache 2.0).

## Contributing

Contributions are welcome! Please feel free to submit issues or pull requests.

## Support

For questions and support:
- LiveKit Documentation: https://docs.livekit.io
- LiveKit Community: https://livekit.io/community
