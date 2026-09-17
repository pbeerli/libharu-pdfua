/*
 * tagged_text_demo.c -- Milestone 6: tagged port merging a representative
 * subset of libharu's original text_demo.c (font size, rendering mode,
 * rotate/skew/scale text matrices, char/word spacing) and text_demo2.c
 * (HPDF_Page_TextRect() paragraph alignment: left/right/center/justify).
 * Merged into one demo, rather than two separate ports, since both
 * originals are fundamentally about the same thing -- real, readable text
 * shown under different HPDF_Page text-state settings -- and this
 * project's own demo set benefits more from one well-tagged text-features
 * demo than two thinner ones.
 *
 * Unlike tagged_histogram_demo.c/tagged_arc_demo.c (one Figure per
 * demo), every sample here is real, readable text (not a chart), so each
 * is tagged as its own Document > P -- the semantically correct role,
 * and real coverage of HPDF_UA_BeginMarkedContent() wrapping many small,
 * independent text-showing spans on one page rather than one large span.
 *
 * Validate with: validate/run_verapdf.sh build/tagged_text_demo.pdf
 */
#include <math.h>
#include <stdio.h>
#include "hpdf_ua/hpdf_ua.h"

static void
error_handler (HPDF_STATUS error_no, HPDF_STATUS detail_no, void *user_data)
{
    (void) user_data;
    fprintf (stderr, "libharu error: error_no=0x%04lX detail_no=%ld\n",
            (unsigned long) error_no, (long) detail_no);
}

/* Wraps `body` (a caller-supplied block of HPDF_Page_* calls) as one
 * Document > P, given it already has `font`/size/color set as needed.
 * Not a macro over arbitrary statements in the original demos' style --
 * each call site below is inlined instead, to keep control flow (the
 * `goto fail` error paths) visible at each tagging call, matching this
 * project's other demos. */
static HPDF_UA_StructElem
begin_p (HPDF_UA_Context ctx, HPDF_UA_StructElem parent, HPDF_Page page,
        HPDF_STATUS *status)
{
    HPDF_UA_StructElem p = HPDF_UA_BeginStructureElement (ctx, parent, HPDF_UA_ROLE_P);
    if (!p)
        return NULL;
    *status = HPDF_UA_BeginMarkedContent (ctx, page, p);
    return p;
}

