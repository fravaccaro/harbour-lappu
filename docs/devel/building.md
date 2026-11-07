---
layout: default
title: Building
parent: Developers
nav_order: 2
---

# Building

## With the Sailfish SDK

With the [Sailfish SDK](https://docs.sailfishos.org/Tools/Sailfish_SDK/Installation/) installed:

```bash
sfdk config target SailfishOS-5.0.0.62-aarch64
sfdk qmake
sfdk make
sfdk rpm
```

Deploy to a connected device:

```bash
sfdk config device "My Phone"
sfdk deploy --sdk
```

The app has been built for Sailfish OS 5.0.

## Documentation

The documentation site is built by [`.github/workflows/pages.yml`](https://github.com/fravaccaro/harbour-lappu/blob/main/.github/workflows/pages.yml) - Jekyll with the just-the-docs theme, configured in `_config.yml` - and deployed to GitHub Pages on every push to `main`.

`README.md` is the home page of that site as well as the README, which is why it carries Jekyll front matter.

The marketing page under `landing/` is kept out of the Jekyll build by the `exclude` list in `_config.yml`; its published copy lives in the portfolio site, and the header comment in `landing/index.html` says where. Keep the two in sync when editing.

Screenshots belong in `docs/screenshots/` as `screenshot1.png` to `screenshot3.png`; the README strip and the hero image of the landing page both expect them there.

## Packaging notes

Harbour refuses an unstripped binary, and the SDK packaging path does not always strip by itself, so `rpm/harbour-lappu.spec` strips the binary before the package is sealed. The spec also carries the Chum metadata block.

The spec sets `__provides_exclude_from` for everything under the data directory, so the `qmldir` files of the bundled Opal modules do not turn into `Provides: qml(Opal.About)` and friends - Harbour forbids an application package from providing QML modules, and nothing outside this package imports them.

The runtime dependencies are the Silica components, the WebView, and the notification QML plugin. Sailfish Secrets is linked through `pkgconfig(sailfishsecrets)` in `harbour-lappu.pro`, which also pulls in `QT += network sql` for the API client and the SQLite copy of the notes.

`sailfishapp.prf` resolves `PKGCONFIG` itself after pulling in `link_pkgconfig`, so the `.pro` must not request `link_pkgconfig` directly - doing so drops `-lsailfishapp` from the link.

## Translations

Italian ships with the app. Translators work on the [Transifex project](https://explore.transifex.com/fravaccaro/lappu). English is already the language the strings are written in, so `translations/harbour-lappu-en.ts` is kept for the translators and is not listed in `TRANSLATIONS`: listing it there is what would compile it to a `.qm`. A new language is a further `.ts` file beside it:

1. Copy `translations/harbour-lappu.ts` to `translations/harbour-lappu-<code>.ts`.
2. Add it to `TRANSLATIONS` in `harbour-lappu.pro`.
3. Translate it with Qt Linguist and build; `sailfishapp_i18n` compiles and installs the `.qm`.
4. Add the desktop file's own lines for the language by hand, as step 3 does not reach them: `Comment[<code>]` under `[Desktop Entry]`, and `Description[<code>]` in each of the two `[X-Share Method ...]` sections.

Compiled `.qm` files are build artefacts and stay out of the repository.

### The strings in the desktop file

`harbour-lappu.desktop` carries four strings of its own: the app's `Comment`, and the `Description` of each share method, which is what the system's sharing list calls Lappu. They are **not** in the `.ts` files and never reach Transifex, so they are written into the file by hand, in the `Comment[it]` style the desktop entry specification defines. A language code alone covers its regions, so `Description[it]` is what an `it_IT` phone reads, and a line is only needed per region where the wording differs.

There is no way round that in a Harbour package. `MDesktopEntry` can take a translation from a catalog instead, through an `X-Amber-Translation-Id-Description` key, but it loads the catalog from `/usr/share/translations`, and a Harbour package may install nothing outside `/usr/share/harbour-lappu`. The Sailfish Share documentation says as much: a method description is localised with desktop entry keys, and falls back to the unlocalised value when the language has none.
