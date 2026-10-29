#ifndef MUISTIO_NOTESMODEL_H
#define MUISTIO_NOTESMODEL_H

#include <QAbstractListModel>
#include <QHash>
#include <QVector>

#include "types.h"

class NotesModel : public QAbstractListModel
{
    Q_OBJECT

public:
    enum Roles {
        IdRole = Qt::UserRole + 1,
        TitleRole,
        CategoryRole,
        FavoriteRole,
        ModifiedRole
    };

    explicit NotesModel(QObject *parent = nullptr);

    int rowCount(const QModelIndex &parent = QModelIndex()) const override;
    QVariant data(const QModelIndex &index, int role) const override;
    QHash<int, QByteArray> roleNames() const override;

    // The notes store arrives with the Notes API; until then this empties the
    // model so the list page can already bind to it.
    void reload();

private:
    QVector<LocalNote> m_notes;
};

#endif
