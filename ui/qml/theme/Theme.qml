pragma Singleton
import QtQuick
import Links.Backend 1.0

QtObject {
    id: theme

    // Bind to ThemeManager singleton
    readonly property bool isDark: ThemeManager.currentTheme === "dark"

    // ==========================================
    // Brand
    // ==========================================
    readonly property color brand:               isDark ? "#3D83FF" : "#1F6FFF"
    readonly property color brandHover:          isDark ? "#5A96FF" : "#3D83FF"
    readonly property color brandPressed:        isDark ? "#2A6BE6" : "#1557D9"
    readonly property color brandSoft:           isDark ? "#1E2A42" : "#EAF1FF"
    readonly property color brandSoftStrong:     isDark ? "#243556" : "#D9E6FF"

    // ==========================================
    // Status
    // ==========================================
    readonly property color success:             isDark ? "#2BC37E" : "#16B26B"
    readonly property color successSoft:         isDark ? "#173327" : "#E6F7EF"
    readonly property color danger:              isDark ? "#FF5C5C" : "#F04A4A"
    readonly property color dangerHover:         isDark ? "#FF7474" : "#E03A3A"
    readonly property color dangerSoft:          isDark ? "#3A1E22" : "#FDECEC"
    readonly property color warning:             isDark ? "#F5B640" : "#F59E0B"
    readonly property color warningSoft:         isDark ? "#3A2E17" : "#FEF4E2"

    // ==========================================
    // Window / Frame
    // ==========================================
    readonly property color windowBackground:   isDark ? "#1C1F26" : "#FFFFFF"
    readonly property color windowBorder:        isDark ? "#2A2E37" : "#E3E8F0"

    // Page background with a soft brand wash at the top
    readonly property color pageBackground:      isDark ? "#14161B" : "#F4F7FC"
    readonly property color pageWashTop:         isDark ? "#171C26" : "#EAF1FE"
    readonly property color pageWashBottom:      isDark ? "#14161B" : "#F7F9FD"

    // ==========================================
    // Sidebar / Rail
    // ==========================================
    readonly property color sidebarBackground:   isDark ? "#181B21" : "#F7F9FC"
    readonly property color sidebarBorder:        isDark ? "#23262E" : "#EDF0F5"
    readonly property color railBackground:      isDark ? "#181B21" : "#FFFFFF"

    // ==========================================
    // Content / Card / Panel
    // ==========================================
    readonly property color contentBackground:   isDark ? "#1C1F26" : "#FFFFFF"
    readonly property color cardBackground:      isDark ? "#1C1F26" : "#FFFFFF"
    readonly property color subtleBackground:    isDark ? "#20242C" : "#F6F8FB"

    // Video stage stays dark in both themes
    readonly property color stageBackground:     "#0E1014"
    readonly property color stageSurface:        "#1A1D23"
    readonly property color stageText:           "#E8ECF3"
    readonly property color stageOverlay:        Qt.rgba(0, 0, 0, 0.55)

    // ==========================================
    // Text
    // ==========================================
    readonly property color textPrimary:         isDark ? "#E8ECF3" : "#0F172A"
    readonly property color textSecondary:       isDark ? "#A7B0BF" : "#475569"
    readonly property color textTertiary:        isDark ? "#8A93A3" : "#64748B"
    readonly property color textMuted:           isDark ? "#6B7485" : "#94A3B8"
    readonly property color textHint:            isDark ? "#4E5664" : "#B6C0CE"
    readonly property color textOnAccent:        "#FFFFFF"

    // ==========================================
    // Icons (tinted through the Icon component)
    // ==========================================
    readonly property color iconPrimary:         isDark ? "#E8ECF3" : "#1E293B"
    readonly property color iconSecondary:       isDark ? "#A7B0BF" : "#5B6679"
    readonly property color iconMuted:           isDark ? "#5E6778" : "#A9B4C4"

    // ==========================================
    // Accent (primary action color) - aliases of brand
    // ==========================================
    readonly property color accentColor:         brand
    readonly property color accentHover:         brandHover
    readonly property color accentPressed:       brandPressed
    readonly property color accentLight:         brandSoft

    // ==========================================
    // Borders
    // ==========================================
    readonly property color borderColor:         isDark ? "#2E323C" : "#DCE2EB"
    readonly property color borderLight:         isDark ? "#262A33" : "#EAEEF4"
    readonly property color borderAccent:        brand

    // ==========================================
    // Hover / Active states
    // ==========================================
    readonly property color hoverBackground:     isDark ? "#252933" : "#F1F4F9"
    readonly property color activeBackground:    brandSoft
    readonly property color pressedBackground:   isDark ? "#2C313C" : "#E7ECF3"

    // Same hue with zero alpha. Use this instead of "transparent" as the resting
    // state of an animated hover background: "transparent" is #00000000, so a
    // ColorAnimation towards it passes through dark half-opaque frames and the
    // background visibly flashes when the mouse leaves.
    function clearOf(c) { return Qt.rgba(c.r, c.g, c.b, 0) }
    readonly property color hoverClear:          clearOf(hoverBackground)

    // ==========================================
    // Separator
    // ==========================================
    readonly property color separatorColor:      isDark ? "#262A33" : "#EDF0F5"

    // ==========================================
    // Cancel / secondary buttons
    // ==========================================
    readonly property color buttonCancelBg:       isDark ? "#252933" : "#FFFFFF"
    readonly property color buttonCancelHoverBg:  isDark ? "#2C313C" : "#F1F4F9"
    readonly property color buttonCancelBorder:   isDark ? "#343944" : "#DCE2EB"
    readonly property color buttonCancelText:     isDark ? "#C9D0DB" : "#334155"

    // ==========================================
    // Secondary button (brand tinted)
    // ==========================================
    readonly property color secondaryBg:         brandSoft
    readonly property color secondaryHoverBg:    brandSoftStrong
    readonly property color secondaryBorder:     "transparent"
    readonly property color secondaryText:       isDark ? "#7FAEFF" : "#1F6FFF"

    // ==========================================
    // Input fields
    // ==========================================
    readonly property color inputBackground:     isDark ? "#20242C" : "#FFFFFF"
    readonly property color inputText:           textPrimary
    readonly property color inputPlaceholder:    textHint
    readonly property color inputBorder:         isDark ? "#2E323C" : "#DCE2EB"
    readonly property color inputBorderFocus:    brand

    // ==========================================
    // ComboBox popup
    // ==========================================
    readonly property color popupBackground:     isDark ? "#22262E" : "#FFFFFF"
    readonly property color popupBorder:         isDark ? "#2E323C" : "#E3E8F0"
    readonly property color popupHighlight:      brandSoft
    readonly property color popupHighlightText:  isDark ? "#7FAEFF" : "#1F6FFF"
    readonly property color popupItemText:       textPrimary

    // ==========================================
    // CheckBox
    // ==========================================
    readonly property color checkboxBg:          inputBackground
    readonly property color checkboxBorder:      isDark ? "#4A5160" : "#C5CDD9"
    readonly property color checkboxCheckedBg:   brand
    readonly property color checkboxCheckedBorder: brand

    // ==========================================
    // PillToggle / Switch
    // ==========================================
    readonly property color pillActiveBg:        brand
    readonly property color pillInactiveBg:      isDark ? "#20242C" : "#F1F4F9"
    readonly property color pillInactiveBorder:  isDark ? "#2E323C" : "#DCE2EB"
    readonly property color pillInactiveText:    textTertiary
    readonly property color switchTrackOff:      isDark ? "#343944" : "#D5DCE6"

    // ==========================================
    // TabButton (segmented)
    // ==========================================
    readonly property color tabActiveBg:         isDark ? "#2C313C" : "#FFFFFF"
    readonly property color tabActiveText:       textPrimary
    readonly property color tabInactiveBg:       isDark ? "#20242C" : "#EEF2F7"
    readonly property color tabInactiveBorder:   "transparent"
    readonly property color tabInactiveText:     textTertiary

    // ==========================================
    // Disabled states
    // ==========================================
    readonly property color disabledBg:          isDark ? "#252933" : "#E7ECF3"
    readonly property color disabledBackground:  disabledBg
    readonly property color disabledText:        isDark ? "#5E6778" : "#A9B4C4"
    readonly property color errorColor:          danger

    // ==========================================
    // Close button hover (red)
    // ==========================================
    readonly property color closeHoverColor:     danger

    // ==========================================
    // Overlay / shadow
    // ==========================================
    readonly property color overlayColor:        isDark ? Qt.rgba(0, 0, 0, 0.6) : Qt.rgba(15 / 255, 23 / 255, 42 / 255, 0.32)
    readonly property color shadowColor:         isDark ? Qt.rgba(0, 0, 0, 0.5) : Qt.rgba(31 / 255, 58 / 255, 110 / 255, 0.16)


    // ==========================================
    // Indicator colors
    // ==========================================
    readonly property color indicatorColor:      iconSecondary
    readonly property color indicatorPressed:    brand

    // ==========================================
    // Metrics
    // ==========================================
    readonly property int radiusSm:  6
    readonly property int radiusMd:  10
    readonly property int radiusLg:  14
    readonly property int controlHeight: 36
}
