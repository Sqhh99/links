#ifndef CORE_MEDIA_AUDIO_FORMAT_H
#define CORE_MEDIA_AUDIO_FORMAT_H

namespace links {
namespace core {

/// Mirrors the QAudioFormat::SampleFormat values the playback path handles.
enum class SampleFormat {
    Unknown,
    UInt8,
    Int16,
    Int32,
    Float,
};

struct AudioFormat {
    int sampleRate{0};
    int channels{0};
    SampleFormat sampleFormat{SampleFormat::Int16};

    bool isValid() const
    {
        return sampleRate > 0 && channels > 0 && sampleFormat != SampleFormat::Unknown;
    }

    int bytesPerSample() const
    {
        switch (sampleFormat) {
        case SampleFormat::UInt8:   return 1;
        case SampleFormat::Int16:   return 2;
        case SampleFormat::Int32:   return 4;
        case SampleFormat::Float:   return 4;
        case SampleFormat::Unknown: return 0;
        }
        return 0;
    }

    friend bool operator==(const AudioFormat& a, const AudioFormat& b)
    {
        return a.sampleRate == b.sampleRate
            && a.channels == b.channels
            && a.sampleFormat == b.sampleFormat;
    }
    friend bool operator!=(const AudioFormat& a, const AudioFormat& b) { return !(a == b); }
};

}  // namespace core
}  // namespace links

#endif  // CORE_MEDIA_AUDIO_FORMAT_H
