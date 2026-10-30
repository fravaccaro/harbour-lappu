#include "notesapi.h"

#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>

#include "url.h"

namespace {

QString notesPath()
{
    return QStringLiteral("/index.php/apps/notes/api/v1/notes");
}

QString notePath(qint64 id)
{
    return notesPath() + QLatin1Char('/') + QString::number(id);
}

LocalNote noteFrom(const QJsonObject &object)
{
    LocalNote note;
    note.id = object.value(QStringLiteral("id")).toInt();
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

// Only these are the caller's to set: the server works the title out from the
// content and stamps the note itself.
QByteArray bodyFor(const LocalNote &note)
{
    QJsonObject object;
    object.insert(QStringLiteral("content"), note.content);
    object.insert(QStringLiteral("category"), note.category);
    object.insert(QStringLiteral("favorite"), note.favorite);
    return QJsonDocument(object).toJson(QJsonDocument::Compact);
}

QString statusError(int status)
{
    if (status == 401)
        return QObject::tr("The server refused the password. Sign in again.");
    if (status == 403)
        return QObject::tr("The note is read-only on the server.");
    if (status == 404)
        return QObject::tr("The Notes app is not enabled on the server.");

    return QObject::tr("The server refused the change (%1).").arg(status);
}

}

NotesApi::NotesApi(QObject *parent)
    : QObject(parent)
    , m_operation(Idle)
    , m_pendingId(0)
{
    m_http.setUserAgent(QStringLiteral("Lappu"));
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

void NotesApi::send(const QByteArray &method, const QString &path, const QByteArray &body)
{
    if (m_server.isEmpty()) {
        m_operation = Idle;
        emit failed(tr("Enter a server address."));
        return;
    }

    HttpClient::Headers headers;
    headers.append(qMakePair(QByteArray("Accept"), QByteArray("application/json")));
    if (!body.isEmpty()) {
        headers.append(qMakePair(QByteArray("Content-Type"),
                                 QByteArray("application/json")));
    }
    m_http.send(method, QUrl(m_server + path), body, headers);
}

void NotesApi::fetchNotes()
{
    m_operation = FetchList;
    send("GET", notesPath(), QByteArray());
}

void NotesApi::createNote(const LocalNote &note)
{
    m_operation = Create;
    m_pendingId = note.id;
    send("POST", notesPath(), bodyFor(note));
}

void NotesApi::updateNote(const LocalNote &note)
{
    m_operation = Update;
    m_pendingId = note.id;
    send("PUT", notePath(note.id), bodyFor(note));
}

void NotesApi::deleteNote(qint64 id)
{
    m_operation = Delete;
    m_pendingId = id;
    send("DELETE", notePath(id), QByteArray());
}

void NotesApi::onFinished(int status, const QByteArray &body, const QString &, const QUrl &)
{
    const Operation operation = m_operation;
    m_operation = Idle;
    if (operation == Idle)
        return;

    if (operation != FetchList) {
        handleWrite(operation, status, body);
        return;
    }

    if (status != 200) {
        emit failed(statusError(status));
        return;
    }

    emit notesReady(notesFrom(body));
}

void NotesApi::handleWrite(Operation operation, int status, const QByteArray &body)
{
    // A note the server no longer has needs nothing further from us, whichever
    // of the two we were asking for.
    if (operation == Delete && (status == 200 || status == 404)) {
        emit deleted(m_pendingId);
        return;
    }

    if (status != 200 && status != 201) {
        if (operation == Update && status == 404)
            emit deleted(m_pendingId);
        else
            emit failed(statusError(status));
        return;
    }

    const LocalNote note = noteFrom(QJsonDocument::fromJson(body).object());
    if (operation == Create)
        emit created(m_pendingId, note);
    else
        emit updated(note);
}

void NotesApi::onFailed(const QString &error)
{
    m_operation = Idle;
    emit failed(error);
}
