import QtQuick 2.0
import Sailfish.Silica 1.0
import "../components"

Page {
    id: page

    function selectAll() {
        app.notes.setFilter("all")
        pageStack.navigateBack()
    }

    function selectFavorites() {
        app.notes.setFilter("favorites")
        pageStack.navigateBack()
    }

    function selectUncategorized() {
        app.notes.setFilter("uncategorized")
        pageStack.navigateBack()
    }

    function selectCategory(name) {
        app.notes.setFilter("category", name)
        pageStack.navigateBack()
    }

    allowedOrientations: Orientation.All

    SilicaListView {
        id: listView

        anchors.fill: parent
        model: app.notes.categories

        PullDownMenu {
            busy: app.notes.busy

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
                onClicked: app.mockRefresh()
            }
        }

        header: Column {
            width: listView.width

            PageHeader {
                title: qsTr("Categories")
            }

            FilterItem {
                width: parent.width
                name: qsTr("All")
                count: app.notes.allNotes.count
                selected: app.notes.filterKind === "all"
                onClicked: page.selectAll()
            }

            FilterItem {
                width: parent.width
                name: qsTr("Favorites")
                count: app.notes.favoriteCount
                selected: app.notes.filterKind === "favorites"
                onClicked: page.selectFavorites()
            }

            FilterItem {
                width: parent.width
                name: qsTr("Uncategorized")
                count: app.notes.uncategorizedCount
                selected: app.notes.filterKind === "uncategorized"
                onClicked: page.selectUncategorized()
            }

            SectionHeader {
                visible: listView.count > 0
                text: qsTr("Categories")
            }
        }

        delegate: ListItem {
            id: item

            readonly property bool selected: app.notes.filterKind === "category" && app.notes.filterCategory === name

            contentHeight: Theme.itemSizeSmall
            onClicked: page.selectCategory(name)

            Label {
                truncationMode: TruncationMode.Fade
                color: item.selected || item.highlighted ? Theme.highlightColor : Theme.primaryColor
                text: app.notes.categoryLabel(name)

                anchors {
                    left: parent.left
                    leftMargin: Theme.horizontalPageMargin
                    right: countLabel.left
                    rightMargin: Theme.paddingMedium
                    verticalCenter: parent.verticalCenter
                }
            }

            Label {
                id: countLabel

                color: Theme.secondaryColor
                font.pixelSize: Theme.fontSizeSmall
                text: count

                anchors {
                    right: parent.right
                    rightMargin: Theme.horizontalPageMargin
                    verticalCenter: parent.verticalCenter
                }
            }
        }

        ViewPlaceholder {
            enabled: listView.count === 0
            text: qsTr("No categories")
        }

        VerticalScrollDecorator {
        }
    }
}
