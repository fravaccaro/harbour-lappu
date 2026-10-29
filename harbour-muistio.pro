TARGET = harbour-muistio

CONFIG += sailfishapp sailfishapp_i18n c++11
QT += network

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
    src/muistio/config.cpp \
    src/muistio/httpclient.cpp \
    src/muistio/loginflow.cpp \
    src/muistio/notesmodel.cpp \
    src/muistio/secretsstore.cpp \
    src/muistio/session.cpp \
    src/muistio/url.cpp

HEADERS += \
    src/muistio/config.h \
    src/muistio/httpclient.h \
    src/muistio/loginflow.h \
    src/muistio/notesmodel.h \
    src/muistio/secretsstore.h \
    src/muistio/session.h \
    src/muistio/types.h \
    src/muistio/url.h

SAILFISHAPP_ICONS = 86x86 108x108 128x128 172x172

appicon.files = icons/appicon.png
appicon.path = /usr/share/$${TARGET}
INSTALLS += appicon

TRANSLATIONS += \
    translations/harbour-muistio-it.ts

DISTFILES += \
    harbour-muistio.desktop \
    icons/harbour-muistio.svg \
    qml/harbour-muistio.qml \
    qml/components/MuistioNotification.qml \
    qml/cover/CoverPage.qml \
    qml/pages/AboutPage.qml \
    qml/pages/LoginPage.qml \
    qml/pages/NotesPage.qml \
    qml/pages/SettingsPage.qml \
    qml/pages/StartPage.qml \
    qml/pages/TranslationsPage.qml \
    qml/pages/WebLoginPage.qml \
    translations/harbour-muistio.ts
