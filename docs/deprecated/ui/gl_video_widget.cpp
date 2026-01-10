#include "gl_video_widget.h"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QResizeEvent>
#include <QIcon>
#include <QDateTime>
#include <QPainter>
#include <QDebug>

// Vertex shader - simple texture mapping with mirror support
static const char* vertexShaderSource = R"(
    #version 330 core
    layout(location = 0) in vec2 position;
    layout(location = 1) in vec2 texCoord;
    
    out vec2 fragTexCoord;
    uniform bool mirror;
    
    void main() {
        gl_Position = vec4(position, 0.0, 1.0);
        if (mirror) {
            fragTexCoord = vec2(1.0 - texCoord.x, texCoord.y);
        } else {
            fragTexCoord = texCoord;
        }
    }
)";

// Fragment shader - texture sampling with aspect ratio preservation
static const char* fragmentShaderSource = R"(
    #version 330 core
    in vec2 fragTexCoord;
    out vec4 fragColor;
    
    uniform sampler2D textureSampler;
    
    void main() {
        fragColor = texture(textureSampler, fragTexCoord);
    }
)";

// Fallback shaders for OpenGL ES / older hardware
static const char* vertexShaderSourceES = R"(
    attribute vec2 position;
    attribute vec2 texCoord;
    
    varying vec2 fragTexCoord;
    uniform bool mirror;
    
    void main() {
        gl_Position = vec4(position, 0.0, 1.0);
        if (mirror) {
            fragTexCoord = vec2(1.0 - texCoord.x, texCoord.y);
        } else {
            fragTexCoord = texCoord;
        }
    }
)";

static const char* fragmentShaderSourceES = R"(
    precision mediump float;
    varying vec2 fragTexCoord;
    
    uniform sampler2D textureSampler;
    
    void main() {
        gl_FragColor = texture2D(textureSampler, fragTexCoord);
    }
)";

GLVideoWidget::GLVideoWidget(QWidget* parent)
    : QOpenGLWidget(parent)
{
    setMinimumSize(160, 90);
    setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
    
    // Enable VSync to reduce tearing and limit frame rate naturally
    QSurfaceFormat format;
    format.setSwapInterval(1);
    format.setDepthBufferSize(0);  // No depth buffer needed for 2D
    format.setStencilBufferSize(0);
    format.setSamples(0);  // No multisampling for video
    setFormat(format);
    
    // Create overlay widget for name and status icons
    overlayWidget_ = new QWidget(this);
    overlayWidget_->setAttribute(Qt::WA_TransparentForMouseEvents);
    
    auto* overlayLayout = new QVBoxLayout(overlayWidget_);
    overlayLayout->setContentsMargins(8, 8, 8, 8);
    overlayLayout->addStretch();
    
    // Bottom row: name on left, status icons on right
    auto* bottomLayout = new QHBoxLayout();
    bottomLayout->setContentsMargins(8, 0, 8, 6);
    bottomLayout->setSpacing(8);
    bottomLayout->setAlignment(Qt::AlignBottom);

    nameLabel_ = new QLabel(overlayWidget_);
    nameLabel_->setStyleSheet(R"(
        QLabel {
            background: rgba(0,0,0,0.45);
            color: white;
            padding: 4px 8px;
            border-radius: 6px;
            font-size: 12px;
            font-weight: 600;
        }
    )");
    nameLabel_->setSizePolicy(QSizePolicy::Minimum, QSizePolicy::Fixed);
    bottomLayout->addWidget(nameLabel_);
    bottomLayout->addStretch();

    statusContainer_ = new QWidget(overlayWidget_);
    statusContainer_->setFixedHeight(24);
    statusContainer_->setMinimumWidth(48);
    statusContainer_->setSizePolicy(QSizePolicy::Minimum, QSizePolicy::Fixed);
    auto* statusLayout = new QHBoxLayout(statusContainer_);
    statusLayout->setContentsMargins(4, 0, 4, 0);
    statusLayout->setSpacing(6);
    
    micIcon_ = new QLabel(statusContainer_);
    micIcon_->setObjectName("micStateIcon");
    micIcon_->setFixedSize(20, 20);
    micIcon_->setScaledContents(true);
    
    camIcon_ = new QLabel(statusContainer_);
    camIcon_->setObjectName("camStateIcon");
    camIcon_->setFixedSize(20, 20);
    camIcon_->setScaledContents(true);
    
    statusLayout->addWidget(micIcon_);
    statusLayout->addWidget(camIcon_);
    bottomLayout->addWidget(statusContainer_, 0, Qt::AlignRight | Qt::AlignVCenter);
    statusContainer_->setVisible(showStatus_);
    
    overlayLayout->addLayout(bottomLayout);
}

