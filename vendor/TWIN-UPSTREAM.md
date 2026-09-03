# Twin upstream snapshot

- Repository: https://github.com/cosmos72/twin
- Commit: `86120b859afa24bf11ab97fc9b65118c8e7ce8c5`
- Commit date: 2025-11-19T18:48:48+01:00
- Imported: 2026-08-28
- Method: copied from a clean local clone, excluding upstream `.git`

This revision matches the locally installed Twin 1.0.0 server. Later upstream
revisions retain socket protocol 4.8.0 while changing the in-process `tcolor`
ABI, so clients built from those revisions cannot create menus on this server.

The files below `vendor/twin` are otherwise unmodified. Twin's server and
clients are distributed under GPL-2.0-or-later; `libtw` and `libtutf` are
distributed under LGPL-2.0-or-later. See `twin/COPYING` and
`twin/COPYING.LIB` for the full license texts.
