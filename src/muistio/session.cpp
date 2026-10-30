#include "session.h"

#include "notesmodel.h"

Session::Session(Config *config, NotesModel *notes, QObject *parent)
    : QObject(parent)
    , m_config(config)
    , m_notes(notes)
    , m_busy(false)
{
    connect(&m_login, &LoginFlow::loginUrlReady, this, &Session::onLoginUrl);
    connect(&m_login, &LoginFlow::finished, this, &Session::onLoggedIn);
    connect(&m_login, &LoginFlow::failed, this, &Session::onLoginFailed);
    connect(&m_api, &NotesApi::notesReady, this, &Session::onNotesReady);
    connect(&m_api, &NotesApi::failed, this, &Session::onApiFailed);
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
    m_cache.open();
    if (m_config->isReady() && m_config->isConfigured())
        applyCredentials();
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
    if (m_config->syncOnStart())
        sync();
}

void Session::onConfiguredChanged()
{
    emit configuredChanged();
    if (m_config->isConfigured())
        applyCredentials();
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
    m_config->clearCredentials();
    m_cache.clear();
    m_notes->reload();
}

void Session::reloadSecrets()
{
    m_config->reloadSecrets();
}

void Session::sync()
{
    if (!m_config->isConfigured() || m_busy)
        return;

    setError(QString());
    setBusy(true);
    m_api.fetchNotes();
}

void Session::onNotesReady(const QVector<LocalNote> &notes)
{
    setBusy(false);
    m_cache.replaceAll(notes);
    reloadModels();
}

void Session::onApiFailed(const QString &error)
{
    setBusy(false);
    setError(error);
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
    emit loginSucceeded();
}

// The sign-in pages show the reason themselves, so it does not also become a
// notice over whatever the user is looking at.
void Session::onLoginFailed(const QString &error)
{
    setBusy(false);
    emit loginFailed(error);
}
