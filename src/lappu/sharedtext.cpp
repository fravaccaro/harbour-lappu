#include "sharedtext.h"

#include <QUrl>

namespace {

// Text arrives from whichever app sent it, so the line endings are made into
// the ones a note is written and stored with, and the characters no editor
// would have let the user type are dropped.
QString cleaned(const QString &text)
{
    QString out = text;
    out.replace(QStringLiteral("\r\n"), QStringLiteral("\n"));
    out.replace(QLatin1Char('\r'), QLatin1Char('\n'));
    out.remove(QChar(QChar::Null));
    return out.trimmed();
}

// An address worth writing down is one a tap on the note could have followed.
// A file path is not: the app reads no directory of the device, so a note
// holding one would promise something it cannot do.
bool followable(const QUrl &url)
{
    const QString scheme = url.scheme().toLower();
    return url.isValid()
            && (scheme == QStringLiteral("http")
                || scheme == QStringLiteral("https")
                || scheme == QStringLiteral("mailto"));
}

// The label of a Markdown link ends at the first closing bracket, so a page
// title carrying one of its own would cut the link in half.
QString linkLabel(const QString &title)
{
    QString label = cleaned(title);
    label.replace(QLatin1Char('\n'), QLatin1Char(' '));
    label.remove(QLatin1Char('['));
    label.remove(QLatin1Char(']'));
    return label.trimmed();
}

// The destination ends at the first closing parenthesis for the same reason,
// and an address holding one is better written plainly than written broken.
bool fitsInLink(const QString &address)
{
    return !address.contains(QLatin1Char(')'));
}

}

QString SharedText::body(const QString &text)
{
    return cleaned(text);
}

QString SharedText::linkBody(const QString &url, const QString &linkTitle)
{
    const QString address = cleaned(url);
    if (!followable(QUrl(address)))
        return QString();

    const QString label = linkLabel(linkTitle);
    if (label.isEmpty() || label == address || !fitsInLink(address))
        return address;

    return QStringLiteral("[%1](%2)").arg(label, address);
}
