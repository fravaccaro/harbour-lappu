#ifndef LAPPU_NOTESAPI_H
#define LAPPU_NOTESAPI_H

#include <QObject>
#include <QString>
#include <QVector>

#include "httpclient.h"
#include "types.h"

class NotesApi : public QObject
{
    Q_OBJECT

public:
    // One request is in flight at a time, so the answer is read as whatever
    // was last asked for.
    enum Operation {
        Idle,
        FetchList,
        Create,
        Update,
        Delete
    };

    explicit NotesApi(QObject *parent = nullptr);

    void setServer(const QString &serverUrl);
    void setCredentials(const QString &user, const QString &password);
    void setIgnoreSslErrors(bool ignore);

    // Asks for every note, content included, as the Notes API hands them out
    // in a single answer.
    void fetchNotes();
    void createNote(const LocalNote &note);
    void updateNote(const LocalNote &note);
    void deleteNote(qint64 id);

signals:
    void notesReady(const QVector<LocalNote> &notes);
    // Carries the id the note was kept under here, so the caller can let go
    // of the one it made up.
    void created(qint64 localId, const LocalNote &note);
    void updated(const LocalNote &note);
    void deleted(qint64 id);
    void failed(const QString &error);
    // A password the server will not take again. It travels apart from failed()
    // because it asks the session to stop rather than to retry.
    void unauthorized(const QString &error);
    // The note as the server holds it, from the answer to a write it refused
    // because the version written over was not the one it had. It travels
    // apart from failed() because nothing is wrong with the server and nothing
    // is lost: the edit is only unsettled until it is said which side to keep.
    void conflicted(const LocalNote &remote);

private:
    void onFinished(int status, const QByteArray &body);
    void onFailed(const QString &error);
    void send(const QByteArray &method, const QString &path, const QByteArray &body,
              const QString &etag = QString());
    void handleWrite(Operation operation, int status, const QByteArray &body);
    void reportWritten(Operation operation, const QByteArray &body);
    void reportConflict(const QByteArray &body);
    void reportStatus(int status);

    HttpClient m_http;
    QString m_server;
    Operation m_operation;
    qint64 m_pendingId;
};

#endif
