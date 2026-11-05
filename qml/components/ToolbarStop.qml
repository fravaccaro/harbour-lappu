/*!
    \qmltype ToolbarStop
    \brief One square stop of a toolbar: a glyph that can be tapped.

    Sized to the finger rather than to the screen, so a bar made of these is
    dragged sideways when it wants more room than there is. It holds no state
    and knows nothing of what it does: the caller sets \c text and answers
    \c clicked.

    \c font is aliased whole, so a caller styles the glyph the way it would
    style any label - \c {font.bold} for a bold B, a larger \c {font.pixelSize}
    for an arrow - without a property here per style.

    A stop with nothing left to do is \c {enabled: false}, and is drawn the way
    Silica draws a menu entry that cannot be chosen.

    \qml
    ToolbarStop {
        text: "B"
        font.bold: true
        onClicked: editor.bold()
    }
    \endqml
*/
import QtQuick 2.0
import Sailfish.Silica 1.0

BackgroundItem {
    id: root

    property alias text: label.text
    property alias font: label.font

    width: Theme.itemSizeSmall
    height: Theme.itemSizeSmall

    Label {
        id: label

        anchors.centerIn: parent
        font.pixelSize: Theme.fontSizeSmall
        // The stop itself, never its parent: Silica hands a background item's
        // children to an inner rectangle, which is neither disabled with the
        // stop nor highlighted with it.
        color: !root.enabled ? Theme.rgba(Theme.secondaryColor, Theme.opacityLow) : (root.highlighted ? Theme.highlightColor : Theme.primaryColor)
    }

}
