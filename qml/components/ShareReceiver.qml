/*!
    \qmltype ShareReceiver
    \brief Hands on the text and the links other apps share with this one.

    It holds the receiving end of the single share method the desktop file
    declares - \c note, which takes both \c text/plain and \c text/x-url - and
    reports what arrives through \c textShared and \c linkShared. What a share
    is worth making of it is the owner's business: this knows nothing of notes,
    so it sanitises nothing and opens nothing.

    Everything is taken off the bus through a \c DBusAdaptor on
    \c /share/note, rather than a \c ShareProvider. A \c ShareProvider would
    claim the same path, and the browser's links arrive as \c status and
    \c linkTitle rather than the \c name and \c data a \c ShareResource is
    specified with, so a provider would drop them before any handler ran. One
    adaptor handles both shapes, which is also what keeps a share working when
    the app is already open.

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
    // first resource that says anything is the one taken. Neither kind here
    // accepts files, so a resource naming one is passed over.
    function takeText(resources) {
        for (var i = 0; i < resources.length; ++i) {
            var item = resources[i];
            var text = item.data || item.status || "";
            if (text !== "") {
                receiver.textShared(text);
                return ;
            }
        }
    }

    // The first link of a share, which arrives as the address the sender was
    // showing and the name it was showing it under.
    function takeLink(resources) {
        if (resources.length > 0)
            receiver.linkShared(resources[0].status || "", resources[0].linkTitle || "");

    }

    // A resource from the bus may be a browser link ({status, linkTitle}) or
    // ordinary text ({name, data}). The type string is what tells them apart
    // when present; a status without data is treated as a link as well, which
    // is the shape the browser still sends.
    function takeShared(resources) {
        if (resources.length === 0)
            return ;

        var item = resources[0];
        if (item.type === "text/x-url" || (item.status && !item.data))
            receiver.takeLink(resources);
        else
            receiver.takeText(resources);

    }

    // Used by the adaptor below to read a share configuration into resources.
    ShareAction {
        id: sharedAction
    }

    // The path matches the method name in the desktop file. This is the only
    // object on that path: a ShareProvider beside it would own /share/note
    // instead, and a share arriving while the app was already open would
    // never reach here.
    DBusAdaptor {
        path: "/share/note"
        iface: "org.sailfishos.share"

        // Named for the method the share framework calls, not for what it does.
        function share(configuration) {
            sharedAction.loadConfiguration(configuration);
            receiver.takeShared(sharedAction.resources);
        }

    }

}
