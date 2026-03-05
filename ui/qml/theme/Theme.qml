pragma Singleton
import QtQuick
import Links.Backend 1.0

QtObject {
    id: theme

    // Bind to ThemeManager singleton
    readonly property bool isDark: ThemeManager.currentTheme === "dark"

    // ==========================================
    // Window / Frame
    // ==========================================
    readonly property color windowBackground:   isDark ? "#1E1E2E" : "#FFFFFF"
    readonly property color windowBorder:        isDark ? "#3A3A4E" : "#E5E7EB"

    // ==========================================
    // Sidebar
    // ==========================================
    readonly property color sidebarBackground:   isDark ? "#252538" : "#F9FAFB"
    readonly property color sidebarBorder:        isDark ? "#3A3A4E" : "#E5E7EB"

    // ==========================================
    // Content / Card / Panel
    // ==========================================
    readonly property color contentBackground:   isDark ? "#1E1E2E" : "#FFFFFF"
    readonly property color cardBackground:      isDark ? "#2A2A3C" : "#FFFFFF"
    readonly property color pageBackground:      isDark ? "#252538" : "#F5F7FA"

    // ==========================================
    // Text
    // ==========================================
    readonly property color textPrimary:         isDark ? "#E4E4EF" : "#111827"
    readonly property color textSecondary:       isDark ? "#B0B0C8" : "#374151"
    readonly property color textTertiary:        isDark ? "#8888A4" : "#4B5563"
    readonly property color textMuted:           isDark ? "#6E6E8A" : "#6B7280"
    readonly property color textHint:            isDark ? "#5A5A74" : "#9CA3AF"
    readonly property color textOnAccent:        "#FFFFFF"

    // ==========================================
    // Accent (primary action color)
    // ==========================================
    readonly property color accentColor:         isDark ? "#5B8DEF" : "#2563EB"
    readonly property color accentHover:         isDark ? "#4A7DE0" : "#1D4ED8"
    readonly property color accentPressed:       isDark ? "#3A6DD0" : "#1E40AF"
    readonly property color accentLight:         isDark ? "#2A3A5C" : "#EFF6FF"

    // ==========================================
    // Borders
    // ==========================================
    readonly property color borderColor:         isDark ? "#3A3A4E" : "#D1D5DB"
    readonly property color borderLight:         isDark ? "#33334A" : "#E5E7EB"
    readonly property color borderAccent:        isDark ? "#5B8DEF" : "#2563EB"

    // ==========================================
    // Hover / Active states
    // ==========================================
    readonly property color hoverBackground:     isDark ? "#33334A" : "#F3F4F6"
    readonly property color activeBackground:    isDark ? "#2A3A5C" : "#EFF6FF"
    readonly property color pressedBackground:   isDark ? "#3A3A5C" : "#E5E7EB"

    // ==========================================
    // Separator
    // ==========================================
    readonly property color separatorColor:      isDark ? "#33334A" : "#E5E7EB"

    // ==========================================
    // Cancel / secondary buttons
    // ==========================================
    readonly property color buttonCancelBg:       isDark ? "#2A2A3C" : "transparent"
    readonly property color buttonCancelHoverBg:  isDark ? "#33334A" : "#F3F4F6"
    readonly property color buttonCancelBorder:   isDark ? "#3A3A4E" : "#D1D5DB"
    readonly property color buttonCancelText:     isDark ? "#B0B0C8" : "#374151"

    // ==========================================
    // Secondary button (outline style)
    // ==========================================
    readonly property color secondaryBg:         isDark ? "#252538" : "#F8FAFC"
    readonly property color secondaryHoverBg:    isDark ? "#2A3A5C" : "#EEF2FF"
    readonly property color secondaryBorder:     isDark ? "#3A4A6C" : "#CBD5F5"
    readonly property color secondaryText:       isDark ? "#7BAAEF" : "#1D4ED8"

    // ==========================================
    // Input fields
    // ==========================================
    readonly property color inputBackground:     isDark ? "#2A2A3C" : "#FFFFFF"
    readonly property color inputText:           isDark ? "#E4E4EF" : "#111827"
    readonly property color inputPlaceholder:    isDark ? "#5A5A74" : "#9CA3AF"
    readonly property color inputBorder:         isDark ? "#3A3A4E" : "#D1D5DB"
    readonly property color inputBorderFocus:    isDark ? "#5B8DEF" : "#2563EB"

    // ==========================================
    // ComboBox popup
    // ==========================================
    readonly property color popupBackground:     isDark ? "#2A2A3C" : "#FFFFFF"
    readonly property color popupBorder:         isDark ? "#3A3A4E" : "#E5E7EB"
    readonly property color popupHighlight:      isDark ? "#5B8DEF" : "#2563EB"
    readonly property color popupHighlightText:  "#FFFFFF"
    readonly property color popupItemText:       isDark ? "#E4E4EF" : "#111827"

    // ==========================================
    // CheckBox
    // ==========================================
    readonly property color checkboxBg:          isDark ? "#2A2A3C" : "#FFFFFF"
    readonly property color checkboxBorder:      isDark ? "#5A5A74" : "#D1D5DB"
    readonly property color checkboxCheckedBg:   isDark ? "#5B8DEF" : "#2563EB"
    readonly property color checkboxCheckedBorder: isDark ? "#5B8DEF" : "#2563EB"

    // ==========================================
    // PillToggle (green active)
    // ==========================================
    readonly property color pillActiveBg:        isDark ? "#10B981" : "#10B981"
    readonly property color pillInactiveBg:      isDark ? "#2A2A3C" : "#F3F4F6"
    readonly property color pillInactiveBorder:  isDark ? "#3A3A4E" : "#D1D5DB"
    readonly property color pillInactiveText:    isDark ? "#8888A4" : "#6B7280"

    // ==========================================
    // TabButton
    // ==========================================
    readonly property color tabActiveBg:         isDark ? "#5B8DEF" : "#2563EB"
    readonly property color tabInactiveBg:       isDark ? "#2A2A3C" : "#F3F4F6"
    readonly property color tabInactiveBorder:   isDark ? "#33334A" : "#E5E7EB"
    readonly property color tabInactiveText:     isDark ? "#8888A4" : "#6B7280"

    // ==========================================
    // Disabled states
    // ==========================================
    readonly property color disabledBg:          isDark ? "#252538" : "#E5E7EB"
    readonly property color disabledBackground:  disabledBg
    readonly property color disabledText:        isDark ? "#5A5A74" : "#9CA3AF"
    readonly property color errorColor:          isDark ? "#F87171" : "#DC2626"

    // ==========================================
    // Close button hover (red)
    // ==========================================
    readonly property color closeHoverColor:     isDark ? "#40ff5252" : "#26ff5252"

    // ==========================================
    // Icon opacity - Higher in dark mode so black icons are visible
    // ==========================================
    readonly property real iconOpacity:          isDark ? 0.85 : 0.7

    // ==========================================
    // Indicator colors (Filled canvas arrow)
    // ==========================================
    readonly property color indicatorColor:      isDark ? "#8888A4" : "#6B7280"
    readonly property color indicatorPressed:    isDark ? "#5B8DEF" : "#2563EB"
}
