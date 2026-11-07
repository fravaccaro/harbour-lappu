---
layout: default
title: Signing in
parent: Using Lappu
nav_order: 1
---

# Signing in

There are two ways in, and the sign-in page offers both. **Sign in** opens the server's own page inside Lappu and lets it create a password for the app. **Use a Nextcloud account** signs in as an account you have already added under *Settings → Accounts*, without Lappu keeping a password at all.

## Through the server's own page

Enter the address of your Nextcloud server, for example `https://cloud.example.org`, and tap **Sign in**. The sign-in page of the server then opens inside Lappu: sign in there as you would in a browser. Single sign-on and two-factor authentication work, because whatever the page accepts is what grants access.

Once the server grants access it creates an app password named Lappu, and Lappu keeps that. Your own password is never asked for and never stored. The app password can be revoked at any time on the server, under *Settings → Security*; Lappu then asks to sign in again.

The app password is kept in **Sailfish Secrets**, in a collection locked with the device lock, not in a plain file.

Afterwards the settings page names both the server and the account the password belongs to, which is worth a look if you keep more than one account on the same server.

A sign-in left unfinished does not wait forever. The server stops accepting the attempt after twenty minutes, so Lappu gives up at the same point and says so rather than waiting on a page that can no longer succeed.

## Through a Nextcloud account of Sailfish OS

**Use a Nextcloud account**, on the sign-in page, lists the Nextcloud accounts you have added under *Settings → Accounts* and signs in as the one you choose. Lappu then reads its password from Sailfish OS **every time it starts** and keeps no copy of its own: the password lives where the system keeps it, beside the one the calendar and the files already use.

That has two consequences worth knowing. Changing the account's password under *Settings → Accounts* is enough - there is nothing to update in Lappu. And signing out of Lappu leaves the account itself alone, since it belongs to the settings and other apps sign in with it too.

The account has to name a server and have at least one of its services switched on, which is how it is set up by default. An account signed in by a token rather than a password cannot be used: the Notes API is reached with a name and a password, and a token is not something Lappu can send.

This does not change what Lappu syncs. The Nextcloud account of Sailfish OS carries the calendar, the contacts and the files, none of which hold the notes; the account is used here for its sign-in and nothing else.

## When the password stops working

A password the server no longer accepts - usually one revoked on the server, or an account whose access has changed - takes you back to the sign-in page with the reason shown. The server address is still filled in, so signing in again is a matter of tapping **Sign in** and letting the server's page take over.

Where the password comes from a Nextcloud account of Sailfish OS, it is read again first. A password changed under *Settings → Accounts* while Lappu held the old one is picked up that way and the refresh simply carries on; only a second refusal takes you back to the sign-in page.

Nothing you wrote is lost in the process. Unlike signing out, this keeps the local copy of the notes and anything still waiting to be uploaded, so a note written before the password expired is sent once you have signed in again.

## Self-signed certificates

A server whose certificate the device does not trust cannot be reached until **Ignore certificate errors** is turned on. The switch sits on the sign-in page and, afterwards, on the settings page. Leave it off for a server with a certificate the device already trusts.

## When the password cannot be read

Lappu waits for the password before it shows anything. If the secure storage cannot be read - usually because the device lock has not been entered since the phone started - the app says so, reports what the storage complained about, and offers **Try again** instead of the notes list. An account of Sailfish OS that cannot be signed in to is reported the same way, naming the account rather than the storage.

## Signing out

**Sign out**, on the settings page, removes the app password from Sailfish Secrets and empties the local copy of the notes. Where the password came from a Nextcloud account of Sailfish OS there is nothing of it to remove, and the account itself is left exactly as it was. Anything not yet uploaded is lost with it, so refresh first if a change is still waiting. The note on the cover is let go of at the same time. The app password itself remains listed on the server until you revoke it there.
