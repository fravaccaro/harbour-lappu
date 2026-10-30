#include "notesmodel.h"

#include <QVariantMap>

namespace {

QVariantMap mapOf(const LocalNote &note)
{
    QVariantMap map;
    map.insert(QStringLiteral("noteId"), note.id);
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
    : QObject(parent)
{
}

void NotesModel::setNotes(const QVector<LocalNote> &notes)
{
    m_notes = notes;
    emit changed();
}

void NotesModel::reload()
{
    m_notes.clear();
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
