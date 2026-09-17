/*
 * tagged_japanese_font_demo.c -- Milestone 6 (CJK batch): tagged port
 * merging libharu's original ttfont_demo_jp.c (an embedded Japanese
 * TrueType font, shown at a few sizes) and jpfont_demo.c (Japanese text
 * via libharu's own predefined CID fonts). Merged into one demo, same
 * reasoning as tagged_text_demo.c: both originals show the same thing --
 * real Japanese text -- and this port's whole point is to always use a
 * real EMBEDDED font instead of jpfont_demo.c's own approach
 * (HPDF_UseJPFonts()'s predefined "MS-Mincho" etc. CID fonts, which are
 * never embedded, the same PDF/UA-1 gap tagged_font_list_demo.c already
 * demonstrates deliberately) -- so there is no separate "non-embedded"
 * variant left to port here; that gap is already covered elsewhere.
 * Also folds in outline_demo_jp.c's one distinguishing feature (a
 * Japanese-titled outline entry via HPDF_GetEncoder(pdf, "90ms-RKSJ-H")),
 * since this project's own tagged_outline_demo.c already covers the
 * general multi-entry-outline mechanism outline_demo_jp.c would
 * otherwise duplicate.
 *
 * fonts/NotoSansJP-Regular.ttf (Milestone 6, see NOTICE.md) is a real
 * TrueType (glyf-outline) build, deliberately not upstream Noto CJK's
 * own CFF-outline OTF/OTC releases, which libharu's TrueType loader
 * cannot parse (confirmed directly before vendoring -- see NOTICE.md).
 * Applying `HPDF_UseJPEncodings()`'s "90ms-RKSJ-H" (Shift-JIS) encoding
 * to this embedded font (rather than to one of libharu's own predefined,
 * non-embedded CID fonts) uses the same generic
 * embedded-TrueType-plus-legacy-double-byte-encoding code path this
 * project's other CJK demo (tagged_chfont_demo.c) also exercises;
 * confirmed correct end to end before writing this file: real embedding,
 * correct rendering, and correct ToUnicode round-tripping (checked with
 * a standalone reproduction against the built library, and again with
 * veraPDF against this file's own actual output) -- no
 * HPDF_UA_SetActualText() turned out to be needed.
 *
 * Structure: Document > H1 (title) > P (embedded-font specimen) > P
 * (a classic Japanese pangram, "Iroha") > P (a longer sample sentence).
 * One outline entry's title is the same sample sentence, Shift-JIS
 * encoded, per outline_demo_jp.c's own original intent.
 *
 * Validate with: validate/run_verapdf.sh build/tagged_japanese_font_demo.pdf
 */
#include <stdio.h>
#include "hpdf_ua/hpdf_ua.h"

/* Shift-JIS byte sequences for real Japanese text, precomputed (Python
 * `text.encode('shift_jis')`) rather than embedded as literal non-ASCII
 * source bytes, so this file itself stays plain ASCII/portable. */

/* Title: "Japanese Font Demo" */
static const char TITLE_SJIS[] = {
    0x93,0xFA,0x96,0x7B,0x8C,0xEA,0x83,0x74,0x83,0x48,0x83,0x93,0x83,0x67,
    0x83,0x66,0x83,0x82,0x00
};

/* "Iroha" pangram: traditional Japanese pangram (every kana used once),
 * the rough equivalent of "the quick brown fox" for Japanese type
 * specimens. */
static const char PANGRAM_SJIS[] = {
    0x82,0xA2,0x82,0xEB,0x82,0xCD,0x82,0xC9,0x82,0xD9,0x82,0xD6,0x82,0xC6,
    0x82,0xBF,0x82,0xE8,0x82,0xCA,0x82,0xE9,0x82,0xF0,0x00
};

