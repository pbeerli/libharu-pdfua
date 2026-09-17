/*
 * tagged_chfont_demo.c -- Milestone 6 (CJK batch): tagged port of
 * libharu's original chfont_demo.c: two different embedded CJK
 * TrueType fonts, each under its own legacy double-byte encoding, in
 * one document -- the original took both font files as command-line
 * arguments (`chfont_demo <gbk-font> <size> <sjis-font> <size>`); this
 * port uses this project's own vendored fonts/NotoSansSC-Regular.ttf
 * (Simplified Chinese) and fonts/NotoSansJP-Regular.ttf (Japanese, see
 * NOTICE.md) instead.
 *
 * Real, non-obvious detail carried over faithfully from the original:
 * `HPDF_UseCNSEncodings()`/`vendor/libharu/src/hpdf_encoder_cns.c`
 * registers a "GBK-EUC-H" encoder with Adobe-GB1 (Simplified Chinese)
 * ordering despite the "CNS" name (CNS normally denotes Traditional
 * Chinese) -- confirmed by reading that file directly, not assumed;
 * this is libharu's own naming, not a bug introduced by this port, and
 * matches the original chfont_demo.c's own usage exactly.
 *
 * Structure: Document > H1, then one Document > P per language (font
 * name + a short sample sentence), demonstrating two independent
 * embedded-TrueType-plus-legacy-CJK-encoding fonts coexisting in the
 * same tagging context -- confirmed end to end (embedding, rendering,
 * ToUnicode round-trip) before writing this file, same as
 * tagged_japanese_font_demo.c.
 *
 * Validate with: validate/run_verapdf.sh build/tagged_chfont_demo.pdf
 */
#include <stdio.h>
#include "hpdf_ua/hpdf_ua.h"

/* GBK byte sequences for Simplified Chinese text, precomputed (Python
 * `text.encode('gbk')`). */

/* "Chinese Font Demo" */
static const char TITLE_GBK[] = {
    0xD6,0xD0,0xCE,0xC4,0xD7,0xD6,0xCC,0xE5,0xD1,0xDD,0xCA,0xBE,0x00
};

/* "Simplified Chinese sample text, hello world." */
static const char SENTENCE_GBK[] = {
    0xBC,0xF2,0xCC,0xE5,0xD6,0xD0,0xCE,0xC4,0xCA,0xBE,0xC0,0xFD,0xCE,0xC4,
    0xD7,0xD6,0xA3,0xAC,0xC4,0xE3,0xBA,0xC3,0xCA,0xC0,0xBD,0xE7,0xA1,0xA3,
    0x00
};

/* Shift-JIS byte sequence for Japanese text (Python
 * `text.encode('shift_jis')`). */

/* "Amenbo akai na aiueo. Ukimo ni ko ebi mo oyoideru." -- same sentence
 * as tagged_japanese_font_demo.c, reused here as this demo's Japanese
 * sample. */
