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

// An edit of ours still owed to the server outranks what it sent - unless the
// server already says exactly what we were going to tell it, which is a write
// that landed and whose answer never reached us. Taking the note then settles
// the row, where leaving it owed would offer the same edit again against a
// version that has since moved on and earn a conflict worth nobody's time. A
// delete is never settled this way: the note still being listed is the whole
// of what is owed.
bool stillOwed(const LocalNote &mine, const LocalNote &theirs)
{
    if (!mine.dirty && !mine.deleted)
        return false;

    return mine.deleted || !sameNote(mine, theirs);
}

// The tags are the server's to keep: the tree and the links are replaced
// whole by every read, and only what the user has set and the server has not
// taken yet is the device's own. The version of the tag database the tree was
// read at lives in a table of one row, since it belongs to none of them.
QStringList tagTableSql()
{
    QStringList statements;
    statements.append(QStringLiteral("CREATE TABLE IF NOT EXISTS tags ("
                                     "id INTEGER PRIMARY KEY,"
                                     "path TEXT)"));
    statements.append(QStringLiteral("CREATE TABLE IF NOT EXISTS note_tags ("
                                     "note_id INTEGER,"
                                     "tag_id INTEGER,"
                                     "PRIMARY KEY (note_id, tag_id))"));
    statements.append(QStringLiteral("CREATE TABLE IF NOT EXISTS owed_tags ("
                                     "note_id INTEGER PRIMARY KEY,"
                                     "paths TEXT)"));
    statements.append(QStringLiteral("CREATE TABLE IF NOT EXISTS tag_state ("
                                     "id INTEGER PRIMARY KEY,"
                                     "etag TEXT,"
                                     "writable INTEGER DEFAULT 1)"));
    return statements;
}

// The folders below the note folder are the server's as well, and are
// replaced whole by every read. Only the path is kept: a folder is counted
// by the notes this device holds, and one holding none is a count of zero.
QString folderTableSql()
{
    return QStringLiteral("CREATE TABLE IF NOT EXISTS folders (path TEXT PRIMARY KEY)");
}

// A tag path cannot hold a newline, the server naming a tag on one line, so
// the paths owed on a note are kept as the lines of one field.
QString joinPaths(const QStringList &paths)
{
    return paths.join(QStringLiteral("\n"));
}

QStringList splitPaths(const QString &field)
{
    if (field.isEmpty())
        return QStringList();

    return field.split(QLatin1Char('\n'), QString::SkipEmptyParts);
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
                                     QStringLiteral("lappu-notes"));
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

    if (!createTagTables())
        return false;

    if (!query.exec(folderTableSql()))
        return false;

    // A table left by a build from before the queue has no room for it. The
    // next pull fills the cache again, so it can simply start over.
    if (query.exec(QStringLiteral("SELECT dirty FROM notes LIMIT 1")))
        return true;

    query.exec(QStringLiteral("DROP TABLE notes"));
    return query.exec(createSql());
}

bool NotesCache::createTagTables()
{
    const QStringList statements = tagTableSql();
    QSqlQuery query(m_db);
    for (int i = 0; i < statements.size(); ++i) {
        if (!query.exec(statements.at(i)))
            return false;
    }

    return true;
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

// The edit is let go of and the row is left holding what it holds until the
// next pull writes the server's version over it. The delete goes with it: a
// note whose delete the server refused is still there to be shown.
void NotesCache::abandonOwed(qint64 id)
{
    if (!isOpen())
        return;

    QSqlQuery query(m_db);
    query.prepare(QStringLiteral("UPDATE notes SET dirty = 0, deleted = 0 WHERE id = ?"));
    query.addBindValue(id);
    query.exec();
}

void NotesCache::setEtag(qint64 id, const QString &etag)
{
    if (!isOpen())
        return;

    QSqlQuery query(m_db);
    query.prepare(QStringLiteral("UPDATE notes SET etag = ? WHERE id = ?"));
    query.addBindValue(etag);
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
    if (oldId != settled.id) {
        // Tags set on a note the server had not yet named are owed on the id
        // it has just been given, the row they were written against being
        // about to go.
        moveOwedTags(oldId, settled.id);
        remove(oldId);
    }

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
    // A note that is gone carries nothing, and owes nothing on what it used
    // to carry.
    forgetTagsOf(id);
}

void NotesCache::forgetTagsOf(qint64 noteId)
{
    QSqlQuery query(m_db);
    query.prepare(QStringLiteral("DELETE FROM note_tags WHERE note_id = ?"));
    query.addBindValue(noteId);
    query.exec();
    clearOwedTags(noteId);
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
        if (mine.id != 0 && stillOwed(mine, notes.at(i)))
            continue;

        write(notes.at(i));
    }
    dropMissing(notes);
    dropOrphanTags();
    m_db.commit();
}

