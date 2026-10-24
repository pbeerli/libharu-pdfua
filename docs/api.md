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

## Multi-page documents and tables

The calls above are enough for a document whose content is laid out
in one pass with fixed coordinates. Report-style output, where a long
table or paragraph is drawn line by line and a helper starts a new page
whenever the next line does not fit, needs a little more bookkeeping.
The tag stack (0.9.0) holds it. It was generalized from the tagging
layer of Migrate-n's PDF report, a program whose tables routinely run
over many pages; every rule below comes from a defect veraPDF or
avalpdf found there.

| Function | What it does |
| --- | --- |
| `HPDF_UA_BeginArtifactOfType(ctx, page, type)` | Like `HPDF_UA_BeginArtifact()`, but writes `/Artifact <</Type /Pagination>> BDC` (or `_LAYOUT`, `_PAGE`, `_BACKGROUND`). Close with `HPDF_UA_EndArtifact()`. |
| `HPDF_UA_NewStack(ctx)` / `HPDF_UA_FreeStack(stack)` | Creates an empty stack on a context; free closes any open span and ends every element still on the stack. Free it before `HPDF_UA_FreeContext()`. |
| `HPDF_UA_Stack_Push(stack, role)` / `HPDF_UA_Stack_Pop(stack)` | Begins a child of the top element (top-level when empty) / ends the top element, closing its content span first. |
| `HPDF_UA_Stack_Top(stack)` / `HPDF_UA_Stack_Depth(stack)` | The top element and the depth. |
| `HPDF_UA_Stack_SetAlternateText` / `_SetActualText` / `_SetTableHeaderScope` | The corresponding context calls, applied to the top element. |
| `HPDF_UA_Stack_BeginContent(stack, page)` / `_BeginContentWithActualText(stack, page, text)` / `_EndContent(stack)` | Opens / closes the marked-content span for the top element. The stack remembers the page. |
| `HPDF_UA_Stack_BeginArtifact(stack, page, type)` / `_EndArtifact(stack)` | Typed artifact span, tracked by the stack. |
| `HPDF_UA_Stack_SuspendForNewPage(stack)` / `_ResumeAfterNewPage(stack, new_page)` | Call around your own "start a new page" code. An open content span is ended and its element popped; on resume a new sibling of the same role is pushed and its content reopened on the new page. An open artifact is closed and reopened with its type. |
| `HPDF_UA_Stack_SuspendContent(stack)` / `_ResumeContent(stack, page)` | The same for a nested element drawn while the outer element's content is open (a figure inside a paragraph). Separate saved state from the page-break pair, because the two can nest. |

A typical "new page" helper:

```c
static void new_page(Doc *d)
{
    HPDF_UA_Stack_SuspendForNewPage(d->stack);
    d->page = HPDF_AddPage(d->pdf);
    HPDF_UA_Stack_BeginArtifact(d->stack, d->page, HPDF_UA_ARTIFACT_PAGINATION);
    /* running header, page number */
    HPDF_UA_Stack_EndArtifact(d->stack);
    d->y = TOP;
    HPDF_UA_Stack_ResumeAfterNewPage(d->stack, d->page);
}
```

Rules, each a real failure in the original code:

- **One element, one page.** `HPDF_UA_BeginMarkedContent()` refuses an
  element whose content is already on another page. A paragraph that
  crosses a page becomes two sibling `P` elements; the suspend/resume
  pair does that for you.
- **Elements join the tree when pushed, not when popped.** An element is
  appended to its parent's `/K` at `Begin`/`Push` time, so popping it
  later does not take it out again. Consequence for tables: the
  page-break pair splits only the innermost element. If that is a `TD`,
  the open `TR` gains an extra cell on the new page and veraPDF reports
  7.2/42 ("table rows shall have the same number of columns"). Draw the
  cell, pop the cell and the row, and only then call the code that may
  start a new page. If one cell's content really must wrap over a page
  (a long sequence, say), close the row yourself at the break and open a
  continuation row with empty cells for the other columns.
- **Repeated table headers are artifacts.** Tag the header row once, as
  `TH` with `/Scope`, on the first page. On continuation pages draw it
  inside a `HPDF_UA_ARTIFACT_PAGINATION` span. A second tagged header row
  makes screen readers announce it again in the middle of the data, and
  avalpdf flags tables with more than one header row. Keep the header on
  the same page as at least the first data row.
- **Decoration is an artifact too**: page borders, rules, zebra shading
  (`_LAYOUT`), running headers and page numbers (`_PAGINATION`).
  Untagged content fails 7.1/3.
- **No newline in `HPDF_Page_TextOut()`.** A trailing `"\n"` (printf habit)
  is drawn as a glyph with no Unicode meaning and fails 7.21.7/2 and
  7.21.8. Strip it before drawing.
- **No padding spaces.** Right-align numbers by measuring them with
  `HPDF_Page_TextWidth()` instead of padding with spaces; padding
  spaces become part of the extracted text (avalpdf "consecutive
  spaces"). Also measure at the size you will draw at: a page break in
  between may have left the footer's font size set.
- **Embed every font.** The Standard 14 fonts are never embedded and fail
  7.21.4.1. When an existing layout was tuned to Helvetica or Courier
  widths, embed a metric-compatible font (Liberation Sans, Sans Italic,
  Mono; SIL OFL) so nothing moves. Migrate-n first tried DejaVu, whose
  wider glyphs pushed fixed-position lines off the page. To avoid
  depending on font files at run time, compile the TTF into a byte array
  and load it with `HPDF_LoadTTFontFromMemory()`.
- **Symbol-font cells get `/ActualText`.** Greek or math glyphs drawn
  through a symbol or CID font may not be recoverable by a checker that
  walks the content stream. Open such a cell with
  `HPDF_UA_Stack_BeginContentWithActualText()` (for example "Theta_1").

`demo/tagged_multipage_table_demo.c` puts all of this together: a 96-row
table over three pages, a notes paragraph that runs onto a fourth page,
and a figure that interrupts that paragraph. It passes veraPDF's
PDF/UA-1 profile (106/106) and scores 100% in avalpdf.

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
