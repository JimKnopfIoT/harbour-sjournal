import QtQuick 2.0
import Sailfish.Silica 1.0
import harbour.sjournal 1.0

Page {
    id: page
    allowedOrientations: Orientation.All

    readonly property var swatches: [
        "#000000", "#e12d2d", "#ef8a17", "#f2c40e",
        "#2e9e3e", "#1a72d0", "#7d3fbf", "#ffffff",
        "#8d5524", "#00a0a0", "#e05c9e", "#7a7a7a"
    ]

    SilicaFlickable {
        anchors.fill: parent
        contentHeight: column.height

        Column {
            id: column
            width: parent.width

            PageHeader { title: qsTr("Fill and outline") }

            Rectangle {
                x: Theme.horizontalPageMargin
                width: parent.width - 2 * Theme.horizontalPageMargin
                height: Theme.itemSizeExtraLarge
                color: "white"
                radius: Theme.paddingSmall

                Rectangle {
                    anchors.centerIn: parent
                    width: parent.width * 0.5
                    height: parent.height * 0.55
                    radius: app.controller.tools.shapeKind === ToolSettings.EllipseKind
                            ? height / 2 : 0
                    color: app.controller.tools.fillEnabled
                           ? Qt.rgba(app.controller.tools.fillColor.r,
                                     app.controller.tools.fillColor.g,
                                     app.controller.tools.fillColor.b,
                                     app.controller.tools.fillAlpha / 255)
                           : "transparent"
                    border.width: Math.max(1, app.controller.tools.penWidth)
                    border.color: app.controller.tools.color
                }
            }

            SectionHeader { text: qsTr("Outline") }

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

            Slider {
                width: parent.width
                minimumValue: 0
                maximumValue: 255
                stepSize: 1
                value: app.controller.tools.color.a * 255
                label: qsTr("Outline opacity")
                valueText: Math.round(value / 2.55) + " %"
                onValueChanged: {
                    var c = app.controller.tools.color
                    app.controller.tools.color = Qt.rgba(c.r, c.g, c.b, value / 255)
                }
            }

            ComboBox {
                width: parent.width
                label: qsTr("Line style")

                property var styles: ["plain", "dash", "dot", "dashdot"]
                currentIndex: Math.max(0, styles.indexOf(app.controller.tools.lineStyle))

                menu: ContextMenu {
                    MenuItem { text: qsTr("Solid") }
                    MenuItem { text: qsTr("Dashed") }
                    MenuItem { text: qsTr("Dotted") }
                    MenuItem { text: qsTr("Dash-dot") }
                }

                onCurrentIndexChanged: app.controller.tools.lineStyle = styles[currentIndex]
            }

            SectionHeader { text: qsTr("Fill") }

            TextSwitch {
                text: qsTr("Fill the shape")
                description: qsTr("A separate fill colour is stored as an extra attribute. "
                                  + "Xournal++ on the desktop ignores it and fills with the "
                                  + "outline colour instead; SVG keeps it exactly.")
                checked: app.controller.tools.fillEnabled
                onClicked: app.controller.tools.fillEnabled = checked
            }

            Slider {
                width: parent.width
                enabled: app.controller.tools.fillEnabled
                minimumValue: 0
                maximumValue: 255
                stepSize: 1
                value: app.controller.tools.fillAlpha
                label: qsTr("Fill opacity")
                valueText: Math.round(value / 2.55) + " %"
                onValueChanged: app.controller.tools.fillAlpha = value
            }

            Grid {
                x: Theme.horizontalPageMargin
                width: parent.width - 2 * Theme.horizontalPageMargin
                columns: 6
                spacing: Theme.paddingMedium
                enabled: app.controller.tools.fillEnabled
                opacity: app.controller.tools.fillEnabled ? 1.0 : 0.4

                Repeater {
                    model: page.swatches

                    delegate: Rectangle {
                        width: (parent.width - 5 * Theme.paddingMedium) / 6
                        height: width
                        radius: width / 2
                        color: modelData
                        border.width: Qt.colorEqual(app.controller.tools.fillColor, modelData) ? 3 : 1
                        border.color: Qt.colorEqual(app.controller.tools.fillColor, modelData)
                                      ? Theme.highlightColor
                                      : Theme.rgba(Theme.primaryColor, 0.35)

                        MouseArea {
                            anchors.fill: parent
                            onClicked: app.controller.tools.fillColor = modelData
                        }
                    }
                }
            }

            SectionHeader { text: qsTr("Shape") }

            TextSwitch {
                text: qsTr("Equal sides")
                description: qsTr("Circles instead of ovals, squares instead of rectangles.")
                checked: app.controller.tools.equalSides
                onClicked: app.controller.tools.equalSides = checked
            }

            Slider {
                width: parent.width
                minimumValue: 3
                maximumValue: 12
                stepSize: 1
                value: app.controller.tools.polygonCorners
                label: qsTr("Polygon corners")
                valueText: value
                onValueChanged: app.controller.tools.polygonCorners = value
            }
        }

        VerticalScrollDecorator { }
    }
}
