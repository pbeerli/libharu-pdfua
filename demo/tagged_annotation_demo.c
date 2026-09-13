/*
 * tagged_annotation_demo.c -- Milestone 6: a tagged port of libharu's own
 * upstream link_annotation.c (see docs/roadmap.md's Milestone 6 section).
 * The original creates several HPDF_Page_CreateLinkAnnot()/
 * HPDF_Page_CreateURILinkAnnot() annotations with no accessibility
 * structure at all -- a real PDF/UA-1 gap this project's own
 * docs/pdf_ua_requirements.md already flagged by name ("Links: need a
 * real Link structure element associated with the link annotation, not
 * just a bare annotation").
 *
 * This version tags each link two ways, both real requirements, not
 * just one: (1) the link's own visible label text is wrapped in marked
 * content under a real HPDF_UA_ROLE_LINK structure element, exactly
 * like every other demo's tagged text; and (2) a new
 * HPDF_UA_TagAnnotation() call (added for this demo, see hpdf_ua.h)
 * associates that same Link element with the annotation object itself
 * via a real /OBJR structure-tree kid, gives the annotation a
 * /StructParent key into this context's /ParentTree, and normalizes its
 * /F flags to Print-set/NoView-clear -- ISO 14289-1:2014 7.18's actual
 * annotation requirement, which no existing demo in this project needed
 * before (none used annotations at all).
 *
 * Structure: Document > [H1 "Annotation Demo", P (intro), three
 * Link elements (two internal page-jump links, one external URI link),
 * each wrapping its own visible label text and each also carrying its
 * own /Alt text]. Two destination pages, each a minimal Document > H1
 * of their own. Validate with:
 *   validate/run_verapdf.sh build/tagged_annotation_demo.pdf
 */
#include <stdio.h>
#include <string.h>
#include "hpdf_ua/hpdf_ua.h"

static const char *INTRO_TEXT =
    "This demo creates real link annotations (two internal page-jump "
    "links and one external URI link) and tags each one properly: its "
    "visible label text is wrapped in marked content under a Link "
    "structure element, and the annotation itself is associated with "
    "that same element via a real object reference, with its /F flags "
    "normalized so it is neither hidden from view nor excluded from "
    "printing.";

static void
error_handler (HPDF_STATUS error_no, HPDF_STATUS detail_no, void *user_data)
{
    (void) user_data;
    fprintf (stderr, "libharu error: error_no=0x%04lX detail_no=%ld\n",
            (unsigned long) error_no, (long) detail_no);
}

/* Draws one minimal, tagged destination page ("Document > H1"). */
static HPDF_STATUS
make_destination_page (HPDF_UA_Context ctx, HPDF_Doc pdf, HPDF_Font font,
                        const char *title, HPDF_Page *page_out)
{
    HPDF_Page page;
    HPDF_UA_StructElem doc_elem, h1_elem;
    HPDF_STATUS status;

    page = HPDF_AddPage (pdf);
    HPDF_Page_SetSize (page, HPDF_PAGE_SIZE_LETTER, HPDF_PAGE_PORTRAIT);

    doc_elem = HPDF_UA_BeginStructureElement (ctx, NULL, HPDF_UA_ROLE_DOCUMENT);
    if (!doc_elem)
        return HPDF_INVALID_OBJECT;

    h1_elem = HPDF_UA_BeginStructureElement (ctx, doc_elem, HPDF_UA_ROLE_H1);
    if (!h1_elem)
        return HPDF_INVALID_OBJECT;

    status = HPDF_UA_BeginMarkedContent (ctx, page, h1_elem);
    if (status != HPDF_OK)
        return status;
    HPDF_Page_SetFontAndSize (page, font, 20);
    HPDF_Page_BeginText (page);
    HPDF_Page_MoveTextPos (page, 50, 700);
    HPDF_Page_ShowText (page, title);
    HPDF_Page_EndText (page);
    status = HPDF_UA_EndMarkedContent (ctx, page);
    if (status != HPDF_OK)
        return status;

    HPDF_UA_EndStructureElement (ctx, h1_elem);
    HPDF_UA_EndStructureElement (ctx, doc_elem);

    *page_out = page;
    return HPDF_OK;
}

/* Draws one link's visible label text on `page` at (x, *y), tags it
 * (Link structure element wrapping the label's own marked-content span,
 * plus real /Alt text), associates `annot` with that same element via
 * HPDF_UA_TagAnnotation(), and advances *y for the next line. `annot`
 * must already exist (built from a rect this function itself computes
 * from the label's real drawn text extent, so the annotation's rect and
 * the tagged text's position always agree) -- so the caller passes a
 * callback-free two-step: call HPDF_Page_ShowText() logic is inlined
 * here rather than split, since the rect must be measured between
 * BeginText/EndText and libharu has no "measure only" text call other
 * than HPDF_Page_TextWidth() (used instead, avoiding a throwaway first
 * draw). */
