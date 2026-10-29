#include "notesmodel.h"

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
    roles.insert(CategoryRole, "category");
    roles.insert(FavoriteRole, "favorite");
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
    case CategoryRole:
        return note.category;
    case FavoriteRole:
        return note.favorite;
    case ModifiedRole:
        return note.modified;
    default:
        return QVariant();
    }
}

void NotesModel::reload()
{
    beginResetModel();
    m_notes.clear();
    endResetModel();
}
