# alien-twin

Birnam Alien wrapper for [Twin](https://github.com/cosmos72/twin), the
network-transparent text-mode window system. The project vendors Twin and
builds its `libtw` client library into a single `libtwin.dylib`; no system Twin
development package is required.

The wrapper targets Birnam 0.14 on arm64 macOS. Using it requires a running Twin
server, but building and running the unit tests does not.

## Build and test

```sh
birnam check
birnam build
birnam test
birnam run
```

With a Twin server running on `:0`, execute the live connection/window demo:

```sh
birnam run -- :0
```

Leave the window up for 30 minutes (or until the Birnam process is stopped):

```sh
birnam run -- :0 --stay
```

The demo creates a window, writes at two cursor positions, changes its title,
resizes it, holds it onscreen for three seconds, samples a queued event, and
cleans up. A tty-backed server does not enable external clients automatically:
press `Pause`, then choose `Modules` -> `Run Socket Server` in Twin first. Use
the server's actual display number (`:0`, `:1`, and so on).

The native build configures the vendored tree in Birnam's private build staging,
builds a static PIC `libtw`, and links it with `native/alien_twin.c`. It does not
modify generated files beneath `vendor/twin`.

## Connect and create a window

Start a Twin server separately. `Twin open` uses `$TWDISPLAY`; `Twin open:`
accepts an explicit Twin display such as `":0"` or `"host:0"`.

```birnam
let connection := [Twin open: ":0"];
[[connection isNull]
  ifTrue: [| [System println: [Twin lastError]]]
  ifFalse: [|
    let window := [connection twinCreateWindow: "Birnam" width: 60 height: 12];
    [connection twinWriteUtf8: window text: "hello from Birnam\n"];
    [connection twinSync]]]
```

Connections are opaque `ForeignAddress` values. Call `twinClose` exactly once
when finished. The wrapper exposes:

- connection metadata: `twinConnectionFd`, `twinLibraryVersion`,
  `twinServerVersion`, `twinDisplayWidth`, and `twinDisplayHeight`;
- connection state: `twinFlush`, `twinSync`, `twinInPanic`, and `twinClose`;
- creation: `twinCreateWindow:width:height:` plus the configurable
  `twinCreateWindow:width:height:cursor:attributes:flags:`;
- content: `twinWriteUtf8:text:`, `twinGoto:x:y:`,
  `twinSetTextColor:foreground:background:`, and
  `twinFillRect:x:y:width:height:codepoint:foreground:background:`;
- geometry/stacking: `twinMove:x:y:`, `twinResize:width:height:`,
  `twinScroll:dx:dy:`, `twinSetVisible:visible:`, `twinFocus:`, `twinRaise:`,
  and `twinLower:`;
- metadata/lifetime: `twinSetTitle:title:` and `twinDeleteWindow:`.

Colors cross the bridge as portable 24-bit `0xRRGGBB` integers; the C side
constructs Twin's ABI-dependent `trgb`, `tcolor`, and `tcell` values. Rectangle
fill similarly accepts a Unicode codepoint and never exposes a native cell
array to Birnam.

All window functions answer `1` on success or `0` on failure. Creation answers
the unsigned Twin object id, with `0` indicating failure. `[Twin lastError]`
returns the most recent bridge or libtw error.

## Events

Read without blocking:

```birnam
let event := [connection twinNextEventWait: 0];
[[event isNull] ifFalse: [|
  [System println: [event twinEventType]];
  [System println: [event twinEventText]];
  [event twinEventFree]]]
```

Pass `1` to wait for an event. Each non-null event is an owned snapshot and must
receive `twinEventFree` exactly once. Available fields are `twinEventType`,
`twinEventWidget`, `twinEventCode`, `twinEventShiftFlags`, `twinEventX`,
`twinEventY`, `twinEventWidth`, `twinEventHeight`, and `twinEventText`. The
`Twin eventDisplay`, `eventKey`, `eventMouse`, `eventChange`, `eventGadget`,
`eventMenuRow`, selection variants, `eventControl`, `eventControlReply`, and
`eventClientMessage` messages answer common event-type constants. `changeResize`
and `changeExpose` name the standard widget-change codes.

## Boundary and scope

Twin's public API contains custom-width integers, packed event unions,
version-dependent color/cell representations, variadic dispatch, and callback
listeners. The C bridge
keeps those ABI details out of Alien and presents only `int`, `unsigned long`,
C strings, and opaque pointers. The wrapper covers connection, configurable
window, UTF-8 output, rectangular cell drawing, geometry/stacking, and
copied-event paths. Menus, gadgets, selection ownership/data transfer,
inter-client messages, and listener callbacks remain available in the vendored
C API but are not exposed until a managed consumer needs those server-side
facilities.

## Vendored source and licensing

`vendor/twin` is an unmodified snapshot of upstream commit
`86120b859afa24bf11ab97fc9b65118c8e7ce8c5` from 2025-11-19. This matches the
locally installed Twin 1.0.0 server; later upstream revisions changed the
in-process color ABI without changing socket protocol 4.8.0. See
[`vendor/TWIN-UPSTREAM.md`](vendor/TWIN-UPSTREAM.md) for provenance.

Twin's server and clients are GPL-2.0-or-later; `libtw` and `libtutf` are
LGPL-2.0-or-later. The upstream `COPYING` and `COPYING.LIB` files are retained
inside the vendored tree.
