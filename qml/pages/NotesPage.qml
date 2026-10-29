import QtQuick 2.0
import Sailfish.Silica 1.0

Page {
    id: page

    allowedOrientations: Orientation.All

    SilicaListView {
        id: listView

        anchors.fill: parent
        model: Notes

        PullDownMenu {
            MenuItem {
                text: qsTr("About")
                onClicked: pageStack.push(Qt.resolvedUrl("AboutPage.qml"))
            }

            MenuItem {
                text: qsTr("Settings")
                onClicked: pageStack.push(Qt.resolvedUrl("SettingsPage.qml"))
            }

            MenuItem {
                text: qsTr("Refresh")
                onClicked: Muistio.sync()
            }
        }

        header: PageHeader {
            title: qsTr("Notes")
        }

        delegate: ListItem {
            id: item

            width: listView.width

            Label {
                x: Theme.horizontalPageMargin
                width: parent.width - 2 * Theme.horizontalPageMargin
                anchors.verticalCenter: parent.verticalCenter
                truncationMode: TruncationMode.Fade
                color: item.highlighted ? Theme.highlightColor : Theme.primaryColor
                text: model.title
            }
        }

        ViewPlaceholder {
            enabled: listView.count === 0
            text: qsTr("No notes")
            hintText: qsTr("The notes list will appear here once syncing is available.")
        }

        VerticalScrollDecorator {
        }
    }
}
