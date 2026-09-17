#!/usr/bin/env bash
# run_all_demos.sh -- real test coverage for this project's demo suite,
# including libharu's OWN original functionality (TrueType font embedding,
# raw-image drawing, link annotations, outlines, tables, path painting),
# not just this project's tagging additions: builds every demo, runs each
# one to (re)generate its PDF, then validates that PDF against the real
# PDF/UA-1 ruleset via validate/run_verapdf.sh -- this is the same
# validator every milestone in docs/roadmap.md was checked against, run
# here automatically instead of by hand.
#
# Each demo has a recorded baseline (see the DEMOS table below): the
# seven fully tagged demos must be fully PDF/UA-1 compliant (veraPDF
# "PASS", i.e. 0 failed rules); docmeta_demo.pdf is Milestone 0 scaffolding
# (untagged page content, Standard-14 font) and is deliberately NOT fully
# compliant -- its own recorded baseline is "at most 3 failed rules" (the
# same two known Milestone-0 gaps documented in docs/roadmap.md's
# Milestone 1 section: no /Metadata, and an un-embedded font -- plus the
# untagged body text itself). This script fails loudly, naming exactly
# which demo and which direction it moved, if any demo's real veraPDF
# result is now WORSE than its recorded baseline; it does not require
# docmeta_demo to reach full PASS, since that was never its scope.
#
# Usage:
#   tests/run_all_demos.sh [build-dir]
#
# Exit status: 0 if every demo meets (or beats) its baseline, 1 if any
# demo regressed (see the per-demo FAIL lines for which one and why),
# 2 for a usage/environment error (missing build dir, no veraPDF/docker
# on PATH -- matches validate/run_verapdf.sh's own exit-code convention).

set -uo pipefail

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
ROOT_DIR="$(cd "$SCRIPT_DIR/.." && pwd)"
BUILD_DIR="${1:-$ROOT_DIR/build}"

if [ ! -d "$BUILD_DIR" ]; then
    echo "error: build directory '$BUILD_DIR' does not exist -- run" >&2
    echo "  cmake -B $BUILD_DIR && cmake --build $BUILD_DIR" >&2
    echo "first, or pass an existing build directory as \$1." >&2
    exit 2
fi

if ! command -v verapdf >/dev/null 2>&1 && ! command -v docker >/dev/null 2>&1; then
    echo "error: neither 'verapdf' nor 'docker' found on PATH -- see" >&2
    echo "  validate/run_verapdf.sh for install instructions." >&2
    exit 2
fi

echo "== building all demos in $BUILD_DIR =="
if ! cmake --build "$BUILD_DIR" >"$BUILD_DIR/.run_all_demos_build.log" 2>&1; then
    echo "FAIL: build itself failed -- see $BUILD_DIR/.run_all_demos_build.log" >&2
    tail -n 40 "$BUILD_DIR/.run_all_demos_build.log" >&2
    exit 1
fi
echo "build OK."
echo

# name:max_failed_rules[:password] -- max_failed_rules is the recorded
# baseline (see the header comment above): 0 for every demo this project
# claims is fully PDF/UA-1 conformant, 3 for docmeta_demo's own
# documented, deliberate partial scope, 2 for tagged_font_list_demo's own
# documented, deliberate partial scope (see that file's own top comment:
# it exists to show libharu's non-embeddable Standard-14 fonts, so ISO
# 14289-1:2014 7.21.4.1 (fonts must be embedded) and 7.21.7 (fonts must
# map to Unicode) are expected, not a bug). The optional third field is a
# user password to pass to veraPDF's own --password flag -- needed only
# for tagged_encryption_demo.pdf, which veraPDF otherwise refuses outright
# ("appears to be an encrypted PDF") rather than reporting a rule result
# at all.
DEMOS=(
    "tagged_table_demo:0"
    "tagged_histogram_demo:0"
    "tagged_skyline_demo:0"
    "tagged_example_demo:0"
    "tagged_font_demo:0"
    "tagged_image_demo:0"
    "tagged_annotation_demo:0"
    "tagged_arc_demo:0"
    "tagged_line_demo:0"
    "tagged_ext_gstate_demo:0"
    "tagged_text_demo:0"
    "tagged_encoding_list_demo:0"
    "tagged_outline_demo:0"
    "tagged_font_list_demo:2"
    "tagged_encryption_demo:0:user"
    "tagged_text_annotation_demo:0"
    "tagged_attach_demo:0"
    "tagged_slide_show_demo:0"
    "tagged_png_demo:0"
    "tagged_image_transform_demo:0"
    "tagged_jpeg_demo:0"
    "tagged_japanese_font_demo:0"
    "tagged_chfont_demo:0"
    "docmeta_demo:3"
)

