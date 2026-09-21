import QtQuick 2.0
import Sailfish.Silica 1.0

Page {
    allowedOrientations: Orientation.All

    SilicaFlickable {
        anchors.fill: parent
        contentHeight: column.height

        Column {
            id: column
            width: parent.width
            spacing: Theme.paddingMedium

            PageHeader { title: qsTr("SJournal") }

            Label {
                x: Theme.horizontalPageMargin
                width: parent.width - 2 * Theme.horizontalPageMargin
                wrapMode: Text.Wrap
                text: qsTr("Handwritten notes and sketches. Notes are stored as vector "
                           + "strokes in the .xopp format, so the same file opens in "
                           + "Xournal++ on a desktop.")
            }

            ComboBox {
                readonly property var codes: ["", "en", "de"]

                label: qsTr("Language")
                currentIndex: Math.max(0, codes.indexOf(languageSetting.language))
                description: languageSetting.restartNeeded
                             ? qsTr("Takes effect the next time SJournal starts.")
                             : ""
                menu: ContextMenu {
                    MenuItem { text: qsTr("System default") }
                    MenuItem { text: "English" }
                    MenuItem { text: "Deutsch" }
                }
                onCurrentIndexChanged: languageSetting.language = codes[currentIndex]
            }

            SectionHeader { text: qsTr("Where notes are kept") }

            Label {
                x: Theme.horizontalPageMargin
                width: parent.width - 2 * Theme.horizontalPageMargin
                wrapMode: Text.Wrap
                font.pixelSize: Theme.fontSizeSmall
                color: Theme.secondaryColor
                text: qsTr("Documents/SJournal — plain .xopp files, visible to the file "
                           + "manager and to any sync tool you already run.")
            }

            SectionHeader { text: qsTr("Stylus") }

            Label {
                x: Theme.horizontalPageMargin
                width: parent.width - 2 * Theme.horizontalPageMargin
                wrapMode: Text.Wrap
                font.pixelSize: Theme.fontSizeSmall
                color: Theme.secondaryColor
                text: qsTr("The screen is a plain capacitive panel with no digitiser, so "
                           + "active pens that rely on one — Microsoft Pen Protocol, Wacom "
                           + "EMR, Apple Pencil — cannot work. A capacitive stylus works, "
                           + "and stroke width comes from speed instead of pressure.")
            }
        }

        VerticalScrollDecorator { }
    }
}
