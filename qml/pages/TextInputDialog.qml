import QtQuick 2.0
import Sailfish.Silica 1.0

Dialog {
    id: dialog

    property alias text: input.text

    canAccept: true

    DialogHeader {
        id: header
        acceptText: qsTr("Place")
        cancelText: qsTr("Discard")
    }

    Column {
        anchors {
            top: header.bottom
            left: parent.left
            right: parent.right
        }

        TextArea {
            id: input
            width: parent.width
            label: qsTr("Text")
            placeholderText: qsTr("Type here")
            focus: true
        }

        Label {
            x: Theme.horizontalPageMargin
            width: parent.width - 2 * Theme.horizontalPageMargin
            wrapMode: Text.Wrap
            font.pixelSize: Theme.fontSizeExtraSmall
            color: Theme.secondaryColor
            text: qsTr("Size, weight and fill come from the toolbar. Leave this empty to "
                       + "delete the text you tapped.")
        }
    }
}
