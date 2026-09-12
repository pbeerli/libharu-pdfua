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
 * Milestone 0: real, working document-level metadata. Doc-scoped, no
 * tagging context needed.
 * ------------------------------------------------------------------- */

/* REAL. Sets the document catalog's /Lang entry to a BCP 47 language tag
 * (e.g. "en-US"). libharu has no catalog-level language setter of any kind
 * -- confirmed absent by direct inspection of the vendored source this
 * project is built on; see docs/pdf_ua_requirements.md. */
HPDF_EXPORT(HPDF_STATUS)
HPDF_UA_SetDocumentLanguage (HPDF_Doc pdf, const char *lang);

/* REAL. Sets /ViewerPreferences /DisplayDocTitle, so a compliant viewer
 * shows the document's /Title (already settable via
 * HPDF_SetInfoAttr(pdf, HPDF_INFO_TITLE, ...)) instead of the filename.
 * PDF/UA-1 requires this in addition to /Title itself being set. */
HPDF_EXPORT(HPDF_STATUS)
HPDF_UA_SetDisplayDocTitle (HPDF_Doc pdf, HPDF_BOOL value);

/* REAL (thin, idempotent). Ensures /MarkInfo /Marked true and a real (not
 * necessarily populated yet) /StructTreeRoot exist. Called automatically
 * by HPDF_UA_NewContext() below; exposed separately because Milestone 0
 * already shipped it and other code may want document-level /MarkInfo
 * without paying for a full tagging context. */
HPDF_EXPORT(HPDF_STATUS)
HPDF_UA_EnableTagging (HPDF_Doc pdf);

/* ---------------------------------------------------------------------
 * Milestone 1: structure tree / marked content. Everything below is
 * scoped to an explicit HPDF_UA_Context rather than the bare HPDF_Doc --
 * populating the structure tree and /ParentTree needs persistent
 * bookkeeping (which pages have been assigned a /StructParents key, each
 * page's marked-content-to-structure-element mapping) that has no natural
 * home in libharu's own HPDF_Doc struct (this project does not modify
 * vendored libharu headers, see NOTICE.md) and should not be hidden in
 * module-global state (would break with more than one HPDF_Doc alive at
 * once, and libharu itself never assumes only one document exists). This
 * mirrors libharu's own style of explicit resource handles (HPDF_MMgr,
 * HPDF_Xref): the application owns the context's lifetime explicitly.
 * ------------------------------------------------------------------- */

typedef struct _HPDF_UA_Context_Rec *HPDF_UA_Context;

/* REAL. Creates a tagging context for `pdf`, calling HPDF_UA_EnableTagging()
 * internally and additionally setting up /ParentTree. Create exactly one
 * context per document you want to tag, before calling any other
 * Milestone 1 function on it. */
HPDF_EXPORT(HPDF_UA_Context)
HPDF_UA_NewContext (HPDF_Doc pdf);

/* REAL. Frees a context created by HPDF_UA_NewContext(). Does not affect
 * `pdf` itself or anything already written into it -- only this project's
 * own bookkeeping memory. Call once, after the document's content is
 * fully generated (typically just before HPDF_SaveToFile()). */
HPDF_EXPORT(void)
HPDF_UA_FreeContext (HPDF_UA_Context ctx);

/* Opaque handle for one node in the logical structure tree. */
typedef struct _HPDF_UA_StructElem_Rec *HPDF_UA_StructElem;

/* Standard PDF structure types this project's role map maps onto.
 * Deliberately named to match the PDF/UA-1 standard structure types
 * directly (see docs/pdf_ua_requirements.md's Role map section) rather
 * than inventing project-specific names that would then need mapping --
 * no /RoleMap entry is needed as long as only these standard types are
 * used. */
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
 * HPDF_UA_SetTableDataHeaders below). */
typedef enum {
    HPDF_UA_SCOPE_ROW = 0,
    HPDF_UA_SCOPE_COLUMN,
    HPDF_UA_SCOPE_BOTH
} HPDF_UA_TableScope;

/* REAL. Begins a new structure element as a child of `parent` (NULL for a
 * top-level element, made a direct child of /StructTreeRoot), returning a
 * handle to it. Returns NULL on allocation/xref failure. */
HPDF_EXPORT(HPDF_UA_StructElem)
HPDF_UA_BeginStructureElement (HPDF_UA_Context ctx, HPDF_UA_StructElem parent,
                                HPDF_UA_StructType role);

/* REAL (mostly a validation no-op -- see hpdf_ua_structure.c for why the
 * tree linkage already happens at Begin time, not here). Closes a
 * structure element opened above. */
HPDF_EXPORT(HPDF_STATUS)
HPDF_UA_EndStructureElement (HPDF_UA_Context ctx, HPDF_UA_StructElem elem);

