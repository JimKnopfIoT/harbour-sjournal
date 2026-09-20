import QtQuick 2.0
import Sailfish.Silica 1.0
import harbour.sjournal 1.0

Page {
    id: page
    allowedOrientations: Orientation.All

    readonly property int index: app.controller.currentPage

    SilicaFlickable {
        anchors.fill: parent
        contentHeight: column.height

        Column {
            id: column
            width: parent.width

            PageHeader {
                title: qsTr("Page setup")
                description: qsTr("Page %1 of %2")
                    .arg(app.controller.currentPage + 1).arg(app.controller.pageCount)
            }

            ComboBox {
                width: parent.width
                label: qsTr("Ruling")

                property var styles: ["plain", "lined", "ruled", "graph", "dotted"]

                currentIndex: Math.max(0, styles.indexOf(
                    app.controller.pageBackgroundStyle(page.index)))

                menu: ContextMenu {
                    MenuItem { text: qsTr("Plain") }
                    MenuItem { text: qsTr("Lined") }
                    MenuItem { text: qsTr("Lined with margin") }
                    MenuItem { text: qsTr("Graph") }
                    MenuItem { text: qsTr("Dotted") }
                }

                onCurrentIndexChanged: app.controller.setPageBackgroundStyle(
                                           page.index, styles[currentIndex])
            }

            SectionHeader { text: qsTr("Size") }

            ComboBox {
                width: parent.width
                label: qsTr("Page size")
                description: qsTr("Screen fills the drawing area; A4 is what a "
                                  + "printer and Xournal++ on the desktop expect.")

                property real screenWidth: 420
                property real screenHeight: Math.round(420 * page.height / Math.max(1, page.width))

                currentIndex: Math.abs(app.controller.pageWidth(page.index) - 595.28) < 1 ? 1 : 0

                menu: ContextMenu {
                    MenuItem { text: qsTr("Screen") }
                    MenuItem { text: qsTr("A4") }
                }

                onCurrentIndexChanged: {
                    if (currentIndex === 1)
                        app.controller.setPageSize(page.index, 595.276, 841.89)
                    else
                        app.controller.setPageSize(page.index, screenWidth, screenHeight)
                }
            }

            SectionHeader { text: qsTr("Pages") }

            Button {
                anchors.horizontalCenter: parent.horizontalCenter
                text: qsTr("Add page")
                onClicked: app.controller.addPage()
            }

            Item { width: 1; height: Theme.paddingMedium }

            Button {
                anchors.horizontalCenter: parent.horizontalCenter
                text: qsTr("Duplicate this page")
                onClicked: app.controller.duplicatePage(page.index)
            }

            Item { width: 1; height: Theme.paddingMedium }

            Button {
                anchors.horizontalCenter: parent.horizontalCenter
                text: qsTr("Delete this page")
                enabled: app.controller.pageCount > 1
                onClicked: remorse.execute(qsTr("Deleting page"), function() {
                    app.controller.removePage(page.index)
                })
            }
        }

        VerticalScrollDecorator { }
    }

    RemorsePopup { id: remorse }
}