/* "Amenbo akai na aiueo. Ukimo ni ko ebi mo oyoideru." -- the same
 * sample sentence outline_demo_jp.c's own mbtext/sjis.txt asset held
 * (reproduced here as a short, ordinary example sentence, not vendored
 * as a separate text-file asset). */
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
    HPDF_Font font;
    const char *font_name;
    HPDF_UA_Context ctx;
    HPDF_UA_StructElem doc_elem, h1_elem, p_elem;
    HPDF_STATUS status;
    HPDF_REAL y;

    pdf = HPDF_New (error_handler, NULL);
    if (!pdf) {
        fprintf (stderr, "failed to create PDF document\n");
        return 1;
    }

    HPDF_SetInfoAttr (pdf, HPDF_INFO_TITLE, "libharu-pdfua tagged-japanese-font demo");
    HPDF_UA_SetDocumentLanguage (pdf, "ja");
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

    HPDF_UseJPEncodings (pdf);

    page = HPDF_AddPage (pdf);
    HPDF_Page_SetSize (page, HPDF_PAGE_SIZE_LETTER, HPDF_PAGE_PORTRAIT);

    /* The real point of this demo: a real EMBEDDED Japanese TrueType
     * font, used via the same legacy-double-byte-encoding mechanism as
     * libharu's own predefined (never-embedded) CID fonts, but actually
     * meeting PDF/UA-1's embedded-fonts requirement. */
    font_name = HPDF_LoadTTFontFromFile (pdf, HPDF_UA_DEMO_JP_FONT_PATH, HPDF_TRUE);
    if (!font_name) {
        fprintf (stderr, "HPDF_LoadTTFontFromFile failed\n");
        HPDF_UA_FreeContext (ctx);
        HPDF_Free (pdf);
        return 1;
    }
    font = HPDF_GetFont (pdf, font_name, "90ms-RKSJ-H");
    if (!font) {
        fprintf (stderr, "HPDF_GetFont failed\n");
        HPDF_UA_FreeContext (ctx);
        HPDF_Free (pdf);
        return 1;
    }

    /* --- outline_demo_jp.c's own distinguishing feature: an
     * outline-entry title in Japanese, Shift-JIS encoded. --- */
    {
        HPDF_Outline outline = HPDF_CreateOutline (pdf, NULL, SENTENCE_SJIS,
                HPDF_GetEncoder (pdf, "90ms-RKSJ-H"));
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
    HPDF_Page_SetFontAndSize (page, font, 20);
    HPDF_Page_BeginText (page);
    HPDF_Page_MoveTextPos (page, 50, y);
    HPDF_Page_ShowText (page, TITLE_SJIS);
    HPDF_Page_EndText (page);
    status = HPDF_UA_EndMarkedContent (ctx, page);
    if (status != HPDF_OK)
        goto fail;
    HPDF_UA_EndStructureElement (ctx, h1_elem);
    y -= 50;

    /* --- font specimen: font name (Latin, WinAnsi) + Iroha pangram --- */
    p_elem = HPDF_UA_BeginStructureElement (ctx, doc_elem, HPDF_UA_ROLE_P);
    if (!p_elem)
        goto fail;
    status = HPDF_UA_BeginMarkedContent (ctx, page, p_elem);
    if (status != HPDF_OK)
        goto fail;
    HPDF_Page_BeginText (page);
    HPDF_Page_SetFontAndSize (page, font, 10);
    HPDF_Page_MoveTextPos (page, 50, y);
    HPDF_Page_ShowText (page, font_name);
    HPDF_Page_ShowText (page, " (embedded subset)");
    HPDF_Page_MoveTextPos (page, 0, -30);
    HPDF_Page_SetFontAndSize (page, font, 20);
    HPDF_Page_ShowText (page, PANGRAM_SJIS);
    HPDF_Page_EndText (page);
    status = HPDF_UA_EndMarkedContent (ctx, page);
    if (status != HPDF_OK)
        goto fail;
    HPDF_UA_EndStructureElement (ctx, p_elem);
    y -= 80;

    /* --- sample sentence at increasing sizes --- */
    p_elem = HPDF_UA_BeginStructureElement (ctx, doc_elem, HPDF_UA_ROLE_P);
    if (!p_elem)
        goto fail;
    status = HPDF_UA_BeginMarkedContent (ctx, page, p_elem);
    if (status != HPDF_OK)
        goto fail;
    HPDF_Page_BeginText (page);
    HPDF_Page_SetFontAndSize (page, font, 12);
    HPDF_Page_MoveTextPos (page, 50, y);
    HPDF_Page_ShowText (page, SENTENCE_SJIS);
    HPDF_Page_MoveTextPos (page, 0, -30);
    HPDF_Page_SetFontAndSize (page, font, 18);
    HPDF_Page_ShowText (page, SENTENCE_SJIS);
    HPDF_Page_EndText (page);
    status = HPDF_UA_EndMarkedContent (ctx, page);
    if (status != HPDF_OK)
        goto fail;
    HPDF_UA_EndStructureElement (ctx, p_elem);

    HPDF_UA_EndStructureElement (ctx, doc_elem);

    HPDF_SaveToFile (pdf, "tagged_japanese_font_demo.pdf");
    HPDF_UA_FreeContext (ctx);
    HPDF_Free (pdf);

    printf ("wrote tagged_japanese_font_demo.pdf -- real, embedded "
            "Japanese TrueType font text, fully tagged (ttfont_demo_jp.c "
            "+ jpfont_demo.c + outline_demo_jp.c port). Validate with "
            "validate/run_verapdf.sh.\n");
    return 0;

fail:
    fprintf (stderr, "tagged_japanese_font_demo: a tagging call failed, see above\n");
    HPDF_UA_FreeContext (ctx);
    HPDF_Free (pdf);
    return 1;
}
