#include "session.h"

#include <QDateTime>
#include <QRegularExpression>
#include <QStringList>
#include <QVariantMap>

#include "notesmodel.h"
#include "sharedtext.h"

namespace {

// The markers that open a heading or a list item are taken off the line that
// becomes the title, the way the server's own renderer names a note, so a
// bullet or a task reads as the thing itself rather than "- [ ] ...".
QString titleFromLine(QString line)
{
    while (line.startsWith(QLatin1Char('#')))
        line.remove(0, 1);

    static const QRegularExpression marker(
        QStringLiteral("^\\s*[-*+]\\s+(\\[[ xX]\\]\\s+)?"));
    line.remove(marker);
    return line.trimmed();
}

// A name for a note that was given none, taken from the first line that says
// anything. The name is worked out here so the pages can show it before the
// next pull confirms it, and it travels with the note because the v1 API keeps
// the title it is given rather than deriving one from the content.
QString noteTitle(const QString &content)
{
    const QStringList lines = content.split(QLatin1Char('\n'));
    for (int i = 0; i < lines.size(); ++i) {
        const QString line = titleFromLine(lines.at(i).trimmed());
        if (!line.isEmpty())
            return line;
    }

    return QObject::tr("New note");
}

// The name the note is saved under: what the writer typed, or, when they typed
// nothing, the first line of the body, so a note jotted down in a hurry is
// still called something. A note that already has a name keeps it, since the
// editor hands that name back untouched: nothing renames a note but the
// writer, which is what keeps a name given on the server from being undone.
QString chosenTitle(const QString &title, const QString &content)
{
    const QString typed = title.trimmed();
    return typed.isEmpty() ? noteTitle(content) : typed;
}

// A note the server has never seen, which is the only kind worth offering it
// as a new one. The server numbers its notes from one, so the temporary id a
// note borrows until then is what tells the two apart.
bool isLocalOnly(const LocalNote &note)
{
    return note.id <= 0;
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

// The two refusals that hold a note shut rather than merely going wrong the
// once: a lock, which the user can lift, and a permission, which they have to
// be given. Either way the note goes on being refused until something changes
// on the server, which is what makes it worth listing rather than only saying.
bool heldShut(int status)
{
    return status == 403 || status == 423;
}

// The note named, since a refusal the user cannot place is one they cannot act
// on. The Notes app answers 423 for a file Nextcloud has locked, after trying
// five times itself, and a lock a user placed has no expiry, so the sentence
// says where it has to be lifted and that nothing here needs redoing.
QString refusalNotice(int status, const QString &title, const QString &error)
{
    if (status == 423) {
        return QObject::tr("%1 is locked on the server. The change is waiting "
                           "until the file is unlocked in Nextcloud.").arg(title);
    }
    if (status == 403)
        return QObject::tr("%1 is read-only on the server. The change is waiting.").arg(title);

    return error;
}

QVariantMap lockedRow(const LocalNote &note, int status)
{
    QVariantMap map;
    map.insert(QStringLiteral("noteId"), note.id);
    map.insert(QStringLiteral("title"), note.title);
    map.insert(QStringLiteral("modified"), note.modified);
    map.insert(QStringLiteral("reason"),
               status == 403 ? QStringLiteral("readOnly") : QStringLiteral("locked"));
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
    , m_owedWhileBusy(false)
    , m_pushed(0)
    , m_syncedOnStart(false)
    , m_askedAccountAgain(false)
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
    connect(m_config, &Config::credentialsErrorChanged, this, &Session::credentialsErrorChanged);
    connect(m_config, &Config::authSourceChanged, this, &Session::authSourceChanged);
    connect(m_config, &Config::pendingAccountIdChanged, this, &Session::pendingAccountIdChanged);
    connect(m_config, &Config::accountSignInNeeded, this, &Session::accountSignInNeeded);
    connect(m_config, &Config::accountSignedIn, this, &Session::onAccountSignedIn);
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
    connect(m_config, &Config::viewModeChanged, this, &Session::viewModeChanged);
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

QString Session::credentialsError() const
{
    return m_config->credentialsError();
}

QString Session::authSource() const
{
    return m_config->authSource();
}

int Session::pendingAccountId() const
{
    return m_config->pendingAccountId();
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

QString Session::viewMode() const
{
    return m_config->viewMode();
}

void Session::setViewMode(const QString &mode)
{
    m_config->setViewMode(mode);
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
    // A note that owes the server nothing is not being held back by it.
    pruneLocked();
}

void Session::applyApiSettings()
{
    m_api.setServer(m_config->serverUrl());
    m_api.setCredentials(m_config->username(), m_config->password());
    m_api.setIgnoreSslErrors(m_config->ignoreSslErrors());
}

void Session::applyCredentials()
{
    // A password that works again earns the next refusal a second ask.
    m_askedAccountAgain = false;
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

// Only after a refusal: the password may be a new one or the same, and
// either way the run it interrupted was abandoned and the timer stopped,
// so nothing but this would set the app going again.
void Session::onAccountSignedIn()
{
    if (!m_askedAccountAgain || !m_config->isConfigured())
        return;

    applyCredentials();
    sync();
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
    // nothing left for an answer to settle, and nothing left owed for the
    // server to be holding shut. Said here rather than left to pruneLocked(),
    // which the cache is emptied without going through.
    m_conflicts.clear();
    emit conflictsChanged();
    m_locked.clear();
    emit lockedChanged();
}

void Session::reloadCredentials()
{
    m_config->reloadCredentials();
}

void Session::useAccount(int accountId)
{
    setError(QString());
    m_config->useAccount(accountId);
}

void Session::applyAccountCredentials(const QString &server, const QString &user,
                                      const QString &password)
{
    m_config->applyAccountCredentials(server, user, password);
}

void Session::reportAccountFailure(const QString &error)
{
    m_config->reportAccountFailure(error);
}

void Session::useLoginFlow()
{
    setError(QString());
    m_config->useLoginFlow();
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
    m_owedWhileBusy = false;
    m_pushed = 0;
    m_pending = m_cache.dirtyNotes();
    setBusy(true);
    pushNext();
}

// Owed edits are offered as soon as they are made, but a server that cannot be
// reached is not worth saying anything about: the note keeps waiting.
void Session::pushDirty()
{
    if (!m_config->isConfigured())
        return;

    // The run going out took its queue when it started, so this note is not
    // in it. It is written down instead and sent once that run ends.
    if (m_busy) {
        m_owedWhileBusy = true;
        return;
    }

    m_pending = m_cache.dirtyNotes();
    if (m_pending.isEmpty())
        return;

    m_pullAfter = false;
    m_announce = false;
    m_owedWhileBusy = false;
    m_pushed = 0;
    setBusy(true);
    pushNext();
}

void Session::flushOwed()
{
    if (!m_owedWhileBusy)
        return;

    m_owedWhileBusy = false;
    pushDirty();
}

void Session::pushNext()
{
    const LocalNote note = takeNextOwed();
    // Kept so the answer can be told from the row it was made from: the user
    // may write to that row again before the answer arrives.
    m_inFlight = note;
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
    flushOwed();
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

    flushOwed();
}

void Session::onCreated(qint64 localId, const LocalNote &note)
{
    ++m_pushed;
    settleWritten(note);
    // The note the cover points at has just been given its server id. Both
    // ways of taking the answer leave the row under that id.
    if (m_config->coverNoteId() == localId)
        m_config->setCoverNoteId(note.id);

    reloadModels();
    pushNext();
}

void Session::onUpdated(const LocalNote &note)
{
    ++m_pushed;
    settleWritten(note);
    reloadModels();
    pushNext();
}

// A delete the user asked for is simply done. A note the server says it no
// longer has, while an edit of the user's was still waiting on it, is another
// thing altogether: the note is gone, but the change is theirs and was never
// offered to anybody, so it starts again as a note the server has never seen
// rather than going down with the one it was made on.
void Session::onDeleted(qint64 id)
{
    ++m_pushed;
    const LocalNote mine = m_cache.note(id);
    m_cache.remove(id);
    if (!mine.deleted && mine.dirty)
        restartOwed(mine);

    reloadModels();
    pushNext();
}

void Session::settleWritten(const LocalNote &saved)
{
    const LocalNote now = m_cache.note(m_inFlight.id);
    // A row written to again while the answer travelled: settling it against
    // the server's echo would throw that newer edit away and, worse, clear the
    // very mark that was to send it. A delete made meanwhile counts the same,
    // since settling would unmark it.
    const bool writtenAgain = now.id != 0
            && (now.deleted || !sameNote(now, m_inFlight));
    if (writtenAgain) {
        keepOwedOver(saved);
        return;
    }

    m_cache.markClean(m_inFlight.id, saved);
}

// The server's id and version are adopted so the next run writes over what
// this one just wrote, rather than being refused for holding an older version
// or starting a second copy of a note that now exists.
void Session::keepOwedOver(const LocalNote &saved)
{
    LocalNote owed = m_cache.note(m_inFlight.id);
    if (owed.id != saved.id)
        m_cache.remove(owed.id);

    owed.id = saved.id;
    owed.etag = saved.etag;
    m_cache.upsertLocal(owed);
}

void Session::restartOwed(const LocalNote &mine)
{
    LocalNote fresh = mine;
    // A note the server has never seen, which is what this edit has become:
    // no id of its own to be refused over, and no version to match against.
    fresh.id = m_cache.nextTempId();
    fresh.etag.clear();
    fresh.deleted = false;
    m_cache.upsertLocal(fresh);
    // It goes out in this run, as the new note it now is.
    m_pending.append(fresh);
    if (m_config->coverNoteId() == mine.id)
        m_config->setCoverNoteId(fresh.id);

    emit notice(tr("%1 was deleted on the server. The change made here is kept "
                   "as a new note.").arg(mine.title));
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
// into anything: there is nothing the app could do about it from here, and a
// file nobody can write is also a file nobody can change, so the edit kept
// waiting hides no version of the note but the one it was made from.
//
// The reason is always said, unlike a server that could not be reached: the
// edit was the user's own, and a tick that did not land is worth knowing about
// whether or not the sync was asked for. It goes out on notice() for the
// reason a conflict does - errorMessage would hold back the second of two
// identical refusals, which is exactly what ticking a box on a locked note
// twice would earn.
void Session::onRefused(int status, const QString &error)
{
    rememberLocked(status);
    emit notice(refusalNotice(status, m_inFlight.title, error));
    pushNext();
}

// The note that was sent is the one that was refused, which is how a refusal
// comes by a name at all: NotesApi knows the status and nothing else.
void Session::rememberLocked(int status)
{
    if (!heldShut(status) || m_inFlight.id == 0)
        return;

    const int at = indexOfLocked(m_inFlight.id);
    const QPair<qint64, int> held = qMakePair(m_inFlight.id, status);
    if (at >= 0) {
        if (m_locked.at(at) == held)
            return;

        m_locked[at] = held;
    } else {
        m_locked.append(held);
    }
    emit lockedChanged();
}

// Called wherever the models are reloaded, so one rule stands for every way a
// note stops being held: the write landed, the change was discarded, the note
// was deleted. A row that owes the server nothing is a row nothing is keeping
// from it.
void Session::pruneLocked()
{
    const int was = m_locked.size();
    for (int i = m_locked.size() - 1; i >= 0; --i) {
        const LocalNote note = m_cache.note(m_locked.at(i).first);
        if (note.id == 0 || !(note.dirty || note.deleted))
            m_locked.remove(i);
    }

    if (m_locked.size() != was)
        emit lockedChanged();
}

int Session::indexOfLocked(qint64 noteId) const
{
    for (int i = 0; i < m_locked.size(); ++i) {
        if (m_locked.at(i).first == noteId)
            return i;
    }

    return -1;
}

QVariantList Session::lockedNotes() const
{
    QVariantList list;
    list.reserve(m_locked.size());
    for (int i = 0; i < m_locked.size(); ++i)
        list.append(lockedRow(m_cache.note(m_locked.at(i).first), m_locked.at(i).second));

    return list;
}

// Only a note the server knows can be put back the way the server holds it,
// which is why a note it has never seen is turned away here as well as by
// its caller.
void Session::abandonOwed(qint64 noteId)
{
    if (noteId <= 0)
        return;

    QPair<LocalNote, LocalNote> asked;
    if (takeConflict(noteId, &asked))
        emit conflictsChanged();

    m_cache.abandonOwed(noteId);
    // The row is left holding the edit until the server's own version is
    // fetched over it, a pull being the only thing that carries one.
    m_pullAfter = true;
    reloadModels();
}

void Session::discardLocal(int noteId)
{
    const LocalNote note = m_cache.note(noteId);
    if (note.id <= 0 || !(note.dirty || note.deleted))
        return;

    abandonOwed(note.id);
    // With a run in flight the pull it was just promised carries the note
    // back; with nothing going out there is no drain to wait for.
    if (!m_busy)
        fetchNotes();
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
//
// A password read from a Sailfish OS account was not chosen here and may have
// been changed in the settings while the app held the old one, so it is asked
// for once more before the user is sent anywhere. The second refusal is the
// one that counts, and arrives here with the flag already set.
void Session::onUnauthorized(const QString &error)
{
    m_syncTimer.stop();
    m_syncedOnStart = false;
    m_pending.clear();
    m_pullAfter = false;
    m_announce = false;
    setBusy(false);
    if (m_config->usesAccount() && !m_askedAccountAgain) {
        m_askedAccountAgain = true;
        m_config->reloadCredentials();
        return;
    }

    m_config->forgetPassword();
    setError(error);
}

void Session::store(const LocalNote &note)
{
    m_cache.upsertLocal(note);
    reloadModels();
    pushDirty();
}

void Session::saveNote(int noteId, const QString &title, const QString &content,
                       const QString &category, bool favorite)
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
    note.title = chosenTitle(title, content);
    note.category = category;
    note.favorite = favorite;
    note.modified = QDateTime::currentDateTimeUtc().toTime_t();
    store(note);
}

QString Session::sharedBody(const QString &text) const
{
    return SharedText::body(text);
}

QString Session::sharedLink(const QString &url, const QString &linkTitle) const
{
    return SharedText::linkBody(url, linkTitle);
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
    m_config->applyLoginFlowPassword(password);
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
