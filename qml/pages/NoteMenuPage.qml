import QtQuick 2.0
import Sailfish.Silica 1.0
import harbour.sjournal 1.0

Page {
    id: page
    allowedOrientations: Orientation.All

    function labelFor(action) {
        switch (action) {
        case "copy":
            return app.controller.selection.count > 0
                    ? qsTr("Copy %n item(s)", "", app.controller.selection.count)
                    : qsTr("Copy — mark an area first")
        case "cut":
            return qsTr("Cut %n item(s)", "", app.controller.selection.count)
        case "delete":
            return qsTr("Delete %n item(s)", "", app.controller.selection.count)
        case "paste":
            return app.controller.selection.clipboardCount > 0
                    ? qsTr("Paste %n item(s)", "", app.controller.selection.clipboardCount)
                    : qsTr("Paste — nothing copied")
        case "openimage":
            return qsTr("Insert image…")
        case "trace":
            return app.controller.selection.hasImage
                    ? qsTr("Trace the selected area")
                    : qsTr("Trace — mark part of a photo")
        case "screenshot":
            return app.controller.latestScreenshotName().length > 0
                    ? qsTr("Sketch on last screenshot")
                    : qsTr("No screenshot to sketch on")
        case "save":
            return app.controller.modified ? qsTr("Save now") : qsTr("Saved")
        case "saveas":    return qsTr("Save as…")
        case "export":    return qsTr("Export…")
        case "addpage":   return qsTr("Add page")
        case "pagesetup": return qsTr("Page setup")
        case "pen":       return qsTr("Pen settings")
        case "fill":      return qsTr("Fill and outline")
        case "about":     return qsTr("About")
        default:          return qsTr("Close note")
        }
    }

    function enabledFor(action) {
        switch (action) {
        case "copy":
        case "cut":
        case "delete":     return app.controller.selection.count > 0
        case "paste":      return app.controller.selection.clipboardCount > 0
        case "trace":      return app.controller.selection.hasImage
        case "screenshot": return app.controller.latestScreenshotName().length > 0
        case "save":       return app.controller.modified
        default:           return true
        }
    }

    SilicaListView {
        anchors.fill: parent

        header: PageHeader {
            title: app.controller.title
            description: qsTr("Page %1 of %2")
                .arg(app.controller.currentPage + 1).arg(app.controller.pageCount)
        }

        model: ListModel {
            ListElement { action: "copy";       section: "edit" }
            ListElement { action: "cut";        section: "edit" }
            ListElement { action: "paste";      section: "edit" }
            ListElement { action: "delete";     section: "edit" }
            ListElement { action: "openimage";  section: "insert" }
            ListElement { action: "screenshot"; section: "insert" }
            ListElement { action: "trace";      section: "insert" }
            ListElement { action: "addpage";    section: "page" }
            ListElement { action: "pagesetup";  section: "page" }
            ListElement { action: "pen";        section: "page" }
            ListElement { action: "fill";       section: "page" }
            ListElement { action: "save";       section: "note" }
            ListElement { action: "saveas";     section: "note" }
            ListElement { action: "export";     section: "note" }
            ListElement { action: "about";      section: "note" }
            ListElement { action: "close";      section: "note" }
        }

        section.property: "section"
        section.delegate: SectionHeader {
            text: section === "edit" ? qsTr("Edit")
                : section === "insert" ? qsTr("Insert")
                : section === "page" ? qsTr("Page")
                : qsTr("Note")
        }

        delegate: BackgroundItem {
            id: item
            width: ListView.view.width
            enabled: page.enabledFor(action)

            Label {
                anchors {
                    left: parent.left
                    right: parent.right
                    leftMargin: Theme.horizontalPageMargin
                    rightMargin: Theme.horizontalPageMargin
                    verticalCenter: parent.verticalCenter
                }
                text: page.labelFor(action)
                truncationMode: TruncationMode.Fade
                color: !item.enabled ? Theme.secondaryColor
                     : item.highlighted ? Theme.highlightColor
                                        : Theme.primaryColor
            }

            onClicked: {
                switch (action) {
                case "copy":
                    app.controller.selection.copy()
                    pageStack.pop()
                    break
                case "cut":
                    app.controller.selection.cut()
                    pageStack.pop()
                    break
                case "paste":
                    app.controller.selection.paste()
                    pageStack.pop()
                    break
                case "delete":
                    app.controller.selection.remove()
                    pageStack.pop()
                    break
                case "openimage":
                    pageStack.replace(Qt.resolvedUrl("ImageBrowserPage.qml"))
                    break
                case "screenshot":
                    if (app.controller.insertLatestScreenshot())
                        pageStack.pop()
                    break
                case "trace":
                    pageStack.replace(Qt.resolvedUrl("TracePage.qml"))
                    break
                case "saveas": {
                    var namer = pageStack.push(Qt.resolvedUrl("TextInputDialog.qml"),
                                               { text: app.controller.title })
                    namer.accepted.connect(function() {
                        var wanted = namer.text.trim()
                        if (wanted.length === 0)
                            return
                        if (!wanted.match(/\.xopp$/i))
                            wanted += ".xopp"
                        app.controller.saveAs(app.controller.notesDirectory()
                                              + "/" + wanted)
                    })
                    break
                }
                case "addpage":
                    app.controller.addPage()
                    pageStack.pop()
                    break
                case "pagesetup":
                    pageStack.replace(Qt.resolvedUrl("PageSetupPage.qml"))
                    break
                case "pen":
                    pageStack.replace(Qt.resolvedUrl("PenPage.qml"))
                    break
                case "fill":
                    pageStack.replace(Qt.resolvedUrl("FillStrokePage.qml"))
                    break
                case "save":
                    app.controller.save()
                    pageStack.pop()
                    break
                case "export":
                    pageStack.replace(Qt.resolvedUrl("ExportPage.qml"))
                    break
                case "about":
                    pageStack.replace(Qt.resolvedUrl("AboutPage.qml"))
                    break
                default:
                    if (app.controller.modified)
                        app.controller.save()
                    app.noteOpen = false
                    pageStack.pop(pageStack.find(function(p) {
                        return p.objectName === "notebooksPage"
                    }))
                    break
                }
            }
        }

        VerticalScrollDecorator { }
    }
}
