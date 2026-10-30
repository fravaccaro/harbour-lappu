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

// The server compares `If-Match` against its own tag wrapped in double quotes,
// while it hands the same tag out bare in the JSON of a note. The quotes are
// put back here, or every write would be refused. An empty tag asks for no
// header at all, which is how a server too old to send one keeps working.
QByteArray ifMatchValue(const QString &etag)
{
    if (etag.isEmpty())
        return QByteArray();

    return '"' + etag.toUtf8() + '"';
}

QString statusError(int status)
{
    if (status == 401)
        return QObject::tr("The server refused the password. Sign in again.");
    if (status == 403)
        return QObject::tr("The note is read-only on the server.");
    if (status == 404)
        return QObject::tr("The Notes app is not enabled on the server.");
    if (status == 412)
        return QObject::tr("The note changed on the server, which did not say how.");

    return QObject::tr("The server refused the change (%1).").arg(status);
}

// A note the server no longer has needs nothing further from us, whichever
// of the two we were asking for.
bool noteIsGone(NotesApi::Operation operation, int status)
{
    if (operation == NotesApi::Delete)
        return status == 200 || status == 404;

    return operation == NotesApi::Update && status == 404;
}

bool writeSucceeded(int status)
{
    return status == 200 || status == 201;
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

void NotesApi::send(const QByteArray &method, const QString &path, const QByteArray &body,
                    const QString &etag)
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

    const QByteArray ifMatch = ifMatchValue(etag);
    if (!ifMatch.isEmpty())
        headers.append(qMakePair(QByteArray("If-Match"), ifMatch));

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

// The version the note was last read at goes with the edit, so a note that
// changed on the server in the meantime is refused rather than written over.
void NotesApi::updateNote(const LocalNote &note)
{
    m_operation = Update;
    m_pendingId = note.id;
    send("PUT", notePath(note.id), bodyFor(note), note.etag);
}

// No tag is sent: the server checks none on a delete, and a note the user
// asked to be rid of is not worth refusing over a version it no longer has.
void NotesApi::deleteNote(qint64 id)
{
    m_operation = Delete;
    m_pendingId = id;
    send("DELETE", notePath(id), QByteArray());
}

void NotesApi::onFinished(int status, const QByteArray &body)
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
        reportStatus(status);
        return;
    }

    emit notesReady(notesFrom(body));
}

void NotesApi::handleWrite(Operation operation, int status, const QByteArray &body)
{
    if (noteIsGone(operation, status)) {
        emit deleted(m_pendingId);
        return;
    }

    if (status == 412) {
        reportConflict(body);
        return;
    }

    if (!writeSucceeded(status)) {
        reportStatus(status);
        return;
    }

    reportWritten(operation, body);
}

void NotesApi::reportWritten(Operation operation, const QByteArray &body)
{
    const LocalNote note = noteFrom(QJsonDocument::fromJson(body).object());
    // Without an id there is nothing to settle the note against, and taking the
    // answer as it stands would leave the note looking new for good: every sync
    // from then on would offer it again as another copy.
    if (note.id <= 0) {
        emit failed(tr("The server did not say which note it saved."));
        return;
    }

    if (operation == Create)
        emit created(m_pendingId, note);
    else
        emit updated(note);
}

// A refused conditional write answers with the whole note as the server holds
// it, so the two versions can be put side by side without asking again.
void NotesApi::reportConflict(const QByteArray &body)
{
    const LocalNote remote = noteFrom(QJsonDocument::fromJson(body).object());
    // Without an id there is no note to settle the edit against, which leaves
    // nothing to do but say the write was refused.
    if (remote.id <= 0) {
        reportStatus(412);
        return;
    }

    emit conflicted(remote);
}

void NotesApi::reportStatus(int status)
{
    if (status == 401) {
        emit unauthorized(statusError(status));
        return;
    }

    emit failed(statusError(status));
}

void NotesApi::onFailed(const QString &error)
{
    m_operation = Idle;
    emit failed(error);
}