GLVideoWidget::~GLVideoWidget()
{
    makeCurrent();
    
    delete texture_;
    texture_ = nullptr;
    
    vbo_.destroy();
    vao_.destroy();
    
    delete shaderProgram_;
    shaderProgram_ = nullptr;
    
    doneCurrent();
}

void GLVideoWidget::initializeGL()
{
    initializeOpenGLFunctions();
    
    // Dark background
    glClearColor(20.0f/255.0f, 20.0f/255.0f, 26.0f/255.0f, 1.0f);
    
    initializeShaders();
    initializeGeometry();
}

void GLVideoWidget::initializeShaders()
{
    shaderProgram_ = new QOpenGLShaderProgram(this);
    
    // Try OpenGL 3.3 Core first
    bool success = shaderProgram_->addShaderFromSourceCode(QOpenGLShader::Vertex, vertexShaderSource);
    success = success && shaderProgram_->addShaderFromSourceCode(QOpenGLShader::Fragment, fragmentShaderSource);
    
    if (!success) {
        // Fallback to OpenGL ES / legacy
        qDebug() << "GLVideoWidget: Falling back to ES shaders";
        shaderProgram_->removeAllShaders();
        shaderProgram_->addShaderFromSourceCode(QOpenGLShader::Vertex, vertexShaderSourceES);
        shaderProgram_->addShaderFromSourceCode(QOpenGLShader::Fragment, fragmentShaderSourceES);
    }
    
    shaderProgram_->bindAttributeLocation("position", 0);
    shaderProgram_->bindAttributeLocation("texCoord", 1);
    
    if (!shaderProgram_->link()) {
        qWarning() << "GLVideoWidget: Shader link failed:" << shaderProgram_->log();
        return;
    }
    
    positionLoc_ = shaderProgram_->attributeLocation("position");
    texCoordLoc_ = shaderProgram_->attributeLocation("texCoord");
    textureLoc_ = shaderProgram_->uniformLocation("textureSampler");
    mirrorLoc_ = shaderProgram_->uniformLocation("mirror");
}

void GLVideoWidget::initializeGeometry()
{
    // Full-screen quad with texture coordinates
    // Position (x, y) and TexCoord (u, v) interleaved
    static const float vertices[] = {
        // Position    // TexCoord
        -1.0f, -1.0f,  0.0f, 1.0f,  // Bottom-left
         1.0f, -1.0f,  1.0f, 1.0f,  // Bottom-right
         1.0f,  1.0f,  1.0f, 0.0f,  // Top-right
        -1.0f,  1.0f,  0.0f, 0.0f   // Top-left
    };
    
    vao_.create();
    vao_.bind();
    
    vbo_.create();
    vbo_.bind();
    vbo_.allocate(vertices, sizeof(vertices));
    
    // Position attribute
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 4 * sizeof(float), nullptr);
    
    // TexCoord attribute
    glEnableVertexAttribArray(1);
    glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 4 * sizeof(float), 
                          reinterpret_cast<void*>(2 * sizeof(float)));
    
    vao_.release();
    vbo_.release();
}

void GLVideoWidget::resizeGL(int w, int h)
{
    // Use device pixel ratio for high-DPI displays
    const qreal dpr = devicePixelRatio();
    glViewport(0, 0, static_cast<int>(w * dpr), static_cast<int>(h * dpr));
    updateOverlayWidgets();
}

