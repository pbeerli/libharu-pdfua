/*
 * tagged_character_map_demo.c -- Milestone 6 (CJK batch): tagged port
 * of a representative slice of libharu's original character_map.c: a
 * glyph-table grid for one Shift-JIS lead byte, drawn against a real
 * font. A deliberate scope cut from the original (which takes an
 * encoding name and a font name as command-line arguments, then scans
 * every possible lead byte and emits one page per valid one -- dozens
 * of pages for a real CJK encoding): this port fixes both the encoding
 * ("90ms-RKSJ-H") and the one lead byte shown (0x82, chosen because it
 * covers the full Hiragana syllabary plus fullwidth digits/Latin --
 * confirmed directly by decoding every byte in that row before writing
 * this file, not assumed), reusing this project's own vendored
 * fonts/NotoSansJP-Regular.ttf (see NOTICE.md) instead of an
 * externally-supplied font file.
 *
 * Real code exercised, same as the original: `HPDF_Encoder_GetByteType()`
 * (to find the row's actual lead-byte range) and
 * `HPDF_Page_TextWidth() > 0` (to skip byte pairs the encoding doesn't
 * actually map to a glyph) -- a genuinely different code path from this
 * project's other CJK demos, which only ever draw already-known-valid
 * text.
 *
 * Unlike this project's other CJK demos, this one's content is a
 * reference/lookup grid, not prose -- the same "real content, but not
 * textual in the ordinary sense" situation tagged_png_demo.c's images
 * are in -- so it is tagged as a single Document > Figure with real
 * /Alt text describing what the grid actually shows, not as individual
 * P elements per glyph.
 *
 * Validate with: validate/run_verapdf.sh build/tagged_character_map_demo.pdf
 */
#include <stdio.h>
#include "hpdf_ua/hpdf_ua.h"

