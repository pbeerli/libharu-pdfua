/*
 * tagged_attach_demo.c -- Milestone 6: tagged port of libharu's original
 * attach.c: an embedded file attachment (HPDF_AttachFile()). Attaches
 * this project's own CHANGES.md rather than the original's
 * pngsuite/basn3p08.png -- avoids vendoring a new binary asset (and its
 * own license entry) just for this port, and CHANGES.md is a real,
 * already-present, meaningfully-sized text file.
 *
 * HPDF_AttachFile() creates a document-level embedded file (the
 * `/Names /EmbeddedFiles` name tree), not a page annotation -- unlike
 * tagged_annotation_demo.c's Link annotations or
 * tagged_text_annotation_demo.c's Text annotations, an embedded file has
 * no page-content presence and no structure-tree association requirement
 * of its own; PDF/UA-1's actual requirements here are all satisfied by
 * this project's existing document-level tagging (a populated structure
 * tree for the visible page content, real /Lang, etc.), so this demo
 * needs no new tagging API.
 *
 * Structure: Document > H1, Document > P (the visible page text
 * mentioning the attachment).
 *
 * Validate with: validate/run_verapdf.sh build/tagged_attach_demo.pdf
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
    HPDF_UA_StructElem doc_elem, h1_elem, p_elem;
    HPDF_STATUS status;

    pdf = HPDF_New (error_handler, NULL);
    if (!pdf) {
        fprintf (stderr, "failed to create PDF document\n");
        return 1;
    }

    HPDF_SetInfoAttr (pdf, HPDF_INFO_TITLE, "libharu-pdfua tagged-attach demo");
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
        HPDF_Outline outline = HPDF_CreateOutline (pdf, NULL, "Attach", NULL);
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
    HPDF_Page_ShowText (page, "Attachment Demo");
    HPDF_Page_EndText (page);
    status = HPDF_UA_EndMarkedContent (ctx, page);
    if (status != HPDF_OK)
        goto fail;
    HPDF_UA_EndStructureElement (ctx, h1_elem);

    p_elem = HPDF_UA_BeginStructureElement (ctx, doc_elem, HPDF_UA_ROLE_P);
    if (!p_elem)
        goto fail;
    status = HPDF_UA_BeginMarkedContent (ctx, page, p_elem);
    if (status != HPDF_OK)
        goto fail;
    HPDF_Page_SetFontAndSize (page, font, 14);
    HPDF_Page_BeginText (page);
    HPDF_Page_MoveTextPos (page, 50, 700);
    HPDF_Page_ShowText (page, "This PDF has an attached file named CHANGES.md.");
    HPDF_Page_EndText (page);
    status = HPDF_UA_EndMarkedContent (ctx, page);
    if (status != HPDF_OK)
        goto fail;
    HPDF_UA_EndStructureElement (ctx, p_elem);

    HPDF_UA_EndStructureElement (ctx, doc_elem);

    if (!HPDF_AttachFile (pdf, HPDF_UA_DEMO_ATTACH_PATH)) {
        fprintf (stderr, "HPDF_AttachFile failed\n");
        goto fail;
    }

    HPDF_SaveToFile (pdf, "tagged_attach_demo.pdf");
    HPDF_UA_FreeContext (ctx);
    HPDF_Free (pdf);

    printf ("wrote tagged_attach_demo.pdf -- a tagged document with a "
            "real embedded file attachment (attach.c port). Validate "
            "with validate/run_verapdf.sh.\n");
    return 0;

fail:
    fprintf (stderr, "tagged_attach_demo: a tagging call failed, see above\n");
    HPDF_UA_FreeContext (ctx);
    HPDF_Free (pdf);
    return 1;
}
