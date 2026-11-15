#include "qownnotesapi.h"

#include <QJsonDocument>
#include <QJsonObject>

#include "notesapi.h"
#include "url.h"

namespace {

// The app publishes itself in the capabilities of the server rather than on a
// path of its own, so one request settles it without guessing at a URL.
QString capabilitiesPath()
{
    return QStringLiteral("/ocs/v1.php/cloud/capabilities?format=json");
}

// The key is the current app's own. The older qownnotesapi app publishes no
// capability at all, which is what leaves it out of this.
bool namesTheApp(const QByteArray &body)
{
    const QJsonObject ocs = QJsonDocument::fromJson(body).object()
            .value(QStringLiteral("ocs")).toObject();
    const QJsonObject capabilities = ocs.value(QStringLiteral("data")).toObject()
            .value(QStringLiteral("capabilities")).toObject();
    return capabilities.value(QStringLiteral("qownnotes")).isObject();
}

}

QownNotesApi::QownNotesApi(QObject *parent)
    : QObject(parent)
    , m_probing(false)
{
    m_http.setUserAgent(QStringLiteral("Lappu"));
    connect(&m_http, &HttpClient::finished, this, &QownNotesApi::onFinished);
    connect(&m_http, &HttpClient::failed, this, &QownNotesApi::onFailed);
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
        report(false);
        return;
    }

    m_probing = true;
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
    if (!m_probing)
        return;

    if (status == 401) {
        emit unauthorized(NotesApi::passwordRefusedError());
        report(false);
        return;
    }

    report(status == 200 && namesTheApp(body));
}

void QownNotesApi::onFailed(const QString &)
{
    if (m_probing)
        report(false);
}

void QownNotesApi::report(bool present)
{
    m_probing = false;
    emit probed(present);
}
