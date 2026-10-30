#ifndef MUISTIO_NOTESMODEL_H
#define MUISTIO_NOTESMODEL_H

#include <QAbstractListModel>
#include <QHash>
#include <QVariantList>
#include <QVector>

#include "types.h"

class NotesModel : public QAbstractListModel
{
    Q_OBJECT

public:
    enum Roles {
        IdRole = Qt::UserRole + 1,
        TitleRole,
        ContentRole,
        CategoryRole,
        FavoriteRole,
        ReadOnlyRole,
        ModifiedRole
    };

    explicit NotesModel(QObject *parent = nullptr);

    int rowCount(const QModelIndex &parent = QModelIndex()) const override;
    QVariant data(const QModelIndex &index, int role) const override;
    QHash<int, QByteArray> roleNames() const override;

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
