import QtQuick 2.0
import Sailfish.Silica 1.0
import harbour.sjournal 1.0

Item {
    id: bar

    property DocumentController controller

    readonly property bool hasSelection: controller.selection.count > 0
    readonly property bool editing: hasSelection || controller.selection.clipboardCount > 0

    property bool fontMenuOpen: false
    property bool pathMenuOpen: false

    readonly property bool textToolActive: controller.tools.tool === ToolSettings.TextTool

    onTextToolActiveChanged: if (!textToolActive) fontMenuOpen = false
    onEditingChanged: if (!editing) pathMenuOpen = false

    readonly property var pathActions: [
        { glyph: "textpath",   action: "text",   name: qsTr("Text to path") },
        { glyph: "joinpaths",  action: "join",   name: qsTr("Join paths") },
        { glyph: "breakapart", action: "break",  name: qsTr("Break apart") },
        { glyph: "unite",      action: "unite",  name: qsTr("Add together") },
        { glyph: "subtract",   action: "cut",    name: qsTr("Subtract") }
    ]

    function runPathAction(action) {
        pathMenuOpen = false
        if (action === "text")
            controller.paths.textToPath()
        else if (action === "join")
            controller.paths.join()
        else if (action === "break")
            controller.paths.breakApart()
        else if (action === "unite")
            controller.paths.unite()
        else
            controller.paths.subtract()
    }

    function openColourPage(forFill) {
        var s = bar.controller.style
        pageStack.push(Qt.resolvedUrl("../pages/ColorPage.qml"), {
            style: s,
            target: forFill ? "fill" : "stroke",
            fillable: s.canFill,
            strokeColor: s.color,
            fillColour: s.fillColor,
            alpha: s.fillAlpha,
            strokeWidth: s.strokeWidth > 0 ? s.strokeWidth : bar.controller.tools.penWidth,
            stops: s.gradientColors,
            angle: s.gradientAngle
        })
    }

    function pathActionEnabled(action) {
        if (action === "text")
            return controller.paths.canConvertText
        if (action === "break")
            return controller.paths.canBreakApart
        return controller.paths.canCombine
    }

    readonly property bool nodeActive: controller.tools.tool === ToolSettings.NodeTool
                                       && controller.nodes.hasNode
    readonly property bool showGeometry: nodeActive || hasSelection

    implicitHeight: topRow.height + geometryRow.height
                    + (editing ? actionRow.height : 0)
    height: implicitHeight



    Rectangle {
        anchors.fill: parent
        color: Theme.rgba(Theme.highlightDimmerColor, 0.92)
    }

    Rectangle {
        anchors { left: parent.left; right: parent.right; bottom: parent.bottom }
        height: 1
        color: Theme.rgba(Theme.primaryColor, 0.2)
    }

    Item {
        id: topRow

        anchors { left: parent.left; right: parent.right; top: parent.top }
        height: Theme.itemSizeSmall

        Row {
            id: leftTools

            anchors {
                left: parent.left
                leftMargin: Theme.horizontalPageMargin / 2
                verticalCenter: parent.verticalCenter
            }
            spacing: Theme.paddingSmall

            Item {
                id: outlineButton
                width: Theme.itemSizeSmall
                height: Theme.itemSizeSmall

                Rectangle {
                    anchors.centerIn: parent
                    width: Theme.iconSizeMedium + Theme.paddingSmall
                    height: width
                    radius: width / 2
                    color: bar.controller.tools.outlineView ? Theme.rgba(Theme.highlightColor, 0.35)
                                                      : "transparent"
                }

                ToolGlyph {
                    anchors.centerIn: parent
                    width: Theme.iconSizeMedium
                    height: width
                    kind: "outline"
                    color: bar.controller.tools.outlineView ? Theme.highlightColor
                                                      : Theme.rgba(Theme.primaryColor, 0.7)
                }

                MouseArea {
                    anchors.fill: parent
                    onClicked: bar.controller.tools.outlineView = !bar.controller.tools.outlineView
                }
            }

            Repeater {
                model: [ "undo", "redo" ]

                delegate: Item {
                    width: Theme.itemSizeSmall
                    height: Theme.itemSizeSmall

                    readonly property bool actionEnabled: modelData === "undo"
                                                          ? bar.controller.canUndo
                                                          : bar.controller.canRedo

                    ToolGlyph {
                        anchors.centerIn: parent
                        width: Theme.iconSizeMedium
                        height: width
                        kind: modelData
                        color: parent.actionEnabled ? Theme.primaryColor
                                                    : Theme.rgba(Theme.primaryColor, 0.3)
                    }

                    MouseArea {
                        anchors.fill: parent
                        enabled: parent.actionEnabled
                        onClicked: {
                            if (modelData === "undo")
                                bar.controller.undo()
                            else
                                bar.controller.redo()
                        }
                    }
                }
            }

            Item {
                id: fontButton
                width: Theme.itemSizeSmall
                height: Theme.itemSizeSmall
                visible: bar.textToolActive

                Rectangle {
                    anchors.centerIn: parent
                    width: Theme.iconSizeMedium + Theme.paddingSmall
                    height: width
                    radius: width / 2
                    color: bar.fontMenuOpen ? Theme.rgba(Theme.highlightColor, 0.35) : "transparent"
                }

                Label {
                    anchors.centerIn: parent
                    text: "A"
                    color: Theme.highlightColor
                    font.pixelSize: Theme.fontSizeMedium
                    font.bold: bar.controller.tools.fontBold
                    font.italic: bar.controller.tools.fontItalic
                    font.underline: bar.controller.tools.fontUnderline
                }

                MouseArea {
                    anchors.fill: parent
                    onClicked: bar.fontMenuOpen = !bar.fontMenuOpen
                }
            }

        }

        Label {
            anchors {
                left: leftTools.right
                leftMargin: Theme.paddingMedium
                right: parent.right
                rightMargin: Theme.horizontalPageMargin / 2
                verticalCenter: parent.verticalCenter
            }
            horizontalAlignment: Text.AlignRight
            truncationMode: TruncationMode.Fade
            visible: bar.controller.tools.outlineView && !bar.editing && !bar.textToolActive
            text: qsTr("Outline")
            font.pixelSize: Theme.fontSizeExtraSmall
            color: Theme.secondaryHighlightColor
        }

        Label {
            anchors {
                left: leftTools.right
                leftMargin: Theme.paddingMedium
                right: parent.right
                rightMargin: Theme.horizontalPageMargin / 2
                verticalCenter: parent.verticalCenter
            }
            horizontalAlignment: Text.AlignRight
            truncationMode: TruncationMode.Fade
            visible: bar.editing
            text: bar.hasSelection
                  ? (bar.controller.selection.count === 1
                     ? qsTr("1 selected")
                     : qsTr("%1 selected").arg(bar.controller.selection.count))
                  : qsTr("%1 copied").arg(bar.controller.selection.clipboardCount)
            font.pixelSize: Theme.fontSizeExtraSmall
            color: Theme.secondaryHighlightColor
        }
    }

    Item {
        id: geometryRow

        anchors { left: parent.left; right: parent.right; top: topRow.bottom }
        height: bar.showGeometry ? Theme.itemSizeExtraSmall * 0.9 : 0
        visible: bar.showGeometry
        clip: true

        Row {
            anchors {
                left: parent.left
                right: parent.right
                leftMargin: Theme.horizontalPageMargin / 2
                rightMargin: Theme.horizontalPageMargin / 2
                verticalCenter: parent.verticalCenter
            }
            spacing: Theme.paddingSmall

            property int slots: bar.nodeActive ? 2 : 4
            property real slotWidth: (width - spacing * (slots - 1)) / slots

            NumberField {
                width: parent.slotWidth
                label: "X"
                value: bar.nodeActive ? bar.controller.nodes.nodeX
                                      : bar.controller.transform.box.x
                onCommitted: {
                    if (bar.nodeActive)
                        bar.controller.nodes.moveNodeTo(newValue, bar.controller.nodes.nodeY)
                    else
                        bar.controller.transform.moveTo(newValue, bar.controller.transform.box.y)
                }
            }

            NumberField {
                width: parent.slotWidth
                label: "Y"
                value: bar.nodeActive ? bar.controller.nodes.nodeY
                                      : bar.controller.transform.box.y
                onCommitted: {
                    if (bar.nodeActive)
                        bar.controller.nodes.moveNodeTo(bar.controller.nodes.nodeX, newValue)
                    else
                        bar.controller.transform.moveTo(bar.controller.transform.box.x, newValue)
                }
            }

            NumberField {
                width: parent.slotWidth
                visible: !bar.nodeActive
                label: "B"
                value: bar.controller.transform.box.width
                onCommitted: bar.controller.transform.resizeTo(
                                 newValue, bar.controller.transform.box.height)
            }

            NumberField {
                width: parent.slotWidth
                visible: !bar.nodeActive
                label: "H"
                value: bar.controller.transform.box.height
                onCommitted: bar.controller.transform.resizeTo(
                                 bar.controller.transform.box.width, newValue)
            }
        }
    }

    Item {
        id: actionRow

        anchors { left: parent.left; right: parent.right; top: geometryRow.bottom }
        height: Theme.itemSizeSmall
        clip: true
        opacity: bar.editing ? 1.0 : 0.0
        visible: opacity > 0.01

        Behavior on opacity { NumberAnimation { duration: 120 } }

        Row {
            anchors {
                right: parent.right
                rightMargin: Theme.horizontalPageMargin / 2
                verticalCenter: parent.verticalCenter
            }
            spacing: Theme.paddingSmall

            Item {
                width: Theme.itemSizeSmall
                height: Theme.itemSizeSmall
                visible: bar.hasSelection

                Rectangle {
                    anchors.centerIn: parent
                    width: Theme.iconSizeMedium + Theme.paddingSmall
                    height: width
                    radius: width / 2
                    color: bar.pathMenuOpen ? Theme.rgba(Theme.highlightColor, 0.25)
                                            : "transparent"
                }

                ToolGlyph {
                    anchors.centerIn: parent
                    width: Theme.iconSizeMedium
                    height: width
                    kind: "pathops"
                    color: bar.pathMenuOpen ? Theme.highlightColor : Theme.primaryColor
                }

                MouseArea {
                    anchors.fill: parent
                    onClicked: bar.pathMenuOpen = !bar.pathMenuOpen
                }
            }

            Item {
                width: Theme.itemSizeSmall
                height: Theme.itemSizeSmall
                visible: bar.hasSelection

                ToolGlyph {
                    anchors.centerIn: parent
                    width: Theme.iconSizeMedium
                    height: width
                    kind: "swatch"
                    color: bar.controller.style.color.a > 0 ? bar.controller.style.color
                                                            : Theme.primaryColor
                }

                Rectangle {
                    anchors.centerIn: parent
                    width: Theme.iconSizeMedium + Theme.paddingSmall
                    height: width
                    radius: width / 2
                    color: "transparent"
                    border.width: 1
                    border.color: Theme.rgba(Theme.primaryColor, 0.4)
                }

                MouseArea {
                    anchors.fill: parent
                    onClicked: bar.openColourPage(false)
                }
            }

            Item {
                width: Theme.itemSizeSmall
                height: Theme.itemSizeSmall
                visible: bar.hasSelection

                readonly property bool actionEnabled: bar.controller.style.canFill

                ToolGlyph {
                    anchors.centerIn: parent
                    width: Theme.iconSizeMedium
                    height: width
                    kind: "bucket"
                    color: !parent.actionEnabled ? Theme.rgba(Theme.primaryColor, 0.3)
                         : bar.controller.style.filled ? Theme.highlightColor
                                                       : Theme.primaryColor
                }

                MouseArea {
                    anchors.fill: parent
                    enabled: parent.actionEnabled
                    onClicked: bar.openColourPage(true)
                    onPressAndHold: bar.controller.style.removeFill()
                }
            }

            Item {
                width: Theme.itemSizeSmall
                height: Theme.itemSizeSmall
                visible: bar.hasSelection

                property bool actionEnabled: bar.controller.selection.canGroup
                                             || bar.controller.selection.canUngroup

                ToolGlyph {
                    anchors.centerIn: parent
                    width: Theme.iconSizeMedium
                    height: width
                    kind: "group"
                    color: parent.actionEnabled ? (bar.controller.selection.canUngroup
                                                   ? Theme.highlightColor : Theme.primaryColor)
                                                : Theme.rgba(Theme.primaryColor, 0.3)
                }

                MouseArea {
                    anchors.fill: parent
                    enabled: parent.actionEnabled
                    onClicked: bar.controller.selection.group()
                    onPressAndHold: bar.controller.selection.ungroup()
                }
            }

            Repeater {
                model: [
                    { kind: "copy",  action: "copy" },
                    { kind: "cut",   action: "cut" },
                    { kind: "paste", action: "paste" },
                    { kind: "trash", action: "delete" }
                ]

                delegate: Item {
                    width: Theme.itemSizeSmall
                    height: Theme.itemSizeSmall

                    property bool actionEnabled:
                        modelData.action === "paste" ? bar.controller.selection.clipboardCount > 0
                                                     : bar.hasSelection

                    ToolGlyph {
                        anchors.centerIn: parent
                        width: Theme.iconSizeMedium
                        height: width
                        kind: modelData.kind
                        color: parent.actionEnabled ? Theme.primaryColor
                                                    : Theme.rgba(Theme.primaryColor, 0.3)
                    }

                    MouseArea {
                        anchors.fill: parent
                        enabled: parent.actionEnabled
                        onClicked: {
                            if (modelData.action === "copy")
                                bar.controller.selection.copy()
                            else if (modelData.action === "cut")
                                bar.controller.selection.cut()
                            else if (modelData.action === "paste")
                                bar.controller.selection.paste()
                            else
                                bar.controller.selection.remove()
                        }
                    }
                }
            }
        }
    }

    MouseArea {
        y: bar.height
        width: bar.width
        height: Screen.height
        enabled: bar.fontMenuOpen
        visible: enabled
        onClicked: bar.fontMenuOpen = false
    }

    MouseArea {
        y: bar.height
        width: bar.width
        height: Screen.height
        enabled: bar.pathMenuOpen
        visible: enabled
        onClicked: bar.pathMenuOpen = false
    }

    Rectangle {
        id: pathMenu

        visible: bar.pathMenuOpen
        x: Math.max(Theme.horizontalPageMargin / 2, bar.width - width - Theme.horizontalPageMargin / 2)
        y: bar.height + Theme.paddingSmall
        width: Math.min(bar.width - Theme.horizontalPageMargin, Theme.itemSizeHuge * 2.4)
        height: pathColumn.height + Theme.paddingLarge
        radius: Theme.paddingMedium
        color: Theme.rgba(Theme.highlightDimmerColor, 0.97)
        border.width: 1
        border.color: Theme.rgba(Theme.primaryColor, 0.25)

        Column {
            id: pathColumn
            anchors.centerIn: parent
            width: parent.width - Theme.paddingLarge
            spacing: Theme.paddingSmall

            Repeater {
                model: bar.pathActions

                delegate: Item {
                    width: pathColumn.width
                    height: Theme.itemSizeExtraSmall

                    readonly property bool available: bar.pathActionEnabled(modelData.action)

                    ToolGlyph {
                        id: actionGlyph
                        anchors {
                            left: parent.left
                            verticalCenter: parent.verticalCenter
                        }
                        width: Theme.iconSizeMedium
                        height: width
                        kind: modelData.glyph
                        color: parent.available ? Theme.primaryColor
                                                : Theme.rgba(Theme.primaryColor, 0.3)
                    }

                    Label {
                        anchors {
                            left: actionGlyph.right
                            leftMargin: Theme.paddingMedium
                            right: parent.right
                            verticalCenter: parent.verticalCenter
                        }
                        truncationMode: TruncationMode.Fade
                        text: modelData.name
                        font.pixelSize: Theme.fontSizeSmall
                        color: parent.available ? Theme.primaryColor
                                                : Theme.rgba(Theme.primaryColor, 0.3)
                    }

                    MouseArea {
                        anchors.fill: parent
                        enabled: parent.available
                        onClicked: bar.runPathAction(modelData.action)
                    }
                }
            }
        }
    }

    Rectangle {
        id: fontMenu

        visible: bar.fontMenuOpen
        x: Theme.horizontalPageMargin / 2
        y: bar.height + Theme.paddingSmall
        width: Math.min(bar.width - Theme.horizontalPageMargin, Theme.itemSizeHuge * 2.6)
        height: menuColumn.height + Theme.paddingLarge
        radius: Theme.paddingMedium
        color: Theme.rgba(Theme.highlightDimmerColor, 0.97)
        border.width: 1
        border.color: Theme.rgba(Theme.primaryColor, 0.25)

        Column {
            id: menuColumn
            anchors.centerIn: parent
            width: parent.width - Theme.paddingLarge
            spacing: Theme.paddingMedium

            Row {
                width: parent.width

                Label {
                    width: parent.width / 2
                    text: qsTr("Size")
                    font.pixelSize: Theme.fontSizeExtraSmall
                    color: Theme.secondaryHighlightColor
                }

                Label {
                    width: parent.width / 2
                    horizontalAlignment: Text.AlignRight
                    text: Math.round(bar.controller.tools.fontSize)
                    font.pixelSize: Theme.fontSizeExtraSmall
                    color: Theme.highlightColor
                }
            }

            Row {
                width: parent.width
                spacing: Theme.paddingSmall

                Repeater {
                    model: [ 10, 12, 16, 24, 36, 48 ]

                    delegate: Item {
                        width: (menuColumn.width - Theme.paddingSmall * 5) / 6
                        height: Theme.itemSizeExtraSmall * 0.8

                        readonly property bool picked:
                            Math.abs(bar.controller.tools.fontSize - modelData) < 0.01

                        Rectangle {
                            anchors.fill: parent
                            radius: Theme.paddingSmall / 2
                            color: parent.picked ? Theme.rgba(Theme.highlightColor, 0.35)
                                                 : "transparent"
                        }

                        Label {
                            anchors.centerIn: parent
                            text: modelData
                            font.pixelSize: Theme.fontSizeExtraSmall
                            color: parent.picked ? Theme.highlightColor : Theme.primaryColor
                        }

                        MouseArea {
                            anchors.fill: parent
                            onClicked: bar.controller.tools.fontSize = modelData
                        }
                    }
                }
            }

            Row {
                width: parent.width
                spacing: Theme.paddingSmall

                Repeater {
                    model: [ -1, 1 ]

                    delegate: Item {
                        width: (menuColumn.width - Theme.paddingSmall) / 2
                        height: Theme.itemSizeExtraSmall * 0.8

                        Rectangle {
                            anchors.fill: parent
                            radius: Theme.paddingSmall / 2
                            color: "transparent"
                            border.width: 1
                            border.color: Theme.rgba(Theme.primaryColor, 0.25)
                        }

                        Label {
                            anchors.centerIn: parent
                            text: modelData < 0 ? "\u2212" : "+"
                            font.pixelSize: Theme.fontSizeMedium
                            color: Theme.primaryColor
                        }

                        MouseArea {
                            anchors.fill: parent
                            onClicked: bar.controller.tools.fontSize =
                                       Math.round(bar.controller.tools.fontSize) + modelData
                            onPressAndHold: bar.controller.tools.fontSize =
                                            Math.round(bar.controller.tools.fontSize) + modelData * 10
                        }
                    }
                }
            }

            Label {
                text: qsTr("Weight")
                font.pixelSize: Theme.fontSizeExtraSmall
                color: Theme.secondaryHighlightColor
            }

            Row {
                spacing: Theme.paddingMedium

                Repeater {
                    model: [ "bold", "italic", "underline" ]

                    delegate: Item {
                        width: Theme.itemSizeSmall
                        height: Theme.itemSizeExtraSmall

                        readonly property bool on:
                            modelData === "bold" ? bar.controller.tools.fontBold
                          : modelData === "italic" ? bar.controller.tools.fontItalic
                                                   : bar.controller.tools.fontUnderline

                        Rectangle {
                            anchors.fill: parent
                            radius: Theme.paddingSmall / 2
                            color: parent.on ? Theme.rgba(Theme.highlightColor, 0.35)
                                             : "transparent"
                        }

                        Label {
                            anchors.centerIn: parent
                            text: modelData === "bold" ? "B"
                                : modelData === "italic" ? "I" : "U"
                            font.pixelSize: Theme.fontSizeMedium
                            font.bold: modelData === "bold"
                            font.italic: modelData === "italic"
                            font.underline: modelData === "underline"
                            color: parent.on ? Theme.highlightColor : Theme.primaryColor
                        }

                        MouseArea {
                            anchors.fill: parent
                            onClicked: {
                                if (modelData === "bold")
                                    bar.controller.tools.fontBold = !bar.controller.tools.fontBold
                                else if (modelData === "italic")
                                    bar.controller.tools.fontItalic = !bar.controller.tools.fontItalic
                                else
                                    bar.controller.tools.fontUnderline = !bar.controller.tools.fontUnderline
                            }
                        }
                    }
                }
            }

            Label {
                text: qsTr("Fill")
                font.pixelSize: Theme.fontSizeExtraSmall
                color: Theme.secondaryHighlightColor
            }

            Row {
                spacing: Theme.paddingMedium

                Repeater {
                    model: [ { glyph: "textfilled",  style: 0, name: qsTr("Filled") },
                             { glyph: "textoutline", style: 1, name: qsTr("Outline") },
                             { glyph: "textinline",  style: 2, name: qsTr("Inline") } ]

                    delegate: Item {
                        id: fillButton

                        width: fillLabel.implicitWidth > Theme.itemSizeSmall
                               ? fillLabel.implicitWidth : Theme.itemSizeSmall
                        height: fillColumn.height

                        readonly property bool on: bar.controller.tools.textStyle === modelData.style

                        Column {
                            id: fillColumn
                            width: parent.width
                            spacing: Theme.paddingSmall / 2

                            ToolGlyph {
                                anchors.horizontalCenter: parent.horizontalCenter
                                width: Theme.iconSizeMedium
                                height: width
                                kind: modelData.glyph
                                color: fillButton.on ? Theme.highlightColor : Theme.primaryColor
                            }

                            Label {
                                id: fillLabel
                                anchors.horizontalCenter: parent.horizontalCenter
                                text: modelData.name
                                font.pixelSize: Theme.fontSizeTiny
                                color: fillButton.on ? Theme.highlightColor
                                                     : Theme.secondaryColor
                            }
                        }

                        MouseArea {
                            anchors.fill: parent
                            onClicked: bar.controller.tools.textStyle = modelData.style
                        }
                    }
                }
            }
        }
    }
}
