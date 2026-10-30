import QtQuick 2.0
import Sailfish.Silica 1.0

CoverBackground {
    id: cover

    readonly property var win: __silica_applicationwindow_instance
    readonly property var store: win ? win.notes : null
    readonly property bool pinned: store !== null && store.pinnedNoteId > 0

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
            text: {
                if (cover.pinned)
                    return store.pinnedTitle
                return store ? store.listTitle : qsTr("Lappu")
            }
        }
    }

    ListView {
        id: noteList

        clip: true
        interactive: false
        spacing: Theme.paddingSmall
        visible: !cover.pinned
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

    Label {
        id: pinnedBody

        visible: cover.pinned
        wrapMode: Text.WordWrap
        font.pixelSize: Theme.fontSizeExtraSmall
        color: Theme.primaryColor
        text: cover.pinned ? store.pinnedText : ""

        anchors {
            top: heading.bottom
            topMargin: Theme.paddingLarge
            left: parent.left
            right: parent.right
            leftMargin: Theme.paddingLarge
            rightMargin: Theme.paddingLarge
        }
    }

    OpacityRampEffect {
        sourceItem: noteList
        direction: 2
        offset: 0.65
        slope: 2
        enabled: noteList.visible && noteList.contentHeight > noteList.height
    }

    // The note runs under the cover actions, so it fades out rather than
    // ending on a hard line of half-cut text.
    OpacityRampEffect {
        sourceItem: pinnedBody
        direction: 2
        offset: 0.6
        slope: 2
        enabled: pinnedBody.visible
                 && pinnedBody.height > cover.height - pinnedBody.y - Theme.itemSizeSmall
    }

    Label {
        horizontalAlignment: Text.AlignHCenter
        wrapMode: Text.WordWrap
        visible: !cover.pinned && (!store || noteList.count === 0)
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
        // Neither action has anywhere to go before anyone has signed in.
        enabled: Lappu.configured

        CoverAction {
            iconSource: "image://theme/icon-cover-new"
            onTriggered: win.openNewNote()
        }

        CoverAction {
            iconSource: "image://theme/icon-cover-sync"
            onTriggered: win.syncNotes()
        }
    }
}
