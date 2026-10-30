# Lappu

A Nextcloud Notes client for Sailfish OS.

Lappu signs in through a Nextcloud server's own login page (login flow v2),
saves the generated app password in the device-lock Sailfish Secrets storage,
and keeps your notes on the device. It is a sibling to
[Askare](https://github.com/fravaccaro/harbour-askare), the Nextcloud tasks
client it shares its plumbing with.

> **Status:** early scaffold. The login flow, secure password storage, and the
> UI shell are in place; the Notes sync layer is not implemented yet, so the
> notes list is empty.

## Building

Built with qmake against the Sailfish OS Qt 5.6 toolchain:

```sh
~/SailfishOS/bin/sfdk build
```

## Layout

See [`AGENTS.md`](AGENTS.md) for the project layout and engineering
conventions.

## License

GPL-3.0-only.