// Tags of notes that are no longer here. A note deleted from this device
// takes its own with it, but one taken away elsewhere is only noticed by a
// pull, and tags owed on it would be offered to a server that has no such
// note to put them on.
void NotesCache::dropOrphanTags()
{
    QSqlQuery query(m_db);
    query.exec(QStringLiteral("DELETE FROM note_tags"
                              " WHERE note_id NOT IN (SELECT id FROM notes)"));
    query.exec(QStringLiteral("DELETE FROM owed_tags"
                              " WHERE note_id NOT IN (SELECT id FROM notes)"));
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

void NotesCache::applyTags(const QVector<NoteTag> &tags, const QString &etag, bool writable)
{
    if (!isOpen())
        return;

    m_db.transaction();
    QSqlQuery query(m_db);
    query.exec(QStringLiteral("DELETE FROM tags"));
    query.prepare(QStringLiteral("INSERT OR REPLACE INTO tags (id, path) VALUES (?, ?)"));
    for (int i = 0; i < tags.size(); ++i) {
        query.addBindValue(tags.at(i).id);
        query.addBindValue(tags.at(i).path);
        query.exec();
    }
    writeTagState(etag, writable);
    m_db.commit();
}

void NotesCache::writeTagState(const QString &etag, bool writable)
{
    QSqlQuery query(m_db);
    query.prepare(QStringLiteral("INSERT OR REPLACE INTO tag_state (id, etag, writable)"
                                 " VALUES (1, ?, ?)"));
    query.addBindValue(etag);
    query.addBindValue(writable);
    query.exec();
}

void NotesCache::applyTagLinks(const QVector<TagLink> &links)
{
    if (!isOpen())
        return;

    m_db.transaction();
    QSqlQuery query(m_db);
    query.exec(QStringLiteral("DELETE FROM note_tags"));
    query.prepare(QStringLiteral("INSERT OR REPLACE INTO note_tags (note_id, tag_id)"
                                 " VALUES (?, ?)"));
    for (int i = 0; i < links.size(); ++i) {
        query.addBindValue(links.at(i).noteId);
        query.addBindValue(links.at(i).tagId);
        query.exec();
    }
    m_db.commit();
}

QVector<NoteTag> NotesCache::tags() const
{
    QVector<NoteTag> tags;
    if (!isOpen())
        return tags;

    QSqlQuery query(m_db);
    if (!query.exec(QStringLiteral("SELECT id, path FROM tags ORDER BY path")))
        return tags;

    while (query.next()) {
        NoteTag tag;
        tag.id = query.value(0).toLongLong();
        tag.path = query.value(1).toString();
        tags.append(tag);
    }

    return tags;
}

// The listing the server last gave, in its place: a folder it no longer has
// is one no view should offer.
void NotesCache::applyFolders(const QStringList &paths)
{
    if (!isOpen())
        return;

    m_db.transaction();
    QSqlQuery query(m_db);
    query.exec(QStringLiteral("DELETE FROM folders"));
    query.prepare(QStringLiteral("INSERT OR REPLACE INTO folders (path) VALUES (?)"));
    for (int i = 0; i < paths.size(); ++i) {
        query.addBindValue(paths.at(i));
        query.exec();
    }
    m_db.commit();
}

QStringList NotesCache::folders() const
{
    QStringList paths;
    if (!isOpen())
        return paths;

    QSqlQuery query(m_db);
    if (!query.exec(QStringLiteral("SELECT path FROM folders")))
        return paths;

    while (query.next())
        paths.append(query.value(0).toString());

    return paths;
}

void NotesCache::clearFolders()
{
    if (!isOpen())
        return;

    QSqlQuery query(m_db);
    query.exec(QStringLiteral("DELETE FROM folders"));
}

QString NotesCache::tagEtag() const
{
    if (!isOpen())
        return QString();

    QSqlQuery query(m_db);
    if (!query.exec(QStringLiteral("SELECT etag FROM tag_state WHERE id = 1")) || !query.next())
        return QString();

    return query.value(0).toString();
}

// A server that has never been asked is taken to be writable: the first
// change is what finds out, and the answer is kept from then on.
bool NotesCache::tagsWritable() const
{
    if (!isOpen())
        return true;

    QSqlQuery query(m_db);
    if (!query.exec(QStringLiteral("SELECT writable FROM tag_state WHERE id = 1")) || !query.next())
        return true;

    return query.value(0).toBool();
}

QHash<qint64, QStringList> NotesCache::noteTags() const
{
    QHash<qint64, QStringList> carried;
    if (!isOpen())
        return carried;

    QSqlQuery query(m_db);
    if (query.exec(QStringLiteral("SELECT note_tags.note_id, tags.path FROM note_tags"
                                  " JOIN tags ON tags.id = note_tags.tag_id"
                                  " ORDER BY tags.path"))) {
        while (query.next())
            carried[query.value(0).toLongLong()].append(query.value(1).toString());
    }

    if (query.exec(QStringLiteral("SELECT note_id, paths FROM owed_tags"))) {
        while (query.next())
            carried.insert(query.value(0).toLongLong(), splitPaths(query.value(1).toString()));
    }

    return carried;
}

QStringList NotesCache::tagsOf(qint64 noteId) const
{
    if (!isOpen())
        return QStringList();

    if (owesTags(noteId))
        return owedTags(noteId);

    QSqlQuery query(m_db);
    query.prepare(QStringLiteral("SELECT tags.path FROM note_tags"
                                 " JOIN tags ON tags.id = note_tags.tag_id"
                                 " WHERE note_tags.note_id = ? ORDER BY tags.path"));
    query.addBindValue(noteId);
    QStringList paths;
    if (!query.exec())
        return paths;

    while (query.next())
        paths.append(query.value(0).toString());

    return paths;
}

void NotesCache::setOwedTags(qint64 noteId, const QStringList &paths)
{
    if (!isOpen())
        return;

    QSqlQuery query(m_db);
    query.prepare(QStringLiteral("INSERT OR REPLACE INTO owed_tags (note_id, paths)"
                                 " VALUES (?, ?)"));
    query.addBindValue(noteId);
    query.addBindValue(joinPaths(paths));
    query.exec();
}

void NotesCache::clearOwedTags(qint64 noteId)
{
    if (!isOpen())
        return;

    QSqlQuery query(m_db);
    query.prepare(QStringLiteral("DELETE FROM owed_tags WHERE note_id = ?"));
    query.addBindValue(noteId);
    query.exec();
}

// A note may owe the server no tags at all, which is not the same as owing it
// nothing: the row is what says the tags were taken off.
bool NotesCache::owesTags(qint64 noteId) const
{
    if (!isOpen())
        return false;

    QSqlQuery query(m_db);
    query.prepare(QStringLiteral("SELECT 1 FROM owed_tags WHERE note_id = ?"));
    query.addBindValue(noteId);
    return query.exec() && query.next();
}

QStringList NotesCache::owedTags(qint64 noteId) const
{
    if (!isOpen())
        return QStringList();

    QSqlQuery query(m_db);
    query.prepare(QStringLiteral("SELECT paths FROM owed_tags WHERE note_id = ?"));
    query.addBindValue(noteId);
    if (!query.exec() || !query.next())
        return QStringList();

    return splitPaths(query.value(0).toString());
}

QVector<qint64> NotesCache::notesOwingTags() const
{
    QVector<qint64> ids;
    if (!isOpen())
        return ids;

    QSqlQuery query(m_db);
    if (!query.exec(QStringLiteral("SELECT note_id FROM owed_tags ORDER BY rowid")))
        return ids;

    while (query.next())
        ids.append(query.value(0).toLongLong());

    return ids;
}

void NotesCache::moveOwedTags(qint64 oldId, qint64 newId)
{
    if (!isOpen() || oldId == newId || !owesTags(oldId))
        return;

    const QStringList paths = owedTags(oldId);
    clearOwedTags(oldId);
    setOwedTags(newId, paths);
}

void NotesCache::clearTags()
{
    if (!isOpen())
        return;

    QSqlQuery query(m_db);
    query.exec(QStringLiteral("DELETE FROM tags"));
    query.exec(QStringLiteral("DELETE FROM note_tags"));
    query.exec(QStringLiteral("DELETE FROM owed_tags"));
    query.exec(QStringLiteral("DELETE FROM tag_state"));
}
