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

Doing it in that order is half of what keeps a note edited on the phone from being overwritten by the older copy coming back from the server. The other half is the check below: an edit carries the version of the note it was made from, so a note that changed on the server as well is not quietly written over but handed back to you to settle.

## Editing with no server in reach

A change made with the server out of reach stays marked as owed. It survives closing the app and is offered again by the next refresh, so nothing has to be retyped.

A note created offline is listed and can be opened and edited like any other; it is only waiting for the server to give it an id. Lappu does not announce a server it could not reach unless you asked for the refresh yourself, so working offline is quiet rather than a stream of complaints.

A network that accepts the connection but never answers - a café or hotel Wi-Fi waiting for you to agree to its terms is the usual culprit - is given about half a minute of silence before the refresh is abandoned and said to have failed. A slow answer is not cut off: the wait only runs out while nothing at all is arriving.

## When the same note changed twice

An edit sent to the server carries the **version of the note it was made from**, and the server refuses it if that is not the version it holds - which is exactly the case where someone else, or another device, changed the same note in the meantime. Nothing is lost by the refusal: the note stops being offered, the refresh carries on with everything else, and the notes page grows a row at the top counting the notes that are waiting to be settled.

Tapping that row opens the two versions, the phone's above the server's, with three answers in the pulley menu:

* **Keep my version** - the edit is sent again, this time over the version the server just showed, so it lands rather than being refused a second time
* **Keep the server's version** - the server's text takes the note, and the edit made on the phone is let go of
* **Keep both** - the server's version takes the note back, and the text written on the phone goes on as a **note of its own**

Where both sides happened to end on the same text, category and star there is nothing to ask, so the note is settled without a word.

A note **deleted** on the phone is deleted on the server whatever became of it there: a delete is never refused over a version.

The questions themselves are not kept anywhere. Closing Lappu before answering one loses it, and the conflict page is empty until the next refresh - which finds the same note again, because the edit is still waiting on the phone.

A server too old to tell Lappu which version of a note it holds is written to the way it always was: last write wins, with nothing to refuse and nothing to ask.

Refreshing before a long edit, and again afterwards, is still the way to be asked as little as possible.

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
