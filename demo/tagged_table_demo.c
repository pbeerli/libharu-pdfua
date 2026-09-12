/*
 * tagged_table_demo.c -- Milestone 1: a real, fully tagged table page.
 *
 * Structure tree: Document > Table > (TR > TH x3, TR > TD x3 x4).
 * Each cell's text is wrapped in its own BDC..EMC marked-content span
 * tied to its own TH/TD structure element. Validate with:
 *   validate/run_verapdf.sh build/tagged_table_demo.pdf
 */
#include <stdio.h>
#include "hpdf_ua/hpdf_ua.h"

#define ROWS 4
#define COLS 3

static const char *HEADERS[COLS] = { "Locus", "Theta", "M" };
static const char *ROW_DATA[ROWS][COLS] = {
    { "1", "0.0123", "1.45" },
    { "2", "0.0098", "1.61" },
    { "3", "0.0151", "1.22" },
    { "All", "0.0119", "1.43" },
};

static void
error_handler (HPDF_STATUS error_no, HPDF_STATUS detail_no, void *user_data)
{
    (void) user_data;
    fprintf (stderr, "libharu error: error_no=0x%04lX detail_no=%ld\n",
            (unsigned long) error_no, (long) detail_no);
}

/* Draws one tagged cell: opens marked content under `cell_elem`, draws the
 * text, closes marked content. Returns HPDF_OK or the first error hit. */
static HPDF_STATUS
draw_tagged_cell (HPDF_UA_Context ctx, HPDF_Page page,
                   HPDF_UA_StructElem cell_elem, HPDF_REAL x, HPDF_REAL y,
                   const char *text)
{
    HPDF_STATUS status;

    status = HPDF_UA_BeginMarkedContent (ctx, page, cell_elem);
    if (status != HPDF_OK)
        return status;

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
    HPDF_UA_Context ctx;
    HPDF_UA_StructElem doc_elem, table_elem, tr_elem, cell_elem;
    int row, col;
    HPDF_REAL x, y;
    const HPDF_REAL col_width = 100;
    const HPDF_REAL row_height = 20;
    const HPDF_REAL left = 50, top = 700;
    HPDF_STATUS status;

    pdf = HPDF_New (error_handler, NULL);
    if (!pdf) {
        fprintf (stderr, "failed to create PDF document\n");
        return 1;
    }

    HPDF_SetInfoAttr (pdf, HPDF_INFO_TITLE, "libharu-pdfua tagged-table demo");
    HPDF_UA_SetDocumentLanguage (pdf, "en-US");
    HPDF_UA_SetDisplayDocTitle (pdf, HPDF_TRUE);

    ctx = HPDF_UA_NewContext (pdf);
    if (!ctx) {
        fprintf (stderr, "HPDF_UA_NewContext failed\n");
        HPDF_Free (pdf);
        return 1;
    }

    page = HPDF_AddPage (pdf);
    HPDF_Page_SetSize (page, HPDF_PAGE_SIZE_LETTER, HPDF_PAGE_PORTRAIT);
    font = HPDF_GetFont (pdf, "Helvetica", NULL);
    HPDF_Page_SetFontAndSize (page, font, 10);

    doc_elem = HPDF_UA_BeginStructureElement (ctx, NULL, HPDF_UA_ROLE_DOCUMENT);
    table_elem = HPDF_UA_BeginStructureElement (ctx, doc_elem, HPDF_UA_ROLE_TABLE);
    if (!doc_elem || !table_elem) {
        fprintf (stderr, "HPDF_UA_BeginStructureElement failed\n");
        goto fail;
    }

    for (row = 0; row < ROWS + 1; row++) {
        y = top - row * row_height;

        tr_elem = HPDF_UA_BeginStructureElement (ctx, table_elem, HPDF_UA_ROLE_TR);
        if (!tr_elem)
            goto fail;

        for (col = 0; col < COLS; col++) {
            x = left + col * col_width;

            cell_elem = HPDF_UA_BeginStructureElement (ctx, tr_elem,
                    row == 0 ? HPDF_UA_ROLE_TH : HPDF_UA_ROLE_TD);
            if (!cell_elem)
                goto fail;

            if (row == 0) {
                status = HPDF_UA_SetTableHeaderScope (ctx, cell_elem,
                        HPDF_UA_SCOPE_COLUMN);
                if (status != HPDF_OK)
                    goto fail;
            }

            status = draw_tagged_cell (ctx, page, cell_elem, x, y,
                    row == 0 ? HEADERS[col] : ROW_DATA[row - 1][col]);
            if (status != HPDF_OK)
                goto fail;

            HPDF_UA_EndStructureElement (ctx, cell_elem);
        }

        HPDF_UA_EndStructureElement (ctx, tr_elem);
    }

    /* Decorative border only -- no information of its own, so it is
     * marked as an Artifact rather than left untagged (veraPDF's real
     * PDF/UA-1 ruleset flags untagged content as a defect, ISO
     * 14289-1:2014 7.1/3 -- caught on this exact rectangle the first
     * time this demo was actually run through validate/run_verapdf.sh). */
    status = HPDF_UA_BeginArtifact (ctx, page);
    if (status != HPDF_OK)
        goto fail;

    HPDF_Page_Rectangle (page, left - 5, top - (ROWS + 1) * row_height + 5,
            COLS * col_width + 10, (ROWS + 1) * row_height + 5);
    HPDF_Page_Stroke (page);

    status = HPDF_UA_EndArtifact (ctx, page);
    if (status != HPDF_OK)
        goto fail;

    HPDF_UA_EndStructureElement (ctx, table_elem);
    HPDF_UA_EndStructureElement (ctx, doc_elem);

    HPDF_SaveToFile (pdf, "tagged_table_demo.pdf");
    HPDF_UA_FreeContext (ctx);
    HPDF_Free (pdf);

    printf ("wrote tagged_table_demo.pdf -- a real tagged table "
            "(Document > Table > TR > TH/TD, each cell its own MCID). "
            "Validate with validate/run_verapdf.sh.\n");
    return 0;

fail:
    fprintf (stderr, "tagged_table_demo: a tagging call failed, see above\n");
    HPDF_UA_FreeContext (ctx);
    HPDF_Free (pdf);
    return 1;
}
