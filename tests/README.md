# Tests

`run_all_demos.sh` -- real test coverage for this project's demo suite,
added in Milestone 6 (see `docs/roadmap.md`). Builds every demo (the
original five plus Milestone 6's three new ports: `tagged_font_demo`,
`tagged_image_demo`, `tagged_annotation_demo`), runs each one to
(re)generate its PDF, and validates every PDF against the real PDF/UA-1
ruleset via `validate/run_verapdf.sh` -- exercising libharu's own
TrueType-embedding, raw-image, annotation, table, and path-painting code
paths through this project's tagging layer, not just this project's own
additions in isolation.

Each demo has a recorded baseline (see the `DEMOS` table inside the
script): the seven fully tagged demos must reach full PDF/UA-1
compliance (veraPDF `PASS`, 0 failed rules); `docmeta_demo` is Milestone
0 scaffolding (untagged page content, a non-embedded Standard-14 font)
and is deliberately not fully compliant -- its own recorded baseline is
"at most 3 failed rules". The script fails loudly (nonzero exit, one
`FAIL:` line per regressed demo naming which demo and how many rules
failed) if any demo's real veraPDF result is worse than its recorded
baseline; it does not require `docmeta_demo` to reach full `PASS`, since
that was never its scope.

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