static HPDF_STATUS
draw_tagged_link (HPDF_UA_Context ctx, HPDF_Page page, HPDF_Font font,
                   HPDF_UA_StructElem doc_elem, HPDF_REAL x, HPDF_REAL *y,
                   const char *label, const char *alt_text,
                   HPDF_Destination dst, const char *uri,
                   HPDF_Annotation *annot_out)
{
    HPDF_UA_StructElem link_elem;
    HPDF_STATUS status;
    HPDF_Rect rect;
    HPDF_REAL text_width;
    HPDF_Annotation annot;

    link_elem = HPDF_UA_BeginStructureElement (ctx, doc_elem, HPDF_UA_ROLE_LINK);
    if (!link_elem)
        return HPDF_INVALID_OBJECT;

    status = HPDF_UA_SetAlternateText (ctx, link_elem, alt_text);
    if (status != HPDF_OK)
        return status;

    status = HPDF_UA_BeginMarkedContent (ctx, page, link_elem);
    if (status != HPDF_OK)
        return status;

    text_width = HPDF_Page_TextWidth (page, label);
    HPDF_Page_BeginText (page);
    HPDF_Page_MoveTextPos (page, x, *y);
    HPDF_Page_ShowText (page, label);
    HPDF_Page_EndText (page);

    status = HPDF_UA_EndMarkedContent (ctx, page);
    if (status != HPDF_OK)
        return status;

    rect.left = x - 2;
    rect.bottom = *y - 4;
    rect.right = x + text_width + 2;
    rect.top = *y + 14;

    /* Internal (destination) links pass `dst`; the external URI link
     * passes `uri` instead and `dst` is NULL -- exactly one of the two
     * is non-NULL, matching libharu's own two separate constructors. */
    annot = dst
        ? HPDF_Page_CreateLinkAnnot (page, rect, dst)
        : HPDF_Page_CreateURILinkAnnot (page, rect, uri);
    if (!annot)
        return HPDF_INVALID_OBJECT;

    status = HPDF_UA_TagAnnotation (ctx, page, link_elem, annot);
    if (status != HPDF_OK)
        return status;

    HPDF_UA_EndStructureElement (ctx, link_elem);

    if (annot_out)
        *annot_out = annot;

    *y -= 24;
    return HPDF_OK;
}

