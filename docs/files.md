---
layout: default
title: Sync, files and permissions
parent: Using Lappu
nav_order: 4
---

# Sync, files and permissions

## When the notes are refreshed

**Refresh** in the pulley menu is always there, and the settings page offers three ways to have it happen by itself:

* **Refresh when the app starts** - one refresh when Lappu opens, and no more
* **Refresh after saving a note** - fetches the server's changes once a saved note has been sent
* **Automatic refresh** - every 5, 15, 30 or 60 minutes, or off

Automatic refresh runs **only while Lappu is running**. Sailfish OS wakes nothing on the app's behalf for this, so a phone with Lappu closed is not refreshed until it is opened again. The interval is counted from the end of the last refresh rather than from when it was chosen, so a refresh you ask for yourself also pushes the next one back.

Signing in refreshes whatever the startup setting says, because a new account has nothing to show until it does.

## What a refresh does

A refresh **sends what is owed before it fetches anything**. Saving a note, favouriting it or deleting it writes the change to the copy on the phone straight away and marks that note as owed; the refresh offers the owed notes to the server first, then pulls the server's notes over the result.

Doing it in that order is what makes a note edited on the phone the one that stands, rather than being overwritten by the older copy coming back from the server.

## Editing with no server in reach

A change made with the server out of reach stays marked as owed. It survives closing the app and is offered again by the next refresh, so nothing has to be retyped.

A note created offline is listed and can be opened and edited like any other; it is only waiting for the server to give it an id. Lappu does not announce a server it could not reach unless you asked for the refresh yourself, so working offline is quiet rather than a stream of complaints.

## When the same note changed twice

A note changed on the phone and on the server between two refreshes is resolved as **last write wins**: the copy sent by the phone replaces what the server held, and no attempt is made to merge the two or to ask. Lappu does not send the version it last read, so the server does not reject the change either.

Refreshing before a long edit, and again afterwards, is the way to avoid losing the other side's work.

## Where the copy is kept

Sailjail gives Lappu three private directories of its own, named after the organisation and application in the desktop file:

| Path | Holds |
|---|---|
| `~/.config/org.frapps/harbour-lappu` | `settings.conf` - the server address, the user name, the sort and sync settings, the pinned note |
| `~/.local/share/org.frapps/harbour-lappu` | `notes.sqlite` - the copy of the notes |
| `~/.cache/org.frapps/harbour-lappu` | whatever the web view leaves behind while signing in; nothing the app writes itself |

The app password is not in any of these. It is kept in Sailfish Secrets, in a collection locked with the device lock.

The config and data directories are declared for the system backup, so a backup made by Sailfish OS carries the settings and the local copy of the notes.

## Permissions

Lappu asks for three:

* **Internet** - to reach your server; there is nothing to show without it
* **Secrets** - to keep the app password in the device-lock storage rather than in a plain file
* **WebView** - to show the sign-in page of the server inside the app, so your own password is never typed into Lappu

No document, picture or other directory of the device is asked for.