void GLVideoWidget::paintGL()
{
    // Use device pixel ratio for high-DPI displays
    const qreal dpr = devicePixelRatio();
    const int physicalWidth = static_cast<int>(width() * dpr);
    const int physicalHeight = static_cast<int>(height() * dpr);
    
    glClear(GL_COLOR_BUFFER_BIT);
    
    // Update texture if new frame is ready
    if (frameReady_.load()) {
        updateTexture();
    }
    
    if (!hasFrame_ || !texture_ || !texture_->isCreated()) {
        // Draw placeholder when no video
        // Reset viewport to full size for placeholder
        glViewport(0, 0, physicalWidth, physicalHeight);
        drawPlaceholder();
        return;
    }
    
    // Calculate aspect-ratio preserving viewport
    float widgetAspect = static_cast<float>(physicalWidth) / physicalHeight;
    float textureAspect = static_cast<float>(textureWidth_) / textureHeight_;
    
    int vpX = 0, vpY = 0, vpW = physicalWidth, vpH = physicalHeight;
    
    if (textureAspect > widgetAspect) {
        // Texture is wider - letterbox top/bottom
        vpH = static_cast<int>(physicalWidth / textureAspect);
        vpY = (physicalHeight - vpH) / 2;
    } else {
        // Texture is taller - pillarbox left/right  
        vpW = static_cast<int>(physicalHeight * textureAspect);
        vpX = (physicalWidth - vpW) / 2;
    }
    
    glViewport(vpX, vpY, vpW, vpH);
    
    shaderProgram_->bind();
    
    texture_->bind(0);
    shaderProgram_->setUniformValue(textureLoc_, 0);
    shaderProgram_->setUniformValue(mirrorLoc_, isMirrored_);
    
    vao_.bind();
    glDrawArrays(GL_TRIANGLE_FAN, 0, 4);
    vao_.release();
    
    texture_->release();
    shaderProgram_->release();
    
    // Reset viewport for any subsequent drawing
    glViewport(0, 0, physicalWidth, physicalHeight);
    // Reset viewport for overlay
    glViewport(0, 0, width(), height());
}

void GLVideoWidget::updateTexture()
{
    QMutexLocker locker(&frameMutex_);
    
    if (pendingFrame_.isNull()) {
        frameReady_.store(false);
        return;
    }
    
    // Convert to RGBA if needed (most efficient format for OpenGL)
    QImage frame = pendingFrame_;
    if (frame.format() != QImage::Format_RGBA8888) {
        frame = frame.convertToFormat(QImage::Format_RGBA8888);
    }
    
    bool sizeChanged = (frame.width() != textureWidth_ || frame.height() != textureHeight_);
    
    if (!texture_ || sizeChanged) {
        // Create or recreate texture
        delete texture_;
        texture_ = new QOpenGLTexture(QOpenGLTexture::Target2D);
        texture_->setFormat(QOpenGLTexture::RGBA8_UNorm);
        texture_->setSize(frame.width(), frame.height());
        texture_->setMinificationFilter(QOpenGLTexture::Linear);
        texture_->setMagnificationFilter(QOpenGLTexture::Linear);
        texture_->setWrapMode(QOpenGLTexture::ClampToEdge);
        texture_->allocateStorage();
        
        textureWidth_ = frame.width();
        textureHeight_ = frame.height();
    }
    
    // Upload texture data (streaming update)
    texture_->setData(QOpenGLTexture::RGBA, QOpenGLTexture::UInt8, frame.constBits());
    
    hasFrame_ = true;
    frameReady_.store(false);
    pendingFrame_ = QImage(); // Clear pending frame to free memory
}

void GLVideoWidget::drawPlaceholder()
{
    // Use QPainter for placeholder text (only when no video)
    QPainter painter(this);
    painter.setRenderHint(QPainter::Antialiasing);
    
    // Dark background
    painter.fillRect(rect(), QColor(42, 42, 53));
    
    // Draw initials
    painter.setPen(QColor(100, 100, 110));
    QFont font = painter.font();
    int fontSize = qMin(width(), height()) / 4;
    font.setPixelSize(qMax(20, fontSize));
    font.setBold(true);
    painter.setFont(font);
    
    QString text = getInitials();
    painter.drawText(rect(), Qt::AlignCenter, text);
    
    painter.end();
}