int
main (void)
{
    HPDF_Doc pdf;
    HPDF_Page page;
    HPDF_Font font;
    const char *font_name;
    HPDF_UA_Context ctx;
    HPDF_UA_StructElem doc_elem, h1_elem, p;
    HPDF_STATUS status;
    const char *samp = "abcdefgABCDEFG123!#$%&+-@?";
    float fsize;

    pdf = HPDF_New (error_handler, NULL);
    if (!pdf) {
        fprintf (stderr, "failed to create PDF document\n");
        return 1;
    }

    HPDF_SetInfoAttr (pdf, HPDF_INFO_TITLE, "libharu-pdfua tagged-text demo");
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
        HPDF_Outline outline = HPDF_CreateOutline (pdf, NULL, "Text", NULL);
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
    HPDF_Page_MoveTextPos (page, 50, 770);
    HPDF_Page_ShowText (page, "Text Demo");
    HPDF_Page_EndText (page);
    status = HPDF_UA_EndMarkedContent (ctx, page);
    if (status != HPDF_OK)
        goto fail;
    HPDF_UA_EndStructureElement (ctx, h1_elem);

    /* --- font sizes (text_demo.c) --- */
    {
        HPDF_REAL y = 730;

        for (fsize = 10; fsize < 40; fsize *= 1.6f) {
            char buf[64];

            (void) snprintf (buf, sizeof (buf), "%s (size %.0f)", samp, fsize);

            p = begin_p (ctx, doc_elem, page, &status);
            if (!p || status != HPDF_OK)
                goto fail;
            HPDF_Page_SetFontAndSize (page, font, fsize);
            HPDF_Page_BeginText (page);
            HPDF_Page_MoveTextPos (page, 50, y);
            HPDF_Page_ShowText (page, buf);
            HPDF_Page_EndText (page);
            status = HPDF_UA_EndMarkedContent (ctx, page);
            if (status != HPDF_OK)
                goto fail;
            HPDF_UA_EndStructureElement (ctx, p);

            y -= fsize + 12;
        }
    }

    /* --- text rendering modes (text_demo.c) --- */
    {
        static const struct { HPDF_TextRenderingMode mode; const char *label; }
        MODES[] = {
            { HPDF_FILL, "RenderingMode=FILL" },
            { HPDF_STROKE, "RenderingMode=STROKE" },
            { HPDF_FILL_THEN_STROKE, "RenderingMode=FILL_THEN_STROKE" },
        };
        HPDF_REAL y = 560;
        size_t i;

        HPDF_Page_SetRGBFill (page, 0.5, 0.5, 0.0);
        HPDF_Page_SetLineWidth (page, 1.2);

        for (i = 0; i < sizeof (MODES) / sizeof (MODES[0]); i++) {
            p = begin_p (ctx, doc_elem, page, &status);
            if (!p || status != HPDF_OK)
                goto fail;
            HPDF_Page_SetFontAndSize (page, font, 24);
            HPDF_Page_SetTextRenderingMode (page, MODES[i].mode);
            HPDF_Page_BeginText (page);
            HPDF_Page_MoveTextPos (page, 50, y);
            HPDF_Page_ShowText (page, MODES[i].label);
            HPDF_Page_MoveTextPos (page, 260, 0);
            HPDF_Page_ShowText (page, "ABCabc123");
            HPDF_Page_EndText (page);
            status = HPDF_UA_EndMarkedContent (ctx, page);
            if (status != HPDF_OK)
                goto fail;
            HPDF_UA_EndStructureElement (ctx, p);

            y -= 40;
        }
        HPDF_Page_SetTextRenderingMode (page, HPDF_FILL);
        HPDF_Page_SetRGBFill (page, 0, 0, 0);
    }

    /* --- rotated/skewed/scaled text (text_demo.c) --- */
    {
        double rad = 20.0 / 180 * 3.14159265;

        p = begin_p (ctx, doc_elem, page, &status);
        if (!p || status != HPDF_OK)
            goto fail;
        HPDF_Page_SetFontAndSize (page, font, 24);
        HPDF_Page_BeginText (page);
        HPDF_Page_SetTextMatrix (page, (float) cos (rad), (float) sin (rad),
                (float) -sin (rad), (float) cos (rad), 50, 410);
        HPDF_Page_ShowText (page, "Rotated text (20 degrees): ABCabc123");
        HPDF_Page_EndText (page);
        status = HPDF_UA_EndMarkedContent (ctx, page);
        if (status != HPDF_OK)
            goto fail;
        HPDF_UA_EndStructureElement (ctx, p);

        p = begin_p (ctx, doc_elem, page, &status);
        if (!p || status != HPDF_OK)
            goto fail;
        HPDF_Page_BeginText (page);
        HPDF_Page_SetTextMatrix (page, 1.6f, 0, 0, 1, 50, 370);
        HPDF_Page_ShowText (page, "Scaled text (1.6x): ABCabc123");
        HPDF_Page_EndText (page);
        status = HPDF_UA_EndMarkedContent (ctx, page);
        if (status != HPDF_OK)
            goto fail;
        HPDF_UA_EndStructureElement (ctx, p);
    }

    /* --- char/word spacing (text_demo.c) --- */
    {
        p = begin_p (ctx, doc_elem, page, &status);
        if (!p || status != HPDF_OK)
            goto fail;
        HPDF_Page_SetFontAndSize (page, font, 16);
        HPDF_Page_SetCharSpace (page, 0);
        HPDF_Page_SetWordSpace (page, 0);
        HPDF_Page_BeginText (page);
        HPDF_Page_MoveTextPos (page, 50, 320);
        HPDF_Page_ShowText (page, "char-spacing 0: The quick brown fox.");
        HPDF_Page_EndText (page);
        status = HPDF_UA_EndMarkedContent (ctx, page);
        if (status != HPDF_OK)
            goto fail;
        HPDF_UA_EndStructureElement (ctx, p);

        p = begin_p (ctx, doc_elem, page, &status);
        if (!p || status != HPDF_OK)
            goto fail;
        HPDF_Page_SetCharSpace (page, 1.5);
        HPDF_Page_SetWordSpace (page, 2.5);
        HPDF_Page_BeginText (page);
        HPDF_Page_MoveTextPos (page, 50, 290);
        HPDF_Page_ShowText (page, "char-spacing 1.5, word-spacing 2.5: The quick brown fox.");
        HPDF_Page_EndText (page);
        status = HPDF_UA_EndMarkedContent (ctx, page);
        if (status != HPDF_OK)
            goto fail;
        HPDF_UA_EndStructureElement (ctx, p);

        HPDF_Page_SetCharSpace (page, 0);
        HPDF_Page_SetWordSpace (page, 0);
    }

    /* --- paragraph alignment via HPDF_Page_TextRect() (text_demo2.c) --- */
    {
        static const struct { HPDF_TextAlignment align; const char *label; }
        ALIGNS[] = {
            { HPDF_TALIGN_LEFT, "HPDF_TALIGN_LEFT" },
            { HPDF_TALIGN_RIGHT, "HPDF_TALIGN_RIGHT" },
            { HPDF_TALIGN_CENTER, "HPDF_TALIGN_CENTER" },
            { HPDF_TALIGN_JUSTIFY, "HPDF_TALIGN_JUSTIFY" },
        };
        const char *para = "The quick brown fox jumps over the lazy dog. "
                "Pack my box with five dozen liquor jugs.";
        HPDF_REAL top = 250;
        size_t i;

        HPDF_Page_SetTextLeading (page, 14);

        for (i = 0; i < sizeof (ALIGNS) / sizeof (ALIGNS[0]); i++) {
            HPDF_REAL left = 50 + (HPDF_REAL) i * 130;
            HPDF_REAL right = left + 120;

            p = begin_p (ctx, doc_elem, page, &status);
            if (!p || status != HPDF_OK)
                goto fail;
            HPDF_Page_SetFontAndSize (page, font, 8);
            HPDF_Page_BeginText (page);
            HPDF_Page_MoveTextPos (page, left, top + 12);
            HPDF_Page_ShowText (page, ALIGNS[i].label);
            HPDF_Page_SetFontAndSize (page, font, 9);
            HPDF_Page_TextRect (page, left, top, right, top - 100, para,
                    ALIGNS[i].align, NULL);
            HPDF_Page_EndText (page);
            status = HPDF_UA_EndMarkedContent (ctx, page);
            if (status != HPDF_OK)
                goto fail;
            HPDF_UA_EndStructureElement (ctx, p);
        }
    }

    HPDF_UA_EndStructureElement (ctx, doc_elem);

    HPDF_SaveToFile (pdf, "tagged_text_demo.pdf");
    HPDF_UA_FreeContext (ctx);
    HPDF_Free (pdf);

    printf ("wrote tagged_text_demo.pdf -- tagged text-feature samples "
            "(font size, rendering mode, rotate/scale, spacing, "
            "paragraph alignment; text_demo.c + text_demo2.c port). "
            "Validate with validate/run_verapdf.sh.\n");
    return 0;

fail:
    fprintf (stderr, "tagged_text_demo: a tagging call failed, see above\n");
    HPDF_UA_FreeContext (ctx);
    HPDF_Free (pdf);
    return 1;
}
