# Provenance

This project vendors libharu 2.4.5 (https://github.com/libharu/libharu,
zlib/libpng-style license, see `LICENSE`) under `vendor/libharu/`, unmodified
except where noted below, and adds a new `hpdf_ua` module on top of it that
does not touch libharu's own source files. Per the license's condition 2
("Altered source versions must be plainly marked as such"): the vendored
copy under `vendor/libharu/` is presented as-is from libharu 2.4.5 with no
edits (verify with a diff against upstream 2.4.5 at any time); all new
functionality lives in `src/ua/` and `include/hpdf_ua/`, and is clearly this
project's own addition, not part of upstream libharu.

Trimmed from the original libharu source tree when vendoring (not needed for
this project's scope, and not carried forward): `demo/` (this project has its
own PDF/UA-focused demos under top-level `demo/`), `bindings/` (Python/C#/
Ruby/etc. language bindings -- out of scope), `win32/` (this project targets
Unix/Linux/macOS and Windows only via a POSIX-compatible layer such as WSL,
Cygwin, or MSYS2 -- no native-Windows build path), `doc/` (Doxygen
configuration for the upstream project's own docs).

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
