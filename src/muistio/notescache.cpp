#include "notescache.h"

#include <QDir>
#include <QStandardPaths>
#include <QVariant>

namespace {

QString databasePath()
{
    const QString dir = QStandardPaths::writableLocation(QStandardPaths::AppDataLocation);
    QDir().mkpath(dir);
    return dir + QStringLiteral("/notes.sqlite");
}

}

NotesCache::NotesCache(QObject *parent)
    : QObject(parent)
{
}

bool NotesCache::open()
{
    if (m_db.isOpen())
        return true;

    m_db = QSqlDatabase::addDatabase(QStringLiteral("QSQLITE"),
                                     QStringLiteral("muistio-notes"));
    m_db.setDatabaseName(databasePath());
    if (!m_db.open())
        return false;

    return migrate();
}

bool NotesCache::isOpen() const
{
    return m_db.isOpen();
}

bool NotesCache::migrate()
{
    QSqlQuery query(m_db);
    return query.exec(QStringLiteral("CREATE TABLE IF NOT EXISTS notes ("
                                     "id INTEGER PRIMARY KEY,"
                                     "etag TEXT,"
                                     "title TEXT,"
                                     "content TEXT,"
                                     "category TEXT,"
                                     "favorite INTEGER,"
                                     "readonly INTEGER,"
                                     "modified INTEGER)"));
}

void NotesCache::replaceAll(const QVector<LocalNote> &notes)
{
    if (!isOpen())
        return;

    m_db.transaction();
    QSqlQuery wipe(m_db);
    wipe.exec(QStringLiteral("DELETE FROM notes"));
    for (int i = 0; i < notes.size(); ++i)
        insert(notes.at(i));

    m_db.commit();
}

void NotesCache::insert(const LocalNote &note)
{
    QSqlQuery query(m_db);
    query.prepare(QStringLiteral("INSERT OR REPLACE INTO notes "
                                 "(id, etag, title, content, category, favorite,"
                                 " readonly, modified) "
                                 "VALUES (?, ?, ?, ?, ?, ?, ?, ?)"));
    query.addBindValue(note.id);
    query.addBindValue(note.etag);
    query.addBindValue(note.title);
    query.addBindValue(note.content);
    query.addBindValue(note.category);
    query.addBindValue(note.favorite);
    query.addBindValue(note.readOnly);
    query.addBindValue(note.modified);
    query.exec();
}

QVector<LocalNote> NotesCache::notes() const
{
    QVector<LocalNote> notes;
    if (!isOpen())
        return notes;

    QSqlQuery query(m_db);
    if (!query.exec(QStringLiteral("SELECT id, etag, title, content, category, favorite,"
                                   " readonly, modified FROM notes ORDER BY modified DESC")))
        return notes;

    while (query.next())
        notes.append(readNote(query));

    return notes;
}

LocalNote NotesCache::readNote(const QSqlQuery &query) const
{
    LocalNote note;
    note.id = query.value(0).toString();
    note.etag = query.value(1).toString();
    note.title = query.value(2).toString();
    note.content = query.value(3).toString();
    note.category = query.value(4).toString();
    note.favorite = query.value(5).toBool();
    note.readOnly = query.value(6).toBool();
    note.modified = query.value(7).toLongLong();
    return note;
}

void NotesCache::clear()
{
    if (!isOpen())
        return;

    QSqlQuery query(m_db);
    query.exec(QStringLiteral("DELETE FROM notes"));
}
