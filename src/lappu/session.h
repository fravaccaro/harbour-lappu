#ifndef LAPPU_SESSION_H
#define LAPPU_SESSION_H

#include <QObject>
#include <QString>
#include <QTimer>

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

    Q_INVOKABLE void startLogin(const QString &serverUrl);
    Q_INVOKABLE void cancelLogin();
    Q_INVOKABLE void logout();
    Q_INVOKABLE void reloadSecrets();
    // Sends what is owed to the server, then pulls the whole account and keeps
    // the answer on the device.
    Q_INVOKABLE void sync();
    // An id of 0 writes a new note; any other id edits that note, including the
    // negative one a note borrows while it waits to be sent. The title follows
    // the first line of the content, as it does on the server.
    Q_INVOKABLE void saveNote(int noteId, const QString &content, const QString &category,
                              bool favorite);
    Q_INVOKABLE void setFavorite(int noteId, bool favorite);
    Q_INVOKABLE void deleteNote(int noteId);

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
    void loginUrlReady(const QString &url);
    void loginSucceeded();
    void loginFailed(const QString &error);
    void notesChanged();

private:
    void setBusy(bool busy);
    void setError(const QString &error);
    void applyCredentials();
    void applyApiSettings();
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
    void store(const LocalNote &note);
    void pushDirty();
    void pushNext();
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
    QString m_loginUrl;
    QString m_error;
    bool m_busy;
    // Whether a pull follows once the queue has drained.
    bool m_pullAfter;
    // A flush the user did not ask for keeps quiet about a server that cannot
    // be reached; the note simply stays owed.
    bool m_announce;
    int m_pushed;
    // The sync a launch asks for is wanted once, not again every time the
    // credentials are applied.
    bool m_syncedOnStart;
};

#endif
