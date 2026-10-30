#include "url.h"

namespace Url {

QString normalizeServer(const QString &input)
{
    QString url = input.trimmed();
    if (url.isEmpty())
        return QString();

    if (!url.contains(QStringLiteral("://")))
        url.prepend(QStringLiteral("https://"));

    while (url.endsWith(QLatin1Char('/')))
        url.chop(1);

    return url;
}

}
