import QtQuick 2.0
import Sailfish.Silica 1.0
import harbour.sjournal 1.0

import "../pages"

Rectangle {
    id: toolbar

    property DocumentController controller
    property bool pinned: true
    property bool zoomMode: false

    signal pinnedToggled()
    signal zoomToggled()
    signal zoomReset()

    readonly property var penKinds: [
        { kind: "pen",    tool: ToolSettings.PenTool,         name: qsTr("Pen") },
        { kind: "bezier", tool: ToolSettings.BezierTool,      name: qsTr("Bezier") },
        { kind: "brush",  tool: ToolSettings.BrushTool,       name: qsTr("Brush") },
        { kind: "marker", tool: ToolSettings.HighlighterTool, name: qsTr("Marker") }
    ]

    readonly property int activePenKind: {
        for (var i = 0; i < penKinds.length; ++i) {
            if (controller.tools.tool === penKinds[i].tool)
                return i
        }
        return -1
    }

    property string lastHint: ""

    function showHint(text) {
        lastHint = text
        controller.tools.reportHint(text)
        hintTimer.restart()
    }

    Timer {
        id: hintTimer
        interval: 1600
        onTriggered: {
            if (toolbar.controller.tools.hintText === toolbar.lastHint)
                toolbar.controller.tools.reportHint("")
        }
    }

    readonly property var selectShapes: [
        { shape: ToolSettings.RectSelect,  name: qsTr("Rectangle") },
        { shape: ToolSettings.LassoSelect, name: qsTr("Lasso") },
        { shape: ToolSettings.PointSelect, name: qsTr("Pick one by one") }
    ]

    function cycleSelectShape() {
        var at = 0
        for (var i = 0; i < selectShapes.length; ++i) {
            if (controller.tools.selectShape === selectShapes[i].shape)
                at = i
        }
        var next = (at + 1) % selectShapes.length
        controller.tools.selectShape = selectShapes[next].shape
        showHint(selectShapes[next].name)
    }

    readonly property var selectModes: [
        { glyph: "select", tool: ToolSettings.SelectTool, name: qsTr("Select paths") },
        { glyph: "nodes",  tool: ToolSettings.NodeTool,   name: qsTr("Select nodes") }
    ]

    readonly property int activeSelectMode: {
        for (var i = 0; i < selectModes.length; ++i) {
            if (controller.tools.tool === selectModes[i].tool)
                return i
        }
        return -1
    }

    property int lastSelectMode: 0

    onActiveSelectModeChanged: if (activeSelectMode >= 0) lastSelectMode = activeSelectMode

    readonly property var transformModes: [
        { glyph: "transform", mode: SelectionTransform.Move,           name: qsTr("Move") },
        { glyph: "resize",    mode: SelectionTransform.Resize,         name: qsTr("Resize") },
        { glyph: "fliph",     mode: SelectionTransform.FlipHorizontal, name: qsTr("Flip horizontally") },
        { glyph: "flipv",     mode: SelectionTransform.FlipVertical,   name: qsTr("Flip vertically") }
    ]

    readonly property int activeTransformMode: {
        if (controller.tools.tool !== ToolSettings.MoveTool)
            return -1
        for (var i = 0; i < transformModes.length; ++i) {
            if (controller.transform.mode === transformModes[i].mode)
                return i
        }
        return 0
    }

    function applyTransformMode(index) {
        var entry = transformModes[index]
        controller.tools.tool = ToolSettings.MoveTool
        controller.transform.mode = entry.mode
        if (entry.mode === SelectionTransform.FlipHorizontal)
            controller.transform.flipHorizontal()
        else if (entry.mode === SelectionTransform.FlipVertical)
            controller.transform.flipVertical()
        showHint(entry.name)
    }

    readonly property var shapeSlots: [
        { counts: false, variants: [ { glyph: "circle", kind: ToolSettings.EllipseKind },
                                     { glyph: "square", kind: ToolSettings.RectKind } ] },
        { counts: true,  variants: [ { glyph: "polygon", kind: ToolSettings.PolygonKind } ] },
        { counts: true,  variants: [ { glyph: "star",    kind: ToolSettings.StarKind } ] },
        { counts: false, variants: [ { glyph: "line",         kind: ToolSettings.LineKind },
                                     { glyph: "polyline",     kind: ToolSettings.PolylineKind },
                                     { glyph: "spline",       kind: ToolSettings.SplineKind },
                                     { glyph: "splineclosed", kind: ToolSettings.ClosedSplineKind } ] },
        { counts: false, variants: [ { glyph: "arrow",   kind: ToolSettings.ArrowKind } ] }
    ]

    property int lastPenKind: 0
    property string openPopup: ""

    onActivePenKindChanged: {
        if (activePenKind >= 0)
            lastPenKind = activePenKind
    }

    readonly property var palette: [
        "#000000", "#e12d2d", "#ef8a17", "#f2c40e",
        "#2e9e3e", "#1a72d0", "#7d3fbf", "#ffffff",
        "#8d5524", "#00a0a0", "#e05c9e", "#7a7a7a"
    ]

    implicitHeight: content.height + Theme.paddingMedium * 2
    color: Theme.rgba(Theme.highlightDimmerColor, 0.92)

    Rectangle {
        anchors { left: parent.left; right: parent.right; top: parent.top }
        height: 1
        color: Theme.rgba(Theme.primaryColor, 0.2)
    }

    Column {
        id: content
        anchors {
            left: parent.left
            right: parent.right
            verticalCenter: parent.verticalCenter
            leftMargin: Theme.horizontalPageMargin / 2
            rightMargin: Theme.horizontalPageMargin / 2
        }
        spacing: Theme.paddingSmall

        readonly property real topSlot: width / 10
        readonly property real bottomSlot: width / 10

        Flickable {
            width: parent.width
            height: Theme.itemSizeSmall
            contentWidth: topRow.width
            flickableDirection: Flickable.HorizontalFlick
            boundsBehavior: Flickable.StopAtBounds
            clip: true

            Row {
                id: topRow
                height: parent.height

                Item {
                    width: content.topSlot
                    height: topRow.height

                    readonly property bool selected: toolbar.activePenKind >= 0

                    Rectangle {
                        anchors.centerIn: parent
                        width: Theme.iconSizeMedium + Theme.paddingSmall
                        height: width
                        radius: width / 2
                        color: parent.selected ? Theme.rgba(Theme.highlightColor, 0.25)
                                               : "transparent"
                    }

                    ToolGlyph {
                        anchors.centerIn: parent
                        width: Theme.iconSizeMedium
                        height: width
                        kind: toolbar.penKinds[toolbar.lastPenKind].kind
                        color: parent.selected ? Theme.highlightColor : Theme.primaryColor
                    }

                    MouseArea {
                        anchors.fill: parent
                        onClicked: {
                            toolbar.controller.tools.tool =
                                    toolbar.penKinds[toolbar.lastPenKind].tool
                            toolbar.openPopup = toolbar.openPopup === "pen" ? "" : "pen"
                        }
                    }
                }

                Item {
                    width: content.topSlot
                    height: topRow.height

                    Rectangle {
                        anchors.centerIn: parent
                        width: Theme.iconSizeMedium + Theme.paddingSmall
                        height: width
                        radius: width / 2
                        color: toolbar.openPopup === "width"
                               ? Theme.rgba(Theme.highlightColor, 0.25) : "transparent"
                    }

                    Rectangle {
                        anchors.centerIn: parent
                        width: Math.max(Theme.paddingSmall,
                                        Math.min(Theme.iconSizeMedium * 0.8,
                                                 toolbar.controller.tools.penWidth * 1.8))
                        height: width
                        radius: width / 2
                        color: toolbar.controller.tools.color
                        border.width: 1
                        border.color: Theme.rgba(Theme.primaryColor, 0.4)

                        Behavior on width { NumberAnimation { duration: 100 } }
                    }

                    MouseArea {
                        anchors.fill: parent
                        onClicked: toolbar.openPopup = toolbar.openPopup === "width" ? "" : "width"
                    }
                }

                Repeater {
                    model: [
                        { kind: "eraser", tool: ToolSettings.EraserTool, name: qsTr("Eraser") },
                        { kind: "text",   tool: ToolSettings.TextTool,   name: qsTr("Text") }
                    ]

                    delegate: Item {
                        width: content.topSlot
                        height: topRow.height

                        property bool selected: toolbar.controller.tools.tool === modelData.tool

                        Rectangle {
                            anchors.centerIn: parent
                            width: Theme.iconSizeMedium + Theme.paddingSmall
                            height: width
                            radius: width / 2
                            color: parent.selected ? Theme.rgba(Theme.highlightColor, 0.25)
                                                   : "transparent"
                        }

                        ToolGlyph {
                            anchors.centerIn: parent
                            width: Theme.iconSizeMedium
                            height: width
                            kind: modelData.kind
                            color: parent.selected ? Theme.highlightColor : Theme.primaryColor
                        }

                        MouseArea {
                            anchors.fill: parent
                            onClicked: {
                                toolbar.openPopup = ""
                                toolbar.controller.tools.tool = modelData.tool
                                toolbar.showHint(modelData.name)
                            }
                        }
                    }
                }

                Item {
                    width: content.topSlot
                    height: topRow.height

                    readonly property int variant: toolbar.activeSelectMode >= 0
                                                   ? toolbar.activeSelectMode
                                                   : toolbar.lastSelectMode
                    readonly property bool selected: toolbar.activeSelectMode >= 0

                    Rectangle {
                        anchors.centerIn: parent
                        width: Theme.iconSizeMedium + Theme.paddingSmall
                        height: width
                        radius: width / 2
                        color: parent.selected ? Theme.rgba(Theme.highlightColor, 0.25)
                                               : "transparent"
                    }

                    ToolGlyph {
                        anchors.centerIn: parent
                        width: Theme.iconSizeMedium
                        height: width
                        kind: parent.variant === 0 && toolbar.controller.tools.selectShape
                                  === ToolSettings.LassoSelect ? "lasso"
                            : parent.variant === 0 && toolbar.controller.tools.selectShape
                                  === ToolSettings.PointSelect ? "pickpoint"
                            : toolbar.selectModes[parent.variant].glyph
                        color: parent.selected ? Theme.highlightColor : Theme.primaryColor
                    }

                    MouseArea {
                        anchors.fill: parent
                        onClicked: {
                            toolbar.openPopup = ""
                            var next = parent.selected
                                    ? (parent.variant + 1) % toolbar.selectModes.length
                                    : parent.variant
                            toolbar.controller.tools.tool = toolbar.selectModes[next].tool
                            toolbar.showHint(toolbar.selectModes[next].name)
                        }
                        onPressAndHold: {
                            if (toolbar.controller.tools.tool === ToolSettings.NodeTool) {
                                if (toolbar.controller.nodes.busy) {
                                    var left = toolbar.controller.nodes.simplify()
                                    toolbar.showHint(left > 0
                                                     ? qsTr("Thinned to %1 nodes").arg(left)
                                                     : qsTr("Nothing to thin out"))
                                }
                            } else {
                                toolbar.controller.tools.tool = ToolSettings.SelectTool
                                toolbar.cycleSelectShape()
                            }
                        }
                    }
                }

                Item {
                    width: content.topSlot
                    height: topRow.height

                    readonly property int variant: Math.max(0, toolbar.activeTransformMode)
                    readonly property bool selected: toolbar.activeTransformMode >= 0

                    Rectangle {
                        anchors.centerIn: parent
                        width: Theme.iconSizeMedium + Theme.paddingSmall
                        height: width
                        radius: width / 2
                        color: parent.selected ? Theme.rgba(Theme.highlightColor, 0.25)
                                               : "transparent"
                    }

                    ToolGlyph {
                        anchors.centerIn: parent
                        width: Theme.iconSizeMedium
                        height: width
                        kind: toolbar.transformModes[parent.variant].glyph
                        color: parent.selected ? Theme.highlightColor : Theme.primaryColor
                    }

                    MouseArea {
                        anchors.fill: parent
                        onClicked: {
                            toolbar.openPopup = ""
                            var next = parent.selected
                                    ? (parent.variant + 1) % toolbar.transformModes.length
                                    : parent.variant
                            toolbar.applyTransformMode(next)
                        }
                        onPressAndHold: {
                            toolbar.controller.tools.equalSides =
                                    !toolbar.controller.tools.equalSides
                            toolbar.showHint(toolbar.controller.tools.equalSides
                                             ? qsTr("Aspect locked") : qsTr("Aspect free"))
                        }
                    }
                }

                Item {
                    width: content.topSlot
                    height: topRow.height

                    Rectangle {
                        anchors.centerIn: parent
                        width: Theme.iconSizeMedium + Theme.paddingSmall
                        height: width
                        radius: width / 2
                        color: toolbar.zoomMode ? Theme.rgba(Theme.highlightColor, 0.35)
                                                : "transparent"
                    }

                    ToolGlyph {
                        anchors.centerIn: parent
                        width: Theme.iconSizeMedium
                        height: width
                        kind: "zoom"
                        color: toolbar.zoomMode ? Theme.highlightColor
                                                : Theme.rgba(Theme.primaryColor, 0.6)
                    }

                    MouseArea {
                        anchors.fill: parent
                        onClicked: {
                            toolbar.zoomToggled()
                            toolbar.showHint(qsTr("Magnifier"))
                        }
                        onPressAndHold: {
                            toolbar.zoomReset()
                            toolbar.showHint(qsTr("Zoom reset to 100 %"))
                        }
                    }
                }

                Item {
                    width: content.topSlot
                    height: topRow.height

                    Rectangle {
                        anchors.centerIn: parent
                        width: Theme.iconSizeMedium + Theme.paddingSmall
                        height: width
                        radius: width / 2
                        color: toolbar.controller.tools.snapToPoints
                               ? Theme.rgba(Theme.highlightColor, 0.35) : "transparent"
                    }

                    ToolGlyph {
                        anchors.centerIn: parent
                        width: Theme.iconSizeMedium
                        height: width
                        kind: "magnet"
                        color: toolbar.controller.tools.snapToPoints
                               ? Theme.highlightColor : Theme.rgba(Theme.primaryColor, 0.6)
                    }

                    MouseArea {
                        anchors.fill: parent
                        onClicked: {
                            toolbar.controller.tools.snapToPoints =
                                    !toolbar.controller.tools.snapToPoints
                            toolbar.showHint(toolbar.controller.tools.snapToPoints
                                             ? qsTr("Snap to points on") : qsTr("Snap to points off"))
                        }
                    }
                }

                Item {
                    width: content.topSlot
                    height: topRow.height

                    Rectangle {
                        anchors.centerIn: parent
                        width: Theme.iconSizeMedium + Theme.paddingSmall
                        height: width
                        radius: width / 2
                        color: toolbar.controller.tools.snapSizes
                               ? Theme.rgba(Theme.highlightColor, 0.35) : "transparent"
                    }

                    ToolGlyph {
                        anchors.centerIn: parent
                        width: Theme.iconSizeMedium
                        height: width
                        kind: "ruler"
                        color: toolbar.controller.tools.snapSizes
                               ? Theme.highlightColor : Theme.rgba(Theme.primaryColor, 0.6)
                    }

                    MouseArea {
                        anchors.fill: parent
                        onClicked: {
                            toolbar.controller.tools.snapSizes =
                                    !toolbar.controller.tools.snapSizes
                            toolbar.showHint(toolbar.controller.tools.snapSizes
                                             ? qsTr("Size grid on") : qsTr("Size grid off"))
                        }
                    }
                }

                Item {
                    width: content.topSlot
                    height: topRow.height

                    Rectangle {
                        anchors.centerIn: parent
                        width: Theme.iconSizeMedium + Theme.paddingSmall
                        height: width
                        radius: width / 2
                        color: toolbar.controller.tools.shapeRecognition
                               ? Theme.rgba(Theme.highlightColor, 0.35) : "transparent"
                    }

                    ToolGlyph {
                        anchors.centerIn: parent
                        width: Theme.iconSizeMedium
                        height: width
                        kind: "shape"
                        color: toolbar.controller.tools.shapeRecognition
                               ? Theme.highlightColor
                               : Theme.rgba(Theme.primaryColor, 0.6)
                    }

                    MouseArea {
                        anchors.fill: parent
                        onClicked: {
                            toolbar.controller.tools.shapeRecognition =
                                    !toolbar.controller.tools.shapeRecognition
                            toolbar.showHint(toolbar.controller.tools.shapeRecognition
                                             ? qsTr("Shape recognition on") : qsTr("Shape recognition off"))
                        }
                    }
                }

                Item {
                    width: content.topSlot
                    height: topRow.height

                    Rectangle {
                        anchors.centerIn: parent
                        width: Theme.iconSizeMedium + Theme.paddingSmall
                        height: width
                        radius: width / 2
                        color: toolbar.pinned ? Theme.rgba(Theme.highlightColor, 0.35)
                                              : "transparent"
                    }

                    ToolGlyph {
                        anchors.centerIn: parent
                        width: Theme.iconSizeMedium
                        height: width
                        kind: "pin"
                        color: toolbar.pinned ? Theme.highlightColor
                                              : Theme.rgba(Theme.primaryColor, 0.5)
                    }

                    MouseArea {
                        anchors.fill: parent
                        onClicked: {
                            toolbar.pinnedToggled()
                            toolbar.showHint(qsTr("Pin page"))
                        }
                    }
                }

                Repeater {
                    model: [
                        { kind: "layers", action: "layers" },
                        { kind: "menu",   action: "menu" }
                    ]

                    delegate: Item {
                        width: content.topSlot
                        height: topRow.height

                        property bool actionEnabled: true

                        ToolGlyph {
                            anchors.centerIn: parent
                            width: Theme.iconSizeMedium
                            height: width
                            kind: modelData.kind
                            color: parent.actionEnabled
                                   ? Theme.primaryColor
                                   : Theme.rgba(Theme.primaryColor, 0.3)
                        }

                        MouseArea {
                            anchors.fill: parent
                            enabled: parent.actionEnabled
                            onClicked: {
                                if (modelData.action === "layers")
                                    pageStack.push(Qt.resolvedUrl("../pages/LayersPage.qml"))
                                else
                                    pageStack.push(Qt.resolvedUrl("../pages/NoteMenuPage.qml"))
                            }
                            onPressAndHold: toolbar.showHint(
                                    modelData.action === "layers" ? qsTr("Layers") : qsTr("More"))
                        }
                    }
                }
            }
        }

        Flickable {
            width: parent.width
            height: Theme.itemSizeSmall * 0.8
            contentWidth: bottomRow.width
            flickableDirection: Flickable.HorizontalFlick
            boundsBehavior: Flickable.StopAtBounds
            clip: true

            Row {
                id: bottomRow
                height: parent.height

                Repeater {
                    model: toolbar.shapeSlots

                    delegate: Item {
                        id: shapeButton

                        width: content.bottomSlot
                        height: bottomRow.height

                        readonly property var variants: modelData.variants

                        readonly property int activeVariant: {
                            if (toolbar.controller.tools.tool !== ToolSettings.ShapeTool)
                                return -1
                            for (var i = 0; i < variants.length; ++i) {
                                if (toolbar.controller.tools.shapeKind === variants[i].kind)
                                    return i
                            }
                            return -1
                        }

                        readonly property bool selected: activeVariant >= 0
                        readonly property bool locked: selected && toolbar.controller.tools.equalSides

                        property int shown: 0

                        onActiveVariantChanged: {
                            if (activeVariant >= 0)
                                shown = activeVariant
                        }

                        function activate(variant) {
                            toolbar.controller.tools.shapeKind = variants[variant].kind
                            toolbar.controller.tools.tool = ToolSettings.ShapeTool
                        }

                        Rectangle {
                            anchors.centerIn: parent
                            width: Theme.iconSizeSmall + Theme.paddingSmall
                            height: width
                            radius: Theme.paddingSmall / 2
                            color: shapeButton.selected ? Theme.rgba(Theme.highlightColor, 0.35)
                                                        : "transparent"
                            border.width: shapeButton.locked ? 2 : 0
                            border.color: Theme.highlightColor
                        }

                        ToolGlyph {
                            anchors.centerIn: parent
                            width: Theme.iconSizeSmall
                            height: width
                            kind: shapeButton.variants[shapeButton.shown].glyph
                            corners: toolbar.controller.tools.polygonCorners
                            color: shapeButton.selected ? Theme.highlightColor
                                                        : Theme.rgba(Theme.primaryColor, 0.75)
                        }

                        MouseArea {
                            anchors.fill: parent

                            onClicked: {
                                if (!shapeButton.selected) {
                                    shapeButton.activate(shapeButton.shown)
                                } else if (shapeButton.variants.length > 1) {
                                    shapeButton.shown =
                                            (shapeButton.shown + 1) % shapeButton.variants.length
                                    shapeButton.activate(shapeButton.shown)
                                } else if (modelData.counts) {
                                    toolbar.controller.tools.polygonCorners =
                                            toolbar.controller.tools.polygonCorners >= 12
                                                ? 3 : toolbar.controller.tools.polygonCorners + 1
                                }
                            }

                            onPressAndHold: {
                                shapeButton.activate(shapeButton.shown)
                                toolbar.controller.tools.equalSides =
                                        !toolbar.controller.tools.equalSides
                            }
                        }
                    }
                }

                Repeater {
                    model: toolbar.palette

                    delegate: Item {
                        width: content.bottomSlot
                        height: bottomRow.height

                        property bool selected: Qt.colorEqual(toolbar.controller.tools.color, modelData)

                        Rectangle {
                            anchors.centerIn: parent
                            width: parent.selected ? Theme.iconSizeSmall
                                                   : Theme.iconSizeSmall * 0.72
                            height: width
                            radius: width / 2
                            color: modelData
                            border.width: parent.selected ? 2 : 1
                            border.color: parent.selected
                                          ? Theme.highlightColor
                                          : Theme.rgba(Theme.primaryColor, 0.35)

                            Behavior on width { NumberAnimation { duration: 100 } }
                        }

                        MouseArea {
                            anchors.fill: parent
                            onClicked: toolbar.controller.tools.color = modelData
                        }
                    }
                }

                Item {
                    width: content.bottomSlot
                    height: bottomRow.height

                    ToolGlyph {
                        anchors.centerIn: parent
                        width: Theme.iconSizeSmall
                        height: width
                        kind: "photo"
                        color: Theme.primaryColor
                    }

                    MouseArea {
                        anchors.fill: parent
                        onClicked: {
                            var picker = pageStack.push(imagePicker)
                            picker.selectedContentChanged.connect(function() {
                                toolbar.controller.insertImage(picker.selectedContent, true)
                            })
                        }
                    }
                }

            }
        }
    }

    MouseArea {
        y: -toolbar.y
        width: toolbar.width
        height: Math.max(0, toolbar.y)
        enabled: toolbar.openPopup !== ""
        visible: enabled
        onClicked: toolbar.openPopup = ""
    }

    Rectangle {
        id: penPopup

        visible: toolbar.openPopup === "pen"
        x: content.x
        y: -height - Theme.paddingSmall
        width: penRow.width + Theme.paddingLarge
        height: penRow.height + Theme.paddingMedium
        radius: Theme.paddingMedium
        color: Theme.rgba(Theme.highlightDimmerColor, 0.97)
        border.width: 1
        border.color: Theme.rgba(Theme.primaryColor, 0.25)

        Row {
            id: penRow
            anchors.centerIn: parent
            spacing: Theme.paddingMedium

            Repeater {
                model: toolbar.penKinds

                delegate: Item {
                    id: penButton

                    width: Math.max(Theme.itemSizeSmall, kindLabel.implicitWidth)
                    height: penColumn.height

                    readonly property bool selected:
                        toolbar.controller.tools.tool === modelData.tool

                    Column {
                        id: penColumn
                        width: parent.width
                        spacing: Theme.paddingSmall / 2

                        ToolGlyph {
                            anchors.horizontalCenter: parent.horizontalCenter
                            width: Theme.iconSizeMedium
                            height: width
                            kind: modelData.kind
                            color: penButton.selected ? Theme.highlightColor : Theme.primaryColor
                        }

                        Label {
                            id: kindLabel
                            anchors.horizontalCenter: parent.horizontalCenter
                            text: modelData.name
                            font.pixelSize: Theme.fontSizeTiny
                            color: penButton.selected ? Theme.highlightColor : Theme.secondaryColor
                        }
                    }

                    MouseArea {
                        anchors.fill: parent
                        onClicked: {
                            toolbar.controller.tools.tool = modelData.tool
                            toolbar.openPopup = ""
                        }
                    }
                }
            }
        }
    }

    Rectangle {
        id: widthPopup

        visible: toolbar.openPopup === "width"
        x: Math.max(content.x,
                    Math.min(content.x + content.topSlot * 1.5 - width / 2,
                             toolbar.width - width - content.x))
        y: -height - Theme.paddingSmall
        width: Math.min(toolbar.width - 2 * content.x, Theme.itemSizeHuge * 2.2)
        height: widthColumn.height + Theme.paddingMedium
        radius: Theme.paddingMedium
        color: Theme.rgba(Theme.highlightDimmerColor, 0.97)
        border.width: 1
        border.color: Theme.rgba(Theme.primaryColor, 0.25)

        Column {
            id: widthColumn
            anchors.centerIn: parent
            width: parent.width - Theme.paddingMedium

            Item {
                width: parent.width
                height: Theme.itemSizeExtraSmall * 0.7

                Rectangle {
                    anchors.centerIn: parent
                    width: parent.width - Theme.paddingLarge
                    height: Math.max(1, toolbar.controller.tools.penWidth * 1.6)
                    radius: height / 2
                    color: toolbar.controller.tools.color
                    opacity: toolbar.controller.tools.tool === ToolSettings.HighlighterTool
                             ? 0.5 : 1.0
                }
            }

            Slider {
                width: parent.width
                minimumValue: 0.3
                maximumValue: 20.0
                stepSize: 0.1
                value: toolbar.controller.tools.penWidth
                valueText: qsTr("%1 pt").arg(value.toFixed(1))
                onValueChanged: toolbar.controller.tools.penWidth = value
            }
        }
    }

    Component {
        id: imagePicker

        ImageBrowserPage { }
    }
}
