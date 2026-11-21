#ifndef LAPPU_SESSION_H
#define LAPPU_SESSION_H

#include <QObject>
#include <QPair>
#include <QString>
#include <QTimer>
#include <QVariantList>

#include "config.h"
#include "loginflow.h"
#include "notesapi.h"
#include "notescache.h"
#include "qownnotesapi.h"
#include "types.h"

class NotesModel;

class Session : public QObject
{
    Q_OBJECT
    Q_PROPERTY(bool ready READ isReady NOTIFY readyChanged)
    Q_PROPERTY(bool configured READ isConfigured NOTIFY configuredChanged)
    Q_PROPERTY(bool busy READ isBusy NOTIFY busyChanged)
    Q_PROPERTY(QString errorMessage READ errorMessage NOTIFY errorMessageChanged)
    Q_PROPERTY(QString secretsError READ secretsError NOTIFY secretsErrorChanged)
    Q_PROPERTY(QString loginUrl READ loginUrl NOTIFY loginUrlChanged)
    Q_PROPERTY(QString serverUrl READ serverUrl NOTIFY serverUrlChanged)
    // The account the app password belongs to, as the server named it.
    Q_PROPERTY(QString username READ username NOTIFY usernameChanged)
    Q_PROPERTY(bool ignoreSslErrors READ ignoreSslErrors WRITE setIgnoreSslErrors
               NOTIFY ignoreSslErrorsChanged)
    Q_PROPERTY(bool syncOnStart READ syncOnStart WRITE setSyncOnStart NOTIFY syncOnStartChanged)
    Q_PROPERTY(int syncInterval READ syncInterval WRITE setSyncInterval
               NOTIFY syncIntervalChanged)
    Q_PROPERTY(bool syncAfterPush READ syncAfterPush WRITE setSyncAfterPush
               NOTIFY syncAfterPushChanged)
    Q_PROPERTY(int coverNoteId READ coverNoteId WRITE setCoverNoteId NOTIFY coverNoteIdChanged)
    Q_PROPERTY(QString sortMode READ sortMode WRITE setSortMode NOTIFY sortModeChanged)
    Q_PROPERTY(QString viewMode READ viewMode WRITE setViewMode NOTIFY viewModeChanged)
    // How many notes changed on both sides and are waiting to be settled.
    Q_PROPERTY(int conflictCount READ conflictCount NOTIFY conflictsChanged)
    // How many notes the server is holding shut, by a lock or by a permission.
    Q_PROPERTY(int lockedCount READ lockedCount NOTIFY lockedChanged)
    // Whether the server carries the QOwnNotes app, which the features that
    // need it are shown by. Asked of the server once an account is known and
    // kept nowhere: it is a fact about this server rather than a setting.
    Q_PROPERTY(bool qownNotes READ qownNotes NOTIFY qownNotesChanged)
    // Whether the tags of the note folder can be read on this server, which
    // takes both the app and a server able to open the database it keeps them
    // in. Everything about tags is drawn and asked for behind this alone.
    Q_PROPERTY(bool tagsAvailable READ tagsAvailable NOTIFY tagsAvailableChanged)
    // Whether the notes deleted from the folder can be looked at on this
    // server, which takes the QOwnNotes app and Nextcloud's own Deleted
    // files app. The way into the trash is drawn behind this alone.
    Q_PROPERTY(bool trashAvailable READ trashAvailable NOTIFY trashAvailableChanged)
    // Whether the trash is being read or written to. It is the trash page's
    // own indicator and is kept apart from busy, which the notes own.
    Q_PROPERTY(bool trashBusy READ trashBusy NOTIFY trashBusyChanged)

public:
    explicit Session(Config *config, NotesModel *notes, QObject *parent = nullptr);

    bool isReady() const;
    bool isConfigured() const;
    bool isBusy() const { return m_busy; }
    QString errorMessage() const { return m_error; }
    QString secretsError() const;
    QString loginUrl() const { return m_loginUrl; }
    QString serverUrl() const;
    QString username() const;
    bool ignoreSslErrors() const;
    void setIgnoreSslErrors(bool ignore);
    bool syncOnStart() const;
    void setSyncOnStart(bool enabled);
    // Minutes between automatic syncs; 0 turns them off.
    int syncInterval() const;
    void setSyncInterval(int minutes);
    bool syncAfterPush() const;
    void setSyncAfterPush(bool enabled);
    // The note the cover shows, or 0 when none is pinned.
    int coverNoteId() const;
    void setCoverNoteId(int id);

