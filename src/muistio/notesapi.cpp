#include "notesapi.h"

#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>

#include "url.h"

namespace {

LocalNote noteFrom(const QJsonObject &object)
{
    LocalNote note;
    note.id = QString::number(object.value(QStringLiteral("id")).toInt());
    note.etag = object.value(QStringLiteral("etag")).toString();
    note.title = object.value(QStringLiteral("title")).toString();
    note.content = object.value(QStringLiteral("content")).toString();
    note.category = object.value(QStringLiteral("category")).toString();
    note.favorite = object.value(QStringLiteral("favorite")).toBool();
    note.readOnly = object.value(QStringLiteral("readonly")).toBool();
    note.modified = static_cast<qint64>(object.value(QStringLiteral("modified")).toDouble());
    return note;
}

QVector<LocalNote> notesFrom(const QByteArray &body)
{
    const QJsonArray array = QJsonDocument::fromJson(body).array();
    QVector<LocalNote> notes;
    notes.reserve(array.size());
    for (int i = 0; i < array.size(); ++i)
        notes.append(noteFrom(array.at(i).toObject()));

    return notes;
}

QString statusError(int status)
{
    if (status == 401)
        return QObject::tr("The server refused the password. Sign in again.");
    if (status == 404)
        return QObject::tr("The Notes app is not enabled on the server.");

    return QObject::tr("The server did not send the notes (%1).").arg(status);
}

}

NotesApi::NotesApi(QObject *parent)
    : QObject(parent)
    , m_fetching(false)
{
    m_http.setUserAgent(QStringLiteral("Muistio"));
    connect(&m_http, &HttpClient::finished, this, &NotesApi::onFinished);
    connect(&m_http, &HttpClient::failed, this, &NotesApi::onFailed);
}

void NotesApi::setServer(const QString &serverUrl)
{
    m_server = Url::normalizeServer(serverUrl);
}

void NotesApi::setCredentials(const QString &user, const QString &password)
{
    m_http.setBasicAuth(user, password);
}

void NotesApi::setIgnoreSslErrors(bool ignore)
{
    m_http.setIgnoreSslErrors(ignore);
}

void NotesApi::fetchNotes()
{
    if (m_server.isEmpty()) {
        emit failed(tr("Enter a server address."));
        return;
    }

    m_fetching = true;
    HttpClient::Headers headers;
    headers.append(qMakePair(QByteArray("Accept"), QByteArray("application/json")));
    const QUrl url(m_server + QStringLiteral("/index.php/apps/notes/api/v1/notes"));
    m_http.send("GET", url, QByteArray(), headers);
}

void NotesApi::onFinished(int status, const QByteArray &body, const QString &, const QUrl &)
{
    if (!m_fetching)
        return;

    m_fetching = false;
    if (status != 200) {
        emit failed(statusError(status));
        return;
    }

    emit notesReady(notesFrom(body));
}

void NotesApi::onFailed(const QString &error)
{
    if (!m_fetching)
        return;

    m_fetching = false;
    emit failed(error);
}
