/*!
    \qmltype TagChips
    \brief Shows the tags of a note as a row of rounded chips.

    Read-only counterpart of TagEditor. The component carries no model
    knowledge: the caller hands in \c tags as an array of tag paths, which are
    drawn with their separator spelled out. An empty array draws the
    \c placeholderText instead, so a note without tags still reads like the
    lines around it.

    \qml
    TagChips { tags: ["Work/Projects", "Urgent"] }
    \endqml
*/
import "../notes/NoteFormat.js" as Format
import QtQuick 2.0
import Sailfish.Silica 1.0

Flow {
    id: root

    property var tags: []
    property string placeholderText: qsTr("None")

    spacing: Theme.paddingSmall

    Label {
        visible: root.tags.length === 0
        color: Theme.secondaryColor
        font.pixelSize: Theme.fontSizeSmall
        text: root.placeholderText
    }

    Repeater {
        model: root.tags

        TagChip {
            text: Format.pathLabel(modelData)
            // Nothing to tap here, so a press reaches the page underneath.
            enabled: false
        }

    }

}
