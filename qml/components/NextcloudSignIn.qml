/*!
    \qmltype NextcloudSignIn
    \brief Reads the sign-in of a Nextcloud account of the Sailfish OS settings.

    Given the id of an account, it asks signond for that account's name and
    password and reports what comes back through \c credentialsReady or
    \c failed. Nothing is kept here and nothing is written anywhere, so an
    owner that wants the password on a later run asks for it again.

    It knows nothing of notes or of any particular app, and touches the
    accounts database only once it exists, so an app that signs in its own
    way need never load it.

    \qml
    NextcloudSignIn {
        id: signIn

        onCredentialsReady: console.log(server, user)
        onFailed: console.log(error)
        Component.onCompleted: signIn.read(accountId)
    }
    \endqml
*/
import QtQuick 2.0
import Sailfish.Accounts 1.0

Item {
    id: root

    // The account being read. It arrives from the manager still filling
    // itself in from the database, so it is held until it says it is done
    // and let go of as soon as it has been read.
    property Account account

    signal credentialsReady(string server, string user, string password)
    signal failed(string error)

    // Asks signond for the password of one account. The answer arrives as
    // credentialsReady() or failed(), never both and never at once.
    function read(accountId) {
        root.account = manager.account(accountId);
        if (!root.account) {
            //: Shown when the account the app signs in as is gone from the settings
            root.failed(qsTr("That Nextcloud account is no longer in the settings."));
            return ;
        }
        root.readWhenLoaded();
    }

    // An account knows its services only once it has read itself out of the
    // database, which it does after handing itself over, so the first look
    // is nearly always too early and the status is what says otherwise.
    function readWhenLoaded() {
        var account = root.account;
        if (!account || account.status === Account.Initializing)
            return ;

        root.account = null;
        var service = root.serviceOf(account);
        if (service === "")
            //: Shown when the chosen Nextcloud account has no service turned on
            root.failed(qsTr("That Nextcloud account has nothing switched on to sign in to."));
        else
            authenticator.signIn(account.identifier, service);
    }

    // The service to sign in on. Every service of an account shares the one
    // credential, so the first one switched on that names a server will do.
    function serviceOf(account) {
        var names = account.supportedServiceNames;
        for (var i = 0; i < names.length; ++i) {
            if (account.isEnabledWithService(names[i]) && root.serverOf(account, names[i]) !== "")
                return names[i];

        }
        return "";
    }

    // The address the account points at, which the service holds rather than
    // the account, and which the Notes API is then reached at.
    function serverOf(account, service) {
        var address = account.configurationValue(service, "server_address");
        return address ? address.toString() : "";
    }

    // The Notes API is reached with a name and a password, so an account
    // that signs in by a token alone carries nothing that can be sent.
    function report(credentials) {
        if (credentials.username === "" || credentials.password === "") {
            //: Shown when the Nextcloud account signs in some way the app cannot use
            root.failed(qsTr("That Nextcloud account does not sign in with a password."));
            return ;
        }
        root.credentialsReady(credentials.serviceSettings["server_address"] || "",
                              credentials.username, credentials.password);
    }

    AccountManager {
        id: manager
    }

    AccountAuthenticator {
        id: authenticator

        onSignInCompleted: root.report(credentials)
        onSignInError: root.failed(errorString)
    }

    Connections {
        target: root.account
        onStatusChanged: root.readWhenLoaded()
    }

}
