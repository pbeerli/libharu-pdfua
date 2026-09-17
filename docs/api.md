# API reference

The full, authoritative API surface is `include/hpdf_ua/hpdf_ua.h` itself --
every function there has its own doc comment (status conventions, exact PDF
mechanism used, ISO 14289-1 clause it addresses where relevant). This page
is a map of that header, grouped by what you're trying to do, plus a
minimal working example. See the demos under `demo/` for real, complete,
veraPDF-validated usage of every function listed here.

**Status conventions** (see `hpdf_ua.h`'s own top-of-file comment):
- **REAL** -- implemented now, safe to call.
- **STUB** -- returns `HPDF_UA_NOT_YET_IMPLEMENTED` and does nothing;
  exists so the intended API surface is fixed and callers get a loud
  failure instead of silently shipping untagged output. Only
  `HPDF_UA_SetTableDataHeaders()` is currently a stub -- see its own
  comment for why (irregular tables aren't this project's motivating
  use case).

## Document-level metadata (no tagging context needed)

| Function | What it does |
| --- | --- |
| `HPDF_UA_SetDocumentLanguage(pdf, lang)` | Sets the catalog's `/Lang` (BCP 47, e.g. `"en-US"`). |
| `HPDF_UA_SetDisplayDocTitle(pdf, value)` | Sets `/ViewerPreferences /DisplayDocTitle`, so viewers show `/Title` instead of the filename. |
| `HPDF_UA_EnableTagging(pdf)` | Ensures `/MarkInfo /Marked true` and a `/StructTreeRoot` exist. Called automatically by `HPDF_UA_NewContext()`. |
| `HPDF_UA_AddMetadata(pdf)` | Writes a minimal XMP `/Metadata` stream declaring PDF/UA-1 conformance (and `dc:title`, if set). |
| `HPDF_UA_AddMetadataWithPDFA(pdf, pdfa_type)` | Same as `HPDF_UA_AddMetadata()`, but also declares PDF/A conformance (`pdfaid:part`/`conformance`, plus a real PDF/A Extension Schema for `pdfuaid`) in the same XMP packet, and generates a trailer `/ID`. Use this instead of libharu's own `HPDF_SetPDFAConformance()`, which would silently discard your structure tree at save time -- see `demo/tagged_pdfa_demo.c`. Supports `HPDF_PDFA_1A` through `HPDF_PDFA_3U`. |

## Tagging context and structure tree

Everything below this point is scoped to an `HPDF_UA_Context`, not the bare
`HPDF_Doc` -- populating the structure tree and `/ParentTree` needs
persistent bookkeeping that has no home in libharu's own `HPDF_Doc` (see
`hpdf_ua.h`'s own design note). Create exactly one context per document.

```c
HPDF_UA_Context ctx = HPDF_UA_NewContext(pdf);
/* ... tag content ... */
HPDF_UA_FreeContext(ctx);   /* just before HPDF_SaveToFile() */
```

| Function | What it does |
| --- | --- |
| `HPDF_UA_NewContext(pdf)` | Creates a tagging context; sets up `/ParentTree`. |
| `HPDF_UA_FreeContext(ctx)` | Frees this project's own bookkeeping (does not touch `pdf`). |
| `HPDF_UA_BeginStructureElement(ctx, parent, role)` | Opens a new structure-tree node (`NULL` parent = top-level, child of `/StructTreeRoot`). Returns a `HPDF_UA_StructElem`. |
| `HPDF_UA_EndStructureElement(ctx, elem)` | Closes it (tree linkage already happened at Begin time). |
| `HPDF_UA_BeginMarkedContent(ctx, page, elem)` / `HPDF_UA_EndMarkedContent(ctx, page)` | Wraps the page-content drawing calls in between in `BDC ... EMC` with a fresh MCID, associating them with `elem`. **Limitation**: one page per element, no nested spans on a page. |
| `HPDF_UA_BeginMarkedContentWithActualText(ctx, page, elem, actual_text)` | Same, plus writes `/ActualText` directly on the marked-content span -- prefer this over a separate `HPDF_UA_SetActualText()` call when a checker walks the content stream itself (e.g. symbol/CID-font text). |
| `HPDF_UA_BeginArtifact(ctx, page)` / `HPDF_UA_EndArtifact(ctx, page)` | Marks decorative content (borders, background grid lines) as a PDF/UA-1 Artifact -- explicitly outside the structure tree. Anything drawn but neither tagged nor marked-as-artifact is a real PDF/UA-1 defect (ISO 14289-1:2014 7.1/3). |

`HPDF_UA_StructType` (`HPDF_UA_ROLE_*`) is the standard-structure-type enum
passed to `HPDF_UA_BeginStructureElement()` -- values match the PDF/UA-1
standard structure types directly (`DOCUMENT`, `SECT`, `DIV`, `P`, `H1`-`H6`,
`L`/`LI`/`LBL`/`LBODY`, `TABLE`/`THEAD`/`TBODY`/`TFOOT`/`TR`/`TH`/`TD`,
`FIGURE`, `FORMULA`, `CAPTION`, `LINK`, `ARTIFACT`), so no `/RoleMap` entry
is ever needed -- **except `ANNOT`** ("Annot"), a deliberate exception:
not an ISO 32000-1 standard type (only formally added in PDF 2.0), but
ISO 14289-1:2014 7.18.1 requires this literal name for wrapping any
non-Widget/PrinterMark/Link annotation, confirmed directly with veraPDF.

## Alternate text, tables, annotations

| Function | What it does |
| --- | --- |
| `HPDF_UA_SetAlternateText(ctx, elem, alt_text)` | Sets `/Alt` on `elem` -- required for anything conveying information non-textually (a chart, a plot). |
| `HPDF_UA_SetActualText(ctx, elem, actual_text)` | Sets `/ActualText` on `elem` (PDF 32000-1 14.9.4); UTF-8 `actual_text` needs `HPDF_UseUTFEncodings()` called on the document first. |
| `HPDF_UA_SetTableHeaderScope(ctx, th_elem, scope)` | Sets a `/TH` element's `/Scope` (`HPDF_UA_SCOPE_ROW` / `_COLUMN` / `_BOTH`) -- simple/regular tables. |
| `HPDF_UA_SetTableDataHeaders(...)` | **STUB.** Explicit `/TD` &rarr; `/TH` `/Headers` references for irregular tables; not needed by this project's own motivating use case (simple tables), so not built. |
| `HPDF_UA_TagAnnotation(ctx, page, elem, annot)` | Associates any annotation with a structure element via `/OBJR`, assigns it a `/StructParent` key, sets `/F` (Print set / NoView clear), and copies `elem`'s `/Alt` onto the annotation's own `/Contents` if set (the separate ISO 14289-1:2014 7.18.5 requirement, Link-specific). Use `HPDF_UA_ROLE_LINK` for Link annotations, `HPDF_UA_ROLE_ANNOT` for everything else (Text/popup, etc. -- see `demo/tagged_text_annotation_demo.c`). |

## A minimal complete example

```c
#include <hpdf_ua/hpdf_ua.h>

int main(void) {
    HPDF_Doc pdf = HPDF_New(NULL, NULL);
    HPDF_UA_SetDocumentLanguage(pdf, "en-US");
    HPDF_UA_SetDisplayDocTitle(pdf, HPDF_TRUE);
    HPDF_SetInfoAttr(pdf, HPDF_INFO_TITLE, "Example");

    HPDF_UA_Context ctx = HPDF_UA_NewContext(pdf);
    HPDF_Page page = HPDF_AddPage(pdf);

    HPDF_UA_StructElem h1 =
        HPDF_UA_BeginStructureElement(ctx, NULL, HPDF_UA_ROLE_H1);
    HPDF_Page_BeginText(page);
    HPDF_UA_BeginMarkedContent(ctx, page, h1);
    HPDF_Page_ShowText(page, "Hello, tagged PDF");
    HPDF_UA_EndMarkedContent(ctx, page);
    HPDF_Page_EndText(page);
    HPDF_UA_EndStructureElement(ctx, h1);

    HPDF_UA_AddMetadata(pdf);          /* XMP /Metadata, PDF/UA-1 conformance */
    HPDF_UA_FreeContext(ctx);
    HPDF_SaveToFile(pdf, "example.pdf");
    HPDF_Free(pdf);
    return 0;
}
```

Validate the result with `validate/run_verapdf.sh example.pdf`. See
`demo/tagged_table_demo.c` (tables), `demo/tagged_histogram_demo.c` /
`tagged_skyline_demo.c` (figures with `/Alt`), `demo/tagged_font_demo.c`
(embedded fonts), `demo/tagged_image_demo.c` (raw images), and
`demo/tagged_annotation_demo.c` (link annotations) for complete,
real, validated programs exercising every function above.

## Using this library from another CMake project

Once installed (`cmake --install build`, or a packaged install elsewhere
on `CMAKE_PREFIX_PATH`):

```cmake
find_package(hpdf_ua REQUIRED)
target_link_libraries(your_target PRIVATE hpdf_ua::hpdf_ua)
```

This pulls in both `hpdf_ua`'s own headers and the vendored libharu
headers/library it's built on -- no separate `find_package(libharu)` or
manual include path needed. See `tests/consume_package/` for a minimal,
CI-verified example of exactly this (`tests/test_install.sh`).
