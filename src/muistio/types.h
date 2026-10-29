#ifndef MUISTIO_TYPES_H
#define MUISTIO_TYPES_H

#include <QString>

// One note as the device keeps it. The Notes API fills these in; the scaffold
// carries the shape so the model and pages can already name the fields.
struct LocalNote
{
    QString id;
    QString title;
    QString content;
    QString category;
    QString etag;
    bool favorite;
    // Seconds since the epoch, as the Notes API reports the last change.
    qint64 modified;

    LocalNote()
        : favorite(false)
        , modified(0)
    {
    }
};

#endif
