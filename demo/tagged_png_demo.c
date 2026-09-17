/*
 * tagged_png_demo.c -- Milestone 6: tagged port of a representative
 * subset of libharu's original png_demo.c: real PNG images loaded via
 * HPDF_LoadPngImageFromFile(), covering all five PNG color types
 * (grayscale, truecolor, palette, grayscale+alpha, truecolor+alpha).
 * A deliberate scope cut from the original (15 PNGSuite images across
 * every bit depth of every color type): one representative bit depth
 * per color type, plus the 1-bit case, is enough to exercise libpng
 * decoding for every color-type code path libharu's own
 * hpdf_image_png.c has, without a 15-image grid.
 *
 * This is the first demo in this project needing a real new build
 * dependency (libpng, via CMake's own `find_package(PNG)`) -- see
 * CMakeLists.txt's own comment on why (previously deliberately disabled)
 * and NOTICE.md for the vendored PNGSuite test images' own license
 * (Willem van Schaik, 1999 -- separate from and independent of both
 * libharu's and this project's own license).
 *
 * Structure: Document > H1, then one Document > Figure per image (real
 * /Alt text naming its actual color type/bit depth -- not decorative,
 * each genuinely different pixel data), followed by one shared
 * Document > Caption.
 *
 * Validate with: validate/run_verapdf.sh build/tagged_png_demo.pdf
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
    HPDF_UA_StructElem doc_elem, h1_elem, caption_elem;
    HPDF_STATUS status;
    size_t i;

    static const struct { const char *file; const char *alt; float x, y; }
    IMAGES[] = {
        { HPDF_UA_DEMO_PNG_DIR "/basn0g01.png",
          "1-bit grayscale PNG test image", 60, 620 },
        { HPDF_UA_DEMO_PNG_DIR "/basn0g08.png",
          "8-bit grayscale PNG test image", 220, 620 },
        { HPDF_UA_DEMO_PNG_DIR "/basn2c08.png",
          "8-bit truecolor (RGB) PNG test image", 380, 620 },
        { HPDF_UA_DEMO_PNG_DIR "/basn3p08.png",
          "8-bit palette-indexed color PNG test image", 60, 480 },
        { HPDF_UA_DEMO_PNG_DIR "/basn4a08.png",
          "8-bit grayscale-with-alpha PNG test image", 220, 480 },
        { HPDF_UA_DEMO_PNG_DIR "/basn6a08.png",
          "8-bit truecolor-with-alpha (RGBA) PNG test image", 380, 480 },
    };

    pdf = HPDF_New (error_handler, NULL);
    if (!pdf) {
        fprintf (stderr, "failed to create PDF document\n");
        return 1;
    }

    HPDF_SetInfoAttr (pdf, HPDF_INFO_TITLE, "libharu-pdfua tagged-png demo");
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
        HPDF_Outline outline = HPDF_CreateOutline (pdf, NULL, "PNG", NULL);
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
    HPDF_Page_ShowText (page, "PNG Demo");
    HPDF_Page_EndText (page);
    status = HPDF_UA_EndMarkedContent (ctx, page);
    if (status != HPDF_OK)
        goto fail;
    HPDF_UA_EndStructureElement (ctx, h1_elem);

    for (i = 0; i < sizeof (IMAGES) / sizeof (IMAGES[0]); i++) {
        HPDF_Image image;
        HPDF_UA_StructElem figure_elem;
        HPDF_REAL iw, ih;

        image = HPDF_LoadPngImageFromFile (pdf, IMAGES[i].file);
        if (!image) {
            fprintf (stderr, "HPDF_LoadPngImageFromFile failed for %s\n",
                    IMAGES[i].file);
            goto fail;
        }
        iw = HPDF_Image_GetWidth (image);
        ih = HPDF_Image_GetHeight (image);

        figure_elem = HPDF_UA_BeginStructureElement (ctx, doc_elem, HPDF_UA_ROLE_FIGURE);
        if (!figure_elem)
            goto fail;
        status = HPDF_UA_SetAlternateText (ctx, figure_elem, IMAGES[i].alt);
        if (status != HPDF_OK)
            goto fail;
        status = HPDF_UA_BeginMarkedContent (ctx, page, figure_elem);
        if (status != HPDF_OK)
            goto fail;
        HPDF_Page_DrawImage (page, image, IMAGES[i].x, IMAGES[i].y,
                iw * 4, ih * 4);
        status = HPDF_UA_EndMarkedContent (ctx, page);
        if (status != HPDF_OK)
            goto fail;
        HPDF_UA_EndStructureElement (ctx, figure_elem);
    }

    caption_elem = HPDF_UA_BeginStructureElement (ctx, doc_elem, HPDF_UA_ROLE_CAPTION);
    if (!caption_elem)
        goto fail;
    status = HPDF_UA_BeginMarkedContent (ctx, page, caption_elem);
    if (status != HPDF_OK)
        goto fail;
    HPDF_Page_SetFontAndSize (page, font, 10);
    HPDF_Page_BeginText (page);
    HPDF_Page_MoveTextPos (page, 50, 440);
    HPDF_Page_ShowText (page, "Figure 1-6: PNGSuite test images across all five PNG color types.");
    HPDF_Page_EndText (page);
    status = HPDF_UA_EndMarkedContent (ctx, page);
    if (status != HPDF_OK)
        goto fail;
    HPDF_UA_EndStructureElement (ctx, caption_elem);

    HPDF_UA_EndStructureElement (ctx, doc_elem);

    HPDF_SaveToFile (pdf, "tagged_png_demo.pdf");
    HPDF_UA_FreeContext (ctx);
    HPDF_Free (pdf);

    printf ("wrote tagged_png_demo.pdf -- six real, individually tagged "
            "PNGSuite test images covering every PNG color type "
            "(png_demo.c port). Validate with validate/run_verapdf.sh.\n");
    return 0;

fail:
    fprintf (stderr, "tagged_png_demo: a tagging call failed, see above\n");
    HPDF_UA_FreeContext (ctx);
    HPDF_Free (pdf);
    return 1;
}
