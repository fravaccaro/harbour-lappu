import "../components"
import QtQuick 2.0
import Sailfish.Silica 1.0

Dialog {
    id: page

    property string category

    function choose(name) {
        newField.text = name;
        page.category = name;
        page.accept();
    }

    allowedOrientations: Orientation.All
    // Seeded once rather than bound, so the field cannot snap back to the
    // opening category and write it over a choice the user just made.
    Component.onCompleted: newField.text = page.category
    onAccepted: page.category = newField.text.trim()

    SilicaFlickable {
        anchors.fill: parent
        contentHeight: column.height + Theme.paddingLarge

        Column {
            id: column

            width: page.width

            DialogHeader {
                acceptText: qsTr("Set")
                title: qsTr("Category")
            }

            TextField {
                id: newField

                width: parent.width
                label: qsTr("Category")
                placeholderText: qsTr("Work/Meetings")
                EnterKey.iconSource: "image://theme/icon-m-enter-accept"
                EnterKey.onClicked: page.accept()
            }

            FilterItem {
                width: parent.width
                name: qsTr("Uncategorized")
                count: app.notes.uncategorizedCount
                selected: page.category === ""
                onClicked: page.choose("")
            }

            Repeater {
                model: app.notes.categories

                FilterItem {
                    width: column.width
                    name: app.notes.categoryLabel(model.name)
                    count: model.count
                    selected: page.category === model.name
                    onClicked: page.choose(model.name)
                }

            }

        }

        VerticalScrollDecorator {
        }

    }

}
