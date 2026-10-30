#include "session.h"

#include <QDateTime>
#include <QStringList>
#include <QVariantMap>

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

// A note the server has never seen, which is the only kind worth offering it
// as a new one. The server numbers its notes from one, so the temporary id a
// note borrows until then is what tells the two apart.
bool isLocalOnly(const LocalNote &note)
{
    return note.id <= 0;
}

// Two notes the user could not tell apart. The stamps differ whenever both
// sides were touched, and the version does too, so neither is worth comparing.
bool sameNote(const LocalNote &mine, const LocalNote &theirs)
{
    return mine.content == theirs.content
            && mine.category == theirs.category
            && mine.favorite == theirs.favorite;
}

// The edit made here as a note of its own, for when the server's version is
// given the original note back. The version is dropped along with the id it
// belonged to, and only the title is labelled: a marker line in the content
// would become the name of the note on the next pull and would be sent to the
// server as text the user never wrote.
LocalNote forkedOnto(const LocalNote &mine, qint64 tempId)
{
    LocalNote fork = mine;
    fork.id = tempId;
    fork.etag.clear();
    fork.title = QObject::tr("%1 (my version)").arg(mine.title);
    fork.deleted = false;
    return fork;
}

QVariantMap conflictRow(const QPair<LocalNote, LocalNote> &pair)
{
    QVariantMap map;
    map.insert(QStringLiteral("noteId"), pair.first.id);
    map.insert(QStringLiteral("mineTitle"), pair.first.title);
    map.insert(QStringLiteral("mineContent"), pair.first.content);
    map.insert(QStringLiteral("mineModified"), pair.first.modified);
    map.insert(QStringLiteral("theirsTitle"), pair.second.title);
    map.insert(QStringLiteral("theirsContent"), pair.second.content);
    map.insert(QStringLiteral("theirsModified"), pair.second.modified);
    map.insert(QStringLiteral("categoryDiffers"), pair.first.category != pair.second.category);
    map.insert(QStringLiteral("favoriteDiffers"), pair.first.favorite != pair.second.favorite);
    return map;
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
    connect(&m_api, &NotesApi::conflicted, this, &Session::onConflict);
    connect(&m_api, &NotesApi::refused, this, &Session::onRefused);
    connect(m_notes, &NotesModel::changed, this, &Session::notesChanged);
    connect(m_config, &Config::readyChanged, this, &Session::readyChanged);
    connect(m_config, &Config::configuredChanged, this, &Session::onConfiguredChanged);
    connect(m_config, &Config::secretsErrorChanged, this, &Session::secretsErrorChanged);
    connect(m_config, &Config::serverUrlChanged, this, &Session::serverUrlChanged);
    connect(m_config, &Config::usernameChanged, this, &Session::usernameChanged);
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

QString Session::username() const
{
    return m_config->username();
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
    // The local side of a pair was one of the rows just dropped, so there is
    // nothing left for an answer to settle.
    m_conflicts.clear();
    emit conflictsChanged();
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
    const LocalNote note = takeNextOwed();
    if (note.id == 0) {
        queueDrained();
        return;
    }

    if (note.deleted)
        m_api.deleteNote(note.id);
    else if (isLocalOnly(note))
        m_api.createNote(note);
    else
        m_api.updateNote(note);
}

// A note waiting for the user to say which version to keep is passed over:
// offering it again would only earn the same refusal and leave the answer
// being waited for twice. An id of 0 is nothing left to send.
LocalNote Session::takeNextOwed()
{
    while (!m_pending.isEmpty()) {
        const LocalNote note = m_pending.takeFirst();
        if (indexOfConflict(note.id) < 0)
            return note;
    }

    return LocalNote();
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

// A refused write is not a failure: the server answered, and answered well,
// so the rest of what is owed still goes out. m_pushed is left alone, since
// nothing was pushed.
void Session::onConflict(const LocalNote &remote)
{
    const LocalNote mine = m_cache.note(remote.id);
    // Gone, or settled while the answer was travelling: a stale refusal with
    // nothing left to argue about.
    if (mine.id == 0 || !mine.dirty) {
        pushNext();
        return;
    }

    // Deleted here. The note stays owed as it is, and the delete the next run
    // sends carries no version for the server to refuse.
    if (mine.deleted) {
        pushNext();
        return;
    }

    // The refusal is proof the server holds something this device has not
    // read, which is the one place the feature weakens queueDrained()'s rule
    // that a run which pushed nothing does not pull. It stays bounded at one
    // pull per run, and a pull has no version to be refused over.
    m_pullAfter = true;
    settleOrAsk(mine, remote);
    pushNext();
}

// Both sides arrived at the same note, so nothing is lost by taking the
// server's word for it and nothing is worth saying about it either.
void Session::settleOrAsk(const LocalNote &mine, const LocalNote &theirs)
{
    if (sameNote(mine, theirs)) {
        m_cache.markClean(mine.id, theirs);
        reloadModels();
        return;
    }

    m_conflicts.append(qMakePair(mine, theirs));
    emit conflictsChanged();
    // Said whether or not the sync was asked for: a note that stopped being
    // sent, and a page that appeared to settle it, both need a reason.
    emit notice(tr("%1 changed on the server as well. Choose which version to keep.")
                .arg(theirs.title));
}

int Session::indexOfConflict(qint64 noteId) const
{
    for (int i = 0; i < m_conflicts.size(); ++i) {
        if (m_conflicts.at(i).first.id == noteId)
            return i;
    }

    return -1;
}

// Takes the pair out of the list, so an answer settles a note once and the
// next sync is free to offer it again.
bool Session::takeConflict(qint64 noteId, QPair<LocalNote, LocalNote> *pair)
{
    const int at = indexOfConflict(noteId);
    if (at < 0)
        return false;

    *pair = m_conflicts.at(at);
    m_conflicts.remove(at);
    return true;
}

QVariantList Session::conflicts() const
{
    QVariantList list;
    list.reserve(m_conflicts.size());
    for (int i = 0; i < m_conflicts.size(); ++i)
        list.append(conflictRow(m_conflicts.at(i)));

    return list;
}

// The row stays dirty and takes on the version the server answered with, so
// what goes out is a conditional write again rather than one that overwrites
// whatever is there by then.
void Session::resolveKeepMine(int noteId)
{
    QPair<LocalNote, LocalNote> pair;
    if (!takeConflict(noteId, &pair))
        return;

    m_cache.setEtag(pair.first.id, pair.second.etag);
    emit conflictsChanged();
    // An answer given while the rest of the run is still going out stops at
    // pushDirty()'s own busy guard. Nothing is lost by that: the row is dirty
    // and now carries the version the server asked for, so the next sync
    // sends it, and the pull this run earned leaves a dirty row alone.
    pushDirty();
}

void Session::resolveKeepTheirs(int noteId)
{
    QPair<LocalNote, LocalNote> pair;
    if (!takeConflict(noteId, &pair))
        return;

    m_cache.markClean(noteId, pair.second);
    reloadModels();
    emit conflictsChanged();
}

// The cover keeps its pin on the original id: that is the note the user
// pinned, and the server's version is the one that goes on carrying it.
void Session::resolveKeepBoth(int noteId)
{
    QPair<LocalNote, LocalNote> pair;
    if (!takeConflict(noteId, &pair))
        return;

    // The note may have been written to again while the question went
    // unanswered, and it is that text the user means by their version.
    const LocalNote mine = m_cache.note(noteId);
    const LocalNote fork = forkedOnto(mine.id != 0 && !mine.deleted ? mine : pair.first,
                                      m_cache.nextTempId());
    // Written to the cache rather than through store(), which would push as
    // well and so re-enter a queue that may still be draining.
    m_cache.upsertLocal(fork);
    m_cache.markClean(noteId, pair.second);
    // The fork borrows a negative id, so it goes out as a new note, and a
    // note the server has never seen has no version to be refused over.
    m_pending.append(fork);
    reloadModels();
    emit conflictsChanged();
    // With a run still draining this stops at the busy guard, the queue it
    // was just added to being the one that carries it; with nothing in
    // flight it starts a run and finds the same row in the cache.
    pushDirty();
}

// A write the server refused by status says nothing against the server itself,
// so the run carries on as it does after a conflict: the note stays owed, the
// next one owed is offered, and nothing is thrown away. A locked note is the
// ordinary reason, and a lock a user placed is held until it is lifted on the
// server, so the row waits however long that takes rather than being retried
// into anything: there is nothing the app could do about it from here.
void Session::onRefused(const QString &error)
{
    if (m_announce)
        setError(error);

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
    // Only an id of 0 asks for a new note. A note still waiting to be sent
    // carries a negative id, and editing it has to find that row rather than
    // start another one, or every save would leave a copy behind.
    LocalNote note = noteId != 0 ? m_cache.note(noteId) : LocalNote();
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
    if (isLocalOnly(note))
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
