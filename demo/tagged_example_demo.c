/*
 * tagged_example_demo.c -- a single combined example: real body text, a
 * table, and a figure all in one tagged, PDF/UA-1 document, plus a real
 * document outline. Exercises everything Milestones 1-4 built together in
 * one file, rather than each shape in isolation -- the one thing the
 * per-shape demos never tested is whether a document that MIXES ordinary
 * paragraph text with a table and a figure holds together correctly (a
 * shared /StructTreeRoot with a Document root spanning heterogeneous
 * children, a shared /ParentTree across mixed content types on one page).
 *
 * Structure: Document > [H1 "Example Document", P (intro paragraph),
 * H2 "Example Table" + Table > TR > TH/TD, H2 "Example Figure" + Figure
 * (bar chart) + Caption]. Outline: one entry per top-level section.
 *
 * Validate with: validate/run_verapdf.sh build/tagged_example_demo.pdf
 */
#include <stdio.h>
#include <string.h>
#include "hpdf_ua/hpdf_ua.h"

#define NBINS 8

static const char *TABLE_HEADERS[3] = { "Locus", "Theta", "M" };
static const char *TABLE_ROWS[3][3] = {
    { "1", "0.0123", "1.45" },
    { "2", "0.0098", "1.61" },
    { "All", "0.0111", "1.53" },
};

static const double DENSITY[NBINS] = {
    0.03, 0.09, 0.18, 0.24, 0.22, 0.14, 0.07, 0.03
};

static const char *INTRO_TEXT =
    "This is an example document produced by libharu-pdfua, showing "
    "ordinary body text, a data table, and a figure together in one "
    "tagged, PDF/UA-1 conformant PDF. Each of these three content types "
    "-- plain paragraph text, a table with header cells, and a bar-chart "
    "figure with alternate text -- is wrapped in real structure elements "
    "and marked content, not just drawn as plain graphics.";

static void
error_handler (HPDF_STATUS error_no, HPDF_STATUS detail_no, void *user_data)
{
    (void) user_data;
    fprintf (stderr, "libharu error: error_no=0x%04lX detail_no=%ld\n",
            (unsigned long) error_no, (long) detail_no);
}

/* Draws one tagged cell (shared by header and data rows). */
static HPDF_STATUS
draw_cell (HPDF_UA_Context ctx, HPDF_Page page, HPDF_Font font,
           HPDF_UA_StructElem cell_elem, HPDF_REAL x, HPDF_REAL y,
           const char *text)
{
    HPDF_STATUS status = HPDF_UA_BeginMarkedContent (ctx, page, cell_elem);
    if (status != HPDF_OK)
        return status;

    HPDF_Page_SetFontAndSize (page, font, 10);
    HPDF_Page_BeginText (page);
    HPDF_Page_MoveTextPos (page, x, y);
    HPDF_Page_ShowText (page, text);
    HPDF_Page_EndText (page);

    return HPDF_UA_EndMarkedContent (ctx, page);
}

