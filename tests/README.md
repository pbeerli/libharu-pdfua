# Tests

No automated tests yet -- this is Milestone 0 (scaffolding). Once Milestone 1
lands (real structure-tree/marked-content output), add a `CMakeLists.txt`
here wiring CTest cases that:

1. Build each demo.
2. Run `validate/run_verapdf.sh` against its output and assert a clean
   PDF/UA-1 pass (exit 0).
3. For document-level metadata (`/Lang`, `/DisplayDocTitle`, `/MarkInfo`),
   a lighter-weight direct check is also reasonable without a full veraPDF
   run -- e.g. `qpdf --qdf --object-streams=disable` the output and grep the
   plain-text QDF form for the expected catalog entries, for a fast
   pre-veraPDF sanity check in CI.

This project's `CMakeLists.txt` already calls `add_subdirectory(tests)` if
this directory contains a `CMakeLists.txt` -- add one here when there is a
real test to run.