FAILURES=0

for entry in "${DEMOS[@]}"; do
    IFS=':' read -r name max_failed password <<< "$entry"
    binary="$BUILD_DIR/$name"
    pdf="$BUILD_DIR/$name.pdf"

    echo "-- $name --"

    if [ ! -x "$binary" ]; then
        echo "FAIL: $name: binary '$binary' not found or not executable" >&2
        FAILURES=$((FAILURES + 1))
        continue
    fi

    # Demos HPDF_SaveToFile() to a plain relative filename, so run them
    # with the build directory as the working directory.
    if ! ( cd "$BUILD_DIR" && "./$name" >".${name}_run.log" 2>&1 ); then
        echo "FAIL: $name: the demo binary itself exited non-zero -- see" \
             "$BUILD_DIR/.${name}_run.log" >&2
        FAILURES=$((FAILURES + 1))
        continue
    fi

    if [ ! -f "$pdf" ]; then
        echo "FAIL: $name: expected output '$pdf' was not created" >&2
        FAILURES=$((FAILURES + 1))
        continue
    fi

    # Not an array: macOS's default bash (3.2) treats "${arr[@]}" as an
    # unbound variable under `set -u` when arr is empty, even though it's
    # declared -- confirmed directly (this script failed every demo with
    # "password_args[@]: unbound variable" until switched to this plain
    # if/else).
    if [ -n "$password" ]; then
        json="$(verapdf --flavour ua1 --format json --password "$password" "$pdf" 2>/dev/null)"
    else
        json="$(verapdf --flavour ua1 --format json "$pdf" 2>/dev/null)"
    fi
    failed_rules="$(printf '%s' "$json" | grep -o '"failedRules" *: *[0-9]*' \
            | head -n1 | grep -o '[0-9]*$')"
    passed_rules="$(printf '%s' "$json" | grep -o '"passedRules" *: *[0-9]*' \
            | head -n1 | grep -o '[0-9]*$')"

    if [ -z "$failed_rules" ] || [ -z "$passed_rules" ]; then
        echo "FAIL: $name: could not parse a veraPDF result for '$pdf' --" \
             "is veraPDF installed and working? (validate/run_verapdf.sh" \
             "'$pdf' to see the raw error)" >&2
        FAILURES=$((FAILURES + 1))
        continue
    fi

    if [ "$failed_rules" -gt "$max_failed" ]; then
        echo "FAIL: $name: veraPDF reports $failed_rules failed rule(s)" \
             "($passed_rules passed) -- worse than this demo's recorded" \
             "baseline of at most $max_failed failed rule(s). Run" \
             "validate/run_verapdf.sh $pdf to see exactly which check(s)" \
             "regressed." >&2
        FAILURES=$((FAILURES + 1))
        continue
    fi

    if [ "$max_failed" -eq 0 ]; then
        echo "PASS: $name -- $passed_rules/$passed_rules PDF/UA-1 rules" \
             "(full compliance)"
    else
        echo "PASS: $name -- $passed_rules passed / $failed_rules failed" \
             "(within its documented baseline of <= $max_failed failed;" \
             "not expected to be fully compliant, see this script's own" \
             "header comment)"
    fi
done

echo
if [ "$FAILURES" -gt 0 ]; then
    echo "== $FAILURES demo(s) regressed -- see FAIL lines above =="
    exit 1
fi

echo "== all ${#DEMOS[@]} demos match or beat their recorded baseline =="
exit 0