#define LEAD_BYTE 0x82
#define CELL_WIDTH 24
#define CELL_HEIGHT 24
#define GRID_LEFT 60
#define GRID_TOP 700
#define COLS 16
#define ROWS 12

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
    HPDF_Font label_font, glyph_font;
    const char *font_name;
    HPDF_Encoder encoder;
    HPDF_UA_Context ctx;
    HPDF_UA_StructElem doc_elem, h1_elem, figure_elem, caption_elem;
    HPDF_STATUS status;
    int row, col;

    pdf = HPDF_New (error_handler, NULL);
    if (!pdf) {
        fprintf (stderr, "failed to create PDF document\n");
        return 1;
    }

    HPDF_SetInfoAttr (pdf, HPDF_INFO_TITLE, "libharu-pdfua tagged-character-map demo");
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

    HPDF_UseJPEncodings (pdf);
    encoder = HPDF_GetEncoder (pdf, "90ms-RKSJ-H");
    if (!encoder || HPDF_Encoder_GetType (encoder) != HPDF_ENCODER_TYPE_DOUBLE_BYTE) {
        fprintf (stderr, "90ms-RKSJ-H is not a double-byte encoder\n");
        HPDF_UA_FreeContext (ctx);
        HPDF_Free (pdf);
        return 1;
    }

    page = HPDF_AddPage (pdf);
    HPDF_Page_SetSize (page, HPDF_PAGE_SIZE_LETTER, HPDF_PAGE_PORTRAIT);

    font_name = HPDF_LoadTTFontFromFile (pdf, HPDF_UA_DEMO_JP_FONT_PATH, HPDF_TRUE);
    if (!font_name) {
        fprintf (stderr, "HPDF_LoadTTFontFromFile failed\n");
        HPDF_UA_FreeContext (ctx);
        HPDF_Free (pdf);
        return 1;
    }
    label_font = HPDF_GetFont (pdf, font_name, "WinAnsiEncoding");
    glyph_font = HPDF_GetFont (pdf, font_name, "90ms-RKSJ-H");
    if (!label_font || !glyph_font) {
        fprintf (stderr, "HPDF_GetFont failed\n");
        HPDF_UA_FreeContext (ctx);
        HPDF_Free (pdf);
        return 1;
    }

    {
        HPDF_Outline outline = HPDF_CreateOutline (pdf, NULL, "Character Map", NULL);
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
    HPDF_Page_ShowText (page, "Character Map Demo");
    HPDF_Page_EndText (page);
    status = HPDF_UA_EndMarkedContent (ctx, page);
    if (status != HPDF_OK)
        goto fail;
    HPDF_UA_EndStructureElement (ctx, h1_elem);

    figure_elem = HPDF_UA_BeginStructureElement (ctx, doc_elem, HPDF_UA_ROLE_FIGURE);
    if (!figure_elem)
        goto fail;
    status = HPDF_UA_SetAlternateText (ctx, figure_elem,
            "Glyph-table grid for Shift-JIS lead byte 0x82 under the "
            "90ms-RKSJ-H encoding: fullwidth digits 0-9, fullwidth Latin "
            "letters A-Z and a-z, and the complete Hiragana syllabary "
            "(a i u e o through wa wi we wo n), each cell showing one "
            "two-byte character rendered in the embedded Noto Sans JP "
            "font.");
    if (status != HPDF_OK)
        goto fail;

    status = HPDF_UA_BeginMarkedContent (ctx, page, figure_elem);
    if (status != HPDF_OK)
        goto fail;

    /* grid lines */
    HPDF_Page_SetLineWidth (page, 0.5);
    for (row = 0; row <= ROWS; row++) {
        HPDF_REAL y = GRID_TOP - row * CELL_HEIGHT;
        HPDF_Page_MoveTo (page, GRID_LEFT, y);
        HPDF_Page_LineTo (page, GRID_LEFT + COLS * CELL_WIDTH, y);
        HPDF_Page_Stroke (page);
    }
    for (col = 0; col <= COLS; col++) {
        HPDF_REAL x = GRID_LEFT + col * CELL_WIDTH;
        HPDF_Page_MoveTo (page, x, GRID_TOP);
        HPDF_Page_LineTo (page, x, GRID_TOP - ROWS * CELL_HEIGHT);
        HPDF_Page_Stroke (page);
    }

    /* glyphs -- same real check the original character_map.c uses:
     * HPDF_Encoder_GetUnicode() on the combined 2-byte code, skipping
     * both 0 (no mapping at all) and 0x25A1 (WHITE SQUARE, the specific
     * "unmapped" placeholder libharu's own CNS/JP encoder tables return
     * -- confirmed by reading character_map.c's own `unicode != 0x25A1`
     * check, not guessed). A simpler `HPDF_Page_TextWidth() > 0` check
     * was tried first and is NOT sufficient -- confirmed directly: it
     * let several .notdef-glyph code points through (nonzero advance
     * width but no real glyph outline), which veraPDF correctly flagged
     * (ISO 14289-1:2014 7.21.4.1/7.21.8, "no .notdef references"). */
    HPDF_Page_SetFontAndSize (page, glyph_font, 14);
    for (row = 0; row < ROWS; row++) {
        for (col = 0; col < COLS; col++) {
            unsigned char buf[3];
            HPDF_UINT16 code;
            HPDF_UNICODE unicode;
            double w;
            HPDF_REAL cx, cy;

            buf[0] = LEAD_BYTE;
            buf[1] = (unsigned char) (0x40 + row * COLS + col);
            buf[2] = 0x00;
            code = (HPDF_UINT16) ((buf[0] << 8) | buf[1]);

            unicode = HPDF_Encoder_GetUnicode (encoder, code);
            if (unicode == 0 || unicode == 0x25A1)
                continue;

            w = HPDF_Page_TextWidth (page, (char *) buf);
            if (w <= 0)
                continue;

            cx = GRID_LEFT + col * CELL_WIDTH + (CELL_WIDTH - (HPDF_REAL) w) / 2;
            cy = GRID_TOP - (row + 1) * CELL_HEIGHT + 6;

            HPDF_Page_BeginText (page);
            HPDF_Page_MoveTextPos (page, cx, cy);
            HPDF_Page_ShowText (page, (char *) buf);
            HPDF_Page_EndText (page);
        }
    }

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
    HPDF_Page_SetFontAndSize (page, label_font, 10);
    HPDF_Page_BeginText (page);
    HPDF_Page_MoveTextPos (page, GRID_LEFT, GRID_TOP - ROWS * CELL_HEIGHT - 25);
    HPDF_Page_ShowText (page, "Figure 1: 90ms-RKSJ-H, lead byte 0x82 (fullwidth digits/Latin, Hiragana).");
    HPDF_Page_EndText (page);
    status = HPDF_UA_EndMarkedContent (ctx, page);
    if (status != HPDF_OK)
        goto fail;
    HPDF_UA_EndStructureElement (ctx, caption_elem);

    HPDF_UA_EndStructureElement (ctx, doc_elem);

    HPDF_SaveToFile (pdf, "tagged_character_map_demo.pdf");
    HPDF_UA_FreeContext (ctx);
    HPDF_Free (pdf);

    printf ("wrote tagged_character_map_demo.pdf -- a tagged Shift-JIS "
            "glyph-table grid against a real embedded font "
            "(character_map.c port). Validate with "
            "validate/run_verapdf.sh.\n");
    return 0;

fail:
    fprintf (stderr, "tagged_character_map_demo: a tagging call failed, see above\n");
    HPDF_UA_FreeContext (ctx);
    HPDF_Free (pdf);
    return 1;
}
