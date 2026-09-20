import QtQuick 2.0
import Sailfish.Silica 1.0

// White inside a thin black outline, so it shows on any colour underneath.
Rectangle {
    width: Theme.iconSizeSmall
    height: width
    radius: width / 2
    color: "transparent"
    border.color: "white"
    border.width: 3

    Rectangle {
        anchors.fill: parent
        anchors.margins: -1
        radius: width / 2
        color: "transparent"
        border.color: "black"
        border.width: 1
    }
}
