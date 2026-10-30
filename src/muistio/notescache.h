#ifndef MUISTIO_NOTESCACHE_H
#define MUISTIO_NOTESCACHE_H

#include <QObject>
#include <QSqlDatabase>
#include <QSqlQuery>
#include <QVector>

#include "types.h"

// Keeps the last pull on the device so the pages have something to show
// before the server answers, or while there is no network at all.
class NotesCache : public QObject
{
    Q_OBJECT

public:
    explicit NotesCache(QObject *parent = nullptr);

    bool open();
    bool isOpen() const;

    // A pull reports the whole account, so the cache mirrors it rather than
    // merging note by note.
    void replaceAll(const QVector<LocalNote> &notes);
    QVector<LocalNote> notes() const;
    void clear();

private:
    bool migrate();
    void insert(const LocalNote &note);
    LocalNote readNote(const QSqlQuery &query) const;

    QSqlDatabase m_db;
};

#endif
