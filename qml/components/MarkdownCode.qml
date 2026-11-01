/*!
    \qmltype MarkdownCode
    \brief The inside of a fenced Markdown code block, drawn as code.

    Takes the lines between the fences as \c content and draws them in a
    fixed-pitch font on a faint panel, exactly as they were written: nothing is
    marked up and nothing is escaped, because inside a fence an asterisk is an
    asterisk and a bracket is a bracket.

    Lines are not wrapped, since where a line of code breaks carries meaning.
    One too wide for the screen is reached by flicking the panel sideways, and
    the note it sits in keeps still while that happens.

    \qml
    MarkdownCode {
        width: parent.width
        content: "def hello(name):\n    print(name)"
    }
    \endqml
*/
import QtQuick 2.0
import Sailfish.Silica 1.0

Item {
    id: root

    property string content

    width: parent ? parent.width : Screen.width
    height: viewport.height

    Rectangle {
        anchors.fill: viewport
        // Enough to tell the block from the prose around it on either
        // ambience, and not enough to compete with the text on it.
        color: Theme.rgba(Theme.highlightBackgroundColor, 0.15)
    }

    Flickable {
        id: viewport

        width: root.width
        height: codeText.height + 2 * Theme.paddingMedium
        contentWidth: Math.max(codeText.width + 2 * Theme.horizontalPageMargin, width)
        contentHeight: height
        flickableDirection: Flickable.HorizontalFlick
        boundsBehavior: Flickable.StopAtBounds
        clip: true

        Label {
            id: codeText

            x: Theme.horizontalPageMargin
            y: Theme.paddingMedium
            // Resolved by fontconfig, which answers DejaVu Sans Mono on the
            // device: Silica names no fixed-pitch family of its own.
            font.family: "monospace"
            font.pixelSize: Theme.fontSizeSmall
            wrapMode: Text.NoWrap
            textFormat: Text.PlainText
            color: Theme.primaryColor
            text: root.content
        }

    }

}
