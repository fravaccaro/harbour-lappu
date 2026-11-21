/*!
    \qmltype FilterItem
    \brief A list row showing a named view and how many notes it holds.

    The row carries no model knowledge: the caller supplies the name, the
    count and whether the view is the selected one, and reacts to \c clicked.
    A view whose size is not known until it is opened sets \c countVisible
    false and shows its name alone. A row that stands under another sets
    \c depth, which sets its name in from the margin by that many steps.

    \qml
    FilterItem {
        name: qsTr("Favorites")
        count: 4
        selected: true
        onClicked: notes.setFilter("favorites")
    }
    \endqml
*/
import QtQuick 2.0
import Sailfish.Silica 1.0

ListItem {
    id: root

    property string name
    property int count: 0
    property bool countVisible: true
    property int depth: 0
    property bool selected: false

    contentHeight: Theme.itemSizeSmall

    Label {
        truncationMode: TruncationMode.Fade
        color: root.selected || root.highlighted ? Theme.highlightColor : Theme.primaryColor
        text: root.name

        anchors {
            left: parent.left
            leftMargin: Theme.horizontalPageMargin + root.depth * Theme.paddingLarge
            right: countLabel.left
            rightMargin: Theme.paddingMedium
            verticalCenter: parent.verticalCenter
        }

    }

    Label {
        id: countLabel

        color: Theme.secondaryColor
        font.pixelSize: Theme.fontSizeSmall
        // Emptied rather than hidden, so a row that counts nothing gives the
        // whole width to its name instead of keeping the space.
        text: root.countVisible ? root.count : ""

        anchors {
            right: parent.right
            rightMargin: Theme.horizontalPageMargin
            verticalCenter: parent.verticalCenter
        }

    }

}
