import Opal.About 1.0 as A
import QtQuick 2.0
import Sailfish.Silica 1.0 as S

A.AboutPageBase {
    id: page

    allowedOrientations: S.Orientation.All
    appName: "Lappu"
    appIcon: Qt.resolvedUrl("../../appicon.png")
    appVersion: Qt.application.version
    description: qsTr("A Nextcloud Notes client for Sailfish OS.")
    authors: ["fravaccaro"]
    homepageUrl: "https://fravaccaro.com/harbour-lappu/"
    sourcesUrl: "https://github.com/fravaccaro/harbour-lappu"
    donations.text: donations.defaultTextCoffee
    donations.services: [
        A.DonationService {
            name: "Liberapay"
            url: "https://liberapay.com/fravaccaro"
        }
    ]
    extraSections: [
        A.InfoSection {
            title: qsTr("Feedback")
            text: qsTr("If you want to provide feedback or report an issue, please use GitHub.")
            buttons: [
                A.InfoButton {
                    text: qsTr("Report an issue")
                    onClicked: openOrCopyUrl("https://github.com/fravaccaro/harbour-lappu/issues", text)
                }
            ]
        },
        A.InfoSection {
            title: qsTr("Translations")
            text: qsTr("Credits for existing translations and how to contribute.")
            buttons: [
                A.InfoButton {
                    text: qsTr("Translations")
                    onClicked: pageStack.push(Qt.resolvedUrl("TranslationsPage.qml"))
                }
            ]
        },
        A.InfoSection {
            title: "Nextcloud"
            text: qsTr("Lappu is not affiliated with Nextcloud GmbH.")
            buttons: [
                A.InfoButton {
                    text: qsTr("About Nextcloud Notes")
                    onClicked: openOrCopyUrl("https://apps.nextcloud.com/apps/notes", text)
                }
            ]
        }
    ]

    licenses: A.License {
        spdxId: "GPL-3.0-only"
    }

}
