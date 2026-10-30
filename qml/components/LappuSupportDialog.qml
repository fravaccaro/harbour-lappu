/*!
    \brief The contents of Lappu's "ask for support" overlay.

    Holds the ways a user can give something back, and the reasons why. It is
    the \c contents of an \c AskForSupport, which decides when to show it; this
    component only says what it says.

    \qml
    AskForSupport {
        contents: Component {
            LappuSupportDialog {
            }
        }
    }
    \endqml
*/
import Opal.SupportMe 1.0
import QtQuick 2.0

SupportDialog {
    greeting: qsTr("Hi there!")
    hook: qsTr("Thank you for using Lappu. If it keeps your notes where you want them, maybe you can give a little back?")
    goodbye: qsTr("Thank you for your support!")

    SupportAction {
        icon: SupportIcon.Liberapay
        title: qsTr("Donate via Liberapay")
        description: qsTr("Send a tip, once or every month.")
        link: "https://liberapay.com/fravaccaro"
    }

    SupportAction {
        icon: SupportIcon.Weblate
        title: qsTr("Help with translations")
        description: qsTr("Bring Lappu to your language on Transifex.")
        link: "https://explore.transifex.com/fravaccaro/lappu"
    }

    SupportAction {
        icon: SupportIcon.Git
        title: qsTr("Report issues on GitHub")
        description: qsTr("Tell me what went wrong, or what is missing.")
        link: "https://github.com/fravaccaro/harbour-lappu/issues"
    }

    DetailsDrawer {
        title: qsTr("Why support this app?")

        DetailsParagraph {
            text: qsTr("Lappu is free software under the GPLv3, written in spare time next to other projects.")
        }

        DetailsParagraph {
            text: qsTr("Support keeps it working with new Nextcloud releases and new versions of Sailfish OS.")
        }
    }

    DetailsDrawer {
        title: qsTr("Other ways to help")

        DetailsParagraph {
            text: qsTr("Report what breaks, suggest what is missing, or tell other Nextcloud users about the app.")
        }
    }
}
