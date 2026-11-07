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

Lappu syncs the notes itself. The Nextcloud account in the Sailfish OS settings syncs the calendar and the files, neither of which carries the notes, so the notes are this app's own business either way. Its sign-in, though, can be borrowed: Lappu signs in either through the server's own page, keeping the password it is given, or as an account from the settings, reading that account's password at every start and keeping none.

## Requirements

* Sailfish OS 5.0 or newer (the app password is kept in Sailfish Secrets, and signing in uses the WebView)
* A reachable Nextcloud server with the Notes app

## The pages

* **[Signing in](signin)** - the server address, the sign-in page of the server, signing in as a Nextcloud account of Sailfish OS, and what Lappu keeps afterwards
* **[Notes and categories](notes)** - the list or grid, the All, Favorites and Uncategorized views, searching, and the cover
* **[Writing a note](editing)** - the title field, text shared in from other apps, the Markdown bar, footnotes, undo and redo, task lines, tables and categories
* **[Sync, files and permissions](files)** - when the notes are refreshed, what happens offline, where the copy is kept, and why the app asks for each permission

## Supporting the app

Once you have used Lappu for a while it asks, once, whether you would like to give something back: a tip, a translation, or a bug report. Dismissing it is an answer, and it waits a long time before asking again.

## Further help

* [Report an issue](https://github.com/fravaccaro/harbour-lappu/issues)
