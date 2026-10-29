import QtQuick 2.0
import Sailfish.Silica 1.0
import "components"
import "pages"

ApplicationWindow {
    id: app

    function showMain() {
        if (pageStack.busy) {
            navigateAgain.restart();
            return ;
        }
        pageStack.replaceAbove(null, Muistio.configured ? notesPage : loginPage);
    }

    // Every passing message goes through one notice, so the app speaks the
    // same way whether it is in front or in the background.
    function showToast(text) {
        if (text !== "")
            globalNotification.toast(text);

    }

    initialPage: Muistio.ready ? (Muistio.configured ? notesPage : loginPage) : startPage
    cover: Qt.resolvedUrl("cover/CoverPage.qml")
    allowedOrientations: defaultAllowedOrientations

    Timer {
        id: navigateAgain

        interval: 100
        onTriggered: app.showMain()
    }

    Component {
        id: startPage

        StartPage {
        }

    }

    Component {
        id: loginPage

        LoginPage {
        }

    }

    Component {
        id: notesPage

        NotesPage {
        }

    }

    Connections {
        target: Muistio
        onReadyChanged: {
            if (Muistio.ready)
                app.showMain();

        }
        onConfiguredChanged: {
            if (!Muistio.configured)
                app.showMain();

        }
        onErrorMessageChanged: app.showToast(Muistio.errorMessage)
    }

    MuistioNotification {
        id: globalNotification
    }

}
