#ifndef MUISTIO_SESSION_H
#define MUISTIO_SESSION_H

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
    Q_PROPERTY(bool ignoreSslErrors READ ignoreSslErrors WRITE setIgnoreSslErrors
               NOTIFY ignoreSslErrorsChanged)

public:
    explicit Session(Config *config, NotesModel *notes, QObject *parent = nullptr);

    bool isReady() const;
    bool isConfigured() const;
    bool isBusy() const { return m_busy; }
    QString errorMessage() const { return m_error; }
    QString secretsError() const;
    QString loginUrl() const { return m_loginUrl; }
    QString serverUrl() const;
    bool ignoreSslErrors() const;
    void setIgnoreSslErrors(bool ignore);

    Q_INVOKABLE void startLogin(const QString &serverUrl);
    Q_INVOKABLE void cancelLogin();
    Q_INVOKABLE void logout();
    Q_INVOKABLE void reloadSecrets();
    // Pulls the whole account from the Notes API and keeps the answer on the
    // device.
    Q_INVOKABLE void sync();

signals:
    void readyChanged();
    void configuredChanged();
    void busyChanged();
    void errorMessageChanged();
    void secretsErrorChanged();
    void loginUrlChanged();
    void serverUrlChanged();
    void ignoreSslErrorsChanged();
    void loginUrlReady(const QString &url);
    void loginSucceeded();
    void loginFailed(const QString &error);
    void notesChanged();

private:
    void setBusy(bool busy);
    void setError(const QString &error);
    void applyCredentials();
    void applyApiSettings();
    void reloadModels();
    void onConfiguredChanged();
    void onLoginUrl(const QString &url);
    void onLoggedIn(const QString &server, const QString &user, const QString &password);
    void onLoginFailed(const QString &error);
    void onNotesReady(const QVector<LocalNote> &notes);
    void onApiFailed(const QString &error);

    Config *m_config;
    NotesModel *m_notes;
    LoginFlow m_login;
    NotesApi m_api;
    NotesCache m_cache;
    QString m_loginUrl;
    QString m_error;
    bool m_busy;
};

#endif
