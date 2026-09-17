/*
 * tagged_line_demo.c -- Milestone 6: tagged port of a representative
 * subset of libharu's original line_demo.c: line widths, dash patterns,
 * line caps/joins, and the three Bezier-curve constructors
 * (CurveTo/CurveTo2/CurveTo3). A deliberate scope cut from the original
 * (which also demos fill/stroke/clip rectangles): this project's own
 * tagged_table_demo.c and tagged_line_demo.c's own clip section already
 * exercise fill/stroke/clip, so this port focuses on the vector-drawing
 * operators no other demo in this project touches yet.
 *
 * Structure: Document > H1, Document > Figure (all the line/curve
 * drawing, one marked-content span, real /Alt text) with each drawing's
 * own on-page label folded into the same span (the labels are part of
 * the figure's own visual content, same pattern as tagged_histogram_demo.c's
 * bar tick labels), then Document > Caption.
 *
 * Validate with: validate/run_verapdf.sh build/tagged_line_demo.pdf
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
label (HPDF_Page page, HPDF_REAL x, HPDF_REAL y, const char *text)
{
    HPDF_Page_BeginText (page);
    HPDF_Page_MoveTextPos (page, x, y);
    HPDF_Page_ShowText (page, text);
    HPDF_Page_EndText (page);
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
    const HPDF_REAL DASH2[] = { 3.0, 7.0 };
    const HPDF_REAL DASH4[] = { 8.0, 7.0, 2.0, 7.0 };

    pdf = HPDF_New (error_handler, NULL);
    if (!pdf) {
        fprintf (stderr, "failed to create PDF document\n");
        return 1;
    }

    HPDF_SetInfoAttr (pdf, HPDF_INFO_TITLE, "libharu-pdfua tagged-line demo");
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
        HPDF_Outline outline = HPDF_CreateOutline (pdf, NULL, "Line", NULL);
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
    label (page, 50, 750, "Line Demo");
    status = HPDF_UA_EndMarkedContent (ctx, page);
    if (status != HPDF_OK)
        goto fail;
    HPDF_UA_EndStructureElement (ctx, h1_elem);

    figure_elem = HPDF_UA_BeginStructureElement (ctx, doc_elem, HPDF_UA_ROLE_FIGURE);
    if (!figure_elem)
        goto fail;
    status = HPDF_UA_SetAlternateText (ctx, figure_elem,
            "Line-drawing reference sheet: three horizontal line widths "
            "(0, 1, 2); three dash patterns (solid, [3 7], [8 7 2 7]); "
            "three line-cap styles (butt, round, projecting square) shown "
            "as thick horizontal strokes; three line-join styles (miter, "
            "round, bevel) shown as angled two-segment paths; and the "
            "three Bezier curve constructors CurveTo, CurveTo2, CurveTo3, "
            "each shown as a solid curve next to its straight-line "
            "control-point construction in a dashed line.");
    if (status != HPDF_OK)
        goto fail;

    status = HPDF_UA_BeginMarkedContent (ctx, page, figure_elem);
    if (status != HPDF_OK)
        goto fail;

    HPDF_Page_SetFontAndSize (page, font, 9);

    /* --- line widths --- */
    HPDF_Page_SetLineWidth (page, 0);
    label (page, 60, 715, "line width = 0");
    HPDF_Page_MoveTo (page, 60, 705); HPDF_Page_LineTo (page, 280, 705);
    HPDF_Page_Stroke (page);

    HPDF_Page_SetLineWidth (page, 1.0);
    label (page, 60, 690, "line width = 1.0");
    HPDF_Page_MoveTo (page, 60, 680); HPDF_Page_LineTo (page, 280, 680);
    HPDF_Page_Stroke (page);

    HPDF_Page_SetLineWidth (page, 2.0);
    label (page, 60, 665, "line width = 2.0");
    HPDF_Page_MoveTo (page, 60, 655); HPDF_Page_LineTo (page, 280, 655);
    HPDF_Page_Stroke (page);

    /* --- dash patterns --- */
    HPDF_Page_SetLineWidth (page, 1.0);
    HPDF_Page_SetDash (page, NULL, 0, 0);
    label (page, 60, 630, "dash: solid");
    HPDF_Page_MoveTo (page, 60, 620); HPDF_Page_LineTo (page, 280, 620);
    HPDF_Page_Stroke (page);

    HPDF_Page_SetDash (page, DASH2, 2, 2.0);
    label (page, 60, 605, "dash_ptn=[3, 7], phase=2");
    HPDF_Page_MoveTo (page, 60, 595); HPDF_Page_LineTo (page, 280, 595);
    HPDF_Page_Stroke (page);

    HPDF_Page_SetDash (page, DASH4, 4, 0);
    label (page, 60, 580, "dash_ptn=[8, 7, 2, 7]");
    HPDF_Page_MoveTo (page, 60, 570); HPDF_Page_LineTo (page, 280, 570);
    HPDF_Page_Stroke (page);
    HPDF_Page_SetDash (page, NULL, 0, 0);

    /* --- line caps --- */
    HPDF_Page_SetLineWidth (page, 16);
    HPDF_Page_SetRGBStroke (page, 0.0, 0.4, 0.0);

    HPDF_Page_SetLineCap (page, HPDF_BUTT_END);
    label (page, 60, 535, "HPDF_BUTT_END");
    HPDF_Page_MoveTo (page, 90, 510); HPDF_Page_LineTo (page, 250, 510);
    HPDF_Page_Stroke (page);

    HPDF_Page_SetLineCap (page, HPDF_ROUND_END);
    label (page, 60, 490, "HPDF_ROUND_END");
    HPDF_Page_MoveTo (page, 90, 465); HPDF_Page_LineTo (page, 250, 465);
    HPDF_Page_Stroke (page);

    HPDF_Page_SetLineCap (page, HPDF_PROJECTING_SQUARE_END);
    label (page, 60, 445, "HPDF_PROJECTING_SQUARE_END");
    HPDF_Page_MoveTo (page, 90, 420); HPDF_Page_LineTo (page, 250, 420);
    HPDF_Page_Stroke (page);

    /* --- line joins --- */
    HPDF_Page_SetLineWidth (page, 16);
    HPDF_Page_SetRGBStroke (page, 0.0, 0.0, 0.5);

    HPDF_Page_SetLineJoin (page, HPDF_MITER_JOIN);
    label (page, 320, 715, "HPDF_MITER_JOIN");
    HPDF_Page_MoveTo (page, 340, 660); HPDF_Page_LineTo (page, 370, 695);
    HPDF_Page_LineTo (page, 400, 660); HPDF_Page_Stroke (page);

    HPDF_Page_SetLineJoin (page, HPDF_ROUND_JOIN);
    label (page, 320, 620, "HPDF_ROUND_JOIN");
    HPDF_Page_MoveTo (page, 340, 565); HPDF_Page_LineTo (page, 370, 600);
    HPDF_Page_LineTo (page, 400, 565); HPDF_Page_Stroke (page);

    HPDF_Page_SetLineJoin (page, HPDF_BEVEL_JOIN);
    label (page, 320, 525, "HPDF_BEVEL_JOIN");
    HPDF_Page_MoveTo (page, 340, 470); HPDF_Page_LineTo (page, 370, 505);
    HPDF_Page_LineTo (page, 400, 470); HPDF_Page_Stroke (page);

    HPDF_Page_SetLineJoin (page, HPDF_MITER_JOIN);
    HPDF_Page_SetRGBStroke (page, 0, 0, 0);
    HPDF_Page_SetLineWidth (page, 1.5);
    HPDF_Page_SetFontAndSize (page, font, 9);

    /* --- curves --- */
    label (page, 60, 380, "CurveTo2(x1, y1, x2, y2)");
    HPDF_Page_SetDash (page, DASH2, 1, 0);
    HPDF_Page_SetLineWidth (page, 0.5);
    HPDF_Page_MoveTo (page, 90, 340); HPDF_Page_LineTo (page, 190, 370);
    HPDF_Page_Stroke (page);
    HPDF_Page_SetDash (page, NULL, 0, 0);
    HPDF_Page_SetLineWidth (page, 1.5);
    HPDF_Page_MoveTo (page, 60, 300);
    HPDF_Page_CurveTo2 (page, 90, 340, 190, 370);
    HPDF_Page_Stroke (page);

    label (page, 320, 380, "CurveTo3(x1, y1, x2, y2)");
    HPDF_Page_SetDash (page, DASH2, 1, 0);
    HPDF_Page_SetLineWidth (page, 0.5);
    HPDF_Page_MoveTo (page, 350, 340); HPDF_Page_LineTo (page, 450, 370);
    HPDF_Page_Stroke (page);
    HPDF_Page_SetDash (page, NULL, 0, 0);
    HPDF_Page_SetLineWidth (page, 1.5);
    HPDF_Page_MoveTo (page, 320, 300);
    HPDF_Page_CurveTo3 (page, 350, 340, 450, 370);
    HPDF_Page_Stroke (page);

    label (page, 60, 220, "CurveTo(x1, y1, x2, y2, x3, y3)");
    HPDF_Page_SetDash (page, DASH2, 1, 0);
    HPDF_Page_SetLineWidth (page, 0.5);
    HPDF_Page_MoveTo (page, 90, 180); HPDF_Page_LineTo (page, 190, 210);
    HPDF_Page_Stroke (page);
    HPDF_Page_MoveTo (page, 190, 210); HPDF_Page_LineTo (page, 260, 150);
    HPDF_Page_Stroke (page);
    HPDF_Page_SetDash (page, NULL, 0, 0);
    HPDF_Page_SetLineWidth (page, 1.5);
    HPDF_Page_MoveTo (page, 60, 140);
    HPDF_Page_CurveTo (page, 90, 180, 190, 210, 260, 150);
    HPDF_Page_Stroke (page);

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
    label (page, 50, 100, "Figure 1: line width, dash, cap, join, and Bezier curve constructors.");
    status = HPDF_UA_EndMarkedContent (ctx, page);
    if (status != HPDF_OK)
        goto fail;
    HPDF_UA_EndStructureElement (ctx, caption_elem);

    HPDF_UA_EndStructureElement (ctx, doc_elem);

    HPDF_SaveToFile (pdf, "tagged_line_demo.pdf");
    HPDF_UA_FreeContext (ctx);
    HPDF_Free (pdf);

    printf ("wrote tagged_line_demo.pdf -- a real tagged line/curve "
            "reference sheet (line_demo.c port). Validate with "
            "validate/run_verapdf.sh.\n");
    return 0;

fail:
    fprintf (stderr, "tagged_line_demo: a tagging call failed, see above\n");
    HPDF_UA_FreeContext (ctx);
    HPDF_Free (pdf);
    return 1;
}