static const char SENTENCE_SJIS[] = {
    0x82,0xA0,0x82,0xDF,0x82,0xF1,0x82,0xDA,0x90,0xD4,0x82,0xA2,0x82,0xC8,
    0x82,0xA0,0x82,0xA2,0x82,0xA4,0x82,0xA6,0x82,0xA8,0x81,0x42,0x95,0x82,
    0x82,0xAB,0x91,0x94,0x82,0xC9,0x8F,0xAC,0x83,0x47,0x83,0x72,0x82,0xE0,
    0x82,0xA8,0x82,0xE6,0x82,0xA2,0x82,0xC5,0x82,0xE9,0x81,0x42,0x00
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
    HPDF_Font title_font, zh_font, ja_font;
    const char *zh_font_name, *ja_font_name;
    HPDF_UA_Context ctx;
    HPDF_UA_StructElem doc_elem, h1_elem, p_elem;
    HPDF_STATUS status;
    HPDF_REAL y;

    pdf = HPDF_New (error_handler, NULL);
    if (!pdf) {
        fprintf (stderr, "failed to create PDF document\n");
        return 1;
    }

    HPDF_SetInfoAttr (pdf, HPDF_INFO_TITLE, "libharu-pdfua tagged-chfont demo");
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

    /* See this file's own top comment: "CNS" is libharu's own name for
     * this encoder, but it registers Simplified Chinese (GB1) mappings. */
    HPDF_UseCNSEncodings (pdf);
    HPDF_UseJPEncodings (pdf);

    page = HPDF_AddPage (pdf);
    HPDF_Page_SetSize (page, HPDF_PAGE_SIZE_LETTER, HPDF_PAGE_PORTRAIT);

    zh_font_name = HPDF_LoadTTFontFromFile (pdf, HPDF_UA_DEMO_SC_FONT_PATH, HPDF_TRUE);
    ja_font_name = HPDF_LoadTTFontFromFile (pdf, HPDF_UA_DEMO_JP_FONT_PATH, HPDF_TRUE);
    if (!zh_font_name || !ja_font_name) {
        fprintf (stderr, "HPDF_LoadTTFontFromFile failed\n");
        HPDF_UA_FreeContext (ctx);
        HPDF_Free (pdf);
        return 1;
    }
    zh_font = HPDF_GetFont (pdf, zh_font_name, "GBK-EUC-H");
    ja_font = HPDF_GetFont (pdf, ja_font_name, "90ms-RKSJ-H");
    title_font = HPDF_GetFont (pdf, zh_font_name, "WinAnsiEncoding");
    if (!zh_font || !ja_font || !title_font) {
        fprintf (stderr, "HPDF_GetFont failed\n");
        HPDF_UA_FreeContext (ctx);
        HPDF_Free (pdf);
        return 1;
    }

    {
        HPDF_Outline outline = HPDF_CreateOutline (pdf, NULL, "CJK Fonts", NULL);
        HPDF_Destination dst = HPDF_Page_CreateDestination (page);

        HPDF_Destination_SetXYZ (dst, 0, HPDF_Page_GetHeight (page), 1);
        HPDF_Outline_SetDestination (outline, dst);
    }

    doc_elem = HPDF_UA_BeginStructureElement (ctx, NULL, HPDF_UA_ROLE_DOCUMENT);
    if (!doc_elem)
        goto fail;

    y = 730;

    h1_elem = HPDF_UA_BeginStructureElement (ctx, doc_elem, HPDF_UA_ROLE_H1);
    if (!h1_elem)
        goto fail;
    status = HPDF_UA_BeginMarkedContent (ctx, page, h1_elem);
    if (status != HPDF_OK)
        goto fail;
    HPDF_Page_SetFontAndSize (page, title_font, 20);
    HPDF_Page_BeginText (page);
    HPDF_Page_MoveTextPos (page, 50, y);
    HPDF_Page_ShowText (page, "CJK Font Demo");
    HPDF_Page_EndText (page);
    status = HPDF_UA_EndMarkedContent (ctx, page);
    if (status != HPDF_OK)
        goto fail;
    HPDF_UA_EndStructureElement (ctx, h1_elem);
    y -= 60;

    /* --- Simplified Chinese (embedded, GBK-EUC-H) --- */
    p_elem = HPDF_UA_BeginStructureElement (ctx, doc_elem, HPDF_UA_ROLE_P);
    if (!p_elem)
        goto fail;
    status = HPDF_UA_BeginMarkedContent (ctx, page, p_elem);
    if (status != HPDF_OK)
        goto fail;
    HPDF_Page_BeginText (page);
    HPDF_Page_SetFontAndSize (page, title_font, 10);
    HPDF_Page_MoveTextPos (page, 50, y);
    HPDF_Page_ShowText (page, zh_font_name);
    HPDF_Page_ShowText (page, " (embedded, GBK-EUC-H)");
    HPDF_Page_MoveTextPos (page, 0, -30);
    HPDF_Page_SetFontAndSize (page, zh_font, 22);
    HPDF_Page_ShowText (page, TITLE_GBK);
    HPDF_Page_MoveTextPos (page, 0, -34);
    HPDF_Page_SetFontAndSize (page, zh_font, 16);
    HPDF_Page_ShowText (page, SENTENCE_GBK);
    HPDF_Page_EndText (page);
    status = HPDF_UA_EndMarkedContent (ctx, page);
    if (status != HPDF_OK)
        goto fail;
    HPDF_UA_EndStructureElement (ctx, p_elem);
    y -= 110;

    /* --- Japanese (embedded, 90ms-RKSJ-H) --- */
    p_elem = HPDF_UA_BeginStructureElement (ctx, doc_elem, HPDF_UA_ROLE_P);
    if (!p_elem)
        goto fail;
    status = HPDF_UA_BeginMarkedContent (ctx, page, p_elem);
    if (status != HPDF_OK)
        goto fail;
    HPDF_Page_BeginText (page);
    HPDF_Page_SetFontAndSize (page, title_font, 10);
    HPDF_Page_MoveTextPos (page, 50, y);
    HPDF_Page_ShowText (page, ja_font_name);
    HPDF_Page_ShowText (page, " (embedded, 90ms-RKSJ-H)");
    HPDF_Page_MoveTextPos (page, 0, -30);
    HPDF_Page_SetFontAndSize (page, ja_font, 16);
    HPDF_Page_ShowText (page, SENTENCE_SJIS);
    HPDF_Page_EndText (page);
    status = HPDF_UA_EndMarkedContent (ctx, page);
    if (status != HPDF_OK)
        goto fail;
    HPDF_UA_EndStructureElement (ctx, p_elem);

    HPDF_UA_EndStructureElement (ctx, doc_elem);

    HPDF_SaveToFile (pdf, "tagged_chfont_demo.pdf");
    HPDF_UA_FreeContext (ctx);
    HPDF_Free (pdf);

    printf ("wrote tagged_chfont_demo.pdf -- two real, independently "
            "embedded CJK TrueType fonts (Simplified Chinese and "
            "Japanese) in one tagged document (chfont_demo.c port). "
            "Validate with validate/run_verapdf.sh.\n");
    return 0;

fail:
    fprintf (stderr, "tagged_chfont_demo: a tagging call failed, see above\n");
    HPDF_UA_FreeContext (ctx);
    HPDF_Free (pdf);
    return 1;
}
