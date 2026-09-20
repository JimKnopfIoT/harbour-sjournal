import QtQuick 2.0
import Sailfish.Silica 1.0
import Qt.labs.folderlistmodel 2.1

Page {
    id: page
    allowedOrientations: Orientation.All

    signal imageSelected(string url)

    property string folder: app.controller.picturesDirectory()
    readonly property string homeDir: app.controller.homeDirectory()

    function goUp() {
        var cut = folder.lastIndexOf("/")
        if (cut > 0)
            folder = folder.substring(0, cut)
    }

    FolderListModel {
        id: folderModel
        folder: "file://" + page.folder
        showDirs: true
        showDirsFirst: true
        showDotAndDotDot: false
        sortField: FolderListModel.Name
        nameFilters: ["*.jpg", "*.jpeg", "*.png", "*.gif", "*.bmp", "*.webp",
                      "*.JPG", "*.JPEG", "*.PNG"]
    }

    SilicaGridView {
        id: grid
        anchors.fill: parent
        cellWidth: Math.floor(width / (page.isPortrait ? 3 : 5))
        cellHeight: cellWidth
        model: folderModel

        PullDownMenu {
            MenuItem {
                text: qsTr("Documents")
                onClicked: page.folder = app.controller.documentsDirectory()
            }
            MenuItem {
                text: qsTr("Downloads")
                onClicked: page.folder = app.controller.downloadsDirectory()
            }
            MenuItem {
                text: qsTr("Screenshots")
                onClicked: page.folder = app.controller.screenshotsDirectory()
            }
            MenuItem {
                text: qsTr("Pictures")
                onClicked: page.folder = app.controller.picturesDirectory()
            }
            MenuItem {
                text: qsTr("Up one level")
                onClicked: page.goUp()
            }
        }

        header: PageHeader {
            title: qsTr("Pick an image")
            description: page.folder.replace(page.homeDir, "~")
        }

        delegate: BackgroundItem {
            width: grid.cellWidth
            height: grid.cellHeight

            onClicked: {
                if (fileIsDir)
                    page.folder = filePath
                else {
                    page.imageSelected("file://" + filePath)
                    pageStack.pop()
                }
            }

            Rectangle {
                anchors {
                    fill: parent
                    margins: Theme.paddingSmall
                }
                color: fileIsDir ? Theme.rgba(Theme.primaryColor, 0.1) : "black"
                radius: Theme.paddingSmall

                Image {
                    anchors.fill: parent
                    anchors.margins: 1
                    visible: !fileIsDir
                    source: fileIsDir ? "" : ("file://" + filePath)
                    fillMode: Image.PreserveAspectCrop
                    asynchronous: true
                    sourceSize.width: grid.cellWidth
                    clip: true
                }

                Label {
                    anchors.centerIn: parent
                    visible: fileIsDir
                    width: parent.width - 2 * Theme.paddingSmall
                    text: fileName
                    horizontalAlignment: Text.AlignHCenter
                    wrapMode: Text.Wrap
                    maximumLineCount: 3
                    elide: Text.ElideRight
                    font.pixelSize: Theme.fontSizeExtraSmall
                    color: Theme.primaryColor
                }
            }
        }

        ViewPlaceholder {
            enabled: folderModel.count === 0
            text: qsTr("Nothing here")
            hintText: qsTr("Pull down to jump to Pictures, Screenshots or Downloads")
        }

        VerticalScrollDecorator { }
    }
}
