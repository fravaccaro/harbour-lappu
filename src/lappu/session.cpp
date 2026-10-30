#include "session.h"

#include <QDateTime>
#include <QStringList>

#include "notesmodel.h"

namespace {

// The server names a note after the first line that says anything, so the
// pages can show the same name before the next pull confirms it.
QString noteTitle(const QString &content)
{
    const QStringList lines = content.split(QLatin1Char('\n'));
    for (int i = 0; i < lines.size(); ++i) {
        QString line = lines.at(i).trimmed();
        while (line.startsWith(QLatin1Char('#')))
            line.remove(0, 1);

        line = line.trimmed();
        if (!line.isEmpty())
            return line;
    }

    return QObject::tr("New note");
}

}

Session::Session(Config *config, NotesModel *notes, QObject *parent)
    : QObject(parent)
    , m_config(config)
    , m_notes(notes)
    , m_busy(false)
    , m_pullAfter(false)
    , m_announce(false)
    , m_pushed(0)
    , m_syncedOnStart(false)
{
    m_syncTimer.setSingleShot(false);
    connect(&m_syncTimer, &QTimer::timeout, this, &Session::sync);
    connect(&m_login, &LoginFlow::loginUrlReady, this, &Session::onLoginUrl);
    connect(&m_login, &LoginFlow::finished, this, &Session::onLoggedIn);
    connect(&m_login, &LoginFlow::failed, this, &Session::onLoginFailed);
    connect(&m_api, &NotesApi::notesReady, this, &Session::onNotesReady);
    connect(&m_api, &NotesApi::created, this, &Session::onCreated);
    connect(&m_api, &NotesApi::updated, this, &Session::onUpdated);
    connect(&m_api, &NotesApi::deleted, this, &Session::onDeleted);
    connect(&m_api, &NotesApi::failed, this, &Session::onApiFailed);
    connect(&m_api, &NotesApi::unauthorized, this, &Session::onUnauthorized);
    connect(m_notes, &NotesModel::changed, this, &Session::notesChanged);
    connect(m_config, &Config::readyChanged, this, &Session::readyChanged);
    connect(m_config, &Config::configuredChanged, this, &Session::onConfiguredChanged);
    connect(m_config, &Config::secretsErrorChanged, this, &Session::secretsErrorChanged);
    connect(m_config, &Config::serverUrlChanged, this, &Session::serverUrlChanged);
    connect(m_config, &Config::ignoreSslErrorsChanged, this, &Session::ignoreSslErrorsChanged);
    connect(m_config, &Config::serverUrlChanged, this, &Session::applyApiSettings);
    connect(m_config, &Config::usernameChanged, this, &Session::applyApiSettings);
    connect(m_config, &Config::passwordChanged, this, &Session::applyApiSettings);
    connect(m_config, &Config::ignoreSslErrorsChanged, this, &Session::applyApiSettings);
    connect(m_config, &Config::syncOnStartChanged, this, &Session::syncOnStartChanged);
    connect(m_config, &Config::syncIntervalChanged, this, &Session::syncIntervalChanged);
    connect(m_config, &Config::syncAfterPushChanged, this, &Session::syncAfterPushChanged);
    connect(m_config, &Config::coverNoteIdChanged, this, &Session::coverNoteIdChanged);
    connect(m_config, &Config::sortModeChanged, this, &Session::sortModeChanged);
    connect(m_config, &Config::syncIntervalChanged, this, &Session::applySyncTimer);
    connect(m_config, &Config::readyChanged, this, &Session::syncIfStartupWanted);
    m_cache.open();
    if (m_config->isReady() && m_config->isConfigured())
        applyCredentials();

    applySyncTimer();
    syncIfStartupWanted();
}

bool Session::isReady() const
{
    return m_config->isReady();
}

bool Session::isConfigured() const
{
    return m_config->isConfigured();
}

QString Session::secretsError() const
{
    return m_config->secretsError();
}

QString Session::serverUrl() const
{
    return m_config->serverUrl();
}

bool Session::ignoreSslErrors() const
{
    return m_config->ignoreSslErrors();
}

void Session::setIgnoreSslErrors(bool ignore)
{
    m_config->setIgnoreSslErrors(ignore);
}

bool Session::syncOnStart() const
{
    return m_config->syncOnStart();
}

