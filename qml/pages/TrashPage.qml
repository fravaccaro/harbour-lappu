import QtQuick 2.0
import Sailfish.Silica 1.0

Page {
    id: page

    // The trash is read when the page opens and again whenever a note leaves
    // it: nothing of it is kept on the device, so there is nothing to show
    // until the server has been asked.
    property var trashed: []

    function restore(note) {
        Lappu.restoreNote(note.originalLocation, note.deleted);
    }

    function deletedAt(stamp) {
        return Qt.formatDateTime(new Date(stamp * 1000), Qt.DefaultLocaleShortDate);
    }

    allowedOrientations: Orientation.All
    Component.onCompleted: Lappu.loadTrash()

    Connections {
        target: Lappu
        onTrashChanged: page.trashed = Lappu.trashedNotes()
    }

    SilicaListView {
        id: listView

        anchors.fill: parent
        model: page.trashed

        PullDownMenu {
            busy: Lappu.trashBusy

            MenuItem {
                text: qsTr("Refresh")
                onClicked: Lappu.loadTrash()
            }

        }

        header: Column {
            width: listView.width
            spacing: Theme.paddingMedium

            PageHeader {
                title: qsTr("Trash")
            }

            Label {
                x: Theme.horizontalPageMargin
                width: parent.width - 2 * Theme.horizontalPageMargin
                visible: listView.count > 0
                wrapMode: Text.WordWrap
                color: Theme.secondaryColor
                font.pixelSize: Theme.fontSizeSmall
                text: qsTr("These notes were deleted and are being kept by Nextcloud. Putting one back returns it to the folder it was deleted from, with its tags.")
            }

            // Keeps the first note off the paragraph above it.
            Item {
                width: 1
                height: Theme.paddingMedium
                visible: listView.count > 0
            }

        }

        delegate: ListItem {
            id: item

            // Read once here rather than from the menu, which is built apart
            // from the row and has no model of its own.
            property var note: modelData

            contentHeight: block.height + 2 * Theme.paddingMedium
            // A deleted note is not opened: there is nothing here to open it
            // into until it is a note of the folder again.
            onClicked: item.openMenu()

            Column {
                id: block

                spacing: Theme.paddingSmall

                anchors {
                    left: parent.left
                    right: parent.right
                    leftMargin: Theme.horizontalPageMargin
                    rightMargin: Theme.horizontalPageMargin
                    verticalCenter: parent.verticalCenter
                }

                Label {
                    width: parent.width
                    truncationMode: TruncationMode.Fade
                    color: item.highlighted ? Theme.highlightColor : Theme.primaryColor
                    text: modelData.title.length > 0 ? modelData.title : modelData.originalLocation
                }

                Label {
                    width: parent.width
                    truncationMode: TruncationMode.Fade
                    font.pixelSize: Theme.fontSizeExtraSmall
                    color: item.highlighted ? Theme.secondaryHighlightColor : Theme.secondaryColor
                    text: qsTr("Deleted %1").arg(page.deletedAt(modelData.deleted))
                }

                // Where it was is worth saying only where that was not the
                // note folder itself.
                Label {
                    width: parent.width
                    truncationMode: TruncationMode.Fade
                    visible: modelData.subFolderPath.length > 0
                    font.pixelSize: Theme.fontSizeExtraSmall
                    color: item.highlighted ? Theme.secondaryHighlightColor : Theme.secondaryColor
                    text: qsTr("In %1").arg(modelData.subFolderPath)
                }

            }

            menu: ContextMenu {

                MenuItem {
                    text: qsTr("Restore")
                    onClicked: page.restore(item.note)
                }

            }

        }

        ViewPlaceholder {
            enabled: listView.count === 0 && !Lappu.trashBusy
            text: qsTr("The trash is empty")
            hintText: qsTr("A note deleted from the folder is kept here until Nextcloud lets go of it.")
        }

        VerticalScrollDecorator {
        }

    }

    BusyIndicator {
        anchors.centerIn: parent
        size: BusyIndicatorSize.Large
        running: Lappu.trashBusy && listView.count === 0
    }

}
