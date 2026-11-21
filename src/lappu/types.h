#ifndef LAPPU_TYPES_H
#define LAPPU_TYPES_H

#include <QString>

// One tag of the QOwnNotes tag tree, flattened onto its path: the names from
// the top level joined with slashes, which is both how a tag is shown and how
// it is named back to the server. The tree is not kept as a tree because
// nothing here nests one: a tag is a filter, and a path tells two apart.
struct NoteTag
{
    qint64 id;
    QString path;

    NoteTag()
        : id(0)
    {
    }
};

// What the server carries, as its capabilities answered for it. The whole of
// the answer travels together because every part of it is settled by the one
// request, and a feature nothing has confirmed is left out of the way: an
// unreachable server is a server carrying none of this.
struct ServerApps
{
    bool present;
    // Where the app serves its APIs. Empty unless the app is there.
    QString apiBase;
    // Whether the tags of the note folder can be read at all.
    bool tags;
    // Whether Nextcloud's Deleted files app is on for this user, which is
    // what gives the note folder a trash to look in.
    bool trash;

    ServerApps()
        : present(false)
        , tags(false)
        , trash(false)
    {
    }
};

// One note carrying one tag. A link the server could not place on a note of
// its own is left out, there being no note here for it to belong to.
struct TagLink
{
    qint64 tagId;
    qint64 noteId;

    TagLink()
        : tagId(0)
        , noteId(0)
    {
    }
};

// One note the server is holding in its trash. It is named by where it was
// and when it went, which is the pair a restore is asked for by: a deleted
// note has no id of its own until it is a note again.
struct TrashedNote
{
    QString title;
    QString fileName;
    QString subFolderPath;
    QString originalLocation;
    // Seconds since the epoch, as the server reports the deletion.
    qint64 deleted;

    TrashedNote()
        : deleted(0)
    {
    }
};

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
    // The server's version of the note, as it was last read. It goes back out
    // in the `If-Match` header of an edit, so a note that changed on the
    // server in the meantime is refused rather than written over. It still
    // says nothing about whether the server knows the note: that is the id.
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

// Two notes the user could not tell apart. The stamps differ whenever both
// sides were touched, and the version does too, so neither is worth comparing.
inline bool sameNote(const LocalNote &mine, const LocalNote &theirs)
{
    return mine.title == theirs.title
            && mine.content == theirs.content
            && mine.category == theirs.category
            && mine.favorite == theirs.favorite;
}

#endif
