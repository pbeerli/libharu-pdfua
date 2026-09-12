/*
 * tagged_table_demo.c -- Milestone 1 target demo (see docs/roadmap.md).
 *
 * Right now this only proves the build/link story end to end and exercises
 * the stub API surface so its exact call shape is validated against real
 * compilation, not just read as a header. It draws an ordinary, UNTAGGED
 * table with plain libharu calls (there is nothing else to draw it with
 * yet), then calls the Milestone 1 stub functions and reports that they
 * are not yet implemented, rather than pretending the table it just drew
 * is actually tagged.
 *
 * Once Milestone 1 lands, this file is where the real tagged version
 * should replace the plain-drawing fallback below -- one table page, real
 * /Table -> /TR -> /TH/TD structure elements, real BDC/EMC-wrapped content,
 * validated with `validate/run_verapdf.sh tagged_table_demo.pdf`.
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

int
main (void)
{
    HPDF_Doc pdf;
    HPDF_Page page;
    HPDF_Font font;
    HPDF_UA_StructElem doc_elem;
    HPDF_UA_StructElem table_elem;
    int row, col;
    HPDF_REAL x, y;
    const HPDF_REAL col_width = 100;
    const HPDF_REAL row_height = 20;
    const HPDF_REAL left = 50, top = 700;

    pdf = HPDF_New (error_handler, NULL);
    if (!pdf) {
        fprintf (stderr, "failed to create PDF document\n");
        return 1;
    }

    HPDF_SetInfoAttr (pdf, HPDF_INFO_TITLE, "libharu-pdfua tagged-table demo");
    HPDF_UA_SetDocumentLanguage (pdf, "en-US");
    HPDF_UA_SetDisplayDocTitle (pdf, HPDF_TRUE);
    HPDF_UA_EnableTagging (pdf);

    page = HPDF_AddPage (pdf);
    HPDF_Page_SetSize (page, HPDF_PAGE_SIZE_LETTER, HPDF_PAGE_PORTRAIT);
    font = HPDF_GetFont (pdf, "Helvetica", NULL);
    HPDF_Page_SetFontAndSize (page, font, 10);

    /* Milestone 1 stub calls -- exercised here so the API shape compiles
     * and links against a real page/document, not implemented yet. */
    doc_elem = HPDF_UA_BeginStructureElement (pdf, NULL, HPDF_UA_ROLE_DOCUMENT);
    table_elem = HPDF_UA_BeginStructureElement (pdf, doc_elem, HPDF_UA_ROLE_TABLE);
    if (!table_elem) {
        fprintf (stderr,
                "HPDF_UA_BeginStructureElement: not yet implemented "
                "(expected until Milestone 1 lands) -- drawing an "
                "UNTAGGED placeholder table instead.\n");
    }

    /* Plain, untagged drawing -- the actual content, regardless of
     * whether the structure-tree calls above did anything real yet. */
    for (row = 0; row < ROWS + 1; row++) {
        y = top - row * row_height;
        HPDF_Page_BeginText (page);
        for (col = 0; col < COLS; col++) {
            x = left + col * col_width;
            HPDF_Page_MoveTextPos (page, x, y);
            HPDF_Page_ShowText (page,
                    row == 0 ? HEADERS[col] : ROW_DATA[row - 1][col]);
            HPDF_Page_MoveTextPos (page, -x, -y); /* reset for next cell */
        }
        HPDF_Page_EndText (page);
    }

    HPDF_Page_Rectangle (page, left - 5, top - (ROWS + 1) * row_height + 5,
            COLS * col_width + 10, (ROWS + 1) * row_height + 5);
    HPDF_Page_Stroke (page);

    HPDF_UA_EndStructureElement (pdf, table_elem);
    HPDF_UA_EndStructureElement (pdf, doc_elem);

    HPDF_SaveToFile (pdf, "tagged_table_demo.pdf");
    HPDF_Free (pdf);

    printf ("wrote tagged_table_demo.pdf (content is NOT yet tagged -- "
            "see docs/roadmap.md Milestone 1)\n");
    return 0;
}
