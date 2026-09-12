/*
 * hpdf_ua.h -- PDF/UA-1 (ISO 14289-1) tagging extensions for libharu.
 *
 * This header is this project's own addition, not part of upstream libharu
 * (see NOTICE.md). It sits on top of libharu's public API (hpdf.h) and adds
 * no dependency on any application beyond libharu itself -- in particular,
 * nothing here depends on Migrate-n or any of its internals.
 *
 * Status conventions used throughout this header (see docs/roadmap.md for
 * the milestone each item belongs to):
 *   - Functions marked REAL are implemented now and safe to call.
 *   - Functions marked STUB return HPDF_UA_NOT_YET_IMPLEMENTED and otherwise
 *     do nothing -- they exist so the intended API surface is fixed early
 *     and callers get an explicit, loud failure instead of silently
 *     shipping untagged output.
 */
#ifndef _HPDF_UA_H
#define _HPDF_UA_H

#include "hpdf.h"

#ifdef __cplusplus
extern "C" {
#endif

/* Extra status code for this module, chosen well outside libharu's own
 * HPDF_STATUS range (see hpdf_error.h) to avoid any collision. */
#define HPDF_UA_NOT_YET_IMPLEMENTED  ((HPDF_STATUS)0x55410000UL) /* 'UA\0\0' */

/* ---------------------------------------------------------------------
 * Milestone 0 (this commit): real, working document-level metadata.
 * ------------------------------------------------------------------- */

/* REAL. Sets the document catalog's /Lang entry to a BCP 47 language tag
 * (e.g. "en-US"). libharu 2.4.5/2.4.6 has no catalog-level language setter
 * of any kind -- confirmed absent by direct inspection of the vendored
 * source this project is built on; see docs/pdf_ua_requirements.md. */
HPDF_EXPORT(HPDF_STATUS)
HPDF_UA_SetDocumentLanguage (HPDF_Doc pdf, const char *lang);

/* REAL. Sets /ViewerPreferences /DisplayDocTitle, so a compliant viewer
 * shows the document's /Title (already settable via
 * HPDF_SetInfoAttr(pdf, HPDF_INFO_TITLE, ...)) instead of the filename.
 * PDF/UA-1 requires this in addition to /Title itself being set --
 * libharu's existing HPDF_Catalog_SetViewerPreference() bitflag mechanism
 * has no DisplayDocTitle flag defined, so this is a small, independent
 * addition rather than an extension of that existing function. */
HPDF_EXPORT(HPDF_STATUS)
HPDF_UA_SetDisplayDocTitle (HPDF_Doc pdf, HPDF_BOOL value);

/* REAL (thin wrapper). Ensures /MarkInfo /Marked true and a real (not
 * empty) /StructTreeRoot exist; safe to call multiple times. Builds on
 * libharu's existing HPDF_PDFA_SetPDFAConformance()-adjacent MarkInfo/
 * StructTreeRoot creation in hpdf_pdfa.c, which today only ever creates an
 * empty placeholder tree -- this function is the entry point Milestone 1's
 * real structure-tree population will extend, not a replacement for it. */
HPDF_EXPORT(HPDF_STATUS)
HPDF_UA_EnableTagging (HPDF_Doc pdf);

/* ---------------------------------------------------------------------
 * Milestone 1+ (structure tree / marked content): declared now so the
 * API shape is fixed, implemented later. All currently return
 * HPDF_UA_NOT_YET_IMPLEMENTED and otherwise do nothing.
 * ------------------------------------------------------------------- */

/* Opaque handle for one node in the logical structure tree. */
typedef struct _HPDF_UA_StructElem_Rec *HPDF_UA_StructElem;

/* Standard PDF structure types this project's role map will map onto.
 * Deliberately named to match the PDF/UA-1 standard structure types
 * directly (see docs/pdf_ua_requirements.md's Role map section) rather
 * than inventing project-specific names that would then need mapping. */
typedef enum {
    HPDF_UA_ROLE_DOCUMENT = 0,
    HPDF_UA_ROLE_SECT,
    HPDF_UA_ROLE_DIV,
    HPDF_UA_ROLE_P,
    HPDF_UA_ROLE_H1, HPDF_UA_ROLE_H2, HPDF_UA_ROLE_H3,
    HPDF_UA_ROLE_H4, HPDF_UA_ROLE_H5, HPDF_UA_ROLE_H6,
    HPDF_UA_ROLE_L, HPDF_UA_ROLE_LI, HPDF_UA_ROLE_LBL, HPDF_UA_ROLE_LBODY,
    HPDF_UA_ROLE_TABLE, HPDF_UA_ROLE_THEAD, HPDF_UA_ROLE_TBODY,
    HPDF_UA_ROLE_TFOOT, HPDF_UA_ROLE_TR, HPDF_UA_ROLE_TH, HPDF_UA_ROLE_TD,
    HPDF_UA_ROLE_FIGURE, HPDF_UA_ROLE_FORMULA, HPDF_UA_ROLE_CAPTION,
    HPDF_UA_ROLE_LINK,
    HPDF_UA_ROLE_ARTIFACT,
    HPDF_UA_ROLE_EOF
} HPDF_UA_StructType;

/* Table-header scope, for /TH elements (PDF/UA-1's simple-table case;
 * irregular tables instead use explicit /Headers references -- see
 * HPDF_UA_SetTableHeaders below). */
typedef enum {
    HPDF_UA_SCOPE_ROW = 0,
    HPDF_UA_SCOPE_COLUMN,
    HPDF_UA_SCOPE_BOTH
} HPDF_UA_TableScope;

/* STUB (Milestone 1). Begins a new structure element as a child of
 * `parent` (NULL for a page's top-level element under the document root),
 * returning a handle to it. */
HPDF_UA_StructElem
HPDF_UA_BeginStructureElement (HPDF_Doc pdf, HPDF_UA_StructElem parent,
                                HPDF_UA_StructType role);

/* STUB (Milestone 1). Closes a structure element opened above. */
HPDF_STATUS
HPDF_UA_EndStructureElement (HPDF_Doc pdf, HPDF_UA_StructElem elem);

/* STUB (Milestone 1). Wraps a region of page content in BDC ... EMC with a
 * fresh, page-unique MCID, associates it with `elem`, and records the
 * mapping in the document's /ParentTree. Must be called between the page's
 * HPDF_Page_BeginText/graphics calls that actually draw the tagged content. */
HPDF_STATUS
HPDF_UA_BeginMarkedContent (HPDF_Doc pdf, HPDF_Page page,
                             HPDF_UA_StructElem elem);
HPDF_STATUS
HPDF_UA_EndMarkedContent (HPDF_Doc pdf, HPDF_Page page);

/* STUB (Milestone 4). Marks a region of page content as a non-content
 * artifact (decorative border, repeated header/footer, background grid)
 * -- explicitly outside the structure tree, per PDF/UA-1's distinction
 * between untagged-by-omission (a defect) and marked-as-artifact
 * (correct for genuinely non-content marks). */
HPDF_STATUS
HPDF_UA_MarkArtifact (HPDF_Doc pdf, HPDF_Page page);

/* STUB (Milestone 2). Sets a Figure structure element's /Alt (alternate
 * text) -- required for every element conveying information
 * non-textually (skyline plots, posterior histograms). */
HPDF_STATUS
HPDF_UA_SetAlternateText (HPDF_Doc pdf, HPDF_UA_StructElem elem,
                           const char *alt_text);

/* STUB (Milestone 1). Associates a /TH element with its scope (simple/
 * regular tables). */
HPDF_STATUS
HPDF_UA_SetTableHeaderScope (HPDF_Doc pdf, HPDF_UA_StructElem th_elem,
                             HPDF_UA_TableScope scope);

/* STUB (Milestone 1). Associates a /TD element with an explicit list of
 * /TH element ids (irregular/complex tables, where /Scope alone is not
 * enough -- see docs/pdf_ua_requirements.md). */
HPDF_STATUS
HPDF_UA_SetTableDataHeaders (HPDF_Doc pdf, HPDF_UA_StructElem td_elem,
                             HPDF_UA_StructElem *th_elems, HPDF_UINT count);

/* STUB (wherever glyphs aren't cleanly Unicode-mappable -- flagged in
 * research as a real concern for this project's actual consumer, which
 * renders Greek letters and mathematical notation via subset/symbol
 * fonts). Sets /ActualText on a marked-content span or structure element. */
HPDF_STATUS
HPDF_UA_SetActualText (HPDF_Doc pdf, HPDF_UA_StructElem elem,
                        const char *actual_text);

#ifdef __cplusplus
}
#endif

#endif /* _HPDF_UA_H */
