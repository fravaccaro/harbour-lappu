TARGET = harbour-lappu

CONFIG += sailfishapp sailfishapp_i18n c++11
QT += network sql

# sailfishapp.prf pulls in link_pkgconfig and resolves PKGCONFIG afterwards, so
# requesting link_pkgconfig here instead would drop -lsailfishapp from the link.
PKGCONFIG += sailfishsecrets

INCLUDEPATH += $$PWD/src

# Autocompletion for the Opal modules bundled under qml/modules. They install
# with the rest of the qml tree; main.cpp adds the matching runtime import path.
QML_IMPORT_PATH += qml/modules

isEmpty(APP_VERSION): APP_VERSION = 0.0-devel
DEFINES += APP_VERSION=\\\"$$APP_VERSION\\\"

CONFIG(debug, debug|release): DEFINES += QT_QML_DEBUG

SOURCES += \
    src/main.cpp \
    src/lappu/config.cpp \
    src/lappu/httpclient.cpp \
    src/lappu/loginflow.cpp \
    src/lappu/notesapi.cpp \
    src/lappu/notescache.cpp \
    src/lappu/notesmodel.cpp \
    src/lappu/secretsstore.cpp \
    src/lappu/session.cpp \
    src/lappu/url.cpp

HEADERS += \
    src/lappu/config.h \
    src/lappu/httpclient.h \
    src/lappu/loginflow.h \
    src/lappu/notesapi.h \
    src/lappu/notescache.h \
    src/lappu/notesmodel.h \
    src/lappu/secretsstore.h \
    src/lappu/session.h \
    src/lappu/types.h \
    src/lappu/url.h

SAILFISHAPP_ICONS = 86x86 108x108 128x128 172x172

appicon.files = icons/appicon.png
appicon.path = /usr/share/$${TARGET}
INSTALLS += appicon

TRANSLATIONS += \
    translations/harbour-lappu-it.ts

DISTFILES += \
    harbour-lappu.desktop \
    icons/harbour-lappu.svg \
    qml/harbour-lappu.qml \
    qml/components/ConflictVersion.qml \
    qml/components/EditHistory.qml \
    qml/components/FilterItem.qml \
    qml/components/LappuNotification.qml \
    qml/components/LappuSupportDialog.qml \
    qml/components/MarkdownCode.qml \
    qml/components/MarkdownTable.qml \
    qml/components/MarkdownToolbar.qml \
    qml/components/NoteBody.qml \
    qml/components/NoteContextMenu.qml \
    qml/components/NotesListHeader.qml \
    qml/components/NotesPullDownMenu.qml \
    qml/cover/CoverPage.qml \
    qml/cover/cover-note.png \
    qml/notes/NoteFormat.js \
    qml/notes/NotesStore.qml \
    qml/pages/AboutPage.qml \
    qml/pages/CategoriesPage.qml \
    qml/pages/CategoryPickerPage.qml \
    qml/pages/ConflictPage.qml \
    qml/pages/LoginPage.qml \
    qml/pages/NoteEditPage.qml \
    qml/pages/NotePage.qml \
    qml/pages/NotesPage.qml \
    qml/pages/SettingsPage.qml \
    qml/pages/StartPage.qml \
    qml/pages/TranslationsPage.qml \
    qml/pages/WebLoginPage.qml \
    translations/harbour-lappu.ts
