#include "qownnotesapi.h"

#include <QJsonArray>
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

// Whether the server can read the note folder database at all, which needs an
// extension of PHP that not every installation carries. A server that cannot
// read it has no tags to speak of, so nothing about them is asked or shown.
bool tagsAvailableIn(const QJsonObject &app)
{
    return app.value(QStringLiteral("tags")).toObject()
            .value(QStringLiteral("available")).toBool();
}

// What the app said of itself, which is the whole of what is known about the
// server until something is asked of it.
ServerApps appsOf(const QJsonObject &app)
{
    ServerApps apps;
    apps.present = true;
    apps.apiBase = apiBaseOf(app);
    apps.tags = tagsAvailableIn(app);
    // The trash is Nextcloud's own Deleted files app, which the user may not
    // have: without it the note folder has no trash to look in.
    apps.trash = app.value(QStringLiteral("trash_app")).toBool();
    return apps;
}

// The tag tree flattened onto one path per tag, the names of the branch
// joined the way a nested category is written. The order the server sorted
// the tree in is kept, a parent standing before the tags under it.
void collectTags(const QJsonArray &nodes, const QString &parentPath, QVector<NoteTag> *tags)
{
    for (int i = 0; i < nodes.size(); ++i) {
        const QJsonObject node = nodes.at(i).toObject();
        const QString name = node.value(QStringLiteral("name")).toString().trimmed();
        if (name.isEmpty())
            continue;

        NoteTag tag;
        tag.id = node.value(QStringLiteral("id")).toVariant().toLongLong();
        tag.path = parentPath.isEmpty() ? name : parentPath + QLatin1Char('/') + name;
        tags->append(tag);
        collectTags(node.value(QStringLiteral("children")).toArray(), tag.path, tags);
    }
}

// A link the server could not place on a note of its own names a file this
// device knows nothing about, and a stale one belongs to a note that is gone:
// neither says anything about the notes here.
QVector<TagLink> linksOf(const QJsonArray &entries)
{
    QVector<TagLink> links;
    for (int i = 0; i < entries.size(); ++i) {
        const QJsonObject entry = entries.at(i).toObject();
        TagLink link;
        link.tagId = entry.value(QStringLiteral("tagId")).toVariant().toLongLong();
        link.noteId = entry.value(QStringLiteral("noteId")).toVariant().toLongLong();
        if (link.noteId == 0 || entry.value(QStringLiteral("stale")).toBool())
            continue;

        links.append(link);
    }

    return links;
}

// What a status other than success means for the tags. The app answers 503
// where it cannot use the database at all, which is the ordinary way a server
// says it carries no tags, and 403 where the file is only to be read.
QString tagStatusError(int status)
{
    if (status == 503)
        return QObject::tr("The server cannot read the tags of the note folder.");

    if (status == 403)
        return QObject::tr("The tags of the note folder are read-only.");

    if (status == 423) {
        return QObject::tr("The tags of the note folder are locked on the server. "
                           "The change is waiting.");
    }
    if (status == 429)
        return QObject::tr("The server is taking too many tag changes. Try again shortly.");

    return QObject::tr("The server would not change the tags.");
}

QVector<TrashedNote> trashedNotesOf(const QJsonArray &entries)
{
    QVector<TrashedNote> notes;
    for (int i = 0; i < entries.size(); ++i) {
        const QJsonObject entry = entries.at(i).toObject();
        TrashedNote note;
        note.title = entry.value(QStringLiteral("title")).toString();
        note.fileName = entry.value(QStringLiteral("fileName")).toString();
        note.subFolderPath = entry.value(QStringLiteral("subFolderPath")).toString();
        note.originalLocation = entry.value(QStringLiteral("originalLocation")).toString();
        note.deleted = entry.value(QStringLiteral("deleted")).toVariant().toLongLong();
        // Where it was is how it is put back, so one that says nothing of
        // that is one nothing could be done with.
        if (!note.originalLocation.isEmpty())
            notes.append(note);
    }

    return notes;
}

// What a status other than success means for the trash. The app answers 503
// where the Deleted files app is off, which is a server that has no trash
// rather than one that went wrong.
QString trashStatusError(int status)
{
    if (status == 503)
        return QObject::tr("The server keeps no trash for the notes.");

    if (status == 403)
        return QObject::tr("The note folder is read-only, so nothing can be put back into it.");

    if (status == 404)
        return QObject::tr("The server no longer has that deleted note.");

    if (status == 429)
        return QObject::tr("The server is taking too many restores. Try again shortly.");

    return QObject::tr("The server would not open its trash.");
}

}

