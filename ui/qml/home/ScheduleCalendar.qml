import QtQuick
import QtQuick.Layouts

Rectangle {
    id: root

    property date selectedDate: new Date()
    property date minDate: new Date(new Date().getFullYear(), new Date().getMonth(), new Date().getDate())
    property date displayedMonth: new Date(selectedDate.getFullYear(), selectedDate.getMonth(), 1)

    signal dateSelected(var value)

    radius: 12
    color: "#F8FAFC"
    border.color: "#E5E7EB"
    border.width: 1

    function normalizeDate(d) {
        return new Date(d.getFullYear(), d.getMonth(), d.getDate())
    }

    function isSameDate(a, b) {
        if (!a || !b)
            return false
        return a.getFullYear() === b.getFullYear()
                && a.getMonth() === b.getMonth()
                && a.getDate() === b.getDate()
    }

    function monthTitle() {
        return displayedMonth.getFullYear() + "年" + (displayedMonth.getMonth() + 1) + "月"
    }

    function canGoPrevious() {
        var min = normalizeDate(minDate)
        var prevMonthLast = new Date(displayedMonth.getFullYear(), displayedMonth.getMonth(), 0)
        return prevMonthLast.getTime() >= min.getTime()
    }

    function refreshModel() {
        dayModel.clear()

        var firstDay = new Date(displayedMonth.getFullYear(), displayedMonth.getMonth(), 1)
        var firstWeekday = (firstDay.getDay() + 6) % 7
        var startDay = new Date(firstDay)
        startDay.setDate(firstDay.getDate() - firstWeekday)

        var today = normalizeDate(new Date())
        var min = normalizeDate(minDate)
        var selected = normalizeDate(selectedDate)

        for (var i = 0; i < 42; ++i) {
            var day = new Date(startDay)
            day.setDate(startDay.getDate() + i)

            var normalized = normalizeDate(day)
            var currentMonth = day.getMonth() === displayedMonth.getMonth()
            var enabled = normalized.getTime() >= min.getTime()

            dayModel.append({
                "year": day.getFullYear(),
                "month": day.getMonth(),
                "day": day.getDate(),
                "label": String(day.getDate()),
                "inCurrentMonth": currentMonth,
                "selectable": enabled,
                "isToday": isSameDate(normalized, today),
                "isSelected": isSameDate(normalized, selected)
            })
        }
    }

    function selectDate(year, month, day) {
        var candidate = new Date(year, month, day)
        var normalizedCandidate = normalizeDate(candidate)
        if (normalizedCandidate.getTime() < normalizeDate(minDate).getTime()) {
            return
        }

        selectedDate = normalizedCandidate
        dateSelected(normalizedCandidate)
        refreshModel()
    }

    onDisplayedMonthChanged: refreshModel()
    onMinDateChanged: refreshModel()
    onSelectedDateChanged: {
        if (selectedDate.getMonth() !== displayedMonth.getMonth()
                || selectedDate.getFullYear() !== displayedMonth.getFullYear()) {
            displayedMonth = new Date(selectedDate.getFullYear(), selectedDate.getMonth(), 1)
            return
        }
        refreshModel()
    }

    Component.onCompleted: {
        selectedDate = normalizeDate(selectedDate)
        displayedMonth = new Date(selectedDate.getFullYear(), selectedDate.getMonth(), 1)
        refreshModel()
    }

    ListModel {
        id: dayModel
    }

    ColumnLayout {
        anchors.fill: parent
        anchors.margins: 8
        spacing: 8

        RowLayout {
            Layout.fillWidth: true

            Rectangle {
                Layout.preferredWidth: 24
                Layout.preferredHeight: 24
                radius: 8
                color: canGoPrevious() ? "#FFFFFF" : "#F3F4F6"
                border.color: canGoPrevious() ? "#D1D5DB" : "#E5E7EB"
                border.width: 1

                Text {
                    anchors.centerIn: parent
                    text: "<"
                    color: canGoPrevious() ? "#374151" : "#9CA3AF"
                    font.pixelSize: 13
                    font.weight: Font.DemiBold
                }

                MouseArea {
                    anchors.fill: parent
                    enabled: canGoPrevious()
                    cursorShape: enabled ? Qt.PointingHandCursor : Qt.ArrowCursor
                    onClicked: {
                        root.displayedMonth = new Date(root.displayedMonth.getFullYear(),
                                                       root.displayedMonth.getMonth() - 1,
                                                       1)
                    }
                }
            }

            Item { Layout.fillWidth: true }

            Text {
                text: root.monthTitle()
                color: "#111827"
                font.pixelSize: 14
                font.weight: Font.DemiBold
            }

            Item { Layout.fillWidth: true }

            Rectangle {
                Layout.preferredWidth: 24
                Layout.preferredHeight: 24
                radius: 8
                color: "#FFFFFF"
                border.color: "#D1D5DB"
                border.width: 1

                Text {
                    anchors.centerIn: parent
                    text: ">"
                    color: "#374151"
                    font.pixelSize: 13
                    font.weight: Font.DemiBold
                }

                MouseArea {
                    anchors.fill: parent
                    cursorShape: Qt.PointingHandCursor
                    onClicked: {
                        root.displayedMonth = new Date(root.displayedMonth.getFullYear(),
                                                       root.displayedMonth.getMonth() + 1,
                                                       1)
                    }
                }
            }
        }

        GridLayout {
            Layout.fillWidth: true
            columns: 7
            rowSpacing: 4
            columnSpacing: 3

            Repeater {
                model: ["一", "二", "三", "四", "五", "六", "日"]
                delegate: Text {
                    Layout.fillWidth: true
                    horizontalAlignment: Text.AlignHCenter
                    text: modelData
                    color: "#6B7280"
                    font.pixelSize: 11
                    font.weight: Font.DemiBold
                }
            }
        }

        GridLayout {
            Layout.fillWidth: true
            columns: 7
            rowSpacing: 4
            columnSpacing: 3

            Repeater {
                model: dayModel
                delegate: Rectangle {
                    Layout.fillWidth: true
                    Layout.preferredHeight: 28
                    radius: 8
                    color: isSelected ? "#2563EB" : "#FFFFFF"
                    border.width: isSelected ? 0 : 1
                    border.color: isToday ? "#2563EB" : "#E5E7EB"
                    opacity: selectable ? 1.0 : 0.45

                    Text {
                        anchors.centerIn: parent
                        text: label
                        color: isSelected
                            ? "#FFFFFF"
                            : (inCurrentMonth ? "#111827" : "#9CA3AF")
                        font.pixelSize: 12
                        font.weight: isSelected ? Font.DemiBold : Font.Normal
                    }

                    MouseArea {
                        anchors.fill: parent
                        enabled: selectable
                        cursorShape: enabled ? Qt.PointingHandCursor : Qt.ArrowCursor
                        onClicked: root.selectDate(year, month, day)
                    }
                }
            }
        }
    }
}
