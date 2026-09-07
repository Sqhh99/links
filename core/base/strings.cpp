#include "strings.h"

#include <algorithm>
#include <cstdio>

namespace links {
namespace core {
namespace str {
namespace {

constexpr char kNoBreakSpace[]   = "\xC2\xA0";      // U+00A0
constexpr char kIdeographicSpace[] = "\xE3\x80\x80";  // U+3000

char lowerAscii(char c)
{
    return (c >= 'A' && c <= 'Z') ? static_cast<char>(c - 'A' + 'a') : c;
}

char upperAscii(char c)
{
    return (c >= 'a' && c <= 'z') ? static_cast<char>(c - 'a' + 'A') : c;
}

bool isAsciiSpace(char c)
{
    return c == ' ' || c == '\t' || c == '\n' || c == '\r' || c == '\f' || c == '\v';
}

/// Length in bytes of a blank run starting at `pos`, or 0 if not blank.
std::size_t blankRunAt(std::string_view text, std::size_t pos)
{
    if (pos >= text.size()) {
        return 0;
    }
    if (isAsciiSpace(text[pos])) {
        return 1;
    }
    if (text.compare(pos, 2, kNoBreakSpace) == 0) {
        return 2;
    }
    if (text.compare(pos, 3, kIdeographicSpace) == 0) {
        return 3;
    }
    return 0;
}

}  // namespace

std::string num(int value) { return std::to_string(value); }
std::string num(long long value) { return std::to_string(value); }
std::string num(unsigned long long value) { return std::to_string(value); }

std::string num(double value, int decimals)
{
    if (decimals < 0) {
        decimals = 0;
    }
    char buffer[64];
    const int written = std::snprintf(buffer, sizeof(buffer), "%.*f", decimals, value);
    if (written <= 0) {
        return std::string();
    }
    return std::string(buffer, static_cast<std::size_t>(std::min<int>(written, sizeof(buffer) - 1)));
}

bool startsWithIgnoreCase(std::string_view text, std::string_view prefix)
{
    if (prefix.size() > text.size()) {
        return false;
    }
    for (std::size_t i = 0; i < prefix.size(); ++i) {
        if (lowerAscii(text[i]) != lowerAscii(prefix[i])) {
            return false;
        }
    }
    return true;
}

bool equalsIgnoreCase(std::string_view a, std::string_view b)
{
    return a.size() == b.size() && startsWithIgnoreCase(a, b);
}

std::string toUpperAscii(std::string_view text)
{
    std::string out(text);
    std::transform(out.begin(), out.end(), out.begin(), upperAscii);
    return out;
}

std::string toLowerAscii(std::string_view text)
{
    std::string out(text);
    std::transform(out.begin(), out.end(), out.begin(), lowerAscii);
    return out;
}

std::string trimWhitespace(std::string_view text)
{
    std::size_t begin = 0;
    while (begin < text.size()) {
        const std::size_t run = blankRunAt(text, begin);
        if (run == 0) {
            break;
        }
        begin += run;
    }

    std::size_t end = text.size();
    while (end > begin) {
        // Walk back over the possible blank encodings, longest first.
        if (end >= begin + 3 && text.compare(end - 3, 3, kIdeographicSpace) == 0) {
            end -= 3;
        } else if (end >= begin + 2 && text.compare(end - 2, 2, kNoBreakSpace) == 0) {
            end -= 2;
        } else if (isAsciiSpace(text[end - 1])) {
            end -= 1;
        } else {
            break;
        }
    }

    return std::string(text.substr(begin, end - begin));
}

std::string join(const std::vector<std::string>& parts, std::string_view separator)
{
    std::string out;
    for (std::size_t i = 0; i < parts.size(); ++i) {
        if (i != 0) {
            out.append(separator);
        }
        out.append(parts[i]);
    }
    return out;
}

}  // namespace str
}  // namespace core
}  // namespace links
