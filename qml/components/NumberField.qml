import QtQuick 2.0
import Sailfish.Silica 1.0

Item {
    id: field

    property string label: ""
    property real value: 0
    property int decimals: 1

    signal committed(real newValue)

    implicitHeight: Theme.itemSizeExtraSmall * 0.72

    function text() {
        return value.toFixed(field.decimals)
    }

    function commit() {
        var parsed = parseFloat(input.text.replace(",", "."))
        if (!isNaN(parsed) && Math.abs(parsed - field.value) > 1e-6)
            field.committed(parsed)
        else
            input.text = field.text()
        input.focus = false
    }

    onValueChanged: if (!input.activeFocus) input.text = field.text()
    Component.onCompleted: input.text = field.text()

    Label {
        id: caption
        anchors { left: parent.left; verticalCenter: parent.verticalCenter }
        text: field.label
        font.pixelSize: Theme.fontSizeExtraSmall
        color: Theme.secondaryHighlightColor
    }

    Rectangle {
        anchors {
            left: caption.right
            leftMargin: Theme.paddingSmall / 2
            right: parent.right
            top: parent.top
            bottom: parent.bottom
        }
        radius: Theme.paddingSmall / 2
        color: input.activeFocus ? Theme.rgba(Theme.highlightColor, 0.25) : "transparent"
        border.width: 1
        border.color: Theme.rgba(Theme.primaryColor, input.activeFocus ? 0.5 : 0.25)

        TextInput {
            id: input

            anchors {
                fill: parent
                leftMargin: Theme.paddingSmall / 2
                rightMargin: Theme.paddingSmall / 2
            }
            verticalAlignment: TextInput.AlignVCenter
            horizontalAlignment: TextInput.AlignRight
            font.pixelSize: Theme.fontSizeExtraSmall
            color: Theme.primaryColor
            selectionColor: Theme.rgba(Theme.highlightColor, 0.5)
            selectByMouse: true
            clip: true

            inputMethodHints: Qt.ImhFormattedNumbersOnly
            validator: DoubleValidator { notation: DoubleValidator.StandardNotation }

            onAccepted: field.commit()
            onActiveFocusChanged: {
                if (activeFocus)
                    selectAll()
                else
                    field.commit()
            }
        }
    }
}