void Session::setSyncOnStart(bool enabled)
{
    m_config->setSyncOnStart(enabled);
}

int Session::syncInterval() const
{
    return m_config->syncInterval();
}

void Session::setSyncInterval(int minutes)
{
    m_config->setSyncInterval(minutes);
}

bool Session::syncAfterPush() const
{
    return m_config->syncAfterPush();
}

void Session::setSyncAfterPush(bool enabled)
{
    m_config->setSyncAfterPush(enabled);
}

int Session::coverNoteId() const
{
    return m_config->coverNoteId();
}

void Session::setCoverNoteId(int id)
{
    m_config->setCoverNoteId(id);
}

QString Session::sortMode() const
{
    return m_config->sortMode();
}

void Session::setSortMode(const QString &mode)
{
    m_config->setSortMode(mode);
}

void Session::setBusy(bool busy)
{
    if (busy == m_busy)
        return;

    m_busy = busy;
    emit busyChanged();
}

void Session::setError(const QString &error)
{
    if (error == m_error)
        return;

    m_error = error;
    emit errorMessageChanged();
}

// The cache answers first, so the pages show the notes from the last pull
// while the server is still being asked.
void Session::reloadModels()
{
    m_notes->setNotes(m_cache.notes());
}

void Session::applyApiSettings()
{
    m_api.setServer(m_config->serverUrl());
    m_api.setCredentials(m_config->username(), m_config->password());
    m_api.setIgnoreSslErrors(m_config->ignoreSslErrors());
}

void Session::applyCredentials()
{
    applyApiSettings();
    reloadModels();
    applySyncTimer();
    pushDirty();
}

// Only while the app is running: Sailfish wakes nothing on its own for this.
void Session::applySyncTimer()
{
    const int minutes = m_config->syncInterval();
    if (minutes <= 0 || !m_config->isConfigured()) {
        m_syncTimer.stop();
        return;
    }

    m_syncTimer.setInterval(minutes * 60 * 1000);
    m_syncTimer.start();
}

void Session::syncIfStartupWanted()
{
    if (m_syncedOnStart || !m_config->isReady() || !m_config->isConfigured())
        return;
    if (!m_config->syncOnStart())
        return;

    m_syncedOnStart = true;
    sync();
}

void Session::onConfiguredChanged()
{
    emit configuredChanged();
    if (!m_config->isConfigured())
        return;

    applyCredentials();
    syncIfStartupWanted();
}

void Session::startLogin(const QString &serverUrl)
{
    setError(QString());
    setBusy(true);
    m_config->setServerUrl(serverUrl);
    m_login.start(serverUrl, m_config->ignoreSslErrors());
}

void Session::cancelLogin()
{
    m_login.cancel();
    setBusy(false);
    if (!m_loginUrl.isEmpty()) {
        m_loginUrl.clear();
        emit loginUrlChanged();
    }
}

void Session::logout()
{
    cancelLogin();
    m_syncTimer.stop();
    m_syncedOnStart = false;
    m_config->clearCredentials();
    m_config->setCoverNoteId(0);
    m_cache.clear();
    m_notes->reload();
}

void Session::reloadSecrets()
{
    m_config->reloadSecrets();
}

// What was written here goes first, so a note edited on the phone is the one
// that stands once the pull arrives.
void Session::sync()
{
    if (!m_config->isConfigured() || m_busy)
        return;

    setError(QString());
    m_pullAfter = true;
    m_announce = true;
    m_pushed = 0;
    m_pending = m_cache.dirtyNotes();
    setBusy(true);
    pushNext();
}

// Owed edits are offered as soon as they are made, but a server that cannot be
// reached is not worth saying anything about: the note keeps waiting.
void Session::pushDirty()
{
    if (!m_config->isConfigured() || m_busy)
        return;

    m_pending = m_cache.dirtyNotes();
    if (m_pending.isEmpty())
        return;

    m_pullAfter = false;
    m_announce = false;
    m_pushed = 0;
    setBusy(true);
    pushNext();
}

void Session::pushNext()
{
    if (m_pending.isEmpty()) {
        queueDrained();
        return;
    }

    const LocalNote note = m_pending.takeFirst();
    if (note.deleted)
        m_api.deleteNote(note.id);
    else if (note.etag.isEmpty())
        m_api.createNote(note);
    else
        m_api.updateNote(note);
}

