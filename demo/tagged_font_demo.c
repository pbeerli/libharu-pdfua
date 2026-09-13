/*
 * tagged_font_demo.c -- Milestone 6: a tagged port of libharu's own
 * upstream ttfont_demo.c (see vendor/libharu's original demo set, and
 * NOTICE.md/docs/roadmap.md's Milestone 6 section for why that directory
 * was trimmed when this project was first vendored). Where the original
 * just showed an embedded TrueType font at a handful of sizes with no
 * accessibility structure at all, this version wraps the same specimen
 * content -- font name, alphabet/digits, and a sample sentence at
 * increasing sizes -- in a real H1/H2/P tag tree, using this project's
 * own tagging API rather than leaving it as plain, untagged text.
 *
 * Structure: Document > [H1 "TrueType Font Demo", P (intro),
 * H2 "Font Specimen" + P (font name + alphabet/digits),
 * H2 "Sample Sentence At Increasing Sizes" + P (the same sentence at
 * three sizes)]. A decorative divider rule is marked as an Artifact,
 * matching tagged_table_demo.c's border-rectangle precedent.
 *
 * Validate with: validate/run_verapdf.sh build/tagged_font_demo.pdf
 */
#include <stdio.h>
#include "hpdf_ua/hpdf_ua.h"

static const char *SAMPLE_SENTENCE =
    "The quick brown fox jumps over the lazy dog.";

