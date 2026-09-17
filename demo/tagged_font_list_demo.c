/*
 * tagged_font_list_demo.c -- Milestone 6: tagged port of libharu's
 * original font_demo.c (a sample-text listing of all fourteen Standard-14
 * Type1 fonts).
 *
 * Unlike this project's other demos, this one is deliberately NOT fully
 * PDF/UA-1 compliant, for the same reason docmeta_demo.pdf isn't: the
 * Standard-14 fonts (Helvetica, Times-Roman, Symbol, ...) are, by PDF
 * convention, never embedded, and PDF/UA-1 requires every rendering font
 * to be embedded (ISO 14289-1:2014 7.21.4.1/1). That is the entire point
 * of this demo -- showing libharu's non-embeddable Standard-14 fonts,
 * the exact opposite of tagged_font_demo.c's embedded-TrueType port --
 * so "fix" here would mean "stop testing what this demo tests." See
 * tests/run_all_demos.sh's own DEMOS table for this demo's recorded
 * baseline (matching docmeta_demo's category, not the fully-compliant
 * majority).
 *
 * Structure: Document > H1, then Document > L (list), one LI per font,
 * each containing its own tagged label (P) and sample-text (P) --
 * genuinely real, separately-tagged text (each cell text-showing
 * operator sequence gets its own marked-content span), the actual point
 * of porting this demo being real coverage of libharu's Standard-14 font
 * metrics/text-measurement code path through the tagging layer, not the
 * (deliberately unmet) embedding requirement.
 *
 * Validate with: validate/run_verapdf.sh build/tagged_font_list_demo.pdf
 */
#include <stdio.h>
#include "hpdf_ua/hpdf_ua.h"

static const char *FONT_LIST[] = {
    "Courier", "Courier-Bold", "Courier-Oblique", "Courier-BoldOblique",
    "Helvetica", "Helvetica-Bold", "Helvetica-Oblique", "Helvetica-BoldOblique",
    "Times-Roman", "Times-Bold", "Times-Italic", "Times-BoldItalic",
    "Symbol", "ZapfDingbats",
    NULL
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
    HPDF_Font label_font;
    HPDF_UA_Context ctx;
    HPDF_UA_StructElem doc_elem, h1_elem, list_elem;
    HPDF_STATUS status;
    HPDF_REAL y = 700;
    int i;

    pdf = HPDF_New (error_handler, NULL);
    if (!pdf) {
        fprintf (stderr, "failed to create PDF document\n");
        return 1;
    }

    HPDF_SetInfoAttr (pdf, HPDF_INFO_TITLE, "libharu-pdfua tagged-font-list demo");
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

    /* label_font is Standard-14 Helvetica too -- deliberately, this whole
     * demo's point is Standard-14, non-embedded text. */
    label_font = HPDF_GetFont (pdf, "Helvetica", NULL);

    {
        HPDF_Outline outline = HPDF_CreateOutline (pdf, NULL, "Font List", NULL);
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
    HPDF_Page_SetFontAndSize (page, label_font, 18);
    HPDF_Page_BeginText (page);
    HPDF_Page_MoveTextPos (page, 50, 750);
    HPDF_Page_ShowText (page, "Standard-14 Font List");
    HPDF_Page_EndText (page);
    status = HPDF_UA_EndMarkedContent (ctx, page);
    if (status != HPDF_OK)
        goto fail;
    HPDF_UA_EndStructureElement (ctx, h1_elem);

    list_elem = HPDF_UA_BeginStructureElement (ctx, doc_elem, HPDF_UA_ROLE_L);
    if (!list_elem)
        goto fail;

    for (i = 0; FONT_LIST[i]; i++) {
        HPDF_UA_StructElem li_elem, lbl_elem, sample_elem;
        HPDF_Font sample_font = HPDF_GetFont (pdf, FONT_LIST[i], NULL);

        li_elem = HPDF_UA_BeginStructureElement (ctx, list_elem, HPDF_UA_ROLE_LI);
        if (!li_elem)
            goto fail;

        lbl_elem = HPDF_UA_BeginStructureElement (ctx, li_elem, HPDF_UA_ROLE_LBL);
        if (!lbl_elem)
            goto fail;
        status = HPDF_UA_BeginMarkedContent (ctx, page, lbl_elem);
        if (status != HPDF_OK)
            goto fail;
        HPDF_Page_SetFontAndSize (page, label_font, 9);
        HPDF_Page_BeginText (page);
        HPDF_Page_MoveTextPos (page, 60, y);
        HPDF_Page_ShowText (page, FONT_LIST[i]);
        HPDF_Page_EndText (page);
        status = HPDF_UA_EndMarkedContent (ctx, page);
        if (status != HPDF_OK)
            goto fail;
        HPDF_UA_EndStructureElement (ctx, lbl_elem);

        sample_elem = HPDF_UA_BeginStructureElement (ctx, li_elem, HPDF_UA_ROLE_LBODY);
        if (!sample_elem)
            goto fail;
        status = HPDF_UA_BeginMarkedContent (ctx, page, sample_elem);
        if (status != HPDF_OK)
            goto fail;
        HPDF_Page_SetFontAndSize (page, sample_font, 16);
        HPDF_Page_BeginText (page);
        HPDF_Page_MoveTextPos (page, 220, y);
        HPDF_Page_ShowText (page, "abcdefgABCDEFG12345");
        HPDF_Page_EndText (page);
        status = HPDF_UA_EndMarkedContent (ctx, page);
        if (status != HPDF_OK)
            goto fail;
        HPDF_UA_EndStructureElement (ctx, sample_elem);

        HPDF_UA_EndStructureElement (ctx, li_elem);

        y -= 40;
    }

    HPDF_UA_EndStructureElement (ctx, list_elem);
    HPDF_UA_EndStructureElement (ctx, doc_elem);

    HPDF_SaveToFile (pdf, "tagged_font_list_demo.pdf");
    HPDF_UA_FreeContext (ctx);
    HPDF_Free (pdf);

    printf ("wrote tagged_font_list_demo.pdf -- a tagged list of all "
            "fourteen Standard-14 fonts (font_demo.c port). Deliberately "
            "NOT fully PDF/UA-1 compliant (Standard-14 fonts are never "
            "embedded) -- see this file's own top comment. Validate with "
            "validate/run_verapdf.sh.\n");
    return 0;

fail:
    fprintf (stderr, "tagged_font_list_demo: a tagging call failed, see above\n");
    HPDF_UA_FreeContext (ctx);
    HPDF_Free (pdf);
    return 1;
}
