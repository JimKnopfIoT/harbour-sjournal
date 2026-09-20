import QtQuick 2.0
import Sailfish.Silica 1.0

import "../components"

Dialog {
    id: page

    property var style                      // controller.style
    property color strokeColor: "#000000"
    property color fillColour: "#1a72d0"
    property int alpha: 255
    property real strokeWidth: 1.41
    property var stops: []                  // colours, two or more make a gradient
    property int angle: 0
    property bool fillable: true

    property string target: "stroke"        // "stroke" or "fill"
    property int activeStop: 0

    readonly property bool gradientOn: stops.length >= 2

    canAccept: true

    function currentColour() {
        if (target === "stroke")
            return strokeColor
        if (gradientOn)
            return stops[Math.min(activeStop, stops.length - 1)]
        return fillColour
    }

    function setCurrentColour(value) {
        if (target === "stroke") {
            strokeColor = value
        } else if (gradientOn) {
            var next = stops.slice()
            next[Math.min(activeStop, next.length - 1)] = value
            stops = next
        } else {
            fillColour = value
        }
    }

    function setStopCount(count) {
        if (count < 2) {
            stops = []
            return
        }
        var next = []
        for (var i = 0; i < count; ++i)
            next.push(i < stops.length ? stops[i]
                                       : (i === 0 ? fillColour : Qt.rgba(1, 1, 1, 1)))
        stops = next
        activeStop = Math.min(activeStop, count - 1)
    }

    SilicaFlickable {
        anchors.fill: parent
        contentHeight: content.height + Theme.paddingLarge

        Column {
            id: content
            width: parent.width
            spacing: Theme.paddingMedium

            DialogHeader { acceptText: qsTr("Apply") }

            Row {
                x: Theme.horizontalPageMargin
                width: parent.width - 2 * Theme.horizontalPageMargin
                spacing: Theme.paddingSmall

                Repeater {
                    model: [ { key: "stroke", name: qsTr("Outline") },
                             { key: "fill",   name: qsTr("Area") } ]

                    delegate: Item {
                        width: (parent.width - Theme.paddingSmall) / 2
                        height: Theme.itemSizeExtraSmall

                        readonly property bool active: page.target === modelData.key
                        readonly property bool usable: modelData.key === "stroke" || page.fillable

                        Rectangle {
                            anchors.fill: parent
                            radius: Theme.paddingSmall / 2
                            color: parent.active ? Theme.rgba(Theme.highlightColor, 0.35)
                                                 : "transparent"
                            border.width: parent.active ? 0 : 1
                            border.color: Theme.rgba(Theme.primaryColor, 0.25)
                        }

                        Label {
                            anchors.centerIn: parent
                            text: modelData.name
                            font.pixelSize: Theme.fontSizeSmall
                            color: !parent.usable ? Theme.rgba(Theme.primaryColor, 0.3)
                                 : parent.active ? Theme.highlightColor : Theme.primaryColor
                        }

                        MouseArea {
                            anchors.fill: parent
                            enabled: parent.usable
                            onClicked: page.target = modelData.key
                        }
                    }
                }
            }

            ColorPicker {
                x: Theme.horizontalPageMargin
                width: parent.width - 2 * Theme.horizontalPageMargin
                chosen: page.currentColour()
                onPicked: page.setCurrentColour(value)
            }

            Slider {
                x: Theme.horizontalPageMargin
                width: parent.width - 2 * Theme.horizontalPageMargin
                visible: page.target === "fill"
                minimumValue: 0
                maximumValue: 255
                stepSize: 1
                value: page.alpha
                label: qsTr("Transparency")
                valueText: Math.round(100 - sliderValue / 2.55) + " %"
                onSliderValueChanged: page.alpha = Math.round(sliderValue)
            }

            Slider {
                x: Theme.horizontalPageMargin
                width: parent.width - 2 * Theme.horizontalPageMargin
                visible: page.target === "stroke"
                minimumValue: 0.2
                maximumValue: 20
                stepSize: 0.1
                value: page.strokeWidth
                label: qsTr("Line width")
                valueText: sliderValue.toFixed(1)
                onSliderValueChanged: page.strokeWidth = sliderValue
            }

            Label {
                x: Theme.horizontalPageMargin
                visible: page.target === "fill" && page.fillable
                text: qsTr("Gradient")
                font.pixelSize: Theme.fontSizeExtraSmall
                color: Theme.secondaryHighlightColor
            }

            Row {
                x: Theme.horizontalPageMargin
                width: parent.width - 2 * Theme.horizontalPageMargin
                spacing: Theme.paddingSmall
                visible: page.target === "fill" && page.fillable

                Repeater {
                    model: [ { count: 0, name: qsTr("Off") }, { count: 2, name: "2" },
                             { count: 3, name: "3" }, { count: 4, name: "4" },
                             { count: 5, name: "5" } ]

                    delegate: Item {
                        width: (parent.width - Theme.paddingSmall * 4) / 5
                        height: Theme.itemSizeExtraSmall * 0.8

                        readonly property bool active: modelData.count === 0
                                                       ? !page.gradientOn
                                                       : page.stops.length === modelData.count

                        Rectangle {
                            anchors.fill: parent
                            radius: Theme.paddingSmall / 2
                            color: parent.active ? Theme.rgba(Theme.highlightColor, 0.35)
                                                 : "transparent"
                            border.width: parent.active ? 0 : 1
                            border.color: Theme.rgba(Theme.primaryColor, 0.25)
                        }

                        Label {
                            anchors.centerIn: parent
                            text: modelData.name
                            font.pixelSize: Theme.fontSizeExtraSmall
                            color: parent.active ? Theme.highlightColor : Theme.primaryColor
                        }

                        MouseArea {
                            anchors.fill: parent
                            onClicked: page.setStopCount(modelData.count)
                        }
                    }
                }
            }

            Row {
                x: Theme.horizontalPageMargin
                width: parent.width - 2 * Theme.horizontalPageMargin
                spacing: Theme.paddingSmall
                visible: page.target === "fill" && page.gradientOn

                Repeater {
                    model: page.stops.length

                    delegate: Rectangle {
                        width: (parent.width - Theme.paddingSmall * (page.stops.length - 1))
                               / page.stops.length
                        height: Theme.itemSizeExtraSmall * 0.9
                        radius: Theme.paddingSmall / 2
                        color: page.stops[index]
                        border.width: page.activeStop === index ? 3 : 1
                        border.color: page.activeStop === index
                                      ? Theme.highlightColor
                                      : Theme.rgba(Theme.primaryColor, 0.4)

                        MouseArea {
                            anchors.fill: parent
                            onClicked: page.activeStop = index
                        }
                    }
                }
            }

            Slider {
                x: Theme.horizontalPageMargin
                width: parent.width - 2 * Theme.horizontalPageMargin
                visible: page.target === "fill" && page.gradientOn
                minimumValue: 0
                maximumValue: 350
                stepSize: 10
                value: page.angle
                label: qsTr("Gradient angle")
                valueText: Math.round(sliderValue) + "°"
                onSliderValueChanged: page.angle = Math.round(sliderValue)
            }

            Item {
                x: Theme.horizontalPageMargin
                width: parent.width - 2 * Theme.horizontalPageMargin
                height: Theme.itemSizeExtraSmall

                Rectangle {
                    anchors.fill: parent
                    radius: Theme.paddingSmall / 2
                    border.width: 1
                    border.color: Theme.rgba(Theme.primaryColor, 0.4)
                    color: page.target === "stroke" ? page.strokeColor
                         : page.gradientOn ? "transparent" : page.fillColour
                    opacity: page.target === "fill" ? page.alpha / 255 : 1

                    Row {
                        anchors.fill: parent
                        anchors.margins: 1
                        visible: page.target === "fill" && page.gradientOn

                        Repeater {
                            model: Math.max(0, page.stops.length - 1)

                            delegate: Item {
                                width: parent.width / Math.max(1, page.stops.length - 1)
                                height: parent.height
                                clip: true

                                Rectangle {
                                    width: parent.height
                                    height: parent.width
                                    anchors.centerIn: parent
                                    rotation: -90
                                    gradient: Gradient {
                                        GradientStop { position: 0.0; color: page.stops[index] }
                                        GradientStop { position: 1.0; color: page.stops[index + 1] }
                                    }
                                }
                            }
                        }
                    }
                }
            }
        }

        VerticalScrollDecorator {}
    }

    onAccepted: {
        if (!style)
            return
        if (target === "stroke") {
            style.applyColor(strokeColor)
            style.applyWidth(strokeWidth)
        } else if (gradientOn) {
            style.applyGradient(stops, angle, alpha)
        } else {
            style.applyFill(fillColour, alpha)
        }
    }
}
