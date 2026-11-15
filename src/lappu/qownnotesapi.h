#ifndef LAPPU_QOWNNOTESAPI_H
#define LAPPU_QOWNNOTESAPI_H

#include <QObject>
#include <QString>

#include "httpclient.h"

// Asks whether the QOwnNotes app is on the server, so the features that need
// it can be shown only where they would work. Only the current app counts:
// the older qownnotesapi app is not supported, and a server carrying nothing
// but that one is answered for as if it carried neither.
class QownNotesApi : public QObject
{
    Q_OBJECT

public:
    explicit QownNotesApi(QObject *parent = nullptr);

    void setServer(const QString &serverUrl);
    void setCredentials(const QString &user, const QString &password);
    void setIgnoreSslErrors(bool ignore);

    // Asks the server once. A probe already on its way is left to finish,
    // since the answer it brings is the one this would ask for.
    void probe();
    // Lets go of a probe still travelling, for when the account it was asked
    // on behalf of is gone: the answer would be about a server the app has
    // signed out of, and is dropped rather than reported.
    void forget();

signals:
    // Whether the app is there. Every answer but the one that names it, an
    // unreachable server included, is reported as absent: a feature nothing
    // has confirmed is one worth leaving out of the way.
    void probed(bool present);
    // A password the server will not take. It travels apart from probed()
    // because it is about the account rather than the app, and sends the user
    // back to signing in; the app is reported absent alongside it.
    void unauthorized(const QString &error);

private:
    void onFinished(int status, const QByteArray &body);
    void onFailed(const QString &error);
    void report(bool present);

    HttpClient m_http;
    QString m_server;
    bool m_probing;
};

#endif
