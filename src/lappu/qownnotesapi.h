#ifndef LAPPU_QOWNNOTESAPI_H
#define LAPPU_QOWNNOTESAPI_H

#include <QObject>
#include <QString>
#include <QStringList>
#include <QVector>

#include "httpclient.h"
#include "types.h"

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

    // The tags of the note folder, which the app keeps in the database its
    // desktop and Android clients keep them in rather than on the notes.
    // Nothing here is asked for before the probe says the app is on the
    // server and can read that database: an empty base sends no request.
    void fetchTags();
    void fetchTagLinks();
    // Sets the tags of one note to exactly these paths. A path the server
    // does not know is created by it, so no tag has to be made first. The
    // version is the one the tag database was last read at, and is sent bare
    // as the tag endpoints hand it out and compare it.
    void setNoteTags(qint64 noteId, const QStringList &paths, const QString &etag);

    // The notes the server is holding in its trash, newest first. The text
    // of each is left unread: nothing here shows it, and a trash can be
    // long. Asked for only when the page that lists them is opened.
    void fetchTrash();
    // Puts one back where it was deleted from, naming it the way the trash
    // lists it. Its tag links come back with it.
    void restoreTrashed(const QString &originalLocation, qint64 deleted);

    // The folders below the note folder, which only this app can name: the
    // Notes API knows a folder by the notes in it, so one holding none is
    // invisible there.
    void fetchSubfolders();

signals:
    // What the server carries. Every answer but the one that names the app,
    // an unreachable server included, is reported as carrying nothing: a
    // feature nothing has confirmed is one worth leaving out of the way.
    void probed(const ServerApps &apps);
    // A password the server will not take. It travels apart from probed()
    // because it is about the account rather than the app, and sends the user
    // back to signing in; the app is reported absent alongside it.
    void unauthorized(const QString &error);
    // The tag tree as paths, with the version of the database it was read at.
    // A database the server may read but not write is still handed over:
    // tags are shown either way, and only a change is turned away.
    void tagsReady(const QString &etag, bool writable, const QVector<NoteTag> &tags);
    void tagLinksReady(const QString &etag, const QVector<TagLink> &links);
    void noteTagsSet(qint64 noteId);
    // The tag database moved while the change was being made, so what went
    // out was built on a version the server no longer holds. It travels apart
    // from tagsFailed() because nothing is wrong and nothing is lost: the
    // tags are read again and the change is offered against what arrives.
    void noteTagsStale(qint64 noteId);
    void tagsFailed(const QString &error);
    void trashReady(const QVector<TrashedNote> &notes);
    // The note is a note again, under the id the server answered with - or
    // under none, where it was restored somewhere that is no note at all.
    void trashRestored(qint64 noteId);
    void trashFailed(const QString &error);
    // Every folder below the note folder, as the path a note's category is
    // written with. A read that goes wrong is not reported: the folders are
    // read again with the next pull, and the notes sync says whatever is to
    // be said about the server.
    void foldersReady(const QStringList &paths);

private:
    // What the tag client last asked for, so its answer is read as the thing
    // it was asked for. One tag request is in flight at a time.
    enum TagRequest {
        NoTagRequest,
        TagTree,
        TagLinks,
        NoteTags
    };

    // The same, for the trash. One request at a time there too.
    enum TrashRequest {
        NoTrashRequest,
        TrashList,
        TrashRestore
    };

    void onFinished(int status, const QByteArray &body);
    void onFailed(const QString &error);
    // Whether the answer now arriving is the one this probe is waiting for.
    // A request disowned by forget() goes on travelling, so a reply is only
    // taken when nothing newer is still on its way.
    bool takeReply();
    void reportAbsent();
    void report(const ServerApps &apps);
    void sendTagRequest(TagRequest request, const QByteArray &method, const QString &path,
                        const QByteArray &body = QByteArray(),
                        const QString &etag = QString());
    void onTagsFinished(int status, const QByteArray &body);
    void onTagsFailed(const QString &error);
    void reportTagTree(const QByteArray &body);
    void reportTagLinks(const QByteArray &body);
    void reportTagStatus(int status);
    void sendTrashRequest(TrashRequest request, const QByteArray &method, const QString &path,
                          const QByteArray &body = QByteArray());
    void onTrashFinished(int status, const QByteArray &body);
    void onTrashFailed(const QString &error);
    void reportTrashStatus(int status);
    void onFoldersFinished(int status, const QByteArray &body);
    void onFoldersFailed(const QString &error);

    HttpClient m_http;
    // The tags travel on a client of their own, so a probe and a tag request
    // cannot be taken for one another's answer. The trash and the folders
    // have one each for the same reason: a read of one is sent while a read
    // of another may still be travelling.
    HttpClient m_tagHttp;
    HttpClient m_trashHttp;
    HttpClient m_folderHttp;
    QString m_server;
    // Where the app said it serves its APIs, and so where the tags are asked
    // for. Empty until a probe answers for the app, and again once it is
    // forgotten: a request with nowhere to go is not made.
    QString m_apiBase;
    bool m_probing;
    // Requests sent and not yet answered.
    int m_outstanding;
    TagRequest m_tagRequest;
    // The note whose tags are being written, or 0 while nothing is.
    qint64 m_tagNote;
    TrashRequest m_trashRequest;
    // Whether a folder read is on its way. One is asked for at a time, and
    // there is only the one kind of folder request to tell apart.
    bool m_folderRequest;
};

#endif