    QString sortMode() const;
    void setSortMode(const QString &mode);
    QString viewMode() const;
    void setViewMode(const QString &mode);
    int conflictCount() const { return m_conflicts.size(); }
    int lockedCount() const { return m_locked.size(); }
    bool qownNotes() const { return m_qownNotes; }
    bool tagsAvailable() const { return m_tagsAvailable; }
    bool trashAvailable() const { return m_trashAvailable; }
    bool trashBusy() const { return m_trashBusy; }

    Q_INVOKABLE void startLogin(const QString &serverUrl);
    Q_INVOKABLE void cancelLogin();
    Q_INVOKABLE void logout();
    Q_INVOKABLE void reloadSecrets();
    // Sends what is owed to the server, then pulls the whole account and keeps
    // the answer on the device.
    Q_INVOKABLE void sync();
    // An id of 0 writes a new note; any other id edits that note, including the
    // negative one a note borrows while it waits to be sent. The title is the
    // note's name and is kept apart from the content: it is saved as it is
    // given, so a note is never renamed by what is written in it. An empty
    // title alone falls back to the first line.
    // Answers with the id the note was saved under, which for a new note is
    // the temporary one it borrows until the server names it, so the caller
    // can go on to say something else about the note it has just written.
    Q_INVOKABLE int saveNote(int noteId, const QString &title, const QString &content,
                             const QString &category, bool favorite);
    // The body a share makes, as plain text and as a link. Neither writes
    // anything: the note is the editor's to save, as one typed by hand is.
    // An empty answer means the share held nothing worth writing down.
    Q_INVOKABLE QString sharedBody(const QString &text) const;
    Q_INVOKABLE QString sharedLink(const QString &url, const QString &linkTitle) const;
    Q_INVOKABLE void setFavorite(int noteId, bool favorite);
    Q_INVOKABLE void deleteNote(int noteId);
    // The unsettled notes as plain rows, each carrying both versions, the way
    // NotesModel::notes() hands the notes themselves to QML.
    Q_INVOKABLE QVariantList conflicts() const;
    // The notes the server would not change, as plain rows carrying the name
    // of each and why it is being held shut.
    Q_INVOKABLE QVariantList lockedNotes() const;
    // Keeps the edit made here and offers it again against the version the
    // server answered with.
    Q_INVOKABLE void resolveKeepMine(int noteId);
    // Takes the server's version and lets go of the edit made here.
    Q_INVOKABLE void resolveKeepTheirs(int noteId);
    // Keeps both: the server's version takes the note back, and the text
    // written here goes on as a note of its own.
    Q_INVOKABLE void resolveKeepBoth(int noteId);
    // Lets go of an edit the server has not taken and puts the note back the
    // way the server holds it. Only a note the server knows can be discarded:
    // one it has never seen has no version to fall back on.
    Q_INVOKABLE void discardLocal(int noteId);

    // The tags of the note folder as plain rows, each the path of a tag and
    // how many notes carry it, the way the notes themselves are handed over.
    Q_INVOKABLE QVariantList tags() const;
    // Every tagged note by id, each with its tags as paths, for the pages
    // that filter the notes by one.
    Q_INVOKABLE QVariantMap noteTags() const;
    Q_INVOKABLE QStringList tagsOfNote(int noteId) const;
    // Sets the tags of one note to exactly these paths. The change is kept
    // here at once and offered to the server after, so a note tagged with no
    // server in reach keeps its tags and sends them later.
    Q_INVOKABLE void setNoteTags(int noteId, const QStringList &paths);

    // The notes the server is holding in its trash. They are read when they
    // are asked for rather than with the notes: the trash is the server's
    // own listing, and nothing here is kept on the device.
    Q_INVOKABLE void loadTrash();
    Q_INVOKABLE QVariantList trashedNotes() const;
    // Puts one back where it was deleted from, named the way the trash lists
    // it, and pulls afterwards so the note itself comes home.
    Q_INVOKABLE void restoreNote(const QString &originalLocation, double deleted);

