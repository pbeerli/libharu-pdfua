/*
 * docmeta_demo.c -- exercises this project's Milestone 0 REAL functions:
 * HPDF_UA_SetDocumentLanguage, HPDF_UA_SetDisplayDocTitle,
 * HPDF_UA_EnableTagging. Produces a real, valid PDF (docmeta_demo.pdf) --
 * still not a fully tagged document (that's Milestone 1+), but every
 * document-level metadata piece this milestone claims is real should be
 * checkable directly in the output, e.g. with `qpdf --qdf --object-streams=disable
 * docmeta_demo.pdf -` or any PDF inspector, looking for /Lang, /MarkInfo,
 * /StructTreeRoot, and /ViewerPreferences /DisplayDocTitle in the catalog.
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
    HPDF_STATUS status;

    pdf = HPDF_New (error_handler, NULL);
    if (!pdf) {
        fprintf (stderr, "failed to create PDF document\n");
        return 1;
    }

    HPDF_SetInfoAttr (pdf, HPDF_INFO_TITLE,
            "libharu-pdfua Milestone 0 demo");

    status = HPDF_UA_SetDocumentLanguage (pdf, "en-US");
    if (status != HPDF_OK)
        fprintf (stderr, "HPDF_UA_SetDocumentLanguage failed: 0x%04lX\n",
                (unsigned long) status);

    status = HPDF_UA_SetDisplayDocTitle (pdf, HPDF_TRUE);
    if (status != HPDF_OK)
        fprintf (stderr, "HPDF_UA_SetDisplayDocTitle failed: 0x%04lX\n",
                (unsigned long) status);

    status = HPDF_UA_EnableTagging (pdf);
    if (status != HPDF_OK)
        fprintf (stderr, "HPDF_UA_EnableTagging failed: 0x%04lX\n",
                (unsigned long) status);

    page = HPDF_AddPage (pdf);
    HPDF_Page_SetSize (page, HPDF_PAGE_SIZE_LETTER, HPDF_PAGE_PORTRAIT);

    font = HPDF_GetFont (pdf, "Helvetica", NULL);
    HPDF_Page_BeginText (page);
    HPDF_Page_SetFontAndSize (page, font, 14);
    HPDF_Page_MoveTextPos (page, 50, 700);
    HPDF_Page_ShowText (page,
            "libharu-pdfua Milestone 0: /Lang, /DisplayDocTitle, ");
    HPDF_Page_MoveTextPos (page, 0, -18);
    HPDF_Page_ShowText (page,
            "/MarkInfo, and an (still empty) /StructTreeRoot are all real.");
    HPDF_Page_MoveTextPos (page, 0, -18);
    HPDF_Page_ShowText (page,
            "This page's own content is NOT yet tagged -- that is Milestone 1.");
    HPDF_Page_EndText (page);

    HPDF_SaveToFile (pdf, "docmeta_demo.pdf");
    HPDF_Free (pdf);

    printf ("wrote docmeta_demo.pdf\n");
    return 0;
}
