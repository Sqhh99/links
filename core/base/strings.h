#ifndef CORE_BASE_STRINGS_H
#define CORE_BASE_STRINGS_H

#include <cstdint>
#include <string>
#include <string_view>
#include <vector>

namespace links {
namespace core {
namespace str {

// ---------------------------------------------------------------------------
// Number formatting. Replaces QString::number and QString::arg overloads.
// ---------------------------------------------------------------------------
std::string num(int value);
std::string num(long long value);
std::string num(unsigned long long value);

/// Fixed-point, `decimals` places. Replaces QString::arg(d, 0, 'f', decimals).
/// std::to_string(double) is NOT equivalent -- it always prints 6 decimals.
std::string num(double value, int decimals);

/// Matches QString::arg(bool), which prints "true"/"false".
inline std::string boolText(bool value) { return value ? "true" : "false"; }

// ---------------------------------------------------------------------------
// ASCII-only helpers.
//
// These are deliberately ASCII-only and must stay that way: the strings they
// operate on are UTF-8, and running std::toupper/tolower over UTF-8 bytes
// would corrupt any multi-byte sequence. Every current caller works on
// protocol tokens (RTP codec MIME types, ICE transport protocols), which are
// ASCII by specification.
// ---------------------------------------------------------------------------
bool startsWithIgnoreCase(std::string_view text, std::string_view prefix);
bool containsIgnoreCase(std::string_view text, std::string_view needle);
bool contains(std::string_view text, std::string_view needle);
bool equalsIgnoreCase(std::string_view a, std::string_view b);
std::string toUpperAscii(std::string_view text);
std::string toLowerAscii(std::string_view text);

/// Strips leading/trailing whitespace. Beyond ASCII space characters this also
/// strips U+00A0 (no-break space) and U+3000 (ideographic space), because
/// QString::trimmed() is Unicode-aware and the chat-message guard in
/// ConferenceManager relies on a CJK full-width space counting as blank.
std::string trimWhitespace(std::string_view text);

std::string join(const std::vector<std::string>& parts, std::string_view separator);

// ---------------------------------------------------------------------------
// Concatenation. Replaces QString("%1...").arg(...).arg(...).
// ---------------------------------------------------------------------------
namespace detail {
inline void append(std::string& out, std::string_view v)  { out.append(v); }
inline void append(std::string& out, const std::string& v) { out.append(v); }
inline void append(std::string& out, const char* v)        { if (v) out.append(v); }
inline void append(std::string& out, char v)               { out.push_back(v); }
inline void append(std::string& out, bool v)               { out.append(boolText(v)); }
inline void append(std::string& out, int v)                { out.append(num(v)); }
inline void append(std::string& out, long v)               { out.append(num(static_cast<long long>(v))); }
inline void append(std::string& out, long long v)          { out.append(num(v)); }
inline void append(std::string& out, unsigned v)           { out.append(num(static_cast<unsigned long long>(v))); }
inline void append(std::string& out, unsigned long v)      { out.append(num(static_cast<unsigned long long>(v))); }
inline void append(std::string& out, unsigned long long v) { out.append(num(v)); }
// float/double are intentionally NOT overloaded: callers must choose a
// precision through num(value, decimals), the way QString::arg made them.
}  // namespace detail

template <typename... Ts>
std::string cat(const Ts&... parts)
{
    std::string out;
    (detail::append(out, parts), ...);
    return out;
}

}  // namespace str
}  // namespace core
}  // namespace links

#endif  // CORE_BASE_STRINGS_H
