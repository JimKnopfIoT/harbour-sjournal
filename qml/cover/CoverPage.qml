import QtQuick 2.0
import Sailfish.Silica 1.0

CoverBackground {
    id: cover

    Column {
        anchors {
            left: parent.left
            right: parent.right
            verticalCenter: parent.verticalCenter
            margins: Theme.paddingLarge
        }
        spacing: Theme.paddingMedium

        Label {
            width: parent.width
            text: app.noteOpen && app.controller.title.length > 0
                  ? app.controller.title
                  : "SJournal"
            font.pixelSize: Theme.fontSizeLarge
            color: Theme.highlightColor
            wrapMode: Text.Wrap
            maximumLineCount: 3
            elide: Text.ElideRight
        }

        Label {
            visible: app.noteOpen
            width: parent.width
            text: qsTr("%n page(s)", "", app.controller.pageCount)
            font.pixelSize: Theme.fontSizeSmall
            color: Theme.secondaryColor
        }

        Label {
            visible: app.noteOpen && app.controller.modified
            width: parent.width
            text: qsTr("Unsaved changes")
            font.pixelSize: Theme.fontSizeSmall
            color: Theme.secondaryHighlightColor
        }
    }
}
