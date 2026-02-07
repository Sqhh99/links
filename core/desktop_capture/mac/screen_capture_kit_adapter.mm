#ifdef __APPLE__

#include "screen_capture_kit_adapter.h"

#include <CoreGraphics/CoreGraphics.h>
#include <CoreMedia/CoreMedia.h>
#include <CoreVideo/CoreVideo.h>
#include <dispatch/dispatch.h>

#include <algorithm>
#include <atomic>
#include <cstdint>
#include <cstring>
#include <optional>
#include <utility>

#if __has_include(<ScreenCaptureKit/ScreenCaptureKit.h>)
#import <ScreenCaptureKit/ScreenCaptureKit.h>
#define HAS_SCREEN_CAPTURE_KIT 1
#else
#define HAS_SCREEN_CAPTURE_KIT 0
#endif

#if !__has_feature(objc_arc)
#define LINKS_AUTORELEASE(value) [(value) autorelease]
#else
#define LINKS_AUTORELEASE(value) (value)
#endif

using links::core::PixelFormat;
using links::core::RawImage;

static std::optional<RawImage> toRawImage(CVPixelBufferRef pixelBuffer)
{
    if (!pixelBuffer) {
        return std::nullopt;
    }

    if (CVPixelBufferGetPixelFormatType(pixelBuffer) != kCVPixelFormatType_32BGRA) {
        return std::nullopt;
    }

    CVPixelBufferLockBaseAddress(pixelBuffer, kCVPixelBufferLock_ReadOnly);

    const std::size_t width = CVPixelBufferGetWidth(pixelBuffer);
    const std::size_t height = CVPixelBufferGetHeight(pixelBuffer);
    const std::size_t stride = CVPixelBufferGetBytesPerRow(pixelBuffer);
    const auto* baseAddress = static_cast<const std::uint8_t*>(CVPixelBufferGetBaseAddress(pixelBuffer));

    if (!baseAddress || width == 0 || height == 0 || stride < width * 4) {
        CVPixelBufferUnlockBaseAddress(pixelBuffer, kCVPixelBufferLock_ReadOnly);
        return std::nullopt;
    }

    RawImage image;
    image.width = static_cast<int>(width);
    image.height = static_cast<int>(height);
    image.stride = static_cast<int>(stride);
    image.format = PixelFormat::BGRA8888;
    image.pixels.resize(stride * height);

    std::memcpy(image.pixels.data(), baseAddress, stride * height);

    CVPixelBufferUnlockBaseAddress(pixelBuffer, kCVPixelBufferLock_ReadOnly);

    if (!image.isValid()) {
        return std::nullopt;
    }

    return image;
}

#if HAS_SCREEN_CAPTURE_KIT

struct OneShotCaptureContext {
    dispatch_semaphore_t completionSemaphore{dispatch_semaphore_create(0)};
    std::optional<RawImage> image;
    std::atomic<bool> finished{false};
};

API_AVAILABLE(macos(12.3))
@interface OneShotStreamOutput : NSObject <SCStreamOutput, SCStreamDelegate>
- (instancetype)initWithContext:(OneShotCaptureContext*)context;
@end

@implementation OneShotStreamOutput {
    OneShotCaptureContext* context_;
}

- (instancetype)initWithContext:(OneShotCaptureContext*)context
{
    self = [super init];
    if (self) {
        context_ = context;
    }
    return self;
}

- (void)stream:(SCStream*)stream
    didOutputSampleBuffer:(CMSampleBufferRef)sampleBuffer
                   ofType:(SCStreamOutputType)type
{
    (void)stream;
    if (type != SCStreamOutputTypeScreen || !sampleBuffer || !CMSampleBufferIsValid(sampleBuffer)) {
        return;
    }

    CVImageBufferRef imageBuffer = CMSampleBufferGetImageBuffer(sampleBuffer);
    if (!imageBuffer) {
        return;
    }

    auto image = toRawImage((CVPixelBufferRef)imageBuffer);
    if (!image.has_value()) {
        return;
    }

    if (context_ && !context_->finished.exchange(true)) {
        context_->image = std::move(image);
        dispatch_semaphore_signal(context_->completionSemaphore);
    }
}

- (void)stream:(SCStream*)stream didStopWithError:(NSError*)error
{
    (void)stream;
    (void)error;
    if (context_ && !context_->finished.exchange(true)) {
        dispatch_semaphore_signal(context_->completionSemaphore);
    }
}

@end

API_AVAILABLE(macos(12.3))
static SCShareableContent* requestShareableContent()
{
    __block SCShareableContent* content = nil;
    dispatch_semaphore_t semaphore = dispatch_semaphore_create(0);

    [SCShareableContent getShareableContentWithCompletionHandler:^(SCShareableContent* availableContent, NSError* error) {
        (void)error;
        content = availableContent;
        dispatch_semaphore_signal(semaphore);
    }];

    const dispatch_time_t timeout = dispatch_time(DISPATCH_TIME_NOW, 2 * NSEC_PER_SEC);
    if (dispatch_semaphore_wait(semaphore, timeout) != 0) {
        return nil;
    }

    return content;
}

