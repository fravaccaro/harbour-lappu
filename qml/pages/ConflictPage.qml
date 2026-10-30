import Opal.LinkHandler 1.0 as L
import QtQuick 2.0
import Sailfish.Silica 1.0
import "../components"

Page {
    id: page

    // The pair being settled, or null once the last one has been answered.
    // Reading conflictCount is what brings the pair back after every answer,
    // since the session reports both through the same signal.
    property var conflict: Lappu.conflictCount > 0 ? Lappu.conflicts()[0] : null

    allowedOrientations: Orientation.All

    SilicaFlickable {
        anchors.fill: parent
        contentHeight: column.height + Theme.paddingLarge

        // Three answers rather than two is why this is a page: a dialog has
        // room for one of each.
        PullDownMenu {
            visible: page.conflict !== null

            MenuItem {
                text: qsTr("Keep the server's version")
                onClicked: Lappu.resolveKeepTheirs(page.conflict.noteId)
            }

            MenuItem {
                text: qsTr("Keep my version")
                onClicked: Lappu.resolveKeepMine(page.conflict.noteId)
            }

            MenuItem {
                text: qsTr("Keep both")
                onClicked: Lappu.resolveKeepBoth(page.conflict.noteId)
            }
        }

        Column {
            id: column

            width: page.width
            spacing: Theme.paddingMedium

            PageHeader {
                title: qsTr("Which version?")
                description: page.conflict ? page.conflict.theirsTitle : ""
            }

            Label {
                x: Theme.horizontalPageMargin
                width: parent.width - 2 * Theme.horizontalPageMargin
                visible: page.conflict !== null
                wrapMode: Text.WordWrap
                color: Theme.secondaryColor
                font.pixelSize: Theme.fontSizeSmall
                text: qsTr("This note was changed here and on the server. Pull down to keep one version, the other, or both.")
            }

            Label {
                x: Theme.horizontalPageMargin
                width: parent.width - 2 * Theme.horizontalPageMargin
                visible: page.conflict !== null
                         && (page.conflict.categoryDiffers || page.conflict.favoriteDiffers)
                wrapMode: Text.WordWrap
                color: Theme.secondaryColor
                font.pixelSize: Theme.fontSizeExtraSmall
                text: qsTr("The category or the star differ between the two as well.")
            }

            Label {
                x: Theme.horizontalPageMargin
                width: parent.width - 2 * Theme.horizontalPageMargin
                visible: Lappu.conflictCount > 1
                wrapMode: Text.WordWrap
                color: Theme.secondaryColor
                font.pixelSize: Theme.fontSizeExtraSmall
                text: qsTr("%n more note(s) wait after this one.", "", Lappu.conflictCount - 1)
            }

            ConflictVersion {
                width: parent.width
                visible: page.conflict !== null
                label: qsTr("On this phone")
                content: page.conflict ? page.conflict.mineContent : ""
                modified: page.conflict ? page.conflict.mineModified : 0
                // Either version may carry links that came from the server, so
                // a link is offered rather than opened, as everywhere else.
                onLinkActivated: L.LinkHandler.openOrCopyUrl(link)
            }

            Separator {
                width: parent.width
                visible: page.conflict !== null
                color: Theme.secondaryColor
                horizontalAlignment: Qt.AlignHCenter
            }

            ConflictVersion {
                width: parent.width
                visible: page.conflict !== null
                label: qsTr("On the server")
                content: page.conflict ? page.conflict.theirsContent : ""
                modified: page.conflict ? page.conflict.theirsModified : 0
                onLinkActivated: L.LinkHandler.openOrCopyUrl(link)
            }
        }

        // The questions are not kept anywhere: an app closed before they were
        // answered has none to show until a refresh meets the same notes again.
        ViewPlaceholder {
            enabled: page.conflict === null
            text: qsTr("Nothing to settle")
            hintText: qsTr("A note that changed on both sides is found again by the next refresh")
        }

        VerticalScrollDecorator {
        }
    }
}
