#include "qownnotesapi.h"

#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonValue>

#include "notesapi.h"
#include "url.h"

namespace {

// The app publishes itself in the capabilities of the server rather than on a
// path of its own, so one request settles it without guessing at a URL.
QString capabilitiesPath()
{
    return QStringLiteral("/ocs/v1.php/cloud/capabilities?format=json");
}

QJsonObject capabilitiesOf(const QByteArray &body)
{
    const QJsonObject ocs = QJsonDocument::fromJson(body).object()
            .value(QStringLiteral("ocs")).toObject();
    return ocs.value(QStringLiteral("data")).toObject()
            .value(QStringLiteral("capabilities")).toObject();
}

// The published path is appended to the server's own URL, so one that is not
// a plain absolute path is no use here: an address of its own would send the
// notes to another host, and a step upwards would leave the app's own API.
bool usableBase(const QString &base)
{
    if (!base.startsWith(QLatin1Char('/')))
        return false;

    return !base.contains(QStringLiteral("//")) && !base.contains(QStringLiteral(".."));
}

// Where the app says it serves its APIs. A server that names nothing usable
// is still a server carrying the app, so the documented path answers for it
// rather than the app being taken for missing.
QString apiBaseOf(const QJsonObject &app)
{
    const QString base = app.value(QStringLiteral("api_base")).toString().trimmed();
    return usableBase(base) ? base : QownNotesApi::defaultApiBase();
}

}

QownNotesApi::QownNotesApi(QObject *parent)
    : QObject(parent)
    , m_probing(false)
    , m_outstanding(0)
{
    m_http.setUserAgent(QStringLiteral("Lappu"));
    connect(&m_http, &HttpClient::finished, this, &QownNotesApi::onFinished);
    connect(&m_http, &HttpClient::failed, this, &QownNotesApi::onFailed);
}

QString QownNotesApi::defaultApiBase()
{
    return QStringLiteral("/index.php/apps/qownnotes/api/v1/");
}

void QownNotesApi::setServer(const QString &serverUrl)
{
    m_server = Url::normalizeServer(serverUrl);
}

void QownNotesApi::setCredentials(const QString &user, const QString &password)
{
    m_http.setBasicAuth(user, password);
}

void QownNotesApi::setIgnoreSslErrors(bool ignore)
{
    m_http.setIgnoreSslErrors(ignore);
}

void QownNotesApi::probe()
{
    if (m_probing)
        return;

    if (m_server.isEmpty()) {
        reportAbsent();
        return;
    }

    m_probing = true;
    ++m_outstanding;
    HttpClient::Headers headers;
    headers.append(qMakePair(QByteArray("Accept"), QByteArray("application/json")));
    // Nextcloud answers an OCS endpoint only where the request owns up to
    // being one, whatever is asked of it.
    headers.append(qMakePair(QByteArray("OCS-APIRequest"), QByteArray("true")));
    m_http.send("GET", QUrl(m_server + capabilitiesPath()), QByteArray(), headers);
}

void QownNotesApi::forget()
{
    m_probing = false;
}

// A server that answers anything else has not said the app is there, and
// nothing about a missing app is worth reporting as having gone wrong: a sync
// of the notes themselves says whatever is to be said about the server.
void QownNotesApi::onFinished(int status, const QByteArray &body)
{
    if (!takeReply())
        return;

    if (status == 401) {
        emit unauthorized(NotesApi::passwordRefusedError());
        reportAbsent();
        return;
    }

    if (status != 200) {
        reportAbsent();
        return;
    }

    // The key is the current app's own. The older qownnotesapi app publishes
    // no capability at all, which is what leaves it out of this.
    const QJsonValue app = capabilitiesOf(body).value(QStringLiteral("qownnotes"));
    if (!app.isObject()) {
        reportAbsent();
        return;
    }

    report(true, apiBaseOf(app.toObject()));
}

void QownNotesApi::onFailed(const QString &)
{
    if (takeReply())
        reportAbsent();
}

bool QownNotesApi::takeReply()
{
    if (m_outstanding > 0)
        --m_outstanding;

    return m_probing && m_outstanding == 0;
}

void QownNotesApi::reportAbsent()
{
    report(false, QString());
}

void QownNotesApi::report(bool present, const QString &apiBase)
{
    m_probing = false;
    emit probed(present, apiBase);
}
