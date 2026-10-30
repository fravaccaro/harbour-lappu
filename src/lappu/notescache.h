#ifndef LAPPU_NOTESCACHE_H
#define LAPPU_NOTESCACHE_H

#include <QObject>
#include <QSqlDatabase>
#include <QSqlQuery>
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
    // Adopts a version onto a row without settling it, so an edit still owed
    // to the server is offered again against the version the server has.
    void setEtag(qint64 id, const QString &etag);
    // The server has the note now, under the id it chose for it.
    void markClean(qint64 oldId, const LocalNote &note);
    void remove(qint64 id);
    void clear();

    // Takes a pull without losing the edits still owed to the server.
    void applyRemote(const QVector<LocalNote> &notes);

private:
    bool migrate();
    void write(const LocalNote &note);
    LocalNote readNote(const QSqlQuery &query) const;
    QVector<LocalNote> select(const QString &where) const;
    void dropMissing(const QVector<LocalNote> &notes);

    QSqlDatabase m_db;
};

#endif
