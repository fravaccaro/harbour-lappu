#include "notesmodel.h"

#include <QVariantMap>

namespace {

QVariantMap mapOf(const LocalNote &note)
{
    QVariantMap map;
    map.insert(QStringLiteral("noteId"), note.id.toLongLong());
    map.insert(QStringLiteral("title"), note.title);
    map.insert(QStringLiteral("content"), note.content);
    map.insert(QStringLiteral("category"), note.category);
    map.insert(QStringLiteral("favorite"), note.favorite);
    map.insert(QStringLiteral("locked"), note.readOnly);
    map.insert(QStringLiteral("modified"), note.modified);
    return map;
}

}

NotesModel::NotesModel(QObject *parent)
    : QAbstractListModel(parent)
{
}

int NotesModel::rowCount(const QModelIndex &parent) const
{
    return parent.isValid() ? 0 : m_notes.size();
}

QHash<int, QByteArray> NotesModel::roleNames() const
{
    QHash<int, QByteArray> roles;
    roles.insert(IdRole, "noteId");
    roles.insert(TitleRole, "title");
    roles.insert(ContentRole, "content");
    roles.insert(CategoryRole, "category");
    roles.insert(FavoriteRole, "favorite");
    roles.insert(ReadOnlyRole, "locked");
    roles.insert(ModifiedRole, "modified");
    return roles;
}

QVariant NotesModel::data(const QModelIndex &index, int role) const
{
    if (!index.isValid() || index.row() >= m_notes.size())
        return QVariant();

    const LocalNote &note = m_notes.at(index.row());
    switch (role) {
    case IdRole:
        return note.id;
    case TitleRole:
        return note.title;
    case ContentRole:
        return note.content;
    case CategoryRole:
        return note.category;
    case FavoriteRole:
        return note.favorite;
    case ReadOnlyRole:
        return note.readOnly;
    case ModifiedRole:
        return note.modified;
    default:
        return QVariant();
    }
}

void NotesModel::setNotes(const QVector<LocalNote> &notes)
{
    beginResetModel();
    m_notes = notes;
    endResetModel();
    emit changed();
}

void NotesModel::reload()
{
    beginResetModel();
    m_notes.clear();
    endResetModel();
    emit changed();
}

QVariantList NotesModel::notes() const
{
    QVariantList list;
    list.reserve(m_notes.size());
    for (int i = 0; i < m_notes.size(); ++i)
        list.append(mapOf(m_notes.at(i)));

    return list;
}