    // Every folder below the note folder, as the server last listed it. The
    // categories of the notes are drawn whatever this answers: it is what
    // adds the folders holding no note, which nothing else can name.
    Q_INVOKABLE QStringList folders() const;

signals:
    void readyChanged();
    void configuredChanged();
    void busyChanged();
    void errorMessageChanged();
    void secretsErrorChanged();
    void loginUrlChanged();
    void serverUrlChanged();
    void usernameChanged();
    void ignoreSslErrorsChanged();
    void syncOnStartChanged();
    void syncIntervalChanged();
    void syncAfterPushChanged();
    void coverNoteIdChanged();
    void sortModeChanged();
    void viewModeChanged();
    void loginUrlReady(const QString &url);
    void loginSucceeded();
    void loginFailed(const QString &error);
    void notesChanged();
    void conflictsChanged();
    void lockedChanged();
    void qownNotesChanged();
    void tagsAvailableChanged();
    // The tags, the links between them and the notes, or what is owed on one
    // of them have moved, so the pages take them again.
    void tagsChanged();
    void trashAvailableChanged();
    void trashBusyChanged();
    // The trash as the server last listed it has changed, by a read or by a
    // note leaving it.
    void trashChanged();
    // The folders of the note folder have been read again, so the views take
    // them again.
    void foldersChanged();
    // Something the user is owed an explanation of, but that is not an error:
    // it goes its own way rather than through errorMessage, which would hold
    // a second identical notice back.
    void notice(const QString &text);

private:
    void setBusy(bool busy);
    void setError(const QString &error);
    void applyCredentials();
    void applyApiSettings();
    // Asks the server what it carries and holds back anything the notes are
    // owed until it answers, the answer being what says where to send them.
    void probeServer();
    // Takes the probe's answer: the flag the pages read, the path the notes
    // go to, and whatever was waiting on it.
    void onProbed(const ServerApps &apps);
    void flushProbeWaiters();
    void setQownNotes(bool present);
    void setTagsAvailable(bool available);
    void setTrashAvailable(bool available);
    void setTrashBusy(bool busy);
    // Reads the tag tree and the links between tags and notes, in that order:
    // a link names a tag by id, so the tree has to be there to read it by.
    // Nothing is asked where the server carries no tags, and a read goes out
    // on its own client, so one that stalls leaves the notes alone.
    void refreshTags();
    void onTagsReady(const QString &etag, bool writable, const QVector<NoteTag> &tags);
    void onTagLinksReady(const QString &etag, const QVector<TagLink> &links);
    // Sends the tags of the next note that owes them. A note the server has
    // never seen is passed over: its tags go out once it has been named.
    void pushOwedTags();
    void onNoteTagsSet(qint64 noteId);
    // Lets go of what was owed on the note the links just read answer for.
    void settleOwedTags();
    void onNoteTagsStale(qint64 noteId);
    void onTagsFailed(const QString &error);
    // Reads the folder tree, which only the QOwnNotes app can answer for. It
    // follows a pull the way the tags do, and needs no second app: a server
    // carrying the one has folders to list.
    void refreshFolders();
    void onFoldersReady(const QStringList &paths);
    void onTrashReady(const QVector<TrashedNote> &notes);
    void onTrashRestored(qint64 noteId);
    void onTrashFailed(const QString &error);
    // Forgets the note that has just left the trash, so the list is right
    // before the server has been asked about it again.
    void dropRestored();
    void applySyncTimer();
    void syncIfStartupWanted();
    void reloadModels();
    void onConfiguredChanged();
    void onLoginUrl(const QString &url);
    void onLoggedIn(const QString &server, const QString &user, const QString &password);
    void onLoginFailed(const QString &error);
    void onNotesReady(const QVector<LocalNote> &notes);
    void onApiFailed(const QString &error);
    void onUnauthorized(const QString &error);
    void onCreated(qint64 localId, const LocalNote &note);
    void onUpdated(const LocalNote &note);
    void onDeleted(qint64 id);
    // Takes the answer to a write onto the row it was made from, settling it
    // unless the user has written to that row again in the meantime.
    void settleWritten(const LocalNote &saved);
    // Moves the row onto the id and version the server just gave it while
    // leaving it owed, for when a newer edit is waiting on it.
    void keepOwedOver(const LocalNote &saved);
    // Starts an owed edit again as a note of its own, for when the server
    // turns out no longer to have the note it was made on.
    void restartOwed(const LocalNote &mine);
    void onConflict(const LocalNote &remote);
    void onRefused(int status, const QString &error);
    // Remembers the note now in flight as one the server is holding shut.
    void rememberLocked(int status);
    // Forgets every note that owes the server nothing any more, which is the
    // one thing that can mean it is no longer being held back.
    void pruneLocked();
    int indexOfLocked(qint64 noteId) const;
    // Lets go of what is owed on one note and asks for the version the server
    // holds, for when the user would rather have the note back than go on
    // waiting for a change to be taken.
    void abandonOwed(qint64 noteId);
    void settleOrAsk(const LocalNote &mine, const LocalNote &theirs);
    int indexOfConflict(qint64 noteId) const;
    bool takeConflict(qint64 noteId, QPair<LocalNote, LocalNote> *pair);
    void store(const LocalNote &note);
    void pushDirty();
    // Sends what the user marked while a run was going out, which pushDirty()
    // could only note down at the time.
    void flushOwed();
    void pushNext();
    LocalNote takeNextOwed();
    void queueDrained();
    void fetchNotes();

