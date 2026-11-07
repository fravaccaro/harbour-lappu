/*!
    \qmltype ShareReceiver
    \brief Hands on the text and the links other apps share with this one.

    It holds the receiving end of the share methods the desktop file declares -
    \c text for \c text/plain and \c link for \c text/x-url - and reports what
    arrives through \c textShared and \c linkShared. What a share is worth
    making of it is the owner's business: this knows nothing of notes, so it
    sanitises nothing and opens nothing.

    The application must claim its own bus name - the organisation and
    application names of the desktop file joined by a dot - from C++ once the
    window has loaded. Claiming it from QML races with the objects put on the
    bus here, which is why neither of them asks for it.

    \qml
    ShareReceiver {
        onTextShared: app.openSharedNote(Lappu.sharedBody(text))
        onLinkShared: app.openSharedNote(Lappu.sharedLink(url, linkTitle))
    }
    \endqml
*/
import QtQuick 2.0
import Sailfish.Share 1.0
import Nemo.DBus 2.0

Item {
    id: receiver

    signal textShared(string text)
    signal linkShared(string url, string linkTitle)

    // A share carries a list, while a note is one thing written down, so the
    // first resource that says anything is the one taken. Neither method here
    // accepts files, so a resource naming one is passed over.
    function takeText(resources) {
        for (var i = 0; i < resources.length; ++i) {
            if (resources[i].type === ShareResource.StringDataType && resources[i].data !== "") {
                receiver.textShared(resources[i].data);
                return ;
            }
        }
    }

    // The first link of a share read off the bus, which arrives as the address
    // the sender was showing and the name it was showing it under.
    function takeLink(resources) {
        if (resources.length > 0)
            receiver.linkShared(resources[0].status || "", resources[0].linkTitle || "");

    }

    ShareProvider {
        method: "text"
        capabilities: ["text/plain"]
        onTriggered: receiver.takeText(resources)
    }

    // Used by the adaptor below to read a share configuration into resources.
    ShareAction {
        id: sharedLink
    }

    // A link is not delivered through a ShareProvider of its own. The browser
    // sends one as `status` and `linkTitle` rather than the `name` and `data`
    // a ShareResource is specified with, and ShareProvider drops it as unusable
    // before any handler is reached. Taking the configuration off the bus is
    // the way round that, and is what the system's own apps do.
    DBusAdaptor {
        path: "/share/link"
        iface: "org.sailfishos.share"

        // Named for the method the share framework calls, not for what it does.
        function share(configuration) {
            sharedLink.loadConfiguration(configuration);
            receiver.takeLink(sharedLink.resources);
        }

    }

}