void Session::queueDrained()
{
    const bool pull = m_pullAfter || (m_pushed > 0 && m_config->syncAfterPush());
    m_pullAfter = false;
    if (pull) {
        fetchNotes();
        return;
    }

    setBusy(false);
    reloadModels();
}

void Session::fetchNotes()
{
    setBusy(true);
    m_api.fetchNotes();
}

void Session::onNotesReady(const QVector<LocalNote> &notes)
{
    setBusy(false);
    m_cache.applyRemote(notes);
    reloadModels();
    // The wait counts from the last sync rather than from when it was asked for.
    if (m_syncTimer.isActive())
        m_syncTimer.start();
}

void Session::onCreated(qint64 localId, const LocalNote &note)
{
    ++m_pushed;
    m_cache.markClean(localId, note);
    // The note the cover points at has just been given its server id.
    if (m_config->coverNoteId() == localId)
        m_config->setCoverNoteId(note.id);

    reloadModels();
    pushNext();
}

void Session::onUpdated(const LocalNote &note)
{
    ++m_pushed;
    m_cache.markClean(note.id, note);
    reloadModels();
    pushNext();
}

void Session::onDeleted(qint64 id)
{
    ++m_pushed;
    m_cache.remove(id);
    reloadModels();
    pushNext();
}

// The rest of the run is abandoned: whatever was owed stays owed, and is
// offered again the next time the notes are synced.
void Session::onApiFailed(const QString &error)
{
    const bool asked = m_announce;
    m_pending.clear();
    m_pullAfter = false;
    m_announce = false;
    setBusy(false);
    reloadModels();
    if (m_syncTimer.isActive())
        m_syncTimer.start();

    if (asked)
        setError(error);
}

// Only the password is let go of: the notes and anything still owed stay where
// they are, so a fresh sign-in sends them rather than losing them. Being sent
// back to the sign-in page always needs a reason, so this one is never silent.
void Session::onUnauthorized(const QString &error)
{
    m_syncTimer.stop();
    m_syncedOnStart = false;
    m_pending.clear();
    m_pullAfter = false;
    m_announce = false;
    setBusy(false);
    m_config->setPassword(QString());
    setError(error);
}

void Session::store(const LocalNote &note)
{
    m_cache.upsertLocal(note);
    reloadModels();
    pushDirty();
}

void Session::saveNote(int noteId, const QString &content, const QString &category,
                       bool favorite)
{
    LocalNote note = noteId > 0 ? m_cache.note(noteId) : LocalNote();
    if (note.readOnly)
        return;

    if (note.id == 0)
        note.id = m_cache.nextTempId();

    note.content = content;
    note.title = noteTitle(content);
    note.category = category;
    note.favorite = favorite;
    note.modified = QDateTime::currentDateTimeUtc().toTime_t();
    store(note);
}

void Session::setFavorite(int noteId, bool favorite)
{
    LocalNote note = m_cache.note(noteId);
    if (note.id == 0 || note.readOnly)
        return;

    note.favorite = favorite;
    store(note);
}

void Session::deleteNote(int noteId)
{
    const LocalNote note = m_cache.note(noteId);
    if (note.id == 0)
        return;

    // A note the server never saw can go at once; the rest must be deleted
    // there too, so it is only marked here.
    if (note.etag.isEmpty())
        m_cache.remove(note.id);
    else
        m_cache.markDeleted(note.id);

    reloadModels();
    pushDirty();
}

void Session::onLoginUrl(const QString &url)
{
    setBusy(false);
    m_loginUrl = url;
    emit loginUrlChanged();
    emit loginUrlReady(url);
}

void Session::onLoggedIn(const QString &server, const QString &user,
                         const QString &password)
{
    m_config->setServerUrl(server);
    m_config->setUsername(user);
    m_config->setPassword(password);
    applyCredentials();
    // A fresh sign-in is worth a pull whatever the startup setting says.
    m_syncedOnStart = true;
    sync();
    emit loginSucceeded();
}

// The sign-in pages show the reason themselves, so it does not also become a
// notice over whatever the user is looking at.
void Session::onLoginFailed(const QString &error)
{
    setBusy(false);
    emit loginFailed(error);
}
