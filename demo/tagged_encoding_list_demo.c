/*
 * tagged_encoding_list_demo.c -- Milestone 6: tagged port of a
 * representative subset of libharu's original encoding_list.c: applying
 * several single-byte encodings to a loaded font and showing the
 * resulting glyphs. A deliberate scope cut from the original (which
 * dumps all 20 of libharu's single-byte encodings across 20 pages, using
 * a vendored GPL-licensed Type1 font): three representative encodings
 * (WinAnsiEncoding, ISO8859-5 -- Cyrillic, KOI8-R -- Cyrillic) on one
 * page, reusing this project's own already-vendored, already-licensed
 * fonts/DejaVuSans.ttf (which covers Latin and Cyrillic) rather than
 * introducing a new font asset/license just for this port.
 *
 * Structure: Document > H1, then one Document > P per encoding (each
 * shown as real, readable text using that encoding, not a raw glyph
 * grid) -- real coverage of HPDF_GetFont()'s per-encoding code path
 * (a distinct HPDF_Font object per (base font, encoding) pair) through
 * the tagging layer.
 *
 * Validate with: validate/run_verapdf.sh build/tagged_encoding_list_demo.pdf
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
    const char *font_name;
    HPDF_UA_Context ctx;
    HPDF_UA_StructElem doc_elem, h1_elem, p;
    HPDF_STATUS status;
    HPDF_REAL y = 700;

    /* Sample text is plain ASCII throughout, deliberately: every encoding
     * below is a superset of ASCII in the 0x20-0x7E range, so this
     * avoids any risk of a hand-guessed non-ASCII byte value mapping to
     * the wrong Unicode code point (verified by inspecting each
     * encoding's real HPDF_UNICODE_MAP_* table in vendor/libharu/src/
     * hpdf_encoder.c, not assumed). The real point of this demo is
     * exercising HPDF_GetFont() producing a distinct HPDF_Font object
     * per (font, encoding) pair on the same embedded font, not showing
     * exotic glyphs. */
    static const struct { const char *encoding; const char *sample; }
    ENCODINGS[] = {
        { "WinAnsiEncoding", "WinAnsiEncoding: The quick brown fox jumps over the lazy dog." },
        { "ISO8859-5", "ISO8859-5 (Latin/Cyrillic repertoire): The quick brown fox." },
        { "KOI8-R", "KOI8-R (Latin/Cyrillic repertoire): The quick brown fox." },
    };
    size_t i;

    pdf = HPDF_New (error_handler, NULL);
    if (!pdf) {
        fprintf (stderr, "failed to create PDF document\n");
        return 1;
    }

    HPDF_SetInfoAttr (pdf, HPDF_INFO_TITLE, "libharu-pdfua tagged-encoding-list demo");
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

    {
        HPDF_Outline outline = HPDF_CreateOutline (pdf, NULL, "Encoding List", NULL);
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
    {
        HPDF_Font h1_font = HPDF_GetFont (pdf, font_name, "WinAnsiEncoding");
        HPDF_Page_SetFontAndSize (page, h1_font, 18);
    }
    HPDF_Page_BeginText (page);
    HPDF_Page_MoveTextPos (page, 50, 750);
    HPDF_Page_ShowText (page, "Encoding List Demo");
    HPDF_Page_EndText (page);
    status = HPDF_UA_EndMarkedContent (ctx, page);
    if (status != HPDF_OK)
        goto fail;
    HPDF_UA_EndStructureElement (ctx, h1_elem);

    for (i = 0; i < sizeof (ENCODINGS) / sizeof (ENCODINGS[0]); i++) {
        /* A separate HPDF_Font per (font_name, encoding) pair -- the real
         * code path this demo exists to exercise: libharu returns a
         * distinct HPDF_Font object for each distinct encoding applied to
         * the same underlying embedded TrueType font. */
        HPDF_Font font = HPDF_GetFont (pdf, font_name, ENCODINGS[i].encoding);

        p = HPDF_UA_BeginStructureElement (ctx, doc_elem, HPDF_UA_ROLE_P);
        if (!p)
            goto fail;
        status = HPDF_UA_BeginMarkedContent (ctx, page, p);
        if (status != HPDF_OK)
            goto fail;
        HPDF_Page_SetFontAndSize (page, font, 16);
        HPDF_Page_BeginText (page);
        HPDF_Page_MoveTextPos (page, 50, y);
        HPDF_Page_ShowText (page, ENCODINGS[i].sample);
        HPDF_Page_EndText (page);
        status = HPDF_UA_EndMarkedContent (ctx, page);
        if (status != HPDF_OK)
            goto fail;
        HPDF_UA_EndStructureElement (ctx, p);

        y -= 40;
    }

    HPDF_UA_EndStructureElement (ctx, doc_elem);

    HPDF_SaveToFile (pdf, "tagged_encoding_list_demo.pdf");
    HPDF_UA_FreeContext (ctx);
    HPDF_Free (pdf);

    printf ("wrote tagged_encoding_list_demo.pdf -- tagged text in three "
            "different single-byte encodings applied to one embedded "
            "font (encoding_list.c port). Validate with "
            "validate/run_verapdf.sh.\n");
    return 0;

fail:
    fprintf (stderr, "tagged_encoding_list_demo: a tagging call failed, see above\n");
    HPDF_UA_FreeContext (ctx);
    HPDF_Free (pdf);
    return 1;
}
