import QtQuick 2.0
import Sailfish.Silica 1.0
import harbour.sjournal 1.0

Page {
    id: page
    allowedOrientations: Orientation.All

    property string result: ""

    SilicaFlickable {
        anchors.fill: parent
        contentHeight: column.height

        Column {
            id: column
            width: parent.width

            PageHeader { title: qsTr("Trace") }

            Label {
                x: Theme.horizontalPageMargin
                width: parent.width - 2 * Theme.horizontalPageMargin
                wrapMode: Text.Wrap
                font.pixelSize: Theme.fontSizeSmall
                color: Theme.secondaryColor
                text: qsTr("Everything darker than the threshold counts as ink. Start there: "
                           + "on a photo, a low threshold picks out only the darkest lines, "
                           + "a high one picks up the shading as well.")
            }

            Slider {
                id: thresholdSlider
                width: parent.width
                minimumValue: 10
                maximumValue: 245
                stepSize: 5
                value: 128
                label: qsTr("Threshold")
                valueText: Math.round(value)
            }

            TextSwitch {
                id: invertSwitch
                text: qsTr("Trace the light parts")
                description: qsTr("For a light drawing on a dark ground.")
            }

            TextSwitch {
                id: fillSwitch
                text: qsTr("Fill the shapes")
                description: qsTr("Off gives outlines, which is what tracing over a photo is "
                                  + "usually for. On fills the traced areas — but a hole in a "
                                  + "shape becomes its own outline, not a hole.")
            }

            Slider {
                id: speckSlider
                width: parent.width
                minimumValue: 0
                maximumValue: 40
                stepSize: 1
                value: 4
                label: qsTr("Ignore specks up to")
                valueText: value === 0 ? qsTr("nothing") : qsTr("%1 px").arg(Math.round(value))
            }

            Item { width: 1; height: Theme.paddingLarge }

            Button {
                anchors.horizontalCenter: parent.horizontalCenter
                text: qsTr("Trace")
                enabled: app.controller.selection.hasImage
                onClicked: {
                    var ok = app.controller.traceSelection(Math.round(thresholdSlider.value),
                                                           invertSwitch.checked,
                                                           fillSwitch.checked,
                                                           Math.round(speckSlider.value))
                    page.result = ok
                            ? qsTr("Traced onto a new layer. Adjust and trace again if it is "
                                   + "too coarse — each run makes its own layer.")
                            : qsTr("Nothing traced: %1").arg(app.controller.errorString)
                }
            }

            Item { width: 1; height: Theme.paddingMedium }

            Label {
                x: Theme.horizontalPageMargin
                width: parent.width - 2 * Theme.horizontalPageMargin
                visible: page.result.length > 0
                text: page.result
                wrapMode: Text.Wrap
                font.pixelSize: Theme.fontSizeExtraSmall
                color: Theme.highlightColor
            }
        }

        VerticalScrollDecorator { }
    }
}
