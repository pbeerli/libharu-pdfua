# Tests

`run_all_demos.sh` -- real test coverage for this project's demo suite,
added in Milestone 6 (see `docs/roadmap.md`). Builds every demo (the
original five, Milestone 6's first pass -- `tagged_font_demo`,
`tagged_image_demo`, `tagged_annotation_demo` -- Milestone 6's second
pass -- `tagged_arc_demo`, `tagged_line_demo`, `tagged_ext_gstate_demo`,
`tagged_font_list_demo`, `tagged_text_demo`, `tagged_encoding_list_demo`,
`tagged_outline_demo` -- Milestone 6's third pass --
`tagged_encryption_demo`, `tagged_text_annotation_demo`,
`tagged_attach_demo`, `tagged_slide_show_demo` -- and Milestone 6's
fourth pass -- `tagged_png_demo`, `tagged_image_transform_demo`, which
need libpng installed, see `CMakeLists.txt`), runs each one to
(re)generate its PDF, and validates every PDF against the real PDF/UA-1
ruleset via `validate/run_verapdf.sh` -- exercising libharu's own
TrueType-embedding, raw-image, annotation, table, path-painting, vector
graphics, extended graphics state, text-feature, encoding, outline,
encryption, attachment, and PNG-decoding code paths through this
project's tagging layer, not just this project's own additions in
isolation.

Each demo has a recorded baseline (see the `DEMOS` table inside the
script): nineteen of the twenty-one demos must reach full PDF/UA-1
compliance (veraPDF `PASS`, 0 failed rules); two are deliberately not
fully compliant, each with its own documented, bounded baseline:
`docmeta_demo` is Milestone 0 scaffolding (untagged page content, a
non-embedded Standard-14 font, "at most 3 failed rules"), and
`tagged_font_list_demo` exists specifically to show libharu's
non-embeddable Standard-14 fonts ("at most 2 failed rules" -- see that
file's own top comment). The script fails loudly (nonzero exit, one
`FAIL:` line per regressed demo naming which demo and how many rules
failed) if any demo's real veraPDF result is worse than its recorded
baseline; it does not require the two intentionally-partial demos to
reach full `PASS`, since that was never their scope.

A `DEMOS` entry may carry a third, optional `:password` field --
`tagged_encryption_demo.pdf` is password-protected, and veraPDF refuses
an encrypted PDF outright ("appears to be an encrypted PDF") unless
given its user password via `--password`, which this script passes
through automatically for any demo whose entry has one.

Usage:

```
cmake -B build && cmake --build build
tests/run_all_demos.sh build
```

Verified to actually catch a regression, not just a happy-path no-op: a
temporary one-line change to `HPDF_UA_SetAlternateText()` (making it a
no-op, simulating a real future refactor accidentally dropping `/Alt`)
was built and run through this script, which correctly reported `FAIL`
for exactly the five demos that rely on `/Alt` (histogram, skyline,
example, image, annotation -- the last of these because
`HPDF_UA_TagAnnotation()`'s `/Contents` copy also depends on `/Alt`
being set) while `tagged_table_demo` and `tagged_font_demo` (neither of
which calls `HPDF_UA_SetAlternateText()`) correctly kept passing. The
change was then reverted and a clean rebuild reconfirmed all eight
demos back at their recorded baselines.

For document-level metadata alone (`/Lang`, `/DisplayDocTitle`,
`/MarkInfo`), a lighter-weight direct check is also reasonable without a
full veraPDF run -- e.g. `qpdf --qdf --object-streams=disable` the
output and grep the plain-text QDF form for the expected catalog
entries -- not currently implemented, since `run_all_demos.sh`'s real
veraPDF run already covers this project's actual verification bar.

`test_install.sh` -- verifies the CMake install target (see
`CMakeLists.txt`'s "installation" section) actually works end-to-end for
a downstream consumer, not just that `cmake --install` runs without
error: it installs to a throwaway prefix, then configures and builds
`consume_package/` (a minimal separate CMake project) against that
install with `find_package(hpdf_ua)`, and runs the resulting binary.

```
tests/test_install.sh build
```

`consume_package/` is that minimal consumer project; it is not
`add_subdirectory()`-ed into the main build, only built by
`test_install.sh` against an installed tree.

Both scripts run in CI (`.github/workflows/ci.yml`) on every push and
pull request, on Linux and macOS.
