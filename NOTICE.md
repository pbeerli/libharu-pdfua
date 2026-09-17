# Provenance

This project vendors libharu 2.4.5 (https://github.com/libharu/libharu,
zlib/libpng-style license, see `LICENSE`) under `vendor/libharu/`, unmodified
except where noted below, and adds a new `hpdf_ua` module on top of it that
does not touch libharu's own source files (with the one exception below).
Per the license's condition 2 ("Altered source versions must be plainly
marked as such"): all new functionality lives in `src/ua/` and
`include/hpdf_ua/`, and is clearly this project's own addition, not part of
upstream libharu.

**One exception, plainly marked here and in the file itself**:
`vendor/libharu/src/hpdf_font_cid.c` carries a real fix, added while
integrating this project's tagging module into Migrate-n's own report code
(`pretty.c`): `HPDF_Type0Font_New()`'s "Identity-H" branch (used by
`HPDF_UseUTFEncodings()`'s "UTF-8" encoder -- the only encoder in this
codebase using that ordering; no CJK encoder is affected) reused
`CreateCMap()`, which emits `cidrange`/`cidchar` operators, for the font's
`/ToUnicode` entry -- valid syntax for a font's `/Encoding` CMap, but not
for `/ToUnicode` (PDF32000-1:2008 9.10.3 requires `bfchar`/`bfrange`); real
consumers (confirmed directly with veraPDF) correctly refuse to resolve any
glyph through the resulting stream. Fixed by adding a separate
`CreateToUnicodeCMap()` that emits a real `bfrange`-based CMap instead
(split into 256-code, single-row chunks -- a bfrange whose low byte
overflows its own row does not resolve correctly in practice, confirmed
directly: ASCII digits worked through a single `<0000> <FFFF>` range,
U+0398 GREEK CAPITAL LETTER THETA, a different row, did not, until
splitting fixed it for both). Verified via veraPDF against a real embedded-
Liberation-Sans-as-Unicode-CID-font page (Migrate-n's own `symbol_Theta()`
family): PDF/UA-1 clause 7.21.7 ("glyph cannot be mapped to Unicode") went
from 64 failing checks to 0, full document compliance achieved
(`isCompliant="true"`), no other libharu code path touched or affected.

Trimmed from the original libharu source tree when vendoring (not needed for
this project's scope, and not carried forward): `demo/` (this project has its
own PDF/UA-focused demos under top-level `demo/`), `bindings/` (Python/C#/
Ruby/etc. language bindings -- out of scope), `win32/` (this project targets
Unix/Linux/macOS and Windows only via a POSIX-compatible layer such as WSL,
Cygwin, or MSYS2 -- no native-Windows build path), `doc/` (Doxygen
configuration for the upstream project's own docs).

**`images/pngsuite/`** (Milestone 6, added for `demo/tagged_png_demo.c` and
`demo/tagged_image_transform_demo.c`): 8 test images from Willem van
Schaik's PNGSuite (http://www.schaik.com/pngsuite/), copied unmodified from
libharu's own upstream `demo/pngsuite/` (trimmed from this project's own
vendored copy along with the rest of `demo/`, see above -- these 8 files are
reintroduced separately, not un-trimmed wholesale). PNGSuite's own license
(`images/pngsuite/README`, copied alongside): "(c) Willem van Schaik, 1999.
Permission to use, copy, and distribute these images for any purpose and
without fee is hereby granted." Not the same license as either upstream
libharu or this project (see `LICENSE`), and independent of both.

**`images/jpeg-demo/`** (Milestone 6, added for `demo/tagged_jpeg_demo.c`,
after an earlier pass deliberately skipped this demo -- see
`docs/roadmap.md`'s fifth-pass entry -- because upstream libharu's own
`demo/images/rgb.jpg`/`gray.jpg` have no stated license anywhere):
`cactus.jpg` and `dragonfly.jpg` are two of the project maintainer's own
photographs (resized from the originals; `dragonfly.jpg` also converted to
grayscale to exercise libharu's separate grayscale-JPEG code path, matching
upstream's own rgb.jpg/gray.jpg split), copyright Peter Beerli, licensed
CC BY 4.0 (`images/jpeg-demo/LICENSE.txt`, copied alongside) -- not the same
license as either upstream libharu or this project (see `LICENSE`), and
independent of both.

**`fonts/NotoSansJP-Regular.ttf`, `fonts/NotoSansSC-Regular.ttf`**
(Milestone 6, added for `demo/tagged_japanese_font_demo.c` and
`demo/tagged_chfont_demo.c`, the CJK demo batch): the "japanese" and
"chinese-simplified" Google Fonts subsets of Noto Sans JP and Noto Sans
SC respectively, downloaded from Google Fonts
(https://fonts.google.com/noto) as real TrueType (glyf-outline) builds --
deliberately not the upstream Noto CJK project's own OTF/OTC releases,
which use CFF outlines libharu's TrueType loader (`vendor/libharu/src/
hpdf_fontdef_tt.c`, requires a `glyf` table) cannot parse; confirmed
directly by inspecting both fonts' own table directories before
vendoring either one. Licensed under the SIL Open Font License 1.1
(`fonts/NotoSansCJK-LICENSE.txt`, copied alongside, same license text
Google Fonts itself distributes with both families) -- the same license
family as `fonts/DejaVuSans-LICENSE.txt`'s Arev-glyph portion, but a
separate, independent grant covering only these two font files.

# Why this project exists

Migrate-n (https://github.com/pbeerli/migrate-5.0.7) generates its graphical
report as a PDF via a vendored copy of libharu. libharu can produce basic
PDF/A metadata (see `hpdf_pdfa.c`: XMP metadata, `/MarkInfo`, an empty
`/StructTreeRoot`) but emits no actual tagged content -- no marked-content
operators, no populated structure tree, no role map, no alternate text, no
table header associations. That is not enough for a screen reader to make
sense of the document, and it is not enough to pass PDF/UA-1 validation
(e.g. with veraPDF). This project's goal is to close that gap in libharu
itself, generically -- with no dependency on Migrate, `world_fmt`, or any
other Migrate internals -- so that Migrate (and anyone else using libharu)
can eventually generate real accessible, tagged PDF output.

See `README.md` for the technical plan and `docs/pdf_ua_requirements.md` for
the concrete PDF/UA-1 checklist this project is building against.
