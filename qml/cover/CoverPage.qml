import QtQuick 2.0
import Sailfish.Silica 1.0

CoverBackground {
    id: cover

    readonly property var win: __silica_applicationwindow_instance
    readonly property var store: win ? win.notes : null
    // A note still waiting to be sent carries a negative id, and is as pinnable
    // as any other: only 0 means nothing is pinned.
    readonly property bool pinned: Lappu.configured && store !== null && store.pinnedNoteId !== 0

    clip: true

    HighlightImage {
        source: Qt.resolvedUrl("cover-note.png")
        color: Theme.primaryColor
        // The sheet is taller than it is wide, so unlike the sibling apps'
        // glyphs its height is what decides how much of the cover it takes.
        sourceSize.height: cover.height * 0.6
        // The same alpha reads much heavier as a dark glyph on a light ambience.
        opacity: Theme.colorScheme === Theme.LightOnDark ? 0.13 : 0.08

        // Only the foot and the outer edge are cut off: the folded corner is
        // what makes the shape readable, so it stays whole in the corner.
        anchors {
            right: parent.right
            rightMargin: -width * 0.08
            bottom: parent.bottom
            bottomMargin: -height * 0.04
        }
    }

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
            // The store outlives a sign-out, so the name stands in for its
            // filter title until there is an account to filter.
            text: {
                if (!Lappu.configured || !store)
                    return qsTr("Lappu")
                return cover.pinned ? store.pinnedTitle : store.listTitle
            }
        }
    }

    ListView {
        id: noteList

        clip: true
        interactive: false
        spacing: Theme.paddingSmall
        visible: !cover.pinned
        model: Lappu.configured && store ? store.visibleNotes : null

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
        visible: !cover.pinned && (!Lappu.configured || noteList.count === 0)
        color: Theme.secondaryColor
        font.pixelSize: Theme.fontSizeExtraSmall
        text: !Lappu.configured ? qsTr("Not signed in") : qsTr("No notes")

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