int
main (void)
{
    HPDF_Doc pdf;
    HPDF_Page index_page, page1, page2;
    HPDF_Font font;
    const char *font_name;
    HPDF_UA_Context ctx;
    HPDF_UA_StructElem doc_elem, h1_elem, intro_elem;
    HPDF_STATUS status;
    HPDF_REAL y;
    HPDF_UINT text_len;
    HPDF_Destination dst;
    HPDF_Annotation annot;
    const char *uri = "http://libharu.org";

    pdf = HPDF_New (error_handler, NULL);
    if (!pdf) {
        fprintf (stderr, "failed to create PDF document\n");
        return 1;
    }

    HPDF_SetInfoAttr (pdf, HPDF_INFO_TITLE, "libharu-pdfua tagged-annotation demo");
    HPDF_UA_SetDocumentLanguage (pdf, "en-US");
    HPDF_UA_SetDisplayDocTitle (pdf, HPDF_TRUE);

    status = HPDF_UA_AddMetadata (pdf);
    if (status != HPDF_OK) {
        fprintf (stderr, "HPDF_UA_AddMetadata failed\n");
        HPDF_Free (pdf);
        return 1;
    }

    ctx = HPDF_UA_NewContext (pdf);
    if (!ctx) {
        fprintf (stderr, "HPDF_UA_NewContext failed\n");
        HPDF_Free (pdf);
        return 1;
    }

    font_name = HPDF_LoadTTFontFromFile (pdf, HPDF_UA_DEMO_FONT_PATH, HPDF_TRUE);
    if (!font_name) {
        fprintf (stderr, "HPDF_LoadTTFontFromFile failed\n");
        HPDF_UA_FreeContext (ctx);
        HPDF_Free (pdf);
        return 1;
    }
    font = HPDF_GetFont (pdf, font_name, "WinAnsiEncoding");

    /* Destination pages first (so their HPDF_Destination objects exist
     * before the index page's links reference them) -- each one is
     * itself a small tagged document, not left as plain untagged text. */
    status = make_destination_page (ctx, pdf, font, "Destination Page 1", &page1);
    if (status != HPDF_OK)
        goto fail;
    status = make_destination_page (ctx, pdf, font, "Destination Page 2", &page2);
    if (status != HPDF_OK)
        goto fail;

    index_page = HPDF_AddPage (pdf);
    HPDF_Page_SetSize (index_page, HPDF_PAGE_SIZE_LETTER, HPDF_PAGE_PORTRAIT);
    HPDF_Page_SetFontAndSize (index_page, font, 11);

    {
        HPDF_Outline outline = HPDF_CreateOutline (pdf, NULL, "Annotation Demo", NULL);
        HPDF_Destination outline_dst = HPDF_Page_CreateDestination (index_page);

        HPDF_Destination_SetXYZ (outline_dst, 0, HPDF_Page_GetHeight (index_page), 1);
        HPDF_Outline_SetDestination (outline, outline_dst);
    }

    doc_elem = HPDF_UA_BeginStructureElement (ctx, NULL, HPDF_UA_ROLE_DOCUMENT);
    if (!doc_elem)
        goto fail;

    y = 730;
    h1_elem = HPDF_UA_BeginStructureElement (ctx, doc_elem, HPDF_UA_ROLE_H1);
    if (!h1_elem)
        goto fail;
    status = HPDF_UA_BeginMarkedContent (ctx, index_page, h1_elem);
    if (status != HPDF_OK)
        goto fail;
    HPDF_Page_SetFontAndSize (index_page, font, 20);
    HPDF_Page_BeginText (index_page);
    HPDF_Page_MoveTextPos (index_page, 50, y);
    HPDF_Page_ShowText (index_page, "Annotation Demo");
    HPDF_Page_EndText (index_page);
    status = HPDF_UA_EndMarkedContent (ctx, index_page);
    if (status != HPDF_OK)
        goto fail;
    HPDF_UA_EndStructureElement (ctx, h1_elem);
    y -= 40;

    intro_elem = HPDF_UA_BeginStructureElement (ctx, doc_elem, HPDF_UA_ROLE_P);
    if (!intro_elem)
        goto fail;
    status = HPDF_UA_BeginMarkedContent (ctx, index_page, intro_elem);
    if (status != HPDF_OK)
        goto fail;
    HPDF_Page_SetFontAndSize (index_page, font, 11);
    HPDF_Page_BeginText (index_page);
    text_len = 0;
    HPDF_Page_TextRect (index_page, 50, y, 512, y - 60, INTRO_TEXT,
            HPDF_TALIGN_LEFT, &text_len);
    HPDF_Page_EndText (index_page);
    status = HPDF_UA_EndMarkedContent (ctx, index_page);
    if (status != HPDF_OK)
        goto fail;
    HPDF_UA_EndStructureElement (ctx, intro_elem);
    y -= 90;

    HPDF_Page_SetFontAndSize (index_page, font, 11);

    /* --- Link 1: jump to Destination Page 1 --- */
    dst = HPDF_Page_CreateDestination (page1);
    HPDF_Destination_SetXYZ (dst, 0, HPDF_Page_GetHeight (page1), 1);
    status = draw_tagged_link (ctx, index_page, font, doc_elem, 50, &y,
            "Jump to Destination Page 1",
            "Link: jump to Destination Page 1", dst, NULL, &annot);
    if (status != HPDF_OK)
        goto fail;
    HPDF_LinkAnnot_SetHighlightMode (annot, HPDF_ANNOT_INVERT_BOX);

    /* --- Link 2: jump to Destination Page 2 (dashed border) --- */
    dst = HPDF_Page_CreateDestination (page2);
    HPDF_Destination_SetXYZ (dst, 0, HPDF_Page_GetHeight (page2), 1);
    status = draw_tagged_link (ctx, index_page, font, doc_elem, 50, &y,
            "Jump to Destination Page 2 (dashed border)",
            "Link: jump to Destination Page 2", dst, NULL, &annot);
    if (status != HPDF_OK)
        goto fail;
    HPDF_LinkAnnot_SetBorderStyle (annot, 1, 3, 2);

    /* --- Link 3: external URI link --- */
    {
        char label[64];
        (void) snprintf (label, sizeof (label), "Visit %s", uri);

        status = draw_tagged_link (ctx, index_page, font, doc_elem, 50, &y,
                label, "Link: opens http://libharu.org in a web browser",
                NULL, uri, &annot);
        if (status != HPDF_OK)
            goto fail;
    }

    HPDF_UA_EndStructureElement (ctx, doc_elem);

    HPDF_SaveToFile (pdf, "tagged_annotation_demo.pdf");
    HPDF_UA_FreeContext (ctx);
    HPDF_Free (pdf);

    printf ("wrote tagged_annotation_demo.pdf -- three real link "
            "annotations (two internal, one URI), each tagged via a "
            "Link structure element plus HPDF_UA_TagAnnotation(), "
            "ported from libharu's original link_annotation.c. Validate "
            "with validate/run_verapdf.sh.\n");
    return 0;

fail:
    fprintf (stderr, "tagged_annotation_demo: a tagging call failed, see above\n");
    HPDF_UA_FreeContext (ctx);
    HPDF_Free (pdf);
    return 1;
}
