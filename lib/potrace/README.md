# potrace (vendored)

The tracing core of [potrace](https://potrace.sourceforge.net/) 1.16 by Peter
Selinger — `potracelib.c`, `curve.c`, `decompose.c`, `trace.c` and their
headers. This is the same library Inkscape's "Trace Bitmap" is built on.

Only the library is here. The command-line front end and every output backend
(EPS, PDF, SVG, XFig, GeoJSON…) are left out: this app turns the traced curves
into its own stroke model and writes its own SVG.

`config.h` is not generated; every include of it is guarded by `HAVE_CONFIG_H`,
which is not defined, so the portable code paths are used.

**Licence: GNU GPL v2 or later** (see `COPYING`), which is why this app is
GPLv3 — the two are compatible in that direction only.

Do not edit these files. To update, copy the four sources and their headers
from a newer potrace release.
