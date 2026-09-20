import QtQuick 2.0
import Sailfish.Silica 1.0
import harbour.sjournal 1.0

import "pages"

ApplicationWindow {
    id: app

    property alias controller: documentController
    property bool noteOpen: false

    DocumentController {
        id: documentController
    }

    initialPage: Component { NotebooksPage { } }
    cover: Qt.resolvedUrl("cover/CoverPage.qml")
    allowedOrientations: defaultAllowedOrientations
}
