#include "config.h"

#include <QDir>
#include <QStandardPaths>

#include "url.h"

namespace {

QString settingsFilePath()
{
    const QString dir = QStandardPaths::writableLocation(QStandardPaths::AppConfigLocation);
    QDir().mkpath(dir);
    return dir + QStringLiteral("/settings.conf");
}

// The password the server's own sign-in page made, kept by this app.
const QString LoginFlowSource = QStringLiteral("loginflow");
// The password of a Nextcloud account in the Sailfish OS settings, asked of
// signond on every run and kept by this app nowhere.
const QString AccountSource = QStringLiteral("account");

}

Config::Config(QObject *parent)
    : QObject(parent)
    , m_settings(settingsFilePath(), QSettings::IniFormat)
    , m_accountId(0)
    , m_ignoreSslErrors(false)
    , m_syncOnStart(true)
    , m_syncInterval(0)
    , m_syncAfterPush(true)
    , m_coverNoteId(0)
    , m_ready(false)
{
    m_serverUrl = m_settings.value(QStringLiteral("server/url")).toString();
    m_username = m_settings.value(QStringLiteral("server/username")).toString();
    m_ignoreSslErrors = m_settings.value(QStringLiteral("server/ignoreSslErrors"), false).toBool();
    m_syncOnStart = m_settings.value(QStringLiteral("sync/onStart"), true).toBool();
    m_syncInterval = m_settings.value(QStringLiteral("sync/intervalMinutes"), 0).toInt();
    m_syncAfterPush = m_settings.value(QStringLiteral("sync/afterPush"), true).toBool();
    m_coverNoteId = m_settings.value(QStringLiteral("cover/noteId"), 0).toInt();
    m_sortMode = m_settings.value(QStringLiteral("notes/sortMode"),
                                  QStringLiteral("modified")).toString();
    m_viewMode = m_settings.value(QStringLiteral("notes/viewMode"),
                                  QStringLiteral("list")).toString();
    m_authSource = m_settings.value(QStringLiteral("server/authSource"),
                                    LoginFlowSource).toString();
    m_accountId = m_settings.value(QStringLiteral("server/accountId"), 0).toUInt();
    connect(&m_secrets, &SecretsStore::loaded, this, &Config::applyStoredPassword);
    connect(&m_secrets, &SecretsStore::failed, this, &Config::setCredentialsError);
    loadCredentials();
}

bool Config::isConfigured() const
{
    return !m_serverUrl.isEmpty() && !m_username.isEmpty() && !m_password.isEmpty();
}

bool Config::usesAccount() const
{
    return m_authSource == AccountSource && m_accountId != 0;
}

// Nobody is listening to accountSignInNeeded() at construction, and nobody
// need be: the component that answers it reads the account as it loads, and
// loads because the app signs in that way.
void Config::loadCredentials()
{
    if (usesAccount())
        emit accountSignInNeeded(pendingAccountId());
    else
        m_secrets.load();
}

void Config::reloadCredentials()
{
    setCredentialsError(QString());
    loadCredentials();
}

void Config::setCredentialsError(const QString &error)
{
    if (error == m_credentialsError)
        return;

    m_credentialsError = error;
    emit credentialsErrorChanged();
}

// The choice is stored before anything is read, so a password the server has
// since changed leaves the app pointed at the account rather than back at a
// sign-in page the user has already answered. Saying authSourceChanged() is
// also what reads it: the component that asks signond is loaded by that
// property and signs in as it arrives.
void Config::useAccount(int accountId)
{
    if (accountId <= 0)
        return;

    setCredentialsError(QString());
    m_authSource = AccountSource;
    m_accountId = static_cast<uint>(accountId);
    store(QStringLiteral("server/authSource"), m_authSource);
    store(QStringLiteral("server/accountId"), m_accountId);
    // Two passwords for one app is one too many: the stored one is let go
    // of as soon as an account is what the app signs in with.
    m_secrets.remove();
    emit pendingAccountIdChanged();
    emit authSourceChanged();
}

void Config::useLoginFlow()
{
    forgetAccount();
    loadCredentials();
}

void Config::forgetAccount()
{
    if (m_authSource == LoginFlowSource)
        return;

    m_authSource = LoginFlowSource;
    m_accountId = 0;
    store(QStringLiteral("server/authSource"), m_authSource);
    store(QStringLiteral("server/accountId"), m_accountId);
    setCredentialsError(QString());
    emit pendingAccountIdChanged();
    emit authSourceChanged();
}

void Config::applyAccountCredentials(const QString &server, const QString &user,
                                     const QString &password)
{
    setCredentialsError(QString());
    setServerUrl(server);
    setUsername(user);
    // Not setPassword(): an account's password is read again on every run
    // and is never written into this app's own storage.
    applyStoredPassword(password);
    emit accountSignedIn();
}

