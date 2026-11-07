#ifndef LAPPU_CONFIG_H
#define LAPPU_CONFIG_H

#include <QObject>
#include <QSettings>
#include <QString>

#include "secretsstore.h"

class Config : public QObject
{
    Q_OBJECT
    Q_PROPERTY(QString serverUrl READ serverUrl WRITE setServerUrl NOTIFY serverUrlChanged)
    Q_PROPERTY(QString username READ username WRITE setUsername NOTIFY usernameChanged)
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
    Q_PROPERTY(bool configured READ isConfigured NOTIFY configuredChanged)
    Q_PROPERTY(bool ready READ isReady NOTIFY readyChanged)
    Q_PROPERTY(QString authSource READ authSource NOTIFY authSourceChanged)
    Q_PROPERTY(int pendingAccountId READ pendingAccountId NOTIFY pendingAccountIdChanged)
    Q_PROPERTY(QString credentialsError READ credentialsError NOTIFY credentialsErrorChanged)

public:
    explicit Config(QObject *parent = nullptr);

    QString serverUrl() const { return m_serverUrl; }
    void setServerUrl(const QString &url);

    QString username() const { return m_username; }
    void setUsername(const QString &username);

    QString password() const { return m_password; }
    void setPassword(const QString &password);

    bool ignoreSslErrors() const { return m_ignoreSslErrors; }
    void setIgnoreSslErrors(bool ignore);

    bool syncOnStart() const { return m_syncOnStart; }
    void setSyncOnStart(bool enabled);

    // Minutes between automatic syncs; 0 turns the timer off.
    int syncInterval() const { return m_syncInterval; }
    void setSyncInterval(int minutes);

    bool syncAfterPush() const { return m_syncAfterPush; }
    void setSyncAfterPush(bool enabled);

    // The note the cover shows, or 0 when none is pinned.
    int coverNoteId() const { return m_coverNoteId; }
    void setCoverNoteId(int id);

    // How the notes list is ordered: "modified" or "title".
    QString sortMode() const { return m_sortMode; }
    void setSortMode(const QString &mode);

    // How the notes are laid out: "list" or "grid".
    QString viewMode() const { return m_viewMode; }
    void setViewMode(const QString &mode);

    bool isConfigured() const;
    bool isReady() const { return m_ready; }

    // Where the password comes from: "loginflow" for one the server's own
    // sign-in page made and this app stored, "account" for one read from a
    // Nextcloud account in the Sailfish OS settings.
    QString authSource() const { return m_authSource; }
    // The account whose password QML is to ask signond for, or 0 when the
    // app does not sign in that way.
    int pendingAccountId() const { return static_cast<int>(m_accountId); }
    bool usesAccount() const;
    QString credentialsError() const { return m_credentialsError; }

    // The password a sign-in through the server's own page created. Storing
    // it is what makes that the way the app signs in, so an account chosen
    // earlier is let go of.
    void applyLoginFlowPassword(const QString &password);
    // Lets the password go without touching where it came from: a stored one
    // is removed, an account's is simply not held any more.
    void forgetPassword();

    Q_INVOKABLE void clearCredentials();
    // Asks for the password again, from wherever this app signs in.
    Q_INVOKABLE void reloadCredentials();
    // Signs in as one of the Nextcloud accounts of the Sailfish OS settings
    // from now on, which is remembered. Reading its password is QML's, which
    // answers the choice by loading the component that asks signond.
    Q_INVOKABLE void useAccount(int accountId);
    // What that component reports back. Said from QML because an account's
    // sign-in is read through Sailfish.Accounts, the only route to it a
    // Harbour package may take.
    Q_INVOKABLE void applyAccountCredentials(const QString &server, const QString &user,
                                             const QString &password);
    Q_INVOKABLE void reportAccountFailure(const QString &error);
    // The way back from an account that cannot be signed in to: the app
    // signs in through the server's own page again, and is ready either
    // way, since a start page nothing can leave is a trap.
    Q_INVOKABLE void useLoginFlow();
    Q_INVOKABLE static QString normalizeServerUrl(const QString &input);

signals:
    void serverUrlChanged();
    void usernameChanged();
    void passwordChanged();
    void ignoreSslErrorsChanged();
    void syncOnStartChanged();
    void syncIntervalChanged();
    void syncAfterPushChanged();
    void coverNoteIdChanged();
    void sortModeChanged();
    void viewModeChanged();
    void configuredChanged();
    void readyChanged();
    void authSourceChanged();
    void pendingAccountIdChanged();
    void credentialsErrorChanged();
    // Asks QML to read an account's password again. Not said at startup,
    // where the component does its own first read as it loads.
    void accountSignInNeeded(int accountId);
    // An account's password has just been read from signond. Said apart
    // from passwordChanged() because the password may well be the same one
    // and the run that asked for it still needs starting again.
    void accountSignedIn();

private:
    void store(const QString &key, const QVariant &value);
    void applyStoredPassword(const QString &password);
    // Asks whichever side holds the password for it.
    void loadCredentials();
    void setCredentialsError(const QString &error);
    // Stops signing in as an account, leaving the account itself alone: it
    // belongs to the settings and is not this app's to remove.
    void forgetAccount();

    QSettings m_settings;
    SecretsStore m_secrets;
    QString m_serverUrl;
    QString m_username;
    QString m_password;
    QString m_credentialsError;
    QString m_sortMode;
    QString m_viewMode;
    QString m_authSource;
    uint m_accountId;
    bool m_ignoreSslErrors;
    bool m_syncOnStart;
    int m_syncInterval;
    bool m_syncAfterPush;
    int m_coverNoteId;
    bool m_ready;
};

#endif
