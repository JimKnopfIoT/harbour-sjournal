import QtQuick 2.0
import Sailfish.Silica 1.0
import harbour.sjournal 1.0

import "../components"

Page {
    id: notePage

    allowedOrientations: Orientation.All

    backNavigation: !pinned

    cutoutMode: CutoutMode.AvoidLandscapeCutout

    readonly property real topInset: orientation === Orientation.Portrait
                                     ? Screen.topCutout.height : 0

    property bool pinned: true

    property bool zoomMode: false

    property real zoom: 1.0
    property real fitZoom: 1.0
    property real fittedWidth: 0

    readonly property bool drawing: !zoomMode
    readonly property int zoomPercent: fitZoom > 0 ? Math.round(zoom / fitZoom * 100) : 100

    function fitToWidth() {
        var pageWidth = app.controller.pageWidth(0)
        if (pageWidth > 0 && canvasView.width > 0) {
            fitZoom = canvasView.width / pageWidth
            zoom = fitZoom
            fittedWidth = canvasView.width
            canvasView.contentX = 0
        }
    }

    function resetZoom() {
        var midY = (canvasView.contentY + canvasView.height / 2) / Math.max(zoom, 0.0001)
        zoom = fitZoom
        canvasView.contentX = 0
        canvasView.contentY = Math.max(0, Math.min(canvasView.contentHeight - canvasView.height,
                                                   midY * zoom - canvasView.height / 2))
    }

    function setZoom(newZoom, centreX, centreY) {
        var clamped = Math.max(fitZoom * 0.5, Math.min(fitZoom * 12, newZoom))
        if (Math.abs(clamped - zoom) < 0.0001)
            return
        var factor = clamped / zoom
        var newX = (canvasView.contentX + centreX) * factor - centreX
        var newY = (canvasView.contentY + centreY) * factor - centreY
        zoom = clamped
        canvasView.contentX = Math.max(0, Math.min(Math.max(0, canvasView.contentWidth - canvasView.width), newX))
        canvasView.contentY = Math.max(0, Math.min(Math.max(0, canvasView.contentHeight - canvasView.height), newY))
    }

    Component.onCompleted: fitToWidth()
    onWidthChanged: if (canvasView.width !== fittedWidth) fitToWidth()

    onStatusChanged: {
        if (status === PageStatus.Deactivating && app.controller.modified)
            app.controller.save()
    }

    TopBar {
        id: topBar
        z: 2
        anchors {
            left: parent.left
            right: parent.right
            top: parent.top
            topMargin: notePage.topInset
        }
        controller: app.controller
    }

    PinchArea {
        id: pinchArea

        anchors {
            left: parent.left
            right: parent.right
            top: topBar.bottom
            bottom: toolbar.top
        }

        property real startZoom: 1.0

        onPinchStarted: {
            startZoom = notePage.zoom
            canvasView.interactive = false
        }

        onPinchUpdated: notePage.setZoom(startZoom * pinch.scale,
                                        pinch.center.x, pinch.center.y)

        onPinchFinished: canvasView.interactive = Qt.binding(function() {
            return !notePage.pinned || notePage.zoomMode
        })

        Flickable {
            id: canvasView

            anchors.fill: parent

            contentWidth: Math.max(width, app.controller.pageWidth(0) * notePage.zoom)
            contentHeight: pageColumn.height
            boundsBehavior: Flickable.StopAtBounds
            clip: true

            interactive: !notePage.pinned || notePage.zoomMode

            Column {
                id: pageColumn
                width: canvasView.contentWidth
                spacing: Theme.paddingLarge

                Item {
                    width: parent.width
                    height: titleLabel.height + Theme.paddingMedium

                    Label {
                        id: titleLabel
                        anchors {
                            right: parent.right
                            rightMargin: Theme.horizontalPageMargin
                            top: parent.top
                        }
                        text: app.controller.title
                        color: Theme.highlightColor
                        font.pixelSize: Theme.fontSizeLarge
                    }
                }

                Repeater {
                    model: app.controller.pageCount

                    delegate: NoteCanvas {
                        controller: app.controller
                        pageIndex: index
                        zoom: notePage.zoom
                        drawingEnabled: notePage.drawing

                        width: app.controller.pageWidth(index) * notePage.zoom
                        height: app.controller.pageHeight(index) * notePage.zoom
                        anchors.horizontalCenter: parent.horizontalCenter

                        eraserRadius: 4 + app.controller.tools.penWidth
                        palmThreshold: 34

                        onDrawingChanged: {
                            if (drawing)
                                app.controller.currentPage = index
                        }
                    }
                }
            }

            MouseArea {
                anchors.fill: parent
                z: -1
                enabled: notePage.zoomMode
                onDoubleClicked: notePage.resetZoom()
            }
        }
    }

    function stepZoom(factor) {
        notePage.setZoom(notePage.zoom * factor,
                         canvasView.width / 2, canvasView.height / 2)
    }

    Row {
        id: zoomBar
        anchors {
            horizontalCenter: pinchArea.horizontalCenter
            top: pinchArea.top
            topMargin: Theme.paddingMedium
        }
        spacing: Theme.paddingMedium
        opacity: notePage.zoomMode || notePage.zoomPercent !== 100 ? 1.0 : 0.0
        visible: opacity > 0.01

        Behavior on opacity { NumberAnimation { duration: 200 } }

        Rectangle {
            width: Theme.itemSizeSmall
            height: Theme.itemSizeSmall
            radius: width / 2
            visible: notePage.zoomMode
            color: Theme.rgba(Theme.highlightDimmerColor, 0.9)
            anchors.verticalCenter: parent.verticalCenter

            Label {
                anchors.centerIn: parent
                text: "−"
                font.pixelSize: Theme.fontSizeLarge
                color: Theme.highlightColor
            }

            MouseArea {
                anchors.fill: parent
                onClicked: notePage.stepZoom(1 / 1.3)
            }
        }

        Rectangle {
            width: zoomLabel.width + Theme.paddingLarge * 2
            height: Theme.itemSizeSmall
            radius: height / 2
            color: Theme.rgba(Theme.highlightDimmerColor, 0.9)
            anchors.verticalCenter: parent.verticalCenter

            Label {
                id: zoomLabel
                anchors.centerIn: parent
                text: notePage.zoomPercent + " %"
                color: Theme.highlightColor
                font.pixelSize: Theme.fontSizeSmall
            }

            MouseArea {
                anchors.fill: parent
                enabled: notePage.zoomPercent !== 100
                onClicked: notePage.resetZoom()
            }
        }

        Rectangle {
            width: Theme.itemSizeSmall
            height: Theme.itemSizeSmall
            radius: width / 2
            visible: notePage.zoomMode
            color: Theme.rgba(Theme.highlightDimmerColor, 0.9)
            anchors.verticalCenter: parent.verticalCenter

            Label {
                anchors.centerIn: parent
                text: "+"
                font.pixelSize: Theme.fontSizeLarge
                color: Theme.highlightColor
            }

            MouseArea {
                anchors.fill: parent
                onClicked: notePage.stepZoom(1.3)
            }
        }
    }

    Rectangle {
        anchors {
            horizontalCenter: pinchArea.horizontalCenter
            bottom: toolbar.top
            bottomMargin: Theme.paddingLarge
        }
        width: sizeLabel.width + Theme.paddingLarge * 2
        height: sizeLabel.height + Theme.paddingMedium
        radius: height / 2
        color: Theme.rgba(Theme.highlightDimmerColor, 0.9)
        visible: app.controller.tools.hintText.length > 0

        Label {
            id: sizeLabel
            anchors.centerIn: parent
            text: app.controller.tools.hintText
            color: Theme.highlightColor
            font.pixelSize: Theme.fontSizeSmall
        }
    }

    Connections {
        target: app.controller
        onTextEditRequested: {
            var dialog = pageStack.push(Qt.resolvedUrl("TextInputDialog.qml"),
                                        { text: app.controller.pendingText })
            dialog.accepted.connect(function() { app.controller.commitText(dialog.text) })
            dialog.rejected.connect(function() { app.controller.cancelTextEdit() })
        }
    }

    NoteToolbar {
        id: toolbar
        anchors {
            left: parent.left
            right: parent.right
            bottom: parent.bottom
        }
        controller: app.controller
        pinned: notePage.pinned
        zoomMode: notePage.zoomMode
        onPinnedToggled: notePage.pinned = !notePage.pinned
        onZoomToggled: notePage.zoomMode = !notePage.zoomMode
        onZoomReset: notePage.resetZoom()
    }
}
