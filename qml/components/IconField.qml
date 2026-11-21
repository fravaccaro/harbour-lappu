/*!
    \qmltype IconField
    \brief Frames content the way a Silica row frames a value.

    A leading theme icon, the content itself, and a caption below it, on the
    margins a Silica value row uses, so a row of chips lines up with the
    fields around it. The component holds no content of its own: the caller
    places some inside and keeps it between \c grooveLeftMargin and
    \c grooveRightMargin, so it runs from the value column to the page margin.
    \c description draws the caption underneath.

    The icon centres on \c lineCenterY and the caption follows \c lineBottomY,
    both of which default to the content itself: an icon beside its middle and
    a caption under it, which is what plain content such as a row of chips
    wants.

    \qml
    IconField {
        id: field

        width: parent.width
        description: qsTr("Tags")
        icon.source: Qt.resolvedUrl("../icons/icon-m-tag.svg")

        TagChips {
            x: field.grooveLeftMargin
            width: parent.width - x - field.grooveRightMargin
            tags: ["Work/Projects"]
        }
    }
    \endqml
*/
import QtQuick 2.0
import Sailfish.Silica 1.0

Item {
    id: root

    property alias icon: iconImage
    property string description
    property bool highlighted
    // Plain content sits flush at the top, where a Silica row keeps a breath
    // above its value. A field that is only content asks for that breath so
    // it shares the rhythm of the rows beside it.
    property real topPadding: 0
    property real lineCenterY: contentItem.height / 2
    property real lineBottomY: contentItem.height
    // Where the value of a row begins, the icon and its padding apart.
    readonly property real contentLeft: Theme.horizontalPageMargin + Theme.iconSizeMedium + Theme.paddingMedium
    readonly property real grooveLeftMargin: Theme.paddingMedium
    readonly property real grooveRightMargin: Theme.horizontalPageMargin
    default property alias _content: contentItem.data

    width: parent ? parent.width : Screen.width
    // The caption closes the row, on the padding a value button keeps under
    // its own text.
    height: caption.visible ? caption.y + caption.height + Theme.paddingMedium : contentItem.y + contentItem.height

    HighlightImage {
        id: iconImage

        x: Theme.horizontalPageMargin
        y: root.topPadding + root.lineCenterY - height / 2
        sourceSize.width: Theme.iconSizeMedium
        sourceSize.height: Theme.iconSizeMedium
        highlighted: root.highlighted
    }

    Item {
        id: contentItem

        x: root.contentLeft - Theme.paddingMedium
        y: root.topPadding
        width: parent.width - x
        height: childrenRect.height
    }

    Label {
        id: caption

        x: root.contentLeft
        y: root.topPadding + root.lineBottomY + Theme.paddingSmall
        width: parent.width - x - Theme.horizontalPageMargin
        visible: text.length
        color: Theme.rgba(root.highlighted ? Theme.highlightColor : Theme.primaryColor, Theme.opacityHigh)
        font.pixelSize: Theme.fontSizeExtraSmall
        truncationMode: TruncationMode.Fade
        text: root.description
    }

}
