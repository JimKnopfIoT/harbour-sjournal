import QtQuick 2.0
import Sailfish.Silica 1.0
import harbour.sjournal 1.0

Page {
    id: page
    allowedOrientations: Orientation.All

    property string lastResult: ""

    SilicaFlickable {
        anchors.fill: parent
        contentHeight: column.height

        Column {
            id: column
            width: parent.width

            PageHeader {
                title: qsTr("Export")
                description: app.controller.title
            }

            Label {
                x: Theme.horizontalPageMargin
                width: parent.width - 2 * Theme.horizontalPageMargin
                wrapMode: Text.Wrap
                font.pixelSize: Theme.fontSizeSmall
                color: Theme.secondaryColor
                text: qsTr("SVG keeps every stroke as vector geometry and every layer as a "
                           + "layer, so the drawing stays editable in Inkscape. Straightened "
                           + "shapes are exported as real rectangles and ellipses.")
            }

            TextSwitch {
                id: transparentSwitch
                text: qsTr("Transparent background")
                description: qsTr("Leaves out the page colour and its ruling, so the export "
                                  + "is the drawing alone and can be laid over something else.")
            }

            Item { width: 1; height: Theme.paddingMedium }

            Button {
                anchors.horizontalCenter: parent.horizontalCenter
                text: qsTr("This page as SVG")
                onClicked: {
                    var path = app.controller.exportSvg(app.controller.currentPage, "",
                                                       !transparentSwitch.checked)
                    page.lastResult = path.length > 0
                            ? qsTr("Written to %1").arg(path)
                            : qsTr("Export failed: %1").arg(app.controller.errorString)
                }
            }

            Item { width: 1; height: Theme.paddingMedium }

            Button {
                anchors.horizontalCenter: parent.horizontalCenter
                visible: app.controller.pageCount > 1
                text: qsTr("All pages as SVG")
                onClicked: {
                    var paths = app.controller.exportSvgAllPages("", !transparentSwitch.checked)
                    page.lastResult = paths.length > 0
                            ? qsTr("Wrote %1 files to %2")
                                .arg(paths.length)
                                .arg(paths[0].substring(0, paths[0].lastIndexOf("/")))
                            : qsTr("Export failed: %1").arg(app.controller.errorString)
                }
            }

            Item { width: 1; height: Theme.paddingMedium }

            Repeater {
                model: app.controller.exportImageFormats()

                delegate: Button {
                    anchors.horizontalCenter: parent.horizontalCenter
                    text: qsTr("This page as %1").arg(modelData.toUpperCase())
                    onClicked: {
                        var path = app.controller.exportImage(app.controller.currentPage,
                                                              modelData, 2048, "",
                                                              !transparentSwitch.checked, 92)
                        page.lastResult = path.length > 0
                                ? qsTr("Written to %1").arg(path)
                                : qsTr("Export failed: %1").arg(app.controller.errorString)
                    }
                }
            }

            Item { width: 1; height: Theme.paddingMedium }

            Label {
                x: Theme.horizontalPageMargin
                width: parent.width - 2 * Theme.horizontalPageMargin
                text: qsTr("PDF keeps the strokes as vectors, one page per note page.")
                wrapMode: Text.Wrap
                font.pixelSize: Theme.fontSizeExtraSmall
                color: Theme.secondaryColor
            }

            Button {
                anchors.horizontalCenter: parent.horizontalCenter
                text: qsTr("All pages as PDF")
                onClicked: {
                    var path = app.controller.exportPdf("", !transparentSwitch.checked)
                    page.lastResult = path.length > 0
                            ? qsTr("Written to %1").arg(path)
                            : qsTr("Export failed: %1").arg(app.controller.errorString)
                }
            }

            Item { width: 1; height: Theme.paddingLarge }

            Label {
                x: Theme.horizontalPageMargin
                width: parent.width - 2 * Theme.horizontalPageMargin
                visible: page.lastResult.length > 0
                text: page.lastResult
                wrapMode: Text.Wrap
                font.pixelSize: Theme.fontSizeExtraSmall
                color: Theme.highlightColor
            }
        }

        VerticalScrollDecorator { }
    }
}
