# Vendored libraries

The image decoders HighWire links against, prebuilt for m68k-atari-mint.
Neither the toolkit container nor the FreeMiNT packages CI installs carry
them, so they live here and every build links the same files. zlib is not
here: the toolchain already has it.

| Library | Version | Origin | Licence |
|---|---|---|---|
| `giflib/` | 5.1.4 | <https://github.com/RajahLone/libpnggif.a> | MIT, in `COPYING` |
| `libpng/` | 1.8.0.git | <https://github.com/pnggroup/libpng> and <https://github.com/RajahLone/libpng.a> | libpng, in `LICENSE` |
| `jpeg/` | 10.0.0 | <https://ijg.org/> and <https://github.com/RajahLone/libjpeg.a>  | IJG, under LEGAL ISSUES in `README` |
| `libnsbmp/` | 0.1.7 | <http://www.netsurf-browser.org/> and <https://github.com/RajahLone/libnsbmp.a>  | MIT, in `COPYING` |
| `ldg/` | 2.35 | <https://sourceforge.net/projects/ldg/files/> | Lesser GPL in `COPYRIGHT` |
| `mdebtls/` | 3.6.7 | <https://github.com/Mbed-TLS/mbedtls> | Apache 2.O in `LICENSE` |

- the top level, for the 68000 and any build with `FPU=0`
- `m68020-60/`, for the 020 and up with an FPU
- `m5475/`, for ColdFire

The IJG licence asks that a binary-only release say, in its documentation,
that it "is based in part on the work of the Independent JPEG Group";
docs/HIGHWIRE.DOC and the help pages do.
