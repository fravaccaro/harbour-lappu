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
#include "types.h"

class NotesModel;

class Session : public QObject
{
    Q_OBJECT
    Q_PROPERTY(bool ready READ isReady NOTIFY readyChanged)
    Q_PROPERTY(bool configured READ isConfigured NOTIFY configuredChanged)
    Q_PROPERTY(bool busy READ isBusy NOTIFY busyChanged)
    Q_PROPERTY(QString errorMessage READ errorMessage NOTIFY errorMessageChanged)
    Q_PROPERTY(QString credentialsError READ credentialsError NOTIFY credentialsErrorChanged)
    // Where the password comes from: "loginflow" or "account".
    Q_PROPERTY(QString authSource READ authSource NOTIFY authSourceChanged)
    // The account QML is to read the password of, or 0 when there is none.
    Q_PROPERTY(int pendingAccountId READ pendingAccountId NOTIFY pendingAccountIdChanged)
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

public:
    explicit Session(Config *config, NotesModel *notes, QObject *parent = nullptr);

    bool isReady() const;
    bool isConfigured() const;
    bool isBusy() const { return m_busy; }
    QString errorMessage() const { return m_error; }
    QString credentialsError() const;
    QString authSource() const;
    int pendingAccountId() const;
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

    Q_INVOKABLE void startLogin(const QString &serverUrl);
    Q_INVOKABLE void cancelLogin();
    Q_INVOKABLE void logout();
    Q_INVOKABLE void reloadCredentials();
    // Signs in as one of the Nextcloud accounts of the Sailfish OS settings.
    // The password is read from signond here and on every later run, and is
    // kept by this app nowhere.
    Q_INVOKABLE void useAccount(int accountId);
    // What the QML that reads an account reports back.
    Q_INVOKABLE void applyAccountCredentials(const QString &server, const QString &user,
                                             const QString &password);
    Q_INVOKABLE void reportAccountFailure(const QString &error);
    // Goes back to signing in through the server's own page, for when the
    // chosen account cannot be read and the start page would otherwise have
    // nothing but Try again on it.
    Q_INVOKABLE void useLoginFlow();
    // Sends what is owed to the server, then pulls the whole account and keeps
    // the answer on the device.
    Q_INVOKABLE void sync();
    // An id of 0 writes a new note; any other id edits that note, including the
    // negative one a note borrows while it waits to be sent. The title is the
    // note's name and is kept apart from the content: it is saved as it is
    // given, so a note is never renamed by what is written in it. An empty
    // title alone falls back to the first line.
    Q_INVOKABLE void saveNote(int noteId, const QString &title, const QString &content,
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

signals:
    void readyChanged();
    void configuredChanged();
    void busyChanged();
    void errorMessageChanged();
    void credentialsErrorChanged();
    void authSourceChanged();
    void pendingAccountIdChanged();
    // Asks the QML that reads an account to read it again, which only a
    // password the server has refused ever calls for.
    void accountSignInNeeded(int accountId);
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
    // Something the user is owed an explanation of, but that is not an error:
    // it goes its own way rather than through errorMessage, which would hold
    // a second identical notice back.
    void notice(const QString &text);

private:
    void setBusy(bool busy);
    void setError(const QString &error);
    void applyCredentials();
    void applyApiSettings();
    void applySyncTimer();
    void syncIfStartupWanted();
    void reloadModels();
    void onConfiguredChanged();
    // Takes up the run a refusal abandoned, once the account has been asked
    // for its password a second time and answered.
    void onAccountSignedIn();
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
    // Whether the password of an account has already been asked for again
    // over one refusal. Only the first 401 of a run is worth a second ask;
    // a second would spin signond for a password the server will not take.
    bool m_askedAccountAgain;
};

#endif
