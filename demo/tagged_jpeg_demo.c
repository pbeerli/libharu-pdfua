/*
 * tagged_jpeg_demo.c -- Milestone 6: tagged port of libharu's original
 * jpeg_demo.c: real JPEG images embedded via HPDF_LoadJpegImageFromFile()
 * (libharu embeds the JPEG's own DCT-encoded byte stream as-is via the
 * PDF /DCTDecode filter -- no decode, no new build dependency, unlike
 * the PNG demos).
 *
 * Unlike the original (and unlike this project's PNG demos, which reuse
 * the public-domain-license PNGSuite test images), this demo's two JPEGs
 * -- images/jpeg-demo/cactus.jpg (24-bit color) and dragonfly.jpg (8-bit
 * grayscale, converted from a color original specifically to exercise
 * libharu's separate grayscale-JPEG /DeviceGray code path, matching the
 * original demo's own rgb.jpg/gray.jpg split) -- are the project
 * maintainer's own photographs, licensed CC BY 4.0 for this specific use
 * (see NOTICE.md), resolving the licensing question that held this demo
 * back in the previous pass (see docs/roadmap.md's fifth-pass entry).
 *
 * Structure: Document > H1, then one Document > Figure per photo (real
 * /Alt text describing the actual photo and crediting it), followed by
 * one shared Document > Caption.
 *
 * Validate with: validate/run_verapdf.sh build/tagged_jpeg_demo.pdf
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

    static const struct { const char *file; const char *alt; float x; }
    IMAGES[] = {
        { HPDF_UA_DEMO_JPEG_DIR "/cactus.jpg",
          "Color photograph of a cactus, by Peter Beerli (CC BY 4.0).", 60 },
        { HPDF_UA_DEMO_JPEG_DIR "/dragonfly.jpg",
          "Grayscale photograph of a dragonfly, by Peter Beerli (CC BY 4.0).", 320 },
    };

    pdf = HPDF_New (error_handler, NULL);
    if (!pdf) {
        fprintf (stderr, "failed to create PDF document\n");
        return 1;
    }

    HPDF_SetInfoAttr (pdf, HPDF_INFO_TITLE, "libharu-pdfua tagged-jpeg demo");
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
    HPDF_Page_SetSize (page, HPDF_PAGE_SIZE_LETTER, HPDF_PAGE_LANDSCAPE);

    font_name = HPDF_LoadTTFontFromFile (pdf, HPDF_UA_DEMO_FONT_PATH, HPDF_TRUE);
    if (!font_name) {
        fprintf (stderr, "HPDF_LoadTTFontFromFile failed\n");
        HPDF_Free (pdf);
        return 1;
    }
    font = HPDF_GetFont (pdf, font_name, "WinAnsiEncoding");

    {
        HPDF_Outline outline = HPDF_CreateOutline (pdf, NULL, "JPEG", NULL);
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
    HPDF_Page_MoveTextPos (page, 50, 560);
    HPDF_Page_ShowText (page, "JPEG Demo");
    HPDF_Page_EndText (page);
    status = HPDF_UA_EndMarkedContent (ctx, page);
    if (status != HPDF_OK)
        goto fail;
    HPDF_UA_EndStructureElement (ctx, h1_elem);

    for (i = 0; i < sizeof (IMAGES) / sizeof (IMAGES[0]); i++) {
        HPDF_Image image;
        HPDF_UA_StructElem figure_elem;
        HPDF_REAL iw, ih, scale, dw, dh;

        image = HPDF_LoadJpegImageFromFile (pdf, IMAGES[i].file);
        if (!image) {
            fprintf (stderr, "HPDF_LoadJpegImageFromFile failed for %s\n",
                    IMAGES[i].file);
            goto fail;
        }
        iw = HPDF_Image_GetWidth (image);
        ih = HPDF_Image_GetHeight (image);
        /* Scale to fit a ~240x240 box -- these are real, full-resolution
         * (post-resize) photographs, not tiny synthetic test images, so
         * unlike the PNGSuite demos this needs real scaling to fit the
         * page rather than 1:1/upscaled drawing. */
        scale = 240 / (iw > ih ? iw : ih);
        dw = iw * scale;
        dh = ih * scale;

        figure_elem = HPDF_UA_BeginStructureElement (ctx, doc_elem, HPDF_UA_ROLE_FIGURE);
        if (!figure_elem)
            goto fail;
        status = HPDF_UA_SetAlternateText (ctx, figure_elem, IMAGES[i].alt);
        if (status != HPDF_OK)
            goto fail;
        status = HPDF_UA_BeginMarkedContent (ctx, page, figure_elem);
        if (status != HPDF_OK)
            goto fail;
        HPDF_Page_DrawImage (page, image, IMAGES[i].x, 280, dw, dh);
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
    HPDF_Page_MoveTextPos (page, 50, 250);
    HPDF_Page_ShowText (page, "Figure 1-2: a 24-bit color JPEG and an 8-bit grayscale JPEG.");
    HPDF_Page_EndText (page);
    status = HPDF_UA_EndMarkedContent (ctx, page);
    if (status != HPDF_OK)
        goto fail;
    HPDF_UA_EndStructureElement (ctx, caption_elem);

    HPDF_UA_EndStructureElement (ctx, doc_elem);

    HPDF_SaveToFile (pdf, "tagged_jpeg_demo.pdf");
    HPDF_UA_FreeContext (ctx);
    HPDF_Free (pdf);

    printf ("wrote tagged_jpeg_demo.pdf -- two real, individually tagged "
            "JPEG photographs, one color and one grayscale (jpeg_demo.c "
            "port). Validate with validate/run_verapdf.sh.\n");
    return 0;

fail:
    fprintf (stderr, "tagged_jpeg_demo: a tagging call failed, see above\n");
    HPDF_UA_FreeContext (ctx);
    HPDF_Free (pdf);
    return 1;
}