/* REAL. Wraps a region of page content in BDC ... EMC with a fresh,
 * page-unique MCID, associates it with `elem`, and records the mapping in
 * the document's /ParentTree. Must be called between the page drawing
 * calls that actually draw the tagged content (this call only writes the
 * BDC operator itself; draw the real content between this call and the
 * matching HPDF_UA_EndMarkedContent()). Limitation (documented, not yet
 * lifted): `elem` may only ever be associated with content on ONE page --
 * a struct element whose content genuinely spans multiple pages needs a
 * per-page /K entry design this milestone does not yet implement, and
 * this function returns an error if `elem` was already used on a
 * different page. At most one marked-content span may be open on a given
 * page at a time (no nesting) -- also enforced, not yet lifted. */
HPDF_EXPORT(HPDF_STATUS)
HPDF_UA_BeginMarkedContent (HPDF_UA_Context ctx, HPDF_Page page,
                             HPDF_UA_StructElem elem);

/* REAL. Closes the marked-content span opened by HPDF_UA_BeginMarkedContent()
 * on this page. */
HPDF_EXPORT(HPDF_STATUS)
HPDF_UA_EndMarkedContent (HPDF_UA_Context ctx, HPDF_Page page);

/* REAL. Sets a structure element's /Alt (alternate text) -- required for
 * every element conveying information non-textually (skyline plots,
 * posterior histograms). Brought forward from its original Milestone 2
 * slot since it is a one-line dict-key addition once HPDF_UA_StructElem
 * exists. */
HPDF_EXPORT(HPDF_STATUS)
HPDF_UA_SetAlternateText (HPDF_UA_Context ctx, HPDF_UA_StructElem elem,
                           const char *alt_text);

/* REAL. Associates a /TH element with its scope (simple/regular tables). */
HPDF_EXPORT(HPDF_STATUS)
HPDF_UA_SetTableHeaderScope (HPDF_UA_Context ctx, HPDF_UA_StructElem th_elem,
                             HPDF_UA_TableScope scope);

/* STUB. Associates a /TD element with an explicit list of /TH element ids
 * (irregular/complex tables, where /Scope alone is not enough -- see
 * docs/pdf_ua_requirements.md). Needs each referenced /TH to carry a
 * unique /ID plus a /Headers array of name references (PDF 32000-2
 * 14.8.4.5.2) -- more machinery than /Scope alone, deliberately not built
 * in this same pass since Migrate's own tables (the motivating consumer)
 * are all simple/regular tables where /Scope is sufficient. */
HPDF_EXPORT(HPDF_STATUS)
HPDF_UA_SetTableDataHeaders (HPDF_UA_Context ctx, HPDF_UA_StructElem td_elem,
                             HPDF_UA_StructElem *th_elems, HPDF_UINT count);

/* REAL. Brought forward from its original Milestone 4 slot: real veraPDF
 * PDF/UA-1 validation of the Milestone 1 table demo immediately surfaced
 * a genuine failure (ISO 14289-1:2014 7.1/3, "content shall be marked as
 * Artifact or tagged as real content") on the demo's own decorative table
 * border -- untagged-by-omission is a real defect, not a lesser one than
 * getting the tagging wrong, so this could not wait for a later
 * milestone once it was an actual, present gap rather than a
 * hypothetical one. Wraps a region of page content (decorative borders,
 * repeated headers/footers, background grid lines -- anything conveying
 * no information) as a PDF/UA-1 Artifact: explicitly OUTSIDE the
 * structure tree and the /ParentTree, per PDF/UA-1's distinction between
 * untagged-by-omission (a defect) and marked-as-artifact (correct for
 * genuinely non-content marks). Unlike HPDF_UA_BeginMarkedContent(), this
 * takes no HPDF_UA_StructElem -- artifacts have no structure-tree
 * membership by definition. At most one artifact span may be open per
 * page at a time (same no-nesting limitation as marked content), and an
 * artifact span may not overlap an open marked-content span on the same
 * page. */
HPDF_EXPORT(HPDF_STATUS)
HPDF_UA_BeginArtifact (HPDF_UA_Context ctx, HPDF_Page page);

/* REAL. Closes the artifact span opened by HPDF_UA_BeginArtifact() on
 * this page. */
HPDF_EXPORT(HPDF_STATUS)
HPDF_UA_EndArtifact (HPDF_UA_Context ctx, HPDF_Page page);

/* ---------------------------------------------------------------------
 * Milestone 4 (remaining item): still a stub. Signature updated to the
 * same Context-based convention as the rest of this header for
 * consistency, even though it doesn't do anything yet.
 * ------------------------------------------------------------------- */

/* STUB (wherever glyphs aren't cleanly Unicode-mappable -- flagged in
 * research as a real concern for this project's actual consumer, which
 * renders Greek letters and mathematical notation via subset/symbol
 * fonts). Sets /ActualText on a marked-content span or structure element. */
HPDF_EXPORT(HPDF_STATUS)
HPDF_UA_SetActualText (HPDF_UA_Context ctx, HPDF_UA_StructElem elem,
                        const char *actual_text);

#ifdef __cplusplus
}
#endif

#endif /* _HPDF_UA_H */
