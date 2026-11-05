/*!
    \qmltype ScrollHint
    \brief Slides a flickable sideways and back once, to say there is more of it than the screen shows.

    A view that scrolls does not say so while it sits still. This nudges
    \c flickable when the owner starts it: \c travel worth of slide and back
    again, or as much of that as is actually hidden, so a view everything
    already fits in does not move at all and the hint costs nothing.

    It pauses before it moves, so the page it stands on has settled first, and
    the owner calls it off the moment the reader scrolls for themselves - a
    hint is only worth giving to somebody who has not already found it.

    The property is \c flickable rather than the \c viewport such a thing is
    usually called, because an object's own properties are resolved before the
    ids around it: \c {viewport: viewport} would bind this to itself.

    \qml
    ScrollHint {
        id: hint

        flickable: viewport
        travel: Theme.itemSizeSmall
    }

    Flickable {
        id: viewport

        onMovementStarted: hint.stop()
    }

    Component.onCompleted: hint.start()
    \endqml
*/
import QtQuick 2.0
import Sailfish.Silica 1.0

SequentialAnimation {
    id: root

    property Flickable flickable
    // How far the nudge travels, and never further than what is hidden:
    // nothing hidden, nothing moves.
    property real travel: Theme.itemSizeSmall
    readonly property real distance: root.flickable ? Math.min(root.travel, Math.max(0, root.flickable.contentWidth - root.flickable.width)) : 0

    PauseAnimation {
        duration: 700
    }

    NumberAnimation {
        target: root.flickable
        property: "contentX"
        to: root.distance
        duration: 500
        easing.type: Easing.InOutQuad
    }

    NumberAnimation {
        target: root.flickable
        property: "contentX"
        to: 0
        duration: 500
        easing.type: Easing.InOutQuad
    }

}
