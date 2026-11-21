#ifndef LAPPU_NOTESCACHE_H
#define LAPPU_NOTESCACHE_H

#include <QHash>
#include <QObject>
#include <QSqlDatabase>
#include <QSqlQuery>
#include <QStringList>
#include <QVector>

#include "types.h"

// Keeps the notes on the device: the last pull, so the pages have something
// to show before the server answers, and the edits the server has not been
// told about yet.
class NotesCache : public QObject
{
    Q_OBJECT

public:
    explicit NotesCache(QObject *parent = nullptr);

    bool open();
    bool isOpen() const;

    // Every note worth showing, newest first; the ones waiting to be deleted
    // are left out.
    QVector<LocalNote> notes() const;
    LocalNote note(qint64 id) const;
    // The edits waiting for the server, in the order they were made.
    QVector<LocalNote> dirtyNotes() const;

    // Writes an edit made here and marks it as owed to the server.
    void upsertLocal(const LocalNote &note);
    // A note written here has no server id yet, so it borrows a negative one.
    qint64 nextTempId() const;
    void markDeleted(qint64 id);
    // Lets go of what is owed on one note without settling it against any
    // version, so the next pull is free to write the server's over it.
    void abandonOwed(qint64 id);
    // Adopts a version onto a row without settling it, so an edit still owed
    // to the server is offered again against the version the server has.
    void setEtag(qint64 id, const QString &etag);
    // The server has the note now, under the id it chose for it.
    void markClean(qint64 oldId, const LocalNote &note);
    void remove(qint64 id);
    void clear();

    // Takes a pull without losing the edits still owed to the server.
    void applyRemote(const QVector<LocalNote> &notes);

    // The tags of the note folder as the server last handed them over, along
    // with the version of the tag database they were read at and whether that
    // database may be written to.
    void applyTags(const QVector<NoteTag> &tags, const QString &etag, bool writable);
    void applyTagLinks(const QVector<TagLink> &links);
    QVector<NoteTag> tags() const;
    QString tagEtag() const;
    bool tagsWritable() const;
    // Every note that carries a tag, each with its tags as paths. A note
    // whose tags are waiting to be sent answers with those rather than with
    // what the server last said, so the pages show the change at once.
    QHash<qint64, QStringList> noteTags() const;
    QStringList tagsOf(qint64 noteId) const;

    // The tags of one note as the user set them, kept until the server takes
    // them. They are written down rather than applied to the links, which are
    // the server's own and are replaced whole by the next read.
    void setOwedTags(qint64 noteId, const QStringList &paths);
    void clearOwedTags(qint64 noteId);
    bool owesTags(qint64 noteId) const;
    QStringList owedTags(qint64 noteId) const;
    // The notes whose tags are owed, oldest first.
    QVector<qint64> notesOwingTags() const;
    // Carries what is owed onto the id the server gave a note, for a note
    // tagged here before it had ever been sent.
    void moveOwedTags(qint64 oldId, qint64 newId);
    // Lets go of every tag the device holds, for an account being left.
    void clearTags();

    // The folders below the note folder as the server last listed them, the
    // empty ones included. They are kept rather than worked out from the
    // notes, which is the one thing that cannot name a folder holding none.
    void applyFolders(const QStringList &paths);
    QStringList folders() const;
    void clearFolders();

private:
    bool migrate();
    bool createTagTables();
    void write(const LocalNote &note);
    void writeTagState(const QString &etag, bool writable);
    LocalNote readNote(const QSqlQuery &query) const;
    QVector<LocalNote> select(const QString &where) const;
    void dropMissing(const QVector<LocalNote> &notes);
    void dropOrphanTags();
    void forgetTagsOf(qint64 noteId);

    QSqlDatabase m_db;
};

#endif
