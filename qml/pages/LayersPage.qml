import QtQuick 2.0
import Sailfish.Silica 1.0
import harbour.sjournal 1.0

import "../components"

Page {
    id: page
    allowedOrientations: Orientation.All

    property var layers: app.controller.layers

    SilicaListView {
        id: listView
        anchors.fill: parent
        model: page.layers

        header: PageHeader {
            title: qsTr("Layers")
            description: qsTr("%1 of %2 visible on page %3")
                .arg(page.layers.visibleCount)
                .arg(page.layers.count)
                .arg(app.controller.currentPage + 1)
        }

        PullDownMenu {
            MenuItem {
                text: qsTr("Hide all but current")
                onClicked: page.layers.hideAll()
            }
            MenuItem {
                text: qsTr("Show all")
                onClicked: page.layers.showAll()
            }
            MenuItem {
                text: qsTr("Add layer")
                onClicked: page.layers.addLayer()
            }
        }

        delegate: ListItem {
            id: delegateItem
            contentHeight: Theme.itemSizeMedium
            highlighted: down || isCurrent

            menu: ContextMenu {
                MenuItem {
                    text: layerLocked ? qsTr("Unlock") : qsTr("Lock")
                    onClicked: page.layers.toggleLock(index)
                }
                MenuItem {
                    text: app.controller.selection.count === 1
                          ? qsTr("Move selection here")
                          : qsTr("Move %n item(s) here", "", app.controller.selection.count)
                    enabled: app.controller.selection.count > 0 && !layerLocked && !isCurrent
                    onClicked: page.layers.moveSelectionHere(index)
                }
                MenuItem {
                    text: qsTr("Rename")
                    onClicked: {
                        var dialog = pageStack.push(renameDialog, { layerName: layerName })
                        dialog.accepted.connect(function() {
                            page.layers.rename(index, dialog.layerName)
                        })
                    }
                }
                MenuItem {
                    text: qsTr("Show only this")
                    onClicked: page.layers.isolate(index)
                }
                MenuItem {
                    text: qsTr("Move up")
                    enabled: index > 0
                    onClicked: page.layers.raise(index)
                }
                MenuItem {
                    text: qsTr("Move down")
                    enabled: index < page.layers.count - 1
                    onClicked: page.layers.lower(index)
                }
                MenuItem {
                    text: qsTr("Clear")
                    enabled: elementCount > 0
                    onClicked: delegateItem.remorseAction(qsTr("Clearing layer"), function() {
                        page.layers.clear(index)
                    })
                }
                MenuItem {
                    text: qsTr("Delete")
                    enabled: page.layers.count > 1
                    onClicked: delegateItem.remorseAction(qsTr("Deleting layer"), function() {
                        page.layers.remove(index)
                    })
                }
            }

            onClicked: page.layers.select(index)

            Rectangle {
                id: marker
                anchors {
                    left: parent.left
                    leftMargin: Theme.horizontalPageMargin
                    verticalCenter: parent.verticalCenter
                }
                width: Theme.paddingSmall
                height: Theme.itemSizeSmall
                radius: width / 2
                color: isCurrent ? Theme.highlightColor : "transparent"
            }

            Column {
                anchors {
                    left: marker.right
                    leftMargin: Theme.paddingMedium
                    right: visibleSwitch.left
                    rightMargin: Theme.paddingMedium
                    verticalCenter: parent.verticalCenter
                }

                Row {
                    width: parent.width
                    spacing: Theme.paddingSmall

                    ToolGlyph {
                        anchors.verticalCenter: parent.verticalCenter
                        visible: layerLocked
                        width: Theme.iconSizeExtraSmall
                        height: width
                        kind: "lock"
                        color: Theme.highlightColor
                    }

                    Label {
                        width: parent.width - (layerLocked
                                               ? Theme.iconSizeExtraSmall + Theme.paddingSmall : 0)
                        text: layerName
                        truncationMode: TruncationMode.Fade
                        color: layerVisible
                               ? (delegateItem.highlighted ? Theme.highlightColor : Theme.primaryColor)
                               : Theme.secondaryColor
                    }
                }
                Label {
                    width: parent.width
                    text: {
                        var what = elementCount === 1
                                ? qsTr("1 element") : qsTr("%1 elements").arg(elementCount)
                        if (layerLocked)
                            return what + " · " + qsTr("locked")
                        return isCurrent ? what + " · " + qsTr("drawing here") : what
                    }
                    font.pixelSize: Theme.fontSizeExtraSmall
                    color: Theme.secondaryColor
                }
            }

            Switch {
                id: visibleSwitch
                anchors {
                    right: parent.right
                    rightMargin: Theme.horizontalPageMargin
                    verticalCenter: parent.verticalCenter
                }
                checked: layerVisible
                automaticCheck: false
                onClicked: page.layers.toggle(index)
            }
        }

        VerticalScrollDecorator { }
    }

    Component {
        id: renameDialog

        Dialog {
            id: renameLayerDialog
            property string layerName: ""

            canAccept: field.text.trim().length > 0
            onAccepted: layerName = field.text.trim()

            Column {
                width: parent.width
                DialogHeader { acceptText: qsTr("Rename") }
                TextField {
                    id: field
                    width: parent.width
                    text: renameLayerDialog.layerName
                    label: qsTr("Layer name")
                    focus: true
                    EnterKey.iconSource: "image://theme/icon-m-enter-accept"
                    EnterKey.onClicked: renameLayerDialog.accept()
                }
            }
        }
    }
}
