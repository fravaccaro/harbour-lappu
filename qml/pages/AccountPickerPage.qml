import QtQuick 2.0
import Sailfish.Accounts 1.0
import Sailfish.Silica 1.0

Page {
    id: page

    // Named with the other sign-in pages so a share waits until the notes
    // are on the stack: a dialog pushed here would be thrown away by the
    // replaceAbove a successful sign-in ends with.
    objectName: "accountPickerPage"
    property bool signingIn

    allowedOrientations: Orientation.All

    Connections {
        target: Lappu
        onConfiguredChanged: {
            if (Lappu.configured)
                pageStack.replaceAbove(null, Qt.resolvedUrl("NotesPage.qml"));

        }
        onCredentialsErrorChanged: page.signingIn = false
    }

    SilicaListView {
        id: listView

        anchors.fill: parent
        opacity: page.signingIn ? 0 : 1

        // The Nextcloud accounts of the Sailfish OS settings, the switched
        // off ones left out: one the user has turned off is not one to sign
        // in as.
        model: AccountModel {
            filterType: AccountModel.ProviderFilter
            filter: "nextcloud"
            filterByEnabled: true
        }

        header: Column {
            width: listView.width
            spacing: Theme.paddingMedium

            PageHeader {
                title: qsTr("Nextcloud accounts")
            }

            Label {
                x: Theme.horizontalPageMargin
                width: parent.width - 2 * Theme.horizontalPageMargin
                visible: listView.count > 0
                wrapMode: Text.WordWrap
                color: Theme.secondaryColor
                font.pixelSize: Theme.fontSizeSmall
                text: qsTr("Lappu reads the password of the account you choose from Sailfish OS every time it starts, and keeps no copy of it. Changing the account password in Settings is enough.")
            }

            Label {
                x: Theme.horizontalPageMargin
                width: parent.width - 2 * Theme.horizontalPageMargin
                visible: Lappu.credentialsError !== ""
                wrapMode: Text.WordWrap
                color: Theme.errorColor
                font.pixelSize: Theme.fontSizeExtraSmall
                text: Lappu.credentialsError
            }

            // Keeps the first account off the paragraph above it.
            Item {
                width: 1
                height: Theme.paddingMedium
                visible: listView.count > 0
            }

        }

        delegate: ListItem {
            id: item

            contentHeight: block.height + 2 * Theme.paddingMedium
            onClicked: {
                page.signingIn = true;
                Lappu.useAccount(model.accountId);
            }

            Column {
                id: block

                spacing: Theme.paddingSmall

                anchors {
                    left: parent.left
                    right: parent.right
                    leftMargin: Theme.horizontalPageMargin
                    rightMargin: Theme.horizontalPageMargin
                    verticalCenter: parent.verticalCenter
                }

                Label {
                    width: parent.width
                    truncationMode: TruncationMode.Fade
                    color: item.highlighted ? Theme.highlightColor : Theme.primaryColor
                    text: model.accountDisplayName
                }

                Label {
                    width: parent.width
                    truncationMode: TruncationMode.Fade
                    font.pixelSize: Theme.fontSizeExtraSmall
                    color: item.highlighted ? Theme.secondaryHighlightColor : Theme.secondaryColor
                    text: model.accountUserName
                }

            }

        }

        ViewPlaceholder {
            enabled: listView.count === 0
            text: qsTr("No Nextcloud account")
            hintText: qsTr("Add one in Settings > Accounts, then come back. Or sign in to Lappu on its own.")
        }

        VerticalScrollDecorator {
        }

    }

    BusyIndicator {
        anchors.centerIn: parent
        size: BusyIndicatorSize.Large
        running: page.signingIn
    }

}
