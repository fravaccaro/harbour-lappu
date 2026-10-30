---
layout: default
title: Using Lappu
nav_order: 2
permalink: docs/guide
has_children: true
has_toc: true
---

# Using Lappu

Lappu is a native client for the notes of a Nextcloud server you already run. The notes live on the server in its Notes app; the phone keeps a copy of them, so they can be read and written while the server is out of reach.

Lappu does not use the Nextcloud account in the Sailfish OS settings. That account syncs the calendar and the files, neither of which carries the notes, so this app signs in on its own.

## Requirements

* Sailfish OS 5.0 or newer (the app password is kept in Sailfish Secrets, and signing in uses the WebView)
* A reachable Nextcloud server with the Notes app

## The pages

* **[Signing in](signin)** - the server address, the sign-in page of the server, and what Lappu keeps afterwards
* **[Notes and categories](notes)** - the list, the All, Favorites and Uncategorized views, searching, and the cover
* **[Writing a note](editing)** - the title, the Markdown bar, undo and redo, task lines and categories
* **[Sync, files and permissions](files)** - when the notes are refreshed, what happens offline, where the copy is kept, and why the app asks for each permission

## Further help

* [Report an issue](https://github.com/fravaccaro/harbour-lappu/issues)
