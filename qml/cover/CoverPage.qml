import QtQuick 2.0
import Sailfish.Silica 1.0

CoverBackground {
    id: cover

    readonly property var win: __silica_applicationwindow_instance
    readonly property var store: win ? win.notes : null

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
            text: store ? store.listTitle : qsTr("Muistio")
        }
    }

    ListView {
        id: noteList

        clip: true
        interactive: false
        spacing: Theme.paddingSmall
        model: store ? store.visibleNotes : null

        anchors {
            top: heading.bottom
            topMargin: Theme.paddingLarge
            left: parent.left
            right: parent.right
            leftMargin: Theme.paddingLarge
            rightMargin: Theme.paddingLarge
            bottom: parent.bottom
            bottomMargin: Theme.itemSizeSmall
        }

        delegate: Label {
            width: noteList.width
            truncationMode: TruncationMode.Fade
            font.pixelSize: Theme.fontSizeSmall
            color: Theme.primaryColor
            text: title
        }
    }

    OpacityRampEffect {
        sourceItem: noteList
        direction: 2
        offset: 0.65
        slope: 2
        enabled: noteList.contentHeight > noteList.height
    }

    Label {
        horizontalAlignment: Text.AlignHCenter
        wrapMode: Text.WordWrap
        visible: !store || noteList.count === 0
        color: Theme.secondaryColor
        font.pixelSize: Theme.fontSizeExtraSmall
        text: qsTr("No notes")

        anchors {
            left: parent.left
            right: parent.right
            verticalCenter: parent.verticalCenter
            margins: Theme.paddingMedium
        }
    }

    CoverActionList {
        CoverAction {
            iconSource: "image://theme/icon-cover-new"
            onTriggered: win.openNewNote()
        }

        CoverAction {
            iconSource: "image://theme/icon-cover-sync"
            onTriggered: win.mockRefresh()
        }
    }
}