QownNotesApi::QownNotesApi(QObject *parent)
    : QObject(parent)
    , m_probing(false)
    , m_outstanding(0)
    , m_tagRequest(NoTagRequest)
    , m_tagNote(0)
    , m_trashRequest(NoTrashRequest)
{
    m_http.setUserAgent(QStringLiteral("Lappu"));
    m_tagHttp.setUserAgent(QStringLiteral("Lappu"));
    m_trashHttp.setUserAgent(QStringLiteral("Lappu"));
    connect(&m_http, &HttpClient::finished, this, &QownNotesApi::onFinished);
    connect(&m_http, &HttpClient::failed, this, &QownNotesApi::onFailed);
    connect(&m_tagHttp, &HttpClient::finished, this, &QownNotesApi::onTagsFinished);
    connect(&m_tagHttp, &HttpClient::failed, this, &QownNotesApi::onTagsFailed);
    connect(&m_trashHttp, &HttpClient::finished, this, &QownNotesApi::onTrashFinished);
    connect(&m_trashHttp, &HttpClient::failed, this, &QownNotesApi::onTrashFailed);
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
    m_tagHttp.setBasicAuth(user, password);
    m_trashHttp.setBasicAuth(user, password);
}

void QownNotesApi::setIgnoreSslErrors(bool ignore)
{
    m_http.setIgnoreSslErrors(ignore);
    m_tagHttp.setIgnoreSslErrors(ignore);
    m_trashHttp.setIgnoreSslErrors(ignore);
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

// The base goes with the probe: an account signed out of is a server nothing
// more is asked of, tags and trash included.
void QownNotesApi::forget()
{
    m_probing = false;
    m_apiBase.clear();
    m_tagRequest = NoTagRequest;
    m_tagNote = 0;
    m_trashRequest = NoTrashRequest;
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

    report(appsOf(app.toObject()));
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

// A server carrying nothing, which is what every answer but the one naming
// the app comes to.
void QownNotesApi::reportAbsent()
{
    report(ServerApps());
}

void QownNotesApi::report(const ServerApps &apps)
{
    m_probing = false;
    m_apiBase = apps.apiBase;
    emit probed(apps);
}

void QownNotesApi::fetchTags()
{
    sendTagRequest(TagTree, "GET", QStringLiteral("tags"));
}

void QownNotesApi::fetchTagLinks()
{
    sendTagRequest(TagLinks, "GET", QStringLiteral("tag-links"));
}

void QownNotesApi::setNoteTags(qint64 noteId, const QStringList &paths, const QString &etag)
{
    QJsonArray wanted;
    for (int i = 0; i < paths.size(); ++i)
        wanted.append(paths.at(i));

    QJsonObject body;
    body.insert(QStringLiteral("tagPaths"), wanted);
    m_tagNote = noteId;
    sendTagRequest(NoteTags, "PUT",
                   QStringLiteral("note/%1/tags").arg(QString::number(noteId)),
                   QJsonDocument(body).toJson(QJsonDocument::Compact), etag);
}

// The tag endpoints hand their version out bare and compare the header
// against it as it was given, unlike the Notes app, which compares one
// wrapped in the quotes of an HTTP tag. An empty version sends no header,
// which the server takes as a change made to the file as it now stands.
void QownNotesApi::sendTagRequest(TagRequest request, const QByteArray &method,
                                  const QString &path, const QByteArray &body,
                                  const QString &etag)
{
    if (m_apiBase.isEmpty())
        return;

    m_tagRequest = request;
    HttpClient::Headers headers;
    headers.append(qMakePair(QByteArray("Accept"), QByteArray("application/json")));
    if (!body.isEmpty())
        headers.append(qMakePair(QByteArray("Content-Type"), QByteArray("application/json")));

    if (!etag.isEmpty())
        headers.append(qMakePair(QByteArray("If-Match"), etag.toUtf8()));

    m_tagHttp.send(method, QUrl(m_server + m_apiBase + path), body, headers);
}

void QownNotesApi::onTagsFinished(int status, const QByteArray &body)
{
    const TagRequest asked = m_tagRequest;
    m_tagRequest = NoTagRequest;
    if (status < 200 || status >= 300) {
        reportTagStatus(status);
        return;
    }

    if (asked == TagTree)
        reportTagTree(body);
    else if (asked == TagLinks)
        reportTagLinks(body);
    else if (asked == NoteTags)
        emit noteTagsSet(m_tagNote);
}

void QownNotesApi::onTagsFailed(const QString &error)
{
    m_tagRequest = NoTagRequest;
    emit tagsFailed(error);
}

void QownNotesApi::reportTagTree(const QByteArray &body)
{
    const QJsonObject answer = QJsonDocument::fromJson(body).object();
    QVector<NoteTag> tags;
    collectTags(answer.value(QStringLiteral("tags")).toArray(), QString(), &tags);
    emit tagsReady(answer.value(QStringLiteral("etag")).toString(),
                   answer.value(QStringLiteral("writable")).toBool(), tags);
}

void QownNotesApi::reportTagLinks(const QByteArray &body)
{
    const QJsonObject answer = QJsonDocument::fromJson(body).object();
    emit tagLinksReady(answer.value(QStringLiteral("etag")).toString(),
                       linksOf(answer.value(QStringLiteral("links")).toArray()));
}

// A refused password is the account's business rather than the tags', so it
// goes where every other one does and the tags are left unspoken for.
void QownNotesApi::reportTagStatus(int status)
{
    if (status == 401) {
        emit unauthorized(NotesApi::passwordRefusedError());
        return;
    }

    if (status == 412) {
        emit noteTagsStale(m_tagNote);
        return;
    }

    emit tagsFailed(tagStatusError(status));
}

// The text of a deleted note is not read: nothing here shows it, and asking
// for it would have the server open every file in the trash.
void QownNotesApi::fetchTrash()
{
    sendTrashRequest(TrashList, "GET", QStringLiteral("trash?content=0"));
}

void QownNotesApi::restoreTrashed(const QString &originalLocation, qint64 deleted)
{
    QJsonObject body;
    body.insert(QStringLiteral("originalLocation"), originalLocation);
    body.insert(QStringLiteral("deleted"), QJsonValue(static_cast<double>(deleted)));
    sendTrashRequest(TrashRestore, "POST", QStringLiteral("trash/restore"),
                     QJsonDocument(body).toJson(QJsonDocument::Compact));
}

// A restore says so to the tags as well: a note that went to the trash left
// its tag links behind as stale ones, and the header is what has the server
// make them its own again rather than returning the note bare.
void QownNotesApi::sendTrashRequest(TrashRequest request, const QByteArray &method,
                                    const QString &path, const QByteArray &body)
{
    if (m_apiBase.isEmpty())
        return;

    m_trashRequest = request;
    HttpClient::Headers headers;
    headers.append(qMakePair(QByteArray("Accept"), QByteArray("application/json")));
    if (!body.isEmpty())
        headers.append(qMakePair(QByteArray("Content-Type"), QByteArray("application/json")));

    if (request == TrashRestore)
        headers.append(qMakePair(QByteArray("X-QOwnNotes-Relink-Tags"), QByteArray("1")));

    m_trashHttp.send(method, QUrl(m_server + m_apiBase + path), body, headers);
}

void QownNotesApi::onTrashFinished(int status, const QByteArray &body)
{
    const TrashRequest asked = m_trashRequest;
    m_trashRequest = NoTrashRequest;
    if (status < 200 || status >= 300) {
        reportTrashStatus(status);
        return;
    }

    const QJsonObject answer = QJsonDocument::fromJson(body).object();
    if (asked == TrashList) {
        emit trashReady(trashedNotesOf(answer.value(QStringLiteral("notes")).toArray()));
        return;
    }

    // A note restored where it is no note of the folder - an ignored place,
    // or one the server would not list - comes back without an id. It is
    // still out of the trash, which is what was asked for.
    if (asked == TrashRestore)
        emit trashRestored(answer.value(QStringLiteral("id")).toVariant().toLongLong());
}

void QownNotesApi::onTrashFailed(const QString &error)
{
    m_trashRequest = NoTrashRequest;
    emit trashFailed(error);
}

void QownNotesApi::reportTrashStatus(int status)
{
    if (status == 401) {
        emit unauthorized(NotesApi::passwordRefusedError());
        return;
    }

    emit trashFailed(trashStatusError(status));
}