QString GLVideoWidget::getInitials() const
{
    if (participantName_.isEmpty()) {
        return "?";
    }
    return participantName_.left(1).toUpper();
}

void GLVideoWidget::setVideoFrame(const QImage& frame)
{
    // Frame rate limiting
    qint64 currentTime = QDateTime::currentMSecsSinceEpoch();
    if (currentTime - lastFrameTime_ < MIN_FRAME_INTERVAL_MS) {
        return; // Skip frame to maintain target frame rate
    }
    lastFrameTime_ = currentTime;
    
    {
        QMutexLocker locker(&frameMutex_);
        pendingFrame_ = frame;
    }
    frameReady_.store(true);
    
    // Schedule repaint
    update();
}

void GLVideoWidget::setVideoFrameRaw(const uint8_t* data, int width, int height, int stride)
{
    if (!data || width <= 0 || height <= 0) {
        return;
    }
    
    // Frame rate limiting  
    qint64 currentTime = QDateTime::currentMSecsSinceEpoch();
    if (currentTime - lastFrameTime_ < MIN_FRAME_INTERVAL_MS) {
        return;
    }
    lastFrameTime_ = currentTime;
    
    {
        QMutexLocker locker(&frameMutex_);
        // Create QImage without copying if stride matches
        if (stride == width * 4) {
            pendingFrame_ = QImage(data, width, height, stride, QImage::Format_RGBA8888).copy();
        } else {
            // Need to copy row by row for non-contiguous data
            pendingFrame_ = QImage(width, height, QImage::Format_RGBA8888);
            for (int y = 0; y < height; ++y) {
                memcpy(pendingFrame_.scanLine(y), data + y * stride, width * 4);
            }
        }
    }
    frameReady_.store(true);
    
    update();
}

void GLVideoWidget::setTrack(std::shared_ptr<livekit::Track> track)
{
    track_ = track;
    updateOverlayWidgets();
}

void GLVideoWidget::clearTrack()
{
    track_ = nullptr;
    hasFrame_ = false;
    
    {
        QMutexLocker locker(&frameMutex_);
        pendingFrame_ = QImage();
    }
    frameReady_.store(false);
    
    update();
}

void GLVideoWidget::setParticipantName(const QString& name)
{
    participantName_ = name;
    if (nameLabel_) {
        nameLabel_->setText(name);
    }
    update();
}

void GLVideoWidget::setMuted(bool muted)
{
    isMuted_ = muted;
    micEnabled_ = !muted;
    updateOverlayWidgets();
}

void GLVideoWidget::setMicEnabled(bool enabled)
{
    micEnabled_ = enabled;
    updateOverlayWidgets();
}

void GLVideoWidget::setCameraEnabled(bool enabled)
{
    cameraEnabled_ = enabled;
    updateOverlayWidgets();
}

void GLVideoWidget::setMirrored(bool mirrored)
{
    isMirrored_ = mirrored;
    update();
}

void GLVideoWidget::setShowStatus(bool show)
{
    showStatus_ = show;
    if (statusContainer_) {
        statusContainer_->setVisible(showStatus_);
    }
}

void GLVideoWidget::resizeEvent(QResizeEvent* event)
{
    QOpenGLWidget::resizeEvent(event);
    updateOverlayWidgets();
}

void GLVideoWidget::updateOverlayWidgets()
{
    if (overlayWidget_) {
        overlayWidget_->setGeometry(rect());
    }
    
    if (micIcon_) {
        QString iconPath = micEnabled_ ? ":/icon/Turn_on_the_microphone.png" : ":/icon/mute_the_microphone.png";
        micIcon_->setPixmap(QIcon(iconPath).pixmap(micIcon_->size()));
    }
    
    if (camIcon_) {
        QString iconPath = cameraEnabled_ ? ":/icon/video.png" : ":/icon/close_video.png";
        camIcon_->setPixmap(QIcon(iconPath).pixmap(camIcon_->size()));
    }
}
