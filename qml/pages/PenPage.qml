import QtQuick 2.0
import Sailfish.Silica 1.0
import harbour.sjournal 1.0

Page {
    id: page
    allowedOrientations: Orientation.All

    SilicaFlickable {
        anchors.fill: parent
        contentHeight: column.height

        Column {
            id: column
            width: parent.width

            PageHeader { title: qsTr("Pen") }

            Rectangle {
                x: Theme.horizontalPageMargin
                width: parent.width - 2 * Theme.horizontalPageMargin
                height: Theme.itemSizeLarge
                color: "white"
                radius: Theme.paddingSmall

                Rectangle {
                    anchors.centerIn: parent
                    width: parent.width - Theme.paddingLarge * 2
                    height: Math.max(1, app.controller.tools.penWidth * 1.6)
                    radius: height / 2
                    color: app.controller.tools.color
                    opacity: app.controller.tools.tool === ToolSettings.HighlighterTool ? 0.5 : 1.0
                }
            }

            Slider {
                width: parent.width
                minimumValue: 0.3
                maximumValue: 20.0
                stepSize: 0.1
                value: app.controller.tools.penWidth
                label: qsTr("Width")
                valueText: qsTr("%1 pt").arg(value.toFixed(1))
                onValueChanged: app.controller.tools.penWidth = value
            }

            TextSwitch {
                text: qsTr("Speed changes the width")
                description: qsTr("Fast strokes are drawn thinner, which is what gives "
                                  + "handwriting its taper. Used whenever the screen "
                                  + "reports no usable pressure.")
                checked: app.controller.tools.dynamicWidth
                onClicked: app.controller.tools.dynamicWidth = checked
            }

            SectionHeader { text: qsTr("What this screen reports") }

            Label {
                x: Theme.horizontalPageMargin
                width: parent.width - 2 * Theme.horizontalPageMargin
                wrapMode: Text.Wrap
                font.pixelSize: Theme.fontSizeSmall
                color: app.controller.tools.pressureSeen ? Theme.highlightColor : Theme.secondaryColor
                text: app.controller.tools.pressureSeen
                      ? qsTr("Pressure is coming through (%1 of 0–1). Stroke width follows "
                             + "it, calibrated to the range this panel actually produces.")
                            .arg(app.controller.tools.pressureRange)
                      : qsTr("No pressure seen yet. Draw a few strokes, pressing lightly and "
                             + "then firmly, and come back — the panel measures pressure in "
                             + "hardware, but it only counts if it survives the trip to the app.")
            }

            TextSwitch {
                text: qsTr("Straighten shapes")
                description: qsTr("A freehand line, rectangle, ellipse or polygon is replaced "
                                  + "by the clean shape when you lift your finger. Exported SVG "
                                  + "then contains a real rect or ellipse, not a wobbly path.")
                checked: app.controller.tools.shapeRecognition
                onClicked: app.controller.tools.shapeRecognition = checked
            }

            SectionHeader { text: qsTr("Colour") }

            Slider {
                id: hueSlider
                width: parent.width
                minimumValue: 0
                maximumValue: 1
                value: {
                    var h = app.controller.tools.color.hslHue
                    return h < 0 ? 0 : h
                }
                label: qsTr("Hue")
                onValueChanged: page.applyColor()
            }

            Slider {
                id: satSlider
                width: parent.width
                minimumValue: 0
                maximumValue: 1
                value: app.controller.tools.color.hslSaturation
                label: qsTr("Saturation")
                onValueChanged: page.applyColor()
            }

            Slider {
                id: lightSlider
                width: parent.width
                minimumValue: 0
                maximumValue: 1
                value: app.controller.tools.color.hslLightness
                label: qsTr("Lightness")
                onValueChanged: page.applyColor()
            }
        }

        VerticalScrollDecorator { }
    }

    property bool applying: false

    function applyColor() {
        if (applying)
            return
        applying = true
        app.controller.tools.color = Qt.hsla(hueSlider.value, satSlider.value, lightSlider.value, 1.0)
        applying = false
    }
}
