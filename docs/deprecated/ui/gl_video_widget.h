#ifndef GL_VIDEO_WIDGET_H
#define GL_VIDEO_WIDGET_H

#include <QOpenGLWidget>
#include <QOpenGLFunctions>
#include <QOpenGLShaderProgram>
#include <QOpenGLTexture>
#include <QOpenGLBuffer>
#include <QOpenGLVertexArrayObject>
#include <QLabel>
#include <QString>
#include <QImage>
#include <QMutex>
#include <memory>
#include <atomic>
#include "livekit/track.h"

/**
 * @brief OpenGL accelerated video widget for efficient video frame rendering.
 * 
 * This widget uses GPU-based rendering instead of CPU-based QPainter rendering,
 * significantly reducing CPU usage during video playback.
 * 
 * Key optimizations:
 * 1. GPU-accelerated texture rendering
 * 2. Frame rate limiting (no excessive repaints)
 * 3. Texture streaming with glTexSubImage2D
 * 4. Double buffering for smooth playback
 */
class GLVideoWidget : public QOpenGLWidget, protected QOpenGLFunctions
{
    Q_OBJECT
    
public:
    explicit GLVideoWidget(QWidget* parent = nullptr);
    ~GLVideoWidget() override;
    
    void setTrack(std::shared_ptr<livekit::Track> track);
    void clearTrack();
    
    /**
     * @brief Set video frame for rendering (thread-safe)
     * @param frame The QImage frame to render. Accepts any format but RGBA8888 is most efficient.
     */
    void setVideoFrame(const QImage& frame);
    
    /**
     * @brief Set video frame from raw RGBA data (most efficient, avoids QImage overhead)
     */
    void setVideoFrameRaw(const uint8_t* data, int width, int height, int stride);
    
    void setParticipantName(const QString& name);
    void setMuted(bool muted);
    void setMicEnabled(bool enabled);
    void setCameraEnabled(bool enabled);
    void setMirrored(bool mirrored);
    void setShowStatus(bool show);
    
    bool hasTrack() const { return track_ != nullptr; }
    QString getParticipantName() const { return participantName_; }
    
protected:
    void initializeGL() override;
    void resizeGL(int w, int h) override;
    void paintGL() override;
    void resizeEvent(QResizeEvent* event) override;
    
private:
    void initializeShaders();
    void initializeGeometry();
    void updateTexture();
    void updateOverlayWidgets();
    void drawPlaceholder();
    QString getInitials() const;
    
    // Track
    std::shared_ptr<livekit::Track> track_;
    QString participantName_;
    bool isMuted_{false};
    bool isMirrored_{false};
    bool micEnabled_{false};
    bool cameraEnabled_{true};
    bool showStatus_{true};
    
    // OpenGL resources
    QOpenGLShaderProgram* shaderProgram_{nullptr};
    QOpenGLTexture* texture_{nullptr};
    QOpenGLBuffer vbo_{QOpenGLBuffer::VertexBuffer};
    QOpenGLVertexArrayObject vao_;
    
    // Shader locations
    int positionLoc_{-1};
    int texCoordLoc_{-1};
    int textureLoc_{-1};
    int mirrorLoc_{-1};
    
    // Frame data (double buffer for thread safety)
    QMutex frameMutex_;
    QImage pendingFrame_;
    std::atomic<bool> frameReady_{false};
    int textureWidth_{0};
    int textureHeight_{0};
    bool hasFrame_{false};
    
    // UI overlay elements
    QWidget* overlayWidget_{nullptr};
    QLabel* nameLabel_{nullptr};
    QWidget* statusContainer_{nullptr};
    QLabel* micIcon_{nullptr};
    QLabel* camIcon_{nullptr};
    
    // Frame rate control
    qint64 lastFrameTime_{0};
    static constexpr int MIN_FRAME_INTERVAL_MS = 16; // ~60fps max
};

#endif // GL_VIDEO_WIDGET_H
