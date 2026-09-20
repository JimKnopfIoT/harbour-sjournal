import QtQuick 2.0
import Sailfish.Silica 1.0

Column {
    id: picker

    property color chosen: "#000000"
    property bool showAlpha: false
    property int alpha: 255

    signal picked(color value)

    spacing: Theme.paddingMedium

    property real fieldX: -1
    property real fieldY: -1
    property real rampX: -1

    function emitChosen(value) {
        chosen = Qt.rgba(value.r, value.g, value.b, 1.0)
        picked(chosen)
    }

    // Place the markers from a colour that came from outside.
    function locate(value) {
        var r = value.r, g = value.g, b = value.b
        var hi = Math.max(r, g, b), lo = Math.min(r, g, b)
        var l = (hi + lo) / 2
        var d = hi - lo

        if (d < 0.004) {
            rampX = (1 - l) * ramp.width
            fieldX = -1
            fieldY = -1
            return
        }

        var h = 0
        if (hi === r)
            h = ((g - b) / d + (g < b ? 6 : 0)) / 6
        else if (hi === g)
            h = ((b - r) / d + 2) / 6
        else
            h = ((r - g) / d + 4) / 6

        fieldX = h * field.width
        fieldY = (1 - l) * field.height
        rampX = -1
    }

    onChosenChanged: if (fieldX < 0 && rampX < 0) locate(chosen)
    Component.onCompleted: locate(chosen)

    Item {
        id: field

        width: parent.width
        height: Math.min(Screen.height * 0.34, width * 0.8)

        // Qt Quick gradients only run vertically, so this one is turned on its side.
        Rectangle {
            width: parent.height
            height: parent.width
            anchors.centerIn: parent
            rotation: -90
            gradient: Gradient {
                GradientStop { position: 0.000; color: "#ff0000" }
                GradientStop { position: 0.167; color: "#ffff00" }
                GradientStop { position: 0.333; color: "#00ff00" }
                GradientStop { position: 0.500; color: "#00ffff" }
                GradientStop { position: 0.667; color: "#0000ff" }
                GradientStop { position: 0.833; color: "#ff00ff" }
                GradientStop { position: 1.000; color: "#ff0000" }
            }
        }

        Rectangle {
            anchors.fill: parent
            gradient: Gradient {
                GradientStop { position: 0.0; color: "#ffffffff" }
                GradientStop { position: 0.5; color: "#00ffffff" }
                GradientStop { position: 0.5; color: "#00000000" }
                GradientStop { position: 1.0; color: "#ff000000" }
            }
        }

        MouseArea {
            anchors.fill: parent
            onPressed: take(mouse.x, mouse.y)
            onPositionChanged: take(mouse.x, mouse.y)

            function take(mx, my) {
                var px = Math.max(0, Math.min(field.width, mx))
                var py = Math.max(0, Math.min(field.height, my))
                picker.fieldX = px
                picker.fieldY = py
                picker.rampX = -1
                picker.emitChosen(Qt.hsla(px / field.width, 1.0,
                                          1.0 - py / field.height, 1.0))
            }
        }

        ColorMarker {
            visible: picker.fieldX >= 0
            x: picker.fieldX - width / 2
            y: picker.fieldY - height / 2
        }
    }

    // White, black and the greys between are nowhere in the field above.
    Item {
        id: ramp

        width: parent.width
        height: Theme.itemSizeExtraSmall * 0.6

        Rectangle {
            width: parent.height
            height: parent.width
            anchors.centerIn: parent
            rotation: -90
            gradient: Gradient {
                GradientStop { position: 0.0; color: "#000000" }
                GradientStop { position: 1.0; color: "#ffffff" }
            }
        }

        MouseArea {
            anchors.fill: parent
            onPressed: take(mouse.x)
            onPositionChanged: take(mouse.x)

            function take(mx) {
                var px = Math.max(0, Math.min(ramp.width, mx))
                picker.rampX = px
                picker.fieldX = -1
                picker.fieldY = -1
                picker.emitChosen(Qt.hsla(0, 0, 1.0 - px / ramp.width, 1.0))
            }
        }

        ColorMarker {
            visible: picker.rampX >= 0
            x: picker.rampX - width / 2
            anchors.verticalCenter: parent.verticalCenter
        }
    }

    Row {
        width: parent.width
        spacing: Theme.paddingMedium

        Rectangle {
            width: parent.width / 2 - Theme.paddingMedium / 2
            height: Theme.itemSizeExtraSmall
            radius: Theme.paddingSmall / 2
            color: picker.chosen
            border.width: 1
            border.color: Theme.rgba(Theme.primaryColor, 0.4)
        }

        TextField {
            width: parent.width / 2 - Theme.paddingMedium / 2
            label: qsTr("Hex code")
            text: picker.chosen.toString().substring(0, 7)
            inputMethodHints: Qt.ImhNoAutoUppercase | Qt.ImhNoPredictiveText
            EnterKey.onClicked: {
                var wanted = text.trim()
                if (/^#?[0-9a-fA-F]{6}$/.test(wanted)) {
                    if (wanted.charAt(0) !== "#")
                        wanted = "#" + wanted
                    picker.fieldX = -1
                    picker.rampX = -1
                    picker.chosen = wanted
                    picker.locate(picker.chosen)
                    picker.picked(picker.chosen)
                }
                focus = false
            }
        }
    }

    Slider {
        width: parent.width
        visible: picker.showAlpha
        minimumValue: 0
        maximumValue: 255
        stepSize: 1
        value: picker.alpha
        label: qsTr("Opacity")
        valueText: Math.round(sliderValue / 2.55) + " %"
        onSliderValueChanged: picker.alpha = Math.round(sliderValue)
    }
}
