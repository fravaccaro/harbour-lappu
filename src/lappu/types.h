#ifndef LAPPU_TYPES_H
#define LAPPU_TYPES_H

#include <QString>

// One note as the device keeps it.
struct LocalNote
{
    // The id the server gave the note, or a negative one while the note has
    // only ever existed here.
    qint64 id;
    // Worked out from the first line of the content, as the server does.
    QString title;
    QString content;
    QString category;
    // The server's version of the note, kept for the `If-Match` header a
    // conditional write would need. Nothing is decided by it: whether the
    // server knows the note at all is told by the id.
    QString etag;
    bool favorite;
    // Shared with the account as view-only, so the pages refuse to edit it.
    bool readOnly;
    // Carries an edit the server has not been told about yet.
    bool dirty;
    // Deleted here, and waiting to be deleted there as well.
    bool deleted;
    // Seconds since the epoch, as the Notes API reports the last change.
    qint64 modified;

    LocalNote()
        : id(0)
        , favorite(false)
        , readOnly(false)
        , dirty(false)
        , deleted(false)
        , modified(0)
    {
    }
};

#endif
