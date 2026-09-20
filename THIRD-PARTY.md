# Third-party code

## potrace

`lib/potrace/` — the tracing core of [potrace](https://potrace.sourceforge.net/)
1.16 by Peter Selinger: `potracelib.c`, `curve.c`, `decompose.c`, `trace.c` and
their headers, vendored unmodified. This is the same library Inkscape's
"Trace Bitmap" is built on.

`lib/potrace/config.h` is **not** upstream: it is a small hand-written stub
standing in for the file potrace's configure script would generate, defining
only the few macros the four vendored sources look at.

**Licence:** GNU General Public License, version 2 or later (`lib/potrace/COPYING`).

That is why this application is GPLv3: GPL-2.0-**or-later** code may be combined
into a GPLv3 work, and the result is GPLv3. GPL-2.0-only code could not have
been used this way.

## zinnia

`lib/zinnia/` — Zinnia, online handwriting recognition from stroke
coordinates, by Taku Kudo. New BSD licence; see `lib/zinnia/COPYING`.
Eight source files plus headers, taken unmodified from
https://github.com/taku910/zinnia. `lib/zinnia/config.h` is a hand written
stand-in for the autotools one, as with potrace.
