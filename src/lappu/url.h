#ifndef LAPPU_URL_H
#define LAPPU_URL_H

#include <QString>
#include <QUrl>

namespace Url {

QString normalizeServer(const QString &input);
QUrl join(const QUrl &base, const QString &href);
QString origin(const QUrl &url);
QString pathWithSlash(const QString &path);

}

#endif
