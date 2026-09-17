/*
 * tagged_image_transform_demo.c -- Milestone 6: tagged port of a
 * representative subset of libharu's original image_demo.c: drawing the
 * same PNG image at actual size, scaled, rotated, and with an image mask
 * and a color mask applied. A deliberate scope cut from the original
 * (which also shows X-only and Y-only scaling and a skew transform
 * separately): actual size, scaling, rotation, image-mask, and
 * color-mask are enough to exercise every real libharu image-drawing
 * code path this demo exists to cover (HPDF_Page_DrawImage(),
 * HPDF_Page_Concat()+HPDF_Page_ExecuteXObject() for the affine
 * transform, HPDF_Image_SetMaskImage(), HPDF_Image_SetColorMask()) --
 * skew is the same HPDF_Page_Concat() mechanism as rotation with
 * different matrix values, not a different code path.
 *
 * Structure: Document > H1, then one Document > Figure per
 * transform/mask (real /Alt text describing what makes that instance
 * different -- not merely "the same image again"), followed by one
 * shared Document > Caption.
 *
 * Validate with: validate/run_verapdf.sh build/tagged_image_transform_demo.pdf
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
    HPDF_Image image, image1, image2, image3;
    HPDF_REAL iw, ih;
    HPDF_REAL x, y;

    pdf = HPDF_New (error_handler, NULL);
    if (!pdf) {
        fprintf (stderr, "failed to create PDF document\n");
        return 1;
    }

    HPDF_SetInfoAttr (pdf, HPDF_INFO_TITLE, "libharu-pdfua tagged-image-transform demo");
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

    image = HPDF_LoadPngImageFromFile (pdf, HPDF_UA_DEMO_PNG_DIR "/basn3p02.png");
    image1 = HPDF_LoadPngImageFromFile (pdf, HPDF_UA_DEMO_PNG_DIR "/basn3p02.png");
    image2 = HPDF_LoadPngImageFromFile (pdf, HPDF_UA_DEMO_PNG_DIR "/basn0g01.png");
    image3 = HPDF_LoadPngImageFromFile (pdf, HPDF_UA_DEMO_PNG_DIR "/maskimage.png");
    if (!image || !image1 || !image2 || !image3) {
        fprintf (stderr, "HPDF_LoadPngImageFromFile failed\n");
        HPDF_UA_FreeContext (ctx);
        HPDF_Free (pdf);
        return 1;
    }
    iw = HPDF_Image_GetWidth (image);
    ih = HPDF_Image_GetHeight (image);

    {
        HPDF_Outline outline = HPDF_CreateOutline (pdf, NULL, "Image Transform", NULL);
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
    HPDF_Page_MoveTextPos (page, 40, 560);
    HPDF_Page_ShowText (page, "Image Transform Demo");
    HPDF_Page_EndText (page);
    status = HPDF_UA_EndMarkedContent (ctx, page);
    if (status != HPDF_OK)
        goto fail;
    HPDF_UA_EndStructureElement (ctx, h1_elem);

    /* --- actual size --- */
    {
        HPDF_UA_StructElem figure_elem = HPDF_UA_BeginStructureElement (
                ctx, doc_elem, HPDF_UA_ROLE_FIGURE);
        if (!figure_elem)
            goto fail;
        status = HPDF_UA_SetAlternateText (ctx, figure_elem,
                "The PNGSuite test image basn3p02.png, drawn at its actual size.");
        if (status != HPDF_OK)
            goto fail;
        status = HPDF_UA_BeginMarkedContent (ctx, page, figure_elem);
        if (status != HPDF_OK)
            goto fail;
        HPDF_Page_DrawImage (page, image, 60, 420, iw * 6, ih * 6);
        status = HPDF_UA_EndMarkedContent (ctx, page);
        if (status != HPDF_OK)
            goto fail;
        HPDF_UA_EndStructureElement (ctx, figure_elem);
    }

    /* --- scaled --- */
    {
        HPDF_UA_StructElem figure_elem = HPDF_UA_BeginStructureElement (
                ctx, doc_elem, HPDF_UA_ROLE_FIGURE);
        if (!figure_elem)
            goto fail;
        status = HPDF_UA_SetAlternateText (ctx, figure_elem,
                "The same test image, scaled 2x wider than it is tall.");
        if (status != HPDF_OK)
            goto fail;
        status = HPDF_UA_BeginMarkedContent (ctx, page, figure_elem);
        if (status != HPDF_OK)
            goto fail;
        HPDF_Page_DrawImage (page, image, 220, 420, iw * 12, ih * 6);
        status = HPDF_UA_EndMarkedContent (ctx, page);
        if (status != HPDF_OK)
            goto fail;
        HPDF_UA_EndStructureElement (ctx, figure_elem);
    }

    /* --- rotated (HPDF_Page_Concat() + HPDF_Page_ExecuteXObject()) --- */
    {
        HPDF_UA_StructElem figure_elem = HPDF_UA_BeginStructureElement (
                ctx, doc_elem, HPDF_UA_ROLE_FIGURE);
        double rad = 30.0 / 180 * 3.141592;

        if (!figure_elem)
            goto fail;
        status = HPDF_UA_SetAlternateText (ctx, figure_elem,
                "The same test image, rotated 30 degrees counterclockwise.");
        if (status != HPDF_OK)
            goto fail;
        status = HPDF_UA_BeginMarkedContent (ctx, page, figure_elem);
        if (status != HPDF_OK)
            goto fail;
        x = 460; y = 420;
        HPDF_Page_GSave (page);
        HPDF_Page_Concat (page, (HPDF_REAL) (iw * 6 * cos (rad)),
                (HPDF_REAL) (iw * 6 * sin (rad)),
                (HPDF_REAL) (ih * 6 * -sin (rad)),
                (HPDF_REAL) (ih * 6 * cos (rad)), x, y);
        HPDF_Page_ExecuteXObject (page, image);
        HPDF_Page_GRestore (page);
        status = HPDF_UA_EndMarkedContent (ctx, page);
        if (status != HPDF_OK)
            goto fail;
        HPDF_UA_EndStructureElement (ctx, figure_elem);
    }

    /* --- image mask (HPDF_Image_SetMaskImage()) --- */
    {
        HPDF_UA_StructElem figure_elem = HPDF_UA_BeginStructureElement (
                ctx, doc_elem, HPDF_UA_ROLE_FIGURE);
        if (!figure_elem)
            goto fail;
        status = HPDF_UA_SetAlternateText (ctx, figure_elem,
                "The same test image, with a 1-bit grayscale test image "
                "applied as its stencil mask.");
        if (status != HPDF_OK)
            goto fail;
        HPDF_Image_SetMaskImage (image1, image2);
        status = HPDF_UA_BeginMarkedContent (ctx, page, figure_elem);
        if (status != HPDF_OK)
            goto fail;
        HPDF_Page_DrawImage (page, image1, 60, 260, iw * 6, ih * 6);
        status = HPDF_UA_EndMarkedContent (ctx, page);
        if (status != HPDF_OK)
            goto fail;
        HPDF_UA_EndStructureElement (ctx, figure_elem);
    }

    /* --- color mask (HPDF_Image_SetColorMask()) --- */
    {
        HPDF_UA_StructElem figure_elem = HPDF_UA_BeginStructureElement (
                ctx, doc_elem, HPDF_UA_ROLE_FIGURE);
        HPDF_REAL iw3 = HPDF_Image_GetWidth (image3);
        HPDF_REAL ih3 = HPDF_Image_GetHeight (image3);

        if (!figure_elem)
            goto fail;
        status = HPDF_UA_SetAlternateText (ctx, figure_elem,
                "An RGB test image with a green color range set as a "
                "transparent color mask.");
        if (status != HPDF_OK)
            goto fail;
        HPDF_Image_SetColorMask (image3, 0, 255, 0, 0, 0, 255);
        status = HPDF_UA_BeginMarkedContent (ctx, page, figure_elem);
        if (status != HPDF_OK)
            goto fail;
        HPDF_Page_DrawImage (page, image3, 220, 260, iw3 * 6, ih3 * 6);
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
    HPDF_Page_MoveTextPos (page, 40, 220);
    HPDF_Page_ShowText (page, "Figure 1-5: actual size, scaling, rotation, image mask, color mask.");
    HPDF_Page_EndText (page);
    status = HPDF_UA_EndMarkedContent (ctx, page);
    if (status != HPDF_OK)
        goto fail;
    HPDF_UA_EndStructureElement (ctx, caption_elem);

    HPDF_UA_EndStructureElement (ctx, doc_elem);

    HPDF_SaveToFile (pdf, "tagged_image_transform_demo.pdf");
    HPDF_UA_FreeContext (ctx);
    HPDF_Free (pdf);

    printf ("wrote tagged_image_transform_demo.pdf -- a real tagged image "
            "gallery: actual size, scaling, rotation, image mask, color "
            "mask (image_demo.c port). Validate with "
            "validate/run_verapdf.sh.\n");
    return 0;

fail:
    fprintf (stderr, "tagged_image_transform_demo: a tagging call failed, see above\n");
    HPDF_UA_FreeContext (ctx);
    HPDF_Free (pdf);
    return 1;
}