static const char *INTRO_TEXT =
    "This demo embeds a real TrueType font (DejaVu Sans, see "
    "fonts/DejaVuSans-LICENSE.txt) and tags the resulting specimen text "
    "as real structure content -- an H1 title, and P elements for the "
    "font-specimen block and the repeated sample sentence -- rather than "
    "leaving it as plain, untagged text the way libharu's original "
    "ttfont_demo.c did.";

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
    HPDF_Font title_font, detail_font;
    const char *font_name;
    HPDF_UA_Context ctx;
    HPDF_UA_StructElem doc_elem, h1_elem, intro_elem;
    HPDF_UA_StructElem h2_specimen_elem, specimen_elem;
    HPDF_UA_StructElem h2_sentence_elem, sentence_elem;
    HPDF_STATUS status;
    HPDF_REAL y;
    HPDF_UINT text_len;

    pdf = HPDF_New (error_handler, NULL);
    if (!pdf) {
        fprintf (stderr, "failed to create PDF document\n");
        return 1;
    }

    HPDF_SetInfoAttr (pdf, HPDF_INFO_TITLE, "libharu-pdfua tagged-font demo");
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

    /* The real point of this demo: an embedded (not Standard-14) font,
     * used for every piece of tagged text below, exercising libharu's
     * own HPDF_LoadTTFontFromFile()/subsetting/embedding code path. */
    font_name = HPDF_LoadTTFontFromFile (pdf, HPDF_UA_DEMO_FONT_PATH, HPDF_TRUE);
    if (!font_name) {
        fprintf (stderr, "HPDF_LoadTTFontFromFile failed\n");
        HPDF_Free (pdf);
        return 1;
    }
    title_font = HPDF_GetFont (pdf, font_name, "WinAnsiEncoding");
    detail_font = title_font; /* one embedded font, used at several sizes */

    {
        HPDF_Outline outline = HPDF_CreateOutline (pdf, NULL, "Font specimen", NULL);
        HPDF_Destination dst = HPDF_Page_CreateDestination (page);

        HPDF_Destination_SetXYZ (dst, 0, HPDF_Page_GetHeight (page), 1);
        HPDF_Outline_SetDestination (outline, dst);
    }

    doc_elem = HPDF_UA_BeginStructureElement (ctx, NULL, HPDF_UA_ROLE_DOCUMENT);
    if (!doc_elem)
        goto fail;

    /* --- H1 title --- */
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
    HPDF_Page_ShowText (page, "TrueType Font Demo");
    HPDF_Page_EndText (page);
    status = HPDF_UA_EndMarkedContent (ctx, page);
    if (status != HPDF_OK)
        goto fail;
    HPDF_UA_EndStructureElement (ctx, h1_elem);

    /* --- Intro paragraph --- */
    y -= 40;
    intro_elem = HPDF_UA_BeginStructureElement (ctx, doc_elem, HPDF_UA_ROLE_P);
    if (!intro_elem)
        goto fail;
    status = HPDF_UA_BeginMarkedContent (ctx, page, intro_elem);
    if (status != HPDF_OK)
        goto fail;
    HPDF_Page_SetFontAndSize (page, title_font, 11);
    HPDF_Page_BeginText (page);
    text_len = 0;
    HPDF_Page_TextRect (page, 50, y, 512, y - 60, INTRO_TEXT,
            HPDF_TALIGN_LEFT, &text_len);
    HPDF_Page_EndText (page);
    status = HPDF_UA_EndMarkedContent (ctx, page);
    if (status != HPDF_OK)
        goto fail;
    HPDF_UA_EndStructureElement (ctx, intro_elem);
    y -= 80;

    /* --- Decorative divider: an Artifact, not left untagged. --- */
    status = HPDF_UA_BeginArtifact (ctx, page);
    if (status != HPDF_OK)
        goto fail;
    HPDF_Page_SetLineWidth (page, 0.5);
    HPDF_Page_MoveTo (page, 50, y);
    HPDF_Page_LineTo (page, 562, y);
    HPDF_Page_Stroke (page);
    status = HPDF_UA_EndArtifact (ctx, page);
    if (status != HPDF_OK)
        goto fail;
    y -= 25;

    /* --- Font specimen section --- */
    h2_specimen_elem = HPDF_UA_BeginStructureElement (ctx, doc_elem, HPDF_UA_ROLE_H2);
    if (!h2_specimen_elem)
        goto fail;
    status = HPDF_UA_BeginMarkedContent (ctx, page, h2_specimen_elem);
    if (status != HPDF_OK)
        goto fail;
    HPDF_Page_SetFontAndSize (page, title_font, 14);
    HPDF_Page_BeginText (page);
    HPDF_Page_MoveTextPos (page, 50, y);
    HPDF_Page_ShowText (page, "Font Specimen");
    HPDF_Page_EndText (page);
    status = HPDF_UA_EndMarkedContent (ctx, page);
    if (status != HPDF_OK)
        goto fail;
    HPDF_UA_EndStructureElement (ctx, h2_specimen_elem);
    y -= 30;

    /* One P covering the whole specimen block (font name, alphabet,
     * digits) -- several ShowText/MoveTextPos calls inside one
     * marked-content span, the same "many draw calls, one BDC..EMC"
     * pattern tagged_histogram_demo.c already established for its bars. */
    specimen_elem = HPDF_UA_BeginStructureElement (ctx, doc_elem, HPDF_UA_ROLE_P);
    if (!specimen_elem)
        goto fail;
    status = HPDF_UA_BeginMarkedContent (ctx, page, specimen_elem);
    if (status != HPDF_OK)
        goto fail;

    HPDF_Page_BeginText (page);
    HPDF_Page_SetFontAndSize (page, title_font, 10);
    HPDF_Page_MoveTextPos (page, 50, y);
    HPDF_Page_ShowText (page, font_name);
    HPDF_Page_ShowText (page, " (embedded subset)");

    HPDF_Page_SetFontAndSize (page, detail_font, 15);
    HPDF_Page_MoveTextPos (page, 0, -22);
    HPDF_Page_ShowText (page, "abcdefghijklmnopqrstuvwxyz");
    HPDF_Page_MoveTextPos (page, 0, -20);
    HPDF_Page_ShowText (page, "ABCDEFGHIJKLMNOPQRSTUVWXYZ");
    HPDF_Page_MoveTextPos (page, 0, -20);
    HPDF_Page_ShowText (page, "1234567890");
    HPDF_Page_EndText (page);

    status = HPDF_UA_EndMarkedContent (ctx, page);
    if (status != HPDF_OK)
        goto fail;
    HPDF_UA_EndStructureElement (ctx, specimen_elem);
    y -= 90;

    /* --- Sample sentence at increasing sizes --- */
    h2_sentence_elem = HPDF_UA_BeginStructureElement (ctx, doc_elem, HPDF_UA_ROLE_H2);
    if (!h2_sentence_elem)
        goto fail;
    status = HPDF_UA_BeginMarkedContent (ctx, page, h2_sentence_elem);
    if (status != HPDF_OK)
        goto fail;
    HPDF_Page_SetFontAndSize (page, title_font, 14);
    HPDF_Page_BeginText (page);
    HPDF_Page_MoveTextPos (page, 50, y);
    HPDF_Page_ShowText (page, "Sample Sentence At Increasing Sizes");
    HPDF_Page_EndText (page);
    status = HPDF_UA_EndMarkedContent (ctx, page);
    if (status != HPDF_OK)
        goto fail;
    HPDF_UA_EndStructureElement (ctx, h2_sentence_elem);
    y -= 30;

    sentence_elem = HPDF_UA_BeginStructureElement (ctx, doc_elem, HPDF_UA_ROLE_P);
    if (!sentence_elem)
        goto fail;
    status = HPDF_UA_BeginMarkedContent (ctx, page, sentence_elem);
    if (status != HPDF_OK)
        goto fail;

    HPDF_Page_BeginText (page);
    HPDF_Page_SetFontAndSize (page, detail_font, 10);
    HPDF_Page_MoveTextPos (page, 50, y);
    HPDF_Page_ShowText (page, SAMPLE_SENTENCE);
    HPDF_Page_MoveTextPos (page, 0, -20);

    HPDF_Page_SetFontAndSize (page, detail_font, 16);
    HPDF_Page_ShowText (page, SAMPLE_SENTENCE);
    HPDF_Page_MoveTextPos (page, 0, -28);

    HPDF_Page_SetFontAndSize (page, detail_font, 22);
    HPDF_Page_ShowText (page, SAMPLE_SENTENCE);
    HPDF_Page_EndText (page);

    status = HPDF_UA_EndMarkedContent (ctx, page);
    if (status != HPDF_OK)
        goto fail;
    HPDF_UA_EndStructureElement (ctx, sentence_elem);

    HPDF_UA_EndStructureElement (ctx, doc_elem);

    HPDF_SaveToFile (pdf, "tagged_font_demo.pdf");
    HPDF_UA_FreeContext (ctx);
    HPDF_Free (pdf);

    printf ("wrote tagged_font_demo.pdf -- an embedded TrueType font "
            "(DejaVu Sans) shown as tagged H1/H2/P content, ported from "
            "libharu's original ttfont_demo.c. Validate with "
            "validate/run_verapdf.sh.\n");
    return 0;

fail:
    fprintf (stderr, "tagged_font_demo: a tagging call failed, see above\n");
    HPDF_UA_FreeContext (ctx);
    HPDF_Free (pdf);
    return 1;
}
