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

    // Where the app serves both of its APIs when it publishes no usable path
    // of its own. It is the app's documented base, and the fallback rather
    // than the first choice: the server is the one that says where it is.
    static QString defaultApiBase();

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
    // Whether the app is there, and the path it serves its APIs under. Every
    // answer but the one that names it, an unreachable server included, is
    // reported as absent: a feature nothing has confirmed is one worth
    // leaving out of the way. The base is empty unless the app is there.
    void probed(bool present, const QString &apiBase);
    // A password the server will not take. It travels apart from probed()
    // because it is about the account rather than the app, and sends the user
    // back to signing in; the app is reported absent alongside it.
    void unauthorized(const QString &error);

private:
    void onFinished(int status, const QByteArray &body);
    void onFailed(const QString &error);
    // Whether the answer now arriving is the one this probe is waiting for.
    // A request disowned by forget() goes on travelling, so a reply is only
    // taken when nothing newer is still on its way.
    bool takeReply();
    void reportAbsent();
    void report(bool present, const QString &apiBase);

    HttpClient m_http;
    QString m_server;
    bool m_probing;
    // Requests sent and not yet answered.
    int m_outstanding;
};

#endif
