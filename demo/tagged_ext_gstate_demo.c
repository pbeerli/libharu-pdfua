/*
 * tagged_ext_gstate_demo.c -- Milestone 6: tagged port of a representative
 * subset of libharu's original ext_gstate_demo.c: transparency
 * (HPDF_ExtGState_SetAlphaFill/Stroke) and a few blend modes
 * (HPDF_ExtGState_SetBlendMode). A deliberate scope cut from the original
 * (which shows all thirteen PDF blend modes): four representative modes
 * plus the two transparency levels are enough to exercise the real
 * HPDF_ExtGState code path through this project's tagging layer; the
 * other nine blend modes are the same PDF mechanism (an /ExtGState
 * resource applied via `gs`) with a different name, not a different code
 * path.
 *
 * Structure: Document > H1, Document > Figure (all the overlapping
 * circles, one marked-content span per group -- each group is its own
 * HPDF_Page_GSave()/HPDF_Page_SetExtGState()/HPDF_Page_GRestore() scope in
 * the original, kept as-is here since /ExtGState is itself a graphics
 * state, not something marked content wraps specially), real /Alt text,
 * then Document > Caption.
 *
 * Validate with: validate/run_verapdf.sh build/tagged_ext_gstate_demo.pdf
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

static void
draw_circles (HPDF_Page page, HPDF_Font font, const char *description,
        HPDF_REAL x, HPDF_REAL y)
{
    HPDF_Page_SetLineWidth (page, 1.0f);
    HPDF_Page_SetRGBStroke (page, 0.0f, 0.0f, 0.0f);
    HPDF_Page_SetRGBFill (page, 1.0f, 0.0f, 0.0f);
    HPDF_Page_Circle (page, x + 30, y + 30, 30);
    HPDF_Page_ClosePathFillStroke (page);
    HPDF_Page_SetRGBFill (page, 0.0f, 1.0f, 0.0f);
    HPDF_Page_Circle (page, x + 75, y + 30, 30);
    HPDF_Page_ClosePathFillStroke (page);
    HPDF_Page_SetRGBFill (page, 0.0f, 0.0f, 1.0f);
    HPDF_Page_Circle (page, x + 52, y + 56, 30);
    HPDF_Page_ClosePathFillStroke (page);

    HPDF_Page_SetRGBFill (page, 0.0f, 0.0f, 0.0f);
    HPDF_Page_SetFontAndSize (page, font, 9);
    HPDF_Page_BeginText (page);
    HPDF_Page_TextOut (page, x, y + 100.0f, description);
    HPDF_Page_EndText (page);
}

int
main (void)
{
    HPDF_Doc pdf;
    HPDF_Page page;
    HPDF_Font font;
    const char *font_name;
    HPDF_ExtGState gstate;
    HPDF_UA_Context ctx;
    HPDF_UA_StructElem doc_elem, h1_elem, figure_elem, caption_elem;
    HPDF_STATUS status;

    pdf = HPDF_New (error_handler, NULL);
    if (!pdf) {
        fprintf (stderr, "failed to create PDF document\n");
        return 1;
    }

    HPDF_SetInfoAttr (pdf, HPDF_INFO_TITLE, "libharu-pdfua tagged-ext-gstate demo");
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

    page = HPDF_AddPage (pdf);
    HPDF_Page_SetSize (page, HPDF_PAGE_SIZE_LETTER, HPDF_PAGE_LANDSCAPE);

    font_name = HPDF_LoadTTFontFromFile (pdf, HPDF_UA_DEMO_FONT_PATH, HPDF_TRUE);
    if (!font_name) {
        fprintf (stderr, "HPDF_LoadTTFontFromFile failed\n");
        HPDF_Free (pdf);
        return 1;
    }
    font = HPDF_GetFont (pdf, font_name, "WinAnsiEncoding");

    {
        HPDF_Outline outline = HPDF_CreateOutline (pdf, NULL, "ExtGState", NULL);
        HPDF_Destination dst = HPDF_Page_CreateDestination (page);

        HPDF_Destination_SetXYZ (dst, 0, HPDF_Page_GetHeight (page), 1);
        HPDF_Outline_SetDestination (outline, dst);
    }

    doc_elem = HPDF_UA_BeginStructureElement (ctx, NULL, HPDF_UA_ROLE_DOCUMENT);
    if (!doc_elem)
        goto fail;

    h1_elem = HPDF_UA_BeginStructureElement (ctx, doc_elem, HPDF_UA_ROLE_H1);
    if (!h1_elem)
        goto fail;
    status = HPDF_UA_BeginMarkedContent (ctx, page, h1_elem);
    if (status != HPDF_OK)
        goto fail;
    HPDF_Page_SetFontAndSize (page, font, 18);
    HPDF_Page_BeginText (page);
    HPDF_Page_MoveTextPos (page, 40, 560);
    HPDF_Page_ShowText (page, "ExtGState Demo");
    HPDF_Page_EndText (page);
    status = HPDF_UA_EndMarkedContent (ctx, page);
    if (status != HPDF_OK)
        goto fail;
    HPDF_UA_EndStructureElement (ctx, h1_elem);

    figure_elem = HPDF_UA_BeginStructureElement (ctx, doc_elem, HPDF_UA_ROLE_FIGURE);
    if (!figure_elem)
        goto fail;
    status = HPDF_UA_SetAlternateText (ctx, figure_elem,
            "Six groups of three overlapping red/green/blue circles, each "
            "demonstrating a different /ExtGState setting: normal "
            "(opaque); fill+stroke alpha 0.8; fill alpha 0.4; and the "
            "Multiply, Screen, and Darken blend modes.");
    if (status != HPDF_OK)
        goto fail;

    status = HPDF_UA_BeginMarkedContent (ctx, page, figure_elem);
    if (status != HPDF_OK)
        goto fail;

    HPDF_Page_GSave (page);
    draw_circles (page, font, "normal", 40.0f, 380.0f);
    HPDF_Page_GRestore (page);

    HPDF_Page_GSave (page);
    gstate = HPDF_CreateExtGState (pdf);
    HPDF_ExtGState_SetAlphaFill (gstate, 0.8);
    HPDF_ExtGState_SetAlphaStroke (gstate, 0.8);
    HPDF_Page_SetExtGState (page, gstate);
    draw_circles (page, font, "alpha = 0.8", 190.0f, 380.0f);
    HPDF_Page_GRestore (page);

    HPDF_Page_GSave (page);
    gstate = HPDF_CreateExtGState (pdf);
    HPDF_ExtGState_SetAlphaFill (gstate, 0.4);
    HPDF_Page_SetExtGState (page, gstate);
    draw_circles (page, font, "fill alpha = 0.4", 340.0f, 380.0f);
    HPDF_Page_GRestore (page);

    HPDF_Page_GSave (page);
    gstate = HPDF_CreateExtGState (pdf);
    HPDF_ExtGState_SetBlendMode (gstate, HPDF_BM_MULTIPLY);
    HPDF_Page_SetExtGState (page, gstate);
    draw_circles (page, font, "BM_MULTIPLY", 490.0f, 380.0f);
    HPDF_Page_GRestore (page);

    HPDF_Page_GSave (page);
    gstate = HPDF_CreateExtGState (pdf);
    HPDF_ExtGState_SetBlendMode (gstate, HPDF_BM_SCREEN);
    HPDF_Page_SetExtGState (page, gstate);
    draw_circles (page, font, "BM_SCREEN", 640.0f, 380.0f);
    HPDF_Page_GRestore (page);

    HPDF_Page_GSave (page);
    gstate = HPDF_CreateExtGState (pdf);
    HPDF_ExtGState_SetBlendMode (gstate, HPDF_BM_DARKEN);
    HPDF_Page_SetExtGState (page, gstate);
    draw_circles (page, font, "BM_DARKEN", 40.0f, 220.0f);
    HPDF_Page_GRestore (page);

    status = HPDF_UA_EndMarkedContent (ctx, page);
    if (status != HPDF_OK)
        goto fail;
    HPDF_UA_EndStructureElement (ctx, figure_elem);

    caption_elem = HPDF_UA_BeginStructureElement (ctx, doc_elem, HPDF_UA_ROLE_CAPTION);
    if (!caption_elem)
        goto fail;
    status = HPDF_UA_BeginMarkedContent (ctx, page, caption_elem);
    if (status != HPDF_OK)
        goto fail;
    HPDF_Page_SetFontAndSize (page, font, 10);
    HPDF_Page_BeginText (page);
    HPDF_Page_MoveTextPos (page, 40, 130);
    HPDF_Page_ShowText (page, "Figure 1: transparency and blend modes via HPDF_ExtGState.");
    HPDF_Page_EndText (page);
    status = HPDF_UA_EndMarkedContent (ctx, page);
    if (status != HPDF_OK)
        goto fail;
    HPDF_UA_EndStructureElement (ctx, caption_elem);

    HPDF_UA_EndStructureElement (ctx, doc_elem);

    HPDF_SaveToFile (pdf, "tagged_ext_gstate_demo.pdf");
    HPDF_UA_FreeContext (ctx);
    HPDF_Free (pdf);

    printf ("wrote tagged_ext_gstate_demo.pdf -- a real tagged "
            "transparency/blend-mode figure (ext_gstate_demo.c port). "
            "Validate with validate/run_verapdf.sh.\n");
    return 0;

fail:
    fprintf (stderr, "tagged_ext_gstate_demo: a tagging call failed, see above\n");
    HPDF_UA_FreeContext (ctx);
    HPDF_Free (pdf);
    return 1;
}