API_AVAILABLE(macos(12.3))
static std::optional<RawImage> captureDisplayWithFilter(SCContentFilter* filter, std::uint32_t displayId)
{
    if (!filter) {
        return std::nullopt;
    }

    const CGRect bounds = CGDisplayBounds(static_cast<CGDirectDisplayID>(displayId));
    const int width = std::max(1, static_cast<int>(bounds.size.width));
    const int height = std::max(1, static_cast<int>(bounds.size.height));

    SCStreamConfiguration* configuration = LINKS_AUTORELEASE([[SCStreamConfiguration alloc] init]);
    configuration.width = width;
    configuration.height = height;
    configuration.pixelFormat = kCVPixelFormatType_32BGRA;
    configuration.showsCursor = NO;
    configuration.colorSpaceName = kCGColorSpaceSRGB;
    configuration.minimumFrameInterval = CMTimeMake(1, 30);

    OneShotCaptureContext context;
    OneShotCaptureContext* contextPtr = &context;
    OneShotStreamOutput* output = LINKS_AUTORELEASE([[OneShotStreamOutput alloc] initWithContext:contextPtr]);
    dispatch_queue_t queue = dispatch_queue_create("links.sck.oneshot", DISPATCH_QUEUE_SERIAL);

    SCStream* stream = LINKS_AUTORELEASE([[SCStream alloc] initWithFilter:filter
                                                             configuration:configuration
                                                                  delegate:output]);

    NSError* addOutputError = nil;
    const BOOL addOutputResult =
        [stream addStreamOutput:output
                           type:SCStreamOutputTypeScreen
             sampleHandlerQueue:queue
                          error:&addOutputError];
    if (!addOutputResult) {
        (void)addOutputError;
        return std::nullopt;
    }

    [stream startCaptureWithCompletionHandler:^(NSError* error) {
        if (error && !contextPtr->finished.exchange(true)) {
            dispatch_semaphore_signal(contextPtr->completionSemaphore);
        }
    }];

    const dispatch_time_t captureTimeout = dispatch_time(DISPATCH_TIME_NOW, 2 * NSEC_PER_SEC);
    const long waitResult = dispatch_semaphore_wait(contextPtr->completionSemaphore, captureTimeout);

    dispatch_semaphore_t stopSemaphore = dispatch_semaphore_create(0);
    [stream stopCaptureWithCompletionHandler:^(NSError* error) {
        (void)error;
        dispatch_semaphore_signal(stopSemaphore);
    }];
    dispatch_semaphore_wait(stopSemaphore, dispatch_time(DISPATCH_TIME_NOW, 500 * NSEC_PER_MSEC));

    if (waitResult != 0) {
        return std::nullopt;
    }

    return contextPtr->image;
}

#endif  // HAS_SCREEN_CAPTURE_KIT

namespace links {
namespace core {
namespace mac {

bool isScreenCaptureKitAvailable()
{
#if HAS_SCREEN_CAPTURE_KIT
    if (@available(macOS 12.3, *)) {
        return true;
    }
#endif
    return false;
}

bool hasScreenRecordingPermission()
{
    if (@available(macOS 10.15, *)) {
        return CGPreflightScreenCaptureAccess();
    }
    return true;
}

std::optional<RawImage> captureDisplayWithScreenCaptureKit(std::uint32_t displayId)
{
#if HAS_SCREEN_CAPTURE_KIT
    if (!isScreenCaptureKitAvailable() || !hasScreenRecordingPermission()) {
        return std::nullopt;
    }

    if (@available(macOS 12.3, *)) {
        @autoreleasepool {
            SCShareableContent* content = requestShareableContent();
            if (!content || !content.displays.count) {
                return std::nullopt;
            }

            SCDisplay* selectedDisplay = nil;
            for (SCDisplay* display in content.displays) {
                if (display.displayID == static_cast<CGDirectDisplayID>(displayId)) {
                    selectedDisplay = display;
                    break;
                }
            }

            if (!selectedDisplay) {
                selectedDisplay = content.displays.firstObject;
            }

            SCContentFilter* filter = LINKS_AUTORELEASE([[SCContentFilter alloc] initWithDisplay:selectedDisplay
                                                                                  excludingWindows:@[]]);
            return captureDisplayWithFilter(filter, static_cast<std::uint32_t>(selectedDisplay.displayID));
        }
    }
#else
    (void)displayId;
#endif

    return std::nullopt;
}

}  // namespace mac
}  // namespace core
}  // namespace links

#undef LINKS_AUTORELEASE

#endif  // __APPLE__
