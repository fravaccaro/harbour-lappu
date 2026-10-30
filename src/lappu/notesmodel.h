#ifndef LAPPU_NOTESMODEL_H
#define LAPPU_NOTESMODEL_H

#include <QObject>
#include <QVariantList>
#include <QVector>

#include "types.h"

// Holds the notes of the last pull for QML to take a copy of. Despite the name
// it is not a list model: no view binds to it, because the QML note store owns
// the filtering and sorting and needs the notes as plain rows to do it.
class NotesModel : public QObject
{
    Q_OBJECT

public:
    explicit NotesModel(QObject *parent = nullptr);

    void setNotes(const QVector<LocalNote> &notes);
    void reload();

    // Every note as a plain map, keyed the way the QML note store names its
    // fields, so a page can keep its own filtered view of the same notes.
    Q_INVOKABLE QVariantList notes() const;

signals:
    void changed();

private:
    QVector<LocalNote> m_notes;
};

#endif
