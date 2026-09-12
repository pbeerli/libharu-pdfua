/*
 * hpdf_ua_structure.c -- Milestone 1+ structure-tree / marked-content API.
 *
 * Deliberately all stubs right now (see docs/roadmap.md, Milestone 1): the
 * function signatures in hpdf_ua.h are fixed so real callers (this
 * project's own demos, and eventually Migrate's report code) can be
 * written against the final API shape immediately, while the actual
 * structure-tree/content-stream-tagging machinery is designed and
 * implemented. Every stub here returns HPDF_UA_NOT_YET_IMPLEMENTED (or
 * NULL for the one function that returns a handle) rather than silently
 * doing nothing -- a caller that forgets to check the return value gets a
 * loud, specific failure instead of quietly shipping an untagged PDF that
 * merely inherits Milestone 0's empty /StructTreeRoot placeholder.
 */
#include "hpdf_ua/hpdf_ua.h"

HPDF_UA_StructElem
HPDF_UA_BeginStructureElement (HPDF_Doc pdf, HPDF_UA_StructElem parent,
                                HPDF_UA_StructType role)
{
    (void) pdf;
    (void) parent;
    (void) role;
    return NULL;
}

HPDF_STATUS
HPDF_UA_EndStructureElement (HPDF_Doc pdf, HPDF_UA_StructElem elem)
{
    (void) pdf;
    (void) elem;
    return HPDF_UA_NOT_YET_IMPLEMENTED;
}

HPDF_STATUS
HPDF_UA_BeginMarkedContent (HPDF_Doc pdf, HPDF_Page page,
                             HPDF_UA_StructElem elem)
{
    (void) pdf;
    (void) page;
    (void) elem;
    return HPDF_UA_NOT_YET_IMPLEMENTED;
}

HPDF_STATUS
HPDF_UA_EndMarkedContent (HPDF_Doc pdf, HPDF_Page page)
{
    (void) pdf;
    (void) page;
    return HPDF_UA_NOT_YET_IMPLEMENTED;
}

HPDF_STATUS
HPDF_UA_MarkArtifact (HPDF_Doc pdf, HPDF_Page page)
{
    (void) pdf;
    (void) page;
    return HPDF_UA_NOT_YET_IMPLEMENTED;
}

HPDF_STATUS
HPDF_UA_SetAlternateText (HPDF_Doc pdf, HPDF_UA_StructElem elem,
                           const char *alt_text)
{
    (void) pdf;
    (void) elem;
    (void) alt_text;
    return HPDF_UA_NOT_YET_IMPLEMENTED;
}

HPDF_STATUS
HPDF_UA_SetTableHeaderScope (HPDF_Doc pdf, HPDF_UA_StructElem th_elem,
                             HPDF_UA_TableScope scope)
{
    (void) pdf;
    (void) th_elem;
    (void) scope;
    return HPDF_UA_NOT_YET_IMPLEMENTED;
}

HPDF_STATUS
HPDF_UA_SetTableDataHeaders (HPDF_Doc pdf, HPDF_UA_StructElem td_elem,
                             HPDF_UA_StructElem *th_elems, HPDF_UINT count)
{
    (void) pdf;
    (void) td_elem;
    (void) th_elems;
    (void) count;
    return HPDF_UA_NOT_YET_IMPLEMENTED;
}

HPDF_STATUS
HPDF_UA_SetActualText (HPDF_Doc pdf, HPDF_UA_StructElem elem,
                        const char *actual_text)
{
    (void) pdf;
    (void) elem;
    (void) actual_text;
    return HPDF_UA_NOT_YET_IMPLEMENTED;
}