int
main (void)
{
    HPDF_Doc pdf;
    HPDF_Page page;
    HPDF_Font font;
    const char *font_name;
    HPDF_UA_Context ctx;
    HPDF_UA_StructElem doc_elem, h1_elem, intro_elem;
    HPDF_UA_StructElem h2_table_elem, table_elem, tr_elem, cell_elem;
    HPDF_UA_StructElem h2_figure_elem, figure_elem, caption_elem;
    HPDF_STATUS status;
    HPDF_REAL y;
    HPDF_UINT text_len;
    int row, col, i;
    double max_density = 0.0;

    for (i = 0; i < NBINS; i++)
        if (DENSITY[i] > max_density)
            max_density = DENSITY[i];

    pdf = HPDF_New (error_handler, NULL);
    if (!pdf) {
        fprintf (stderr, "failed to create PDF document\n");
        return 1;
    }

    HPDF_SetInfoAttr (pdf, HPDF_INFO_TITLE, "libharu-pdfua combined example");
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

    doc_elem = HPDF_UA_BeginStructureElement (ctx, NULL, HPDF_UA_ROLE_DOCUMENT);
    if (!doc_elem)
        goto fail;

    /* --- H1 + intro paragraph --- */
    y = 730;
    h1_elem = HPDF_UA_BeginStructureElement (ctx, doc_elem, HPDF_UA_ROLE_H1);
    if (!h1_elem)
        goto fail;
    status = HPDF_UA_BeginMarkedContent (ctx, page, h1_elem);
    if (status != HPDF_OK)
        goto fail;
    HPDF_Page_SetFontAndSize (page, font, 20);
    HPDF_Page_BeginText (page);
    HPDF_Page_MoveTextPos (page, 50, y);
    HPDF_Page_ShowText (page, "Example Document");
    HPDF_Page_EndText (page);
    status = HPDF_UA_EndMarkedContent (ctx, page);
    if (status != HPDF_OK)
        goto fail;
    HPDF_UA_EndStructureElement (ctx, h1_elem);

    y -= 40;
    intro_elem = HPDF_UA_BeginStructureElement (ctx, doc_elem, HPDF_UA_ROLE_P);
    if (!intro_elem)
        goto fail;
    status = HPDF_UA_BeginMarkedContent (ctx, page, intro_elem);
    if (status != HPDF_OK)
        goto fail;
    HPDF_Page_SetFontAndSize (page, font, 11);
    HPDF_Page_BeginText (page);
    text_len = 0;
    HPDF_Page_TextRect (page, 50, y, 512, y - 70, INTRO_TEXT,
            HPDF_TALIGN_LEFT, &text_len);
    HPDF_Page_EndText (page);
    status = HPDF_UA_EndMarkedContent (ctx, page);
    if (status != HPDF_OK)
        goto fail;
    HPDF_UA_EndStructureElement (ctx, intro_elem);
    y -= 90;

    /* --- Table section --- */
    h2_table_elem = HPDF_UA_BeginStructureElement (ctx, doc_elem, HPDF_UA_ROLE_H2);
    if (!h2_table_elem)
        goto fail;
    status = HPDF_UA_BeginMarkedContent (ctx, page, h2_table_elem);
    if (status != HPDF_OK)
        goto fail;
    HPDF_Page_SetFontAndSize (page, font, 14);
    HPDF_Page_BeginText (page);
    HPDF_Page_MoveTextPos (page, 50, y);
    HPDF_Page_ShowText (page, "Example Table");
    HPDF_Page_EndText (page);
    status = HPDF_UA_EndMarkedContent (ctx, page);
    if (status != HPDF_OK)
        goto fail;
    HPDF_UA_EndStructureElement (ctx, h2_table_elem);
    y -= 25;

    table_elem = HPDF_UA_BeginStructureElement (ctx, doc_elem, HPDF_UA_ROLE_TABLE);
    if (!table_elem)
        goto fail;

    for (row = 0; row < 4; row++) {
        tr_elem = HPDF_UA_BeginStructureElement (ctx, table_elem, HPDF_UA_ROLE_TR);
        if (!tr_elem)
            goto fail;

        for (col = 0; col < 3; col++) {
            HPDF_REAL x = 50 + col * 110;

            cell_elem = HPDF_UA_BeginStructureElement (ctx, tr_elem,
                    row == 0 ? HPDF_UA_ROLE_TH : HPDF_UA_ROLE_TD);
            if (!cell_elem)
                goto fail;

            if (row == 0) {
                status = HPDF_UA_SetTableHeaderScope (ctx, cell_elem, HPDF_UA_SCOPE_COLUMN);
                if (status != HPDF_OK)
                    goto fail;
            }

            status = draw_cell (ctx, page, font, cell_elem, x, y,
                    row == 0 ? TABLE_HEADERS[col] : TABLE_ROWS[row - 1][col]);
            if (status != HPDF_OK)
                goto fail;

            HPDF_UA_EndStructureElement (ctx, cell_elem);
        }

        HPDF_UA_EndStructureElement (ctx, tr_elem);
        y -= 18;
    }
    HPDF_UA_EndStructureElement (ctx, table_elem);
    y -= 20;

    /* --- Figure section --- */
    h2_figure_elem = HPDF_UA_BeginStructureElement (ctx, doc_elem, HPDF_UA_ROLE_H2);
    if (!h2_figure_elem)
        goto fail;
    status = HPDF_UA_BeginMarkedContent (ctx, page, h2_figure_elem);
    if (status != HPDF_OK)
        goto fail;
    HPDF_Page_SetFontAndSize (page, font, 14);
    HPDF_Page_BeginText (page);
    HPDF_Page_MoveTextPos (page, 50, y);
    HPDF_Page_ShowText (page, "Example Figure");
    HPDF_Page_EndText (page);
    status = HPDF_UA_EndMarkedContent (ctx, page);
    if (status != HPDF_OK)
        goto fail;
    HPDF_UA_EndStructureElement (ctx, h2_figure_elem);
    y -= 25;

    figure_elem = HPDF_UA_BeginStructureElement (ctx, doc_elem, HPDF_UA_ROLE_FIGURE);
    if (!figure_elem)
        goto fail;
    status = HPDF_UA_SetAlternateText (ctx, figure_elem,
            "Bar chart: an example posterior-density histogram across 8 "
            "bins, unimodal, peak density 0.24 at bin 4, near zero at "
            "both range ends.");
    if (status != HPDF_OK)
        goto fail;

    status = HPDF_UA_BeginMarkedContent (ctx, page, figure_elem);
    if (status != HPDF_OK)
        goto fail;
    {
        HPDF_REAL chart_left = 50, chart_baseline = y - 160, chart_top = y;
        HPDF_REAL bar_width = 40, bar_gap = 6;

        HPDF_Page_SetLineWidth (page, 1);
        HPDF_Page_MoveTo (page, chart_left, chart_baseline);
        HPDF_Page_LineTo (page, chart_left, chart_top);
        HPDF_Page_Stroke (page);
        HPDF_Page_MoveTo (page, chart_left, chart_baseline);
        HPDF_Page_LineTo (page, chart_left + NBINS * (bar_width + bar_gap), chart_baseline);
        HPDF_Page_Stroke (page);

        HPDF_Page_SetRGBFill (page, 0.25, 0.45, 0.75);
        for (i = 0; i < NBINS; i++) {
            HPDF_REAL x = chart_left + i * (bar_width + bar_gap) + bar_gap;
            HPDF_REAL h = (HPDF_REAL) (DENSITY[i] / max_density * (chart_top - chart_baseline - 10));

            HPDF_Page_Rectangle (page, x, chart_baseline, bar_width, h);
            HPDF_Page_Fill (page);
        }
    }
    status = HPDF_UA_EndMarkedContent (ctx, page);
    if (status != HPDF_OK)
        goto fail;
    HPDF_UA_EndStructureElement (ctx, figure_elem);
    y -= 180;

    caption_elem = HPDF_UA_BeginStructureElement (ctx, doc_elem, HPDF_UA_ROLE_CAPTION);
    if (!caption_elem)
        goto fail;
    status = HPDF_UA_BeginMarkedContent (ctx, page, caption_elem);
    if (status != HPDF_OK)
        goto fail;
    HPDF_Page_SetFontAndSize (page, font, 10);
    HPDF_Page_BeginText (page);
    HPDF_Page_MoveTextPos (page, 50, y);
    HPDF_Page_ShowText (page, "Figure 1: Example posterior-density histogram.");
    HPDF_Page_EndText (page);
    status = HPDF_UA_EndMarkedContent (ctx, page);
    if (status != HPDF_OK)
        goto fail;
    HPDF_UA_EndStructureElement (ctx, caption_elem);

    HPDF_UA_EndStructureElement (ctx, doc_elem);

    /* --- Outline: one entry per top-level section --- */
    {
        HPDF_Outline outline;
        HPDF_Destination dst = HPDF_Page_CreateDestination (page);
        HPDF_Destination_SetXYZ (dst, 0, HPDF_Page_GetHeight (page), 1);

        outline = HPDF_CreateOutline (pdf, NULL, "Introduction", NULL);
        HPDF_Outline_SetDestination (outline, dst);

        outline = HPDF_CreateOutline (pdf, NULL, "Example Table", NULL);
        HPDF_Outline_SetDestination (outline, dst);

        outline = HPDF_CreateOutline (pdf, NULL, "Example Figure", NULL);
        HPDF_Outline_SetDestination (outline, dst);
    }

    HPDF_SaveToFile (pdf, "tagged_example_demo.pdf");
    HPDF_UA_FreeContext (ctx);
    HPDF_Free (pdf);

    printf ("wrote tagged_example_demo.pdf -- text + table + figure "
            "together in one tagged document. Validate with "
            "validate/run_verapdf.sh.\n");
    return 0;

fail:
    fprintf (stderr, "tagged_example_demo: a tagging call failed, see above\n");
    HPDF_UA_FreeContext (ctx);
    HPDF_Free (pdf);
    return 1;
}