// Before the app is ready the start page says this and offers to try again.
// Once it is running there is a page over it, so letting the password go is
// what sends the user back to the sign-in with a reason.
void Config::reportAccountFailure(const QString &error)
{
    setCredentialsError(error);
    if (m_ready)
        applyStoredPassword(QString());
}

void Config::applyLoginFlowPassword(const QString &password)
{
    forgetAccount();
    setPassword(password);
}

void Config::forgetPassword()
{
    if (usesAccount())
        applyStoredPassword(QString());
    else
        setPassword(QString());
}

// Where both ways of signing in end up, and the one place ready is set. It
// is said once and never again: an account hands its password over on every
// run and over a refusal too, and the window answers readyChanged by
// replacing the whole stack, which would throw away the page the user is on.
void Config::applyStoredPassword(const QString &password)
{
    const bool wasConfigured = isConfigured();
    m_password = password;
    emit passwordChanged();
    if (wasConfigured != isConfigured())
        emit configuredChanged();
    if (m_ready)
        return;

    m_ready = true;
    emit readyChanged();
}

void Config::store(const QString &key, const QVariant &value)
{
    m_settings.setValue(key, value);
    m_settings.sync();
}

void Config::setServerUrl(const QString &url)
{
    const QString normalized = Url::normalizeServer(url);
    if (normalized == m_serverUrl)
        return;

    const bool wasConfigured = isConfigured();
    m_serverUrl = normalized;
    store(QStringLiteral("server/url"), m_serverUrl);
    emit serverUrlChanged();
    if (wasConfigured != isConfigured())
        emit configuredChanged();
}

void Config::setUsername(const QString &username)
{
    if (username == m_username)
        return;

    const bool wasConfigured = isConfigured();
    m_username = username;
    store(QStringLiteral("server/username"), m_username);
    emit usernameChanged();
    if (wasConfigured != isConfigured())
        emit configuredChanged();
}

void Config::setPassword(const QString &password)
{
    if (password == m_password)
        return;

    const bool wasConfigured = isConfigured();
    m_password = password;
    if (m_password.isEmpty())
        m_secrets.remove();
    else
        m_secrets.save(m_password);
    emit passwordChanged();
    if (wasConfigured != isConfigured())
        emit configuredChanged();
}

void Config::setIgnoreSslErrors(bool ignore)
{
    if (ignore == m_ignoreSslErrors)
        return;

    m_ignoreSslErrors = ignore;
    store(QStringLiteral("server/ignoreSslErrors"), m_ignoreSslErrors);
    emit ignoreSslErrorsChanged();
}

void Config::setSyncOnStart(bool enabled)
{
    if (enabled == m_syncOnStart)
        return;

    m_syncOnStart = enabled;
    store(QStringLiteral("sync/onStart"), m_syncOnStart);
    emit syncOnStartChanged();
}

void Config::setSyncInterval(int minutes)
{
    const int safe = minutes < 0 ? 0 : minutes;
    if (safe == m_syncInterval)
        return;

    m_syncInterval = safe;
    store(QStringLiteral("sync/intervalMinutes"), m_syncInterval);
    emit syncIntervalChanged();
}

void Config::setSyncAfterPush(bool enabled)
{
    if (enabled == m_syncAfterPush)
        return;

    m_syncAfterPush = enabled;
    store(QStringLiteral("sync/afterPush"), m_syncAfterPush);
    emit syncAfterPushChanged();
}

void Config::setCoverNoteId(int id)
{
    if (id == m_coverNoteId)
        return;

    m_coverNoteId = id;
    store(QStringLiteral("cover/noteId"), m_coverNoteId);
    emit coverNoteIdChanged();
}

// Anything but "title" is the order notes arrive in, so an unknown value read
// back from a hand-edited settings file falls there rather than nowhere.
void Config::setSortMode(const QString &mode)
{
    const QString safe = mode == QStringLiteral("title") ? mode : QStringLiteral("modified");
    if (safe == m_sortMode)
        return;

    m_sortMode = safe;
    store(QStringLiteral("notes/sortMode"), m_sortMode);
    emit sortModeChanged();
}

// The list is what the notes have always been shown as, so it is where an
// unknown value falls: a settings file that says nothing useful about the
// layout leaves the app looking the way it did before.
void Config::setViewMode(const QString &mode)
{
    const QString safe = mode == QStringLiteral("grid") ? mode : QStringLiteral("list");
    if (safe == m_viewMode)
        return;

    m_viewMode = safe;
    store(QStringLiteral("notes/viewMode"), m_viewMode);
    emit viewModeChanged();
}

// The account itself is left alone: it belongs to the Sailfish OS settings,
// where other apps sign in with it, and is not this app's to remove.
void Config::clearCredentials()
{
    forgetPassword();
    forgetAccount();
    setUsername(QString());
}

QString Config::normalizeServerUrl(const QString &input)
{
    return Url::normalizeServer(input);
}
