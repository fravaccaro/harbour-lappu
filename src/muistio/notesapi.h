#ifndef MUISTIO_NOTESAPI_H
#define MUISTIO_NOTESAPI_H

#include <QObject>
#include <QString>
#include <QUrl>
#include <QVector>

#include "httpclient.h"
#include "types.h"

class NotesApi : public QObject
{
    Q_OBJECT

public:
    explicit NotesApi(QObject *parent = nullptr);

    void setServer(const QString &serverUrl);
    void setCredentials(const QString &user, const QString &password);
    void setIgnoreSslErrors(bool ignore);

    // Asks for every note, content included, as the Notes API hands them out
    // in a single answer.
    void fetchNotes();

signals:
    void notesReady(const QVector<LocalNote> &notes);
    void failed(const QString &error);

private:
    void onFinished(int status, const QByteArray &body, const QString &etag,
                    const QUrl &url);
    void onFailed(const QString &error);

    HttpClient m_http;
    QString m_server;
    bool m_fetching;
};

#endif
