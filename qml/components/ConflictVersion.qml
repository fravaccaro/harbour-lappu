/*!
    \qmltype ConflictVersion
    \brief One side of a note that changed in two places at once.

    Shows \c label with the time of \c modified beside it, then \c content as
    the note would read. The text is drawn by \l NoteBody, so Markdown and task
    lines look the way they do everywhere else, and the task lines are left
    untappable: this is a version being compared, not a note being edited.

    A link in the text is only re-emitted through \c linkActivated. The owner
    decides what happens to it, as either version may carry links that came
    from the server.

    \c modified counts seconds since the epoch, as the Notes API reports it;
    0 leaves the time out.

    \qml
    ConflictVersion {
        width: parent.width
        label: qsTr("On this phone")
        content: conflict.mineContent
        modified: conflict.mineModified
        onLinkActivated: L.LinkHandler.openOrCopyUrl(link)
    }
    \endqml
*/
import QtQuick 2.0
import Sailfish.Silica 1.0

Column {
    id: root

    property string label
    property string content
    property int modified

    signal linkActivated(string link)

    width: parent ? parent.width : Screen.width
    spacing: Theme.paddingSmall

    Item {
        width: root.width
        height: labelText.height

        Label {
            id: labelText

            color: Theme.highlightColor
            font.pixelSize: Theme.fontSizeSmall
            truncationMode: TruncationMode.Fade
            text: root.label

            anchors {
                left: parent.left
                leftMargin: Theme.horizontalPageMargin
                right: timeText.left
                rightMargin: Theme.paddingMedium
            }
        }

        Label {
            id: timeText

            color: Theme.secondaryColor
            font.pixelSize: Theme.fontSizeExtraSmall
            text: root.modified > 0
                  ? Qt.formatDateTime(new Date(root.modified * 1000), Qt.DefaultLocaleShortDate)
                  : ""

            anchors {
                right: parent.right
                rightMargin: Theme.horizontalPageMargin
                verticalCenter: labelText.verticalCenter
            }
        }
    }

    NoteBody {
        width: root.width
        visible: root.content !== ""
        content: root.content
        locked: true
        onLinkActivated: root.linkActivated(link)
    }

    Label {
        x: Theme.horizontalPageMargin
        width: root.width - 2 * Theme.horizontalPageMargin
        visible: root.content === ""
        wrapMode: Text.WordWrap
        color: Theme.secondaryColor
        font.pixelSize: Theme.fontSizeSmall
        text: qsTr("This version is empty.")
    }
}
