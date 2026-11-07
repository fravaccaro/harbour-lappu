#ifndef LAPPU_SHAREDTEXT_H
#define LAPPU_SHAREDTEXT_H

#include <QString>

namespace SharedText {

// The body a share of plain text makes: the text as it was sent, with the
// line endings of wherever it was written made into the ones a note is
// stored with.
QString body(const QString &text);

// The body a shared link makes: a Markdown link when the sender named the
// page, the address on its own otherwise. An address nothing in a note could
// have followed gives an empty string, which is what tells the caller there
// was nothing worth writing down.
QString linkBody(const QString &url, const QString &linkTitle);

}

#endif
