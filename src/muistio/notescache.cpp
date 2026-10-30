#include "notescache.h"

#include <QDir>
#include <QStandardPaths>
#include <QStringList>
#include <QVariant>

namespace {

QString databasePath()
{
    const QString dir = QStandardPaths::writableLocation(QStandardPaths::AppDataLocation);
    QDir().mkpath(dir);
    return dir + QStringLiteral("/notes.sqlite");
}

QString columns()
{
    return QStringLiteral("id, etag, title, content, category, favorite,"
                          " readonly, dirty, deleted, modified");
}

QString createSql()
{
    return QStringLiteral("CREATE TABLE IF NOT EXISTS notes ("
                          "id INTEGER PRIMARY KEY,"
                          "etag TEXT,"
                          "title TEXT,"
                          "content TEXT,"
                          "category TEXT,"
                          "favorite INTEGER DEFAULT 0,"
                          "readonly INTEGER DEFAULT 0,"
                          "dirty INTEGER DEFAULT 0,"
                          "deleted INTEGER DEFAULT 0,"
                          "modified INTEGER DEFAULT 0)");
}

QStringList idList(const QVector<LocalNote> &notes)
{
    QStringList ids;
    for (int i = 0; i < notes.size(); ++i)
        ids.append(QString::number(notes.at(i).id));

    return ids;
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
    if (!query.exec(createSql()))
        return false;

    // A table left by a build from before the queue has no room for it. The
    // next pull fills the cache again, so it can simply start over.
    if (query.exec(QStringLiteral("SELECT dirty FROM notes LIMIT 1")))
        return true;

    query.exec(QStringLiteral("DROP TABLE notes"));
    return query.exec(createSql());
}

QVector<LocalNote> NotesCache::notes() const
{
    return select(QStringLiteral("deleted = 0 ORDER BY modified DESC"));
}

QVector<LocalNote> NotesCache::dirtyNotes() const
{
    return select(QStringLiteral("dirty = 1 ORDER BY modified ASC"));
}

LocalNote NotesCache::note(qint64 id) const
{
    const QVector<LocalNote> found =
            select(QStringLiteral("id = %1").arg(QString::number(id)));
    return found.isEmpty() ? LocalNote() : found.first();
}

QVector<LocalNote> NotesCache::select(const QString &where) const
{
    QVector<LocalNote> notes;
    if (!isOpen())
        return notes;

    QSqlQuery query(m_db);
    if (!query.exec(QStringLiteral("SELECT ") + columns()
                    + QStringLiteral(" FROM notes WHERE ") + where))
        return notes;

    while (query.next())
        notes.append(readNote(query));

    return notes;
}

LocalNote NotesCache::readNote(const QSqlQuery &query) const
{
    LocalNote note;
    note.id = query.value(0).toLongLong();
    note.etag = query.value(1).toString();
    note.title = query.value(2).toString();
    note.content = query.value(3).toString();
    note.category = query.value(4).toString();
    note.favorite = query.value(5).toBool();
    note.readOnly = query.value(6).toBool();
    note.dirty = query.value(7).toBool();
    note.deleted = query.value(8).toBool();
    note.modified = query.value(9).toLongLong();
    return note;
}

void NotesCache::write(const LocalNote &note)
{
    QSqlQuery query(m_db);
    query.prepare(QStringLiteral("INSERT OR REPLACE INTO notes (") + columns()
                  + QStringLiteral(") VALUES (?, ?, ?, ?, ?, ?, ?, ?, ?, ?)"));
    query.addBindValue(note.id);
    query.addBindValue(note.etag);
    query.addBindValue(note.title);
    query.addBindValue(note.content);
    query.addBindValue(note.category);
    query.addBindValue(note.favorite);
    query.addBindValue(note.readOnly);
    query.addBindValue(note.dirty);
    query.addBindValue(note.deleted);
    query.addBindValue(note.modified);
    query.exec();
}

void NotesCache::upsertLocal(const LocalNote &note)
{
    if (!isOpen())
        return;

    LocalNote owed = note;
    owed.dirty = true;
    write(owed);
}

qint64 NotesCache::nextTempId() const
{
    if (!isOpen())
        return -1;

    QSqlQuery query(m_db);
    if (!query.exec(QStringLiteral("SELECT MIN(id) FROM notes")) || !query.next())
        return -1;

    return qMin(query.value(0).toLongLong(), Q_INT64_C(0)) - 1;
}

void NotesCache::markDeleted(qint64 id)
{
    if (!isOpen())
        return;

    QSqlQuery query(m_db);
    query.prepare(QStringLiteral("UPDATE notes SET deleted = 1, dirty = 1 WHERE id = ?"));
    query.addBindValue(id);
    query.exec();
}

void NotesCache::markClean(qint64 oldId, const LocalNote &note)
{
    if (!isOpen())
        return;

    LocalNote settled = note;
    settled.dirty = false;
    settled.deleted = false;
    if (oldId != settled.id)
        remove(oldId);

    write(settled);
}

void NotesCache::remove(qint64 id)
{
    if (!isOpen())
        return;

    QSqlQuery query(m_db);
    query.prepare(QStringLiteral("DELETE FROM notes WHERE id = ?"));
    query.addBindValue(id);
    query.exec();
}

void NotesCache::clear()
{
    if (!isOpen())
        return;

    QSqlQuery query(m_db);
    query.exec(QStringLiteral("DELETE FROM notes"));
}

void NotesCache::applyRemote(const QVector<LocalNote> &notes)
{
    if (!isOpen())
        return;

    m_db.transaction();
    for (int i = 0; i < notes.size(); ++i) {
        const LocalNote mine = note(notes.at(i).id);
        // An edit of ours still owed to the server outranks what it sent.
        if (mine.id != 0 && (mine.dirty || mine.deleted))
            continue;

        write(notes.at(i));
    }
    dropMissing(notes);
    m_db.commit();
}

// Notes the server no longer lists are gone, unless they are only here so far.
void NotesCache::dropMissing(const QVector<LocalNote> &notes)
{
    QString sql = QStringLiteral("DELETE FROM notes WHERE dirty = 0 AND deleted = 0");
    const QStringList ids = idList(notes);
    if (!ids.isEmpty())
        sql += QStringLiteral(" AND id NOT IN (") + ids.join(QStringLiteral(",")) + QLatin1Char(')');

    QSqlQuery query(m_db);
    query.exec(sql);
}
