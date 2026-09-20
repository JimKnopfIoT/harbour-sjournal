import QtQuick 2.0
import Sailfish.Silica 1.0
import harbour.sjournal 1.0

Page {
    id: page
    objectName: "notebooksPage"

    allowedOrientations: Orientation.All

    onStatusChanged: {
        if (status === PageStatus.Active)
            notebooks.refresh()
    }

    NotebookModel {
        id: notebooks
    }

    Timer {
        id: openCreatedNote
        interval: 60
        onTriggered: pageStack.replace(Qt.resolvedUrl("NotePage.qml"))
    }

    SilicaListView {
        id: listView
        anchors.fill: parent
        model: notebooks

        PullDownMenu {
            MenuItem {
                text: qsTr("About")
                onClicked: pageStack.push(Qt.resolvedUrl("AboutPage.qml"))
            }
            MenuItem {
                text: qsTr("New note")
                onClicked: pageStack.push(newNoteDialog)
            }
        }

        header: Column {
            width: listView.width

            PageHeader {
                title: qsTr("Notes")
                description: notebooks.count === 1
                             ? qsTr("1 note") : qsTr("%1 notes").arg(notebooks.count)
            }

            SearchField {
                width: parent.width
                placeholderText: qsTr("Search title and text")
                onTextChanged: notebooks.filter = text
                EnterKey.onClicked: focus = false
            }
        }

        delegate: ListItem {
            id: item
            contentHeight: Theme.itemSizeExtraLarge

            menu: ContextMenu {
                MenuItem {
                    text: qsTr("Rename")
                    onClicked: {
                        var dialog = pageStack.push(renameDialog, { noteTitle: title })
                        dialog.accepted.connect(function() {
                            notebooks.rename(index, dialog.noteTitle)
                        })
                    }
                }
                MenuItem {
                    text: qsTr("Delete")
                    onClicked: item.remorseDelete(function() { notebooks.remove(index) })
                }
            }

            onClicked: {
                if (app.controller.open(filePath)) {
                    app.noteOpen = true
                    pageStack.push(Qt.resolvedUrl("NotePage.qml"))
                }
            }

            Row {
                anchors {
                    left: parent.left
                    right: parent.right
                    leftMargin: Theme.horizontalPageMargin
                    rightMargin: Theme.horizontalPageMargin
                    verticalCenter: parent.verticalCenter
                }
                spacing: Theme.paddingLarge

                Rectangle {
                    id: thumb
                    width: Theme.itemSizeMedium * 0.72
                    height: Theme.itemSizeExtraLarge - Theme.paddingMedium * 2
                    anchors.verticalCenter: parent.verticalCenter
                    color: "white"
                    radius: Theme.paddingSmall / 2
                    clip: true

                    Image {
                        anchors.fill: parent
                        fillMode: Image.PreserveAspectCrop
                        asynchronous: true
                        cache: false
                        source: preview
                        sourceSize.width: thumb.width * 2
                    }

                    Rectangle {
                        anchors.fill: parent
                        color: "transparent"
                        radius: parent.radius
                        border.width: 1
                        border.color: Theme.rgba(Theme.primaryColor, 0.2)
                    }
                }

                Column {
                    width: parent.width - thumb.width - Theme.paddingLarge
                    anchors.verticalCenter: parent.verticalCenter
                    spacing: Theme.paddingSmall / 2

                    Label {
                        width: parent.width
                        text: title
                        truncationMode: TruncationMode.Fade
                        color: item.highlighted ? Theme.highlightColor : Theme.primaryColor
                    }
                    Label {
                        width: parent.width
                        text: modifiedText
                        font.pixelSize: Theme.fontSizeExtraSmall
                        color: item.highlighted ? Theme.secondaryHighlightColor : Theme.secondaryColor
                    }
                    Label {
                        width: parent.width
                        text: (pageCount === 1 ? qsTr("1 page") : qsTr("%1 pages").arg(pageCount))
                              + " · " + sizeText
                        font.pixelSize: Theme.fontSizeExtraSmall
                        color: item.highlighted ? Theme.secondaryHighlightColor : Theme.secondaryColor
                    }
                }
            }
        }

        ViewPlaceholder {
            enabled: notebooks.count === 0
            text: notebooks.filter.length > 0 ? qsTr("Nothing found") : qsTr("No notes yet")
            hintText: notebooks.filter.length > 0
                      ? qsTr("No note matches this search")
                      : qsTr("Pull down to start a new note")
        }

        VerticalScrollDecorator { }
    }

    Component {
        id: newNoteDialog

        Dialog {
            id: createDialog

            canAccept: titleField.text.trim().length > 0

            onAccepted: {
                var w = 420
                var h = Math.round(w * page.height / Math.max(1, page.width))
                if (app.controller.createNote(titleField.text, w, h)) {
                    app.controller.save()
                    app.noteOpen = true
                    openCreatedNote.start()
                }
            }

            Column {
                width: parent.width

                DialogHeader { acceptText: qsTr("Create") }

                TextField {
                    id: titleField
                    width: parent.width
                    placeholderText: qsTr("Note title")
                    label: qsTr("Note title")
                    focus: true
                    EnterKey.iconSource: "image://theme/icon-m-enter-accept"
                    EnterKey.onClicked: createDialog.accept()
                }
            }
        }
    }

    Component {
        id: renameDialog

        Dialog {
            id: renameNoteDialog
            property string noteTitle: ""

            canAccept: field.text.trim().length > 0
            onAccepted: noteTitle = field.text.trim()

            Column {
                width: parent.width

                DialogHeader { acceptText: qsTr("Rename") }

                TextField {
                    id: field
                    width: parent.width
                    text: renameNoteDialog.noteTitle
                    label: qsTr("Note title")
                    focus: true
                    EnterKey.iconSource: "image://theme/icon-m-enter-accept"
                    EnterKey.onClicked: renameNoteDialog.accept()
                }
            }
        }
    }
}
