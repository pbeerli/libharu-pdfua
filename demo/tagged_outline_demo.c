/*
 * tagged_outline_demo.c -- Milestone 6: tagged port of libharu's original
 * outline_demo.c: a document outline (bookmarks) tree with several
 * entries, each pointing at a different page.
 *
 * Every other demo in this project creates at most one outline entry per
 * document; this one is the real test of a multi-entry outline TREE (one
 * root, three children, each with its own HPDF_Destination) -- the actual
 * code path outline_demo.c exists to exercise.
 *
 * Structure: three separate pages, each its own Document > H1 (this
 * project's tagging context and structure tree are document-wide, not
 * per-page, so all three H1s are still children of the same single
 * HPDF_UA_ROLE_DOCUMENT root -- matching how tagged_annotation_demo.c
 * already tags multiple pages under one context).
 *
 * Validate with: validate/run_verapdf.sh build/tagged_outline_demo.pdf
 */
#include <stdio.h>
#include "hpdf_ua/hpdf_ua.h"

static void
error_handler (HPDF_STATUS error_no, HPDF_STATUS detail_no, void *user_data)
{
    (void) user_data;
    fprintf (stderr, "libharu error: error_no=0x%04lX detail_no=%ld\n",
            (unsigned long) error_no, (long) detail_no);
}

static HPDF_Page
make_page (HPDF_Doc pdf, HPDF_UA_Context ctx, HPDF_UA_StructElem doc_elem,
        HPDF_Font font, int page_num, HPDF_STATUS *status)
{
    HPDF_Page page = HPDF_AddPage (pdf);
    HPDF_UA_StructElem h1_elem;
    char buf[32];

    HPDF_Page_SetSize (page, HPDF_PAGE_SIZE_LETTER, HPDF_PAGE_PORTRAIT);

    h1_elem = HPDF_UA_BeginStructureElement (ctx, doc_elem, HPDF_UA_ROLE_H1);
    if (!h1_elem) {
        *status = HPDF_FAILED_TO_ALLOC_MEM;
        return NULL;
    }
    *status = HPDF_UA_BeginMarkedContent (ctx, page, h1_elem);
    if (*status != HPDF_OK)
        return NULL;

    (void) snprintf (buf, sizeof (buf), "Page %d", page_num);
    HPDF_Page_SetFontAndSize (page, font, 24);
    HPDF_Page_BeginText (page);
    HPDF_Page_MoveTextPos (page, 50, 750);
    HPDF_Page_ShowText (page, buf);
    HPDF_Page_EndText (page);

    *status = HPDF_UA_EndMarkedContent (ctx, page);
    if (*status != HPDF_OK)
        return NULL;
    HPDF_UA_EndStructureElement (ctx, h1_elem);

    return page;
}

int
main (void)
{
    HPDF_Doc pdf;
    HPDF_Page page1, page2, page3;
    HPDF_Font font;
    const char *font_name;
    HPDF_UA_Context ctx;
    HPDF_UA_StructElem doc_elem;
    HPDF_STATUS status;
    HPDF_Outline root, outline1, outline2, outline3;
    HPDF_Destination dst;

    pdf = HPDF_New (error_handler, NULL);
    if (!pdf) {
        fprintf (stderr, "failed to create PDF document\n");
        return 1;
    }

    HPDF_SetInfoAttr (pdf, HPDF_INFO_TITLE, "libharu-pdfua tagged-outline demo");
    HPDF_UA_SetDocumentLanguage (pdf, "en-US");
    HPDF_UA_SetDisplayDocTitle (pdf, HPDF_TRUE);
    HPDF_SetPageMode (pdf, HPDF_PAGE_MODE_USE_OUTLINE);

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

    doc_elem = HPDF_UA_BeginStructureElement (ctx, NULL, HPDF_UA_ROLE_DOCUMENT);
    if (!doc_elem)
        goto fail;

    page1 = make_page (pdf, ctx, doc_elem, font, 1, &status);
    if (!page1 || status != HPDF_OK)
        goto fail;
    page2 = make_page (pdf, ctx, doc_elem, font, 2, &status);
    if (!page2 || status != HPDF_OK)
        goto fail;
    page3 = make_page (pdf, ctx, doc_elem, font, 3, &status);
    if (!page3 || status != HPDF_OK)
        goto fail;

    HPDF_UA_EndStructureElement (ctx, doc_elem);

    /* --- the real point of this demo: a 3-entry outline tree, one entry
     * per page, matching outline_demo.c's own structure. --- */
    root = HPDF_CreateOutline (pdf, NULL, "OutlineRoot", NULL);
    HPDF_Outline_SetOpened (root, HPDF_TRUE);

    outline1 = HPDF_CreateOutline (pdf, root, "Page 1", NULL);
    dst = HPDF_Page_CreateDestination (page1);
    HPDF_Destination_SetXYZ (dst, 0, HPDF_Page_GetHeight (page1), 1);
    HPDF_Outline_SetDestination (outline1, dst);

    outline2 = HPDF_CreateOutline (pdf, root, "Page 2", NULL);
    dst = HPDF_Page_CreateDestination (page2);
    HPDF_Destination_SetXYZ (dst, 0, HPDF_Page_GetHeight (page2), 1);
    HPDF_Outline_SetDestination (outline2, dst);

    outline3 = HPDF_CreateOutline (pdf, root, "Page 3", NULL);
    dst = HPDF_Page_CreateDestination (page3);
    HPDF_Destination_SetXYZ (dst, 0, HPDF_Page_GetHeight (page3), 1);
    HPDF_Outline_SetDestination (outline3, dst);

    HPDF_SaveToFile (pdf, "tagged_outline_demo.pdf");
    HPDF_UA_FreeContext (ctx);
    HPDF_Free (pdf);

    printf ("wrote tagged_outline_demo.pdf -- a real 3-entry tagged "
            "document outline, one per page (outline_demo.c port). "
            "Validate with validate/run_verapdf.sh.\n");
    return 0;

fail:
    fprintf (stderr, "tagged_outline_demo: a tagging call failed, see above\n");
    HPDF_UA_FreeContext (ctx);
    HPDF_Free (pdf);
    return 1;
}
