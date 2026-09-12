#!/usr/bin/env bash
# run_verapdf.sh -- validate a PDF against the PDF/UA-1 ruleset with veraPDF.
#
# veraPDF (https://verapdf.org, Apache-2.0/MPL-2.0 build) is a Java tool;
# install it with either:
#   - the veraPDF installer (https://software.verapdf.org/) -- installs a
#     `verapdf` launcher script, or
#   - the published Docker image: docker pull verapdf/cli
#
# Usage:
#   validate/run_verapdf.sh path/to/file.pdf
#
# Exit status: 0 if the file passes PDF/UA-1 validation, non-zero otherwise
# (matches veraPDF's own exit code). Prints the human-readable text report;
# pass --xml or --json as a second argument for a machine-readable report
# instead, e.g.:
#   validate/run_verapdf.sh path/to/file.pdf --json

set -euo pipefail

if [ "$#" -lt 1 ]; then
    echo "usage: $0 <file.pdf> [--xml|--json]" >&2
    exit 2
fi

PDF_FILE="$1"
FORMAT="text"
if [ "${2:-}" = "--xml" ]; then
    FORMAT="mrr"
elif [ "${2:-}" = "--json" ]; then
    FORMAT="json"
fi

if [ ! -f "$PDF_FILE" ]; then
    echo "error: $PDF_FILE does not exist" >&2
    exit 2
fi

if command -v verapdf >/dev/null 2>&1; then
    exec verapdf --flavour ua1 --format "$FORMAT" "$PDF_FILE"
elif command -v docker >/dev/null 2>&1; then
    echo "note: local 'verapdf' launcher not found on PATH, falling back to" \
         "the published Docker image (verapdf/cli)." >&2
    ABS_DIR="$(cd "$(dirname "$PDF_FILE")" && pwd)"
    BASE="$(basename "$PDF_FILE")"
    exec docker run --rm -v "$ABS_DIR:/data" verapdf/cli \
        --flavour ua1 --format "$FORMAT" "/data/$BASE"
else
    echo "error: neither a 'verapdf' launcher nor 'docker' was found." >&2
    echo "Install veraPDF from https://software.verapdf.org/ (installs the" >&2
    echo "'verapdf' CLI launcher), or install Docker and this script will" >&2
    echo "use the published verapdf/cli image instead." >&2
    exit 3
fi
