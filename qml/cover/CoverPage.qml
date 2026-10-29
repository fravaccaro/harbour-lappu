import QtQuick 2.0
import Sailfish.Silica 1.0

CoverBackground {
    clip: true

    Column {
        id: heading

        spacing: Theme.paddingSmall

        anchors {
            top: parent.top
            left: parent.left
            right: parent.right
            margins: Theme.paddingLarge
        }

        Label {
            width: parent.width
            truncationMode: TruncationMode.Fade
            color: Theme.secondaryHighlightColor
            font.pixelSize: Theme.fontSizeMedium
            text: qsTr("Muistio")
        }

    }

    Label {
        horizontalAlignment: Text.AlignHCenter
        wrapMode: Text.WordWrap
        color: Theme.secondaryColor
        font.pixelSize: Theme.fontSizeExtraSmall
        text: Muistio.configured ? qsTr("No notes") : qsTr("Not signed in")

        anchors {
            left: parent.left
            right: parent.right
            verticalCenter: parent.verticalCenter
            margins: Theme.paddingMedium
        }

    }

    CoverActionList {
        enabled: Muistio.configured

        CoverAction {
            iconSource: "image://theme/icon-cover-sync"
            onTriggered: Muistio.sync()
        }

    }

}
