/*
 * tagged_arc_demo.c -- Milestone 6: tagged port of libharu's original
 * arc_demo.c (HPDF_Page_Arc()/HPDF_Page_Circle() pie-chart drawing),
 * exercising the same real path-painting-inside-marked-content code path
 * as tagged_histogram_demo.c, now for arcs rather than rectangles.
 *
 * Structure: Document > H1, then Document > Figure (the whole pie chart:
 * four wedges plus the center circle, one marked-content span) with real
 * /Alt text describing the proportions, then Document > Caption.
 *
 * Validate with: validate/run_verapdf.sh build/tagged_arc_demo.pdf
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

int
main (void)
{
    HPDF_Doc pdf;
    HPDF_Page page;
    HPDF_Font font;
    const char *font_name;
    HPDF_UA_Context ctx;
    HPDF_UA_StructElem doc_elem, h1_elem, figure_elem, caption_elem;
    HPDF_STATUS status;
    HPDF_Point pos;

    pdf = HPDF_New (error_handler, NULL);
    if (!pdf) {
        fprintf (stderr, "failed to create PDF document\n");
        return 1;
    }

    HPDF_SetInfoAttr (pdf, HPDF_INFO_TITLE, "libharu-pdfua tagged-arc demo");
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
    HPDF_Page_SetSize (page, HPDF_PAGE_SIZE_LETTER, HPDF_PAGE_PORTRAIT);

    font_name = HPDF_LoadTTFontFromFile (pdf, HPDF_UA_DEMO_FONT_PATH, HPDF_TRUE);
    if (!font_name) {
        fprintf (stderr, "HPDF_LoadTTFontFromFile failed\n");
        HPDF_Free (pdf);
        return 1;
    }
    font = HPDF_GetFont (pdf, font_name, "WinAnsiEncoding");

    {
        HPDF_Outline outline = HPDF_CreateOutline (pdf, NULL, "Arc", NULL);
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
    HPDF_Page_MoveTextPos (page, 50, 750);
    HPDF_Page_ShowText (page, "Arc Demo");
    HPDF_Page_EndText (page);
    status = HPDF_UA_EndMarkedContent (ctx, page);
    if (status != HPDF_OK)
        goto fail;
    HPDF_UA_EndStructureElement (ctx, h1_elem);

    /* --- Figure: the whole pie chart is one marked-content span, exactly
     * mirroring arc_demo.c's own four wedges (Arc()) plus a center circle
     * (Circle()) -- both real path-painting operators wrapped in
     * BDC..EMC, same code path tagged_histogram_demo.c already proved for
     * rectangles. --- */
    figure_elem = HPDF_UA_BeginStructureElement (ctx, doc_elem, HPDF_UA_ROLE_FIGURE);
    if (!figure_elem)
        goto fail;

    status = HPDF_UA_SetAlternateText (ctx, figure_elem,
            "Pie chart with four wedges: A (red) 45%, B (blue) 25%, "
            "C (green) 15%, D (yellow) 15%, and a white circle at the "
            "center.");
    if (status != HPDF_OK)
        goto fail;

    status = HPDF_UA_BeginMarkedContent (ctx, page, figure_elem);
    if (status != HPDF_OK)
        goto fail;

    /* A: 45% red */
    HPDF_Page_SetRGBFill (page, 1.0, 0, 0);
    HPDF_Page_MoveTo (page, 200, 550);
    HPDF_Page_LineTo (page, 200, 630);
    HPDF_Page_Arc (page, 200, 550, 80, 0, 360 * 0.45);
    pos = HPDF_Page_GetCurrentPos (page);
    HPDF_Page_LineTo (page, 200, 550);
    HPDF_Page_Fill (page);

    /* B: 25% blue */
    HPDF_Page_SetRGBFill (page, 0, 0, 1.0);
    HPDF_Page_MoveTo (page, 200, 550);
    HPDF_Page_LineTo (page, pos.x, pos.y);
    HPDF_Page_Arc (page, 200, 550, 80, 360 * 0.45, 360 * 0.7);
    pos = HPDF_Page_GetCurrentPos (page);
    HPDF_Page_LineTo (page, 200, 550);
    HPDF_Page_Fill (page);

    /* C: 15% green */
    HPDF_Page_SetRGBFill (page, 0, 1.0, 0);
    HPDF_Page_MoveTo (page, 200, 550);
    HPDF_Page_LineTo (page, pos.x, pos.y);
    HPDF_Page_Arc (page, 200, 550, 80, 360 * 0.7, 360 * 0.85);
    pos = HPDF_Page_GetCurrentPos (page);
    HPDF_Page_LineTo (page, 200, 550);
    HPDF_Page_Fill (page);

    /* D: 15% yellow */
    HPDF_Page_SetRGBFill (page, 1.0, 1.0, 0);
    HPDF_Page_MoveTo (page, 200, 550);
    HPDF_Page_LineTo (page, pos.x, pos.y);
    HPDF_Page_Arc (page, 200, 550, 80, 360 * 0.85, 360);
    HPDF_Page_LineTo (page, 200, 550);
    HPDF_Page_Fill (page);

    /* center circle */
    HPDF_Page_SetGrayStroke (page, 0);
    HPDF_Page_SetGrayFill (page, 1);
    HPDF_Page_Circle (page, 200, 550, 30);
    HPDF_Page_Fill (page);

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
    HPDF_Page_MoveTextPos (page, 50, 440);
    HPDF_Page_ShowText (page, "Figure 1: HPDF_Page_Arc()/HPDF_Page_Circle() pie chart.");
    HPDF_Page_EndText (page);
    status = HPDF_UA_EndMarkedContent (ctx, page);
    if (status != HPDF_OK)
        goto fail;
    HPDF_UA_EndStructureElement (ctx, caption_elem);

    HPDF_UA_EndStructureElement (ctx, doc_elem);

    HPDF_SaveToFile (pdf, "tagged_arc_demo.pdf");
    HPDF_UA_FreeContext (ctx);
    HPDF_Free (pdf);

    printf ("wrote tagged_arc_demo.pdf -- a real tagged pie chart "
            "(Document > Figure with /Alt text, arc_demo.c port). "
            "Validate with validate/run_verapdf.sh.\n");
    return 0;

fail:
    fprintf (stderr, "tagged_arc_demo: a tagging call failed, see above\n");
    HPDF_UA_FreeContext (ctx);
    HPDF_Free (pdf);
    return 1;
}
