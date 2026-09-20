/*
 * Minimal replacement for the config.h potrace's configure script would
 * generate. Only the handful of macros the four vendored sources actually
 * look at are defined here.
 *
 * Not part of upstream potrace — written for this build.
 */
#ifndef XN_POTRACE_CONFIG_H
#define XN_POTRACE_CONFIG_H

/* Reported by potrace_version(); keep it in step with the vendored release. */
#define VERSION "1.16"

/* decompose.c needs uint64_t, which lives in <inttypes.h> / <stdint.h>.
   Every target this app builds for is C99 or later. */
#define HAVE_INTTYPES_H 1
#define HAVE_STDINT_H 1

/* No i386 assembly: this builds for aarch64 and armv7hl, and the portable
   bit-counting fallbacks in bitops.h are fast enough for tracing a photo. */

#endif