    Config *m_config;
    NotesModel *m_notes;
    LoginFlow m_login;
    NotesApi m_api;
    QownNotesApi m_qown;
    NotesCache m_cache;
    QTimer m_syncTimer;
    // What is still owed to the server in this run.
    QVector<LocalNote> m_pending;
    // The note the request now in flight was built from. The answer is taken
    // against this rather than against the row, which the user may have
    // written to again while the answer was on its way.
    LocalNote m_inFlight;
    // The notes that changed on both sides and have not been settled, as the
    // version written here paired with the one the server answered with.
    // Deliberately kept nowhere else: the local side of a pair is the dirty
    // row in the cache, so an app closed before the user answers loses only
    // the question, and the next sync's refused write asks it again.
    QVector<QPair<LocalNote, LocalNote> > m_conflicts;
    // The notes the server would not change, as the id of each paired with the
    // status it was refused with. The id rather than the note, so the rows
    // handed to QML are read from the cache and no name goes stale. Kept
    // nowhere else for the reason the conflicts are: the row is still owed, so
    // the next run earns the same refusal and makes the list again.
    QVector<QPair<qint64, int> > m_locked;
    QString m_loginUrl;
    QString m_error;
    bool m_busy;
    // Whether a pull follows once the queue has drained.
    bool m_pullAfter;
    // A flush the user did not ask for keeps quiet about a server that cannot
    // be reached; the note simply stays owed.
    bool m_announce;
    // A note written or deleted while a run was in flight. The run that ends
    // sends it, rather than leaving it for the next sync. Only an edit of the
    // user's sets this, so a note the server keeps refusing cannot spin a run
    // of its own.
    bool m_owedWhileBusy;
    int m_pushed;
    // The sync a launch asks for is wanted once, not again every time the
    // credentials are applied.
    bool m_syncedOnStart;
    // Whether the QOwnNotes app answered for itself on this server, which is
    // false until a probe says otherwise.
    bool m_qownNotes;
    // Whether that app can read the tags of the note folder. Kept beside the
    // flag above rather than folded into it: the app may be there and the
    // tags still out of reach, and what is drawn then is a note without them.
    bool m_tagsAvailable;
    // Whether a tag request is in flight. One goes out at a time, a read of
    // the tree being followed by a read of the links and then by whatever
    // tags are owed.
    bool m_tagRequest;
    // The note whose tags are being written, or 0 while the tags are only
    // being read. A read that fails is not worth a word; a write is.
    qint64 m_tagNoteInFlight;
    // The tags that went out with that write. The answer is taken against
    // them rather than against the row as it now stands: a note tagged again
    // while the write travelled is owed what was chosen second, and clearing
    // the row blindly would lose it, as settling a written note blindly
    // would lose a newer edit.
    QStringList m_tagsSent;
    // The note whose tags the server has taken but whose links have not been
    // read back yet. What was owed on it is held until they are: dropping it
    // the moment the write landed would leave the note showing the tags it
    // had before, for as long as the read takes, so a tag just added would
    // be seen to vanish and come back.
    qint64 m_tagSettled;
    // Whether the server keeps a trash for the note folder.
    bool m_trashAvailable;
    // The trash as the server last listed it. Held in memory alone, as the
    // conflicts are: it is a listing of the server's rather than anything of
    // this device's, and a page that is not open has no use for it.
    QVector<TrashedNote> m_trash;
    bool m_trashBusy;
    // The note being put back, so the row can be taken out of the list the
    // moment the server says it is a note again.
    TrashedNote m_restoring;
    // Whether the tags have already been read again for a change the server
    // said was built on an older version of them. One such retry is enough:
    // a second refusal is the database moving faster than the app can answer.
    bool m_tagRetried;
    // Whether the probe for these credentials has answered. Until it has,
    // nothing is sent: the answer says which path the notes are served under,
    // and a request made before it would ask the wrong app.
    bool m_probeAnswered;
    // A sync and an owed edit that arrived while the probe was out. They are
    // run once it answers rather than dropped, since what was waiting on it
    // was only the path, not the server.
    bool m_syncWhenProbed;
    bool m_owedWhileProbing;
};

#endif
