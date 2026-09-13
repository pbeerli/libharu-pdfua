/*
 * tagged_image_demo.c -- Milestone 6: a tagged port of libharu's own
 * upstream raw_image_demo.c (see docs/roadmap.md's Milestone 6 section;
 * png_demo.c/jpeg_demo.c were not chosen because this project's CMake
 * build deliberately disables libpng discovery, see CMakeLists.txt's
 * "no bundled PNG/JPEG image support" comment -- raw_image_demo.c's
 * approach needs no image-decoding library at all, matching the
 * roadmap's own "raw_image_demo if that's simpler" suggestion). Where
 * the original just drew three untagged raw-pixel images side by side,
 * this version computes two small, genuinely describable raw images at
 * runtime (an RGB gradient swatch and a grayscale ramp -- no binary
 * image asset needs vendoring, so no new NOTICE.md/license entry is
 * needed either) and tags each as its own Document > Figure with real
 * /Alt text, followed by its own Document > Caption, matching
 * tagged_histogram_demo.c's Figure+Caption pattern exactly.
 *
 * Validate with: validate/run_verapdf.sh build/tagged_image_demo.pdf
 */
#include <stdio.h>
#include <stdlib.h>
#include "hpdf_ua/hpdf_ua.h"

#define GRADIENT_W 64
#define GRADIENT_H 64
#define RAMP_W 64
#define RAMP_H 16

static void
error_handler (HPDF_STATUS error_no, HPDF_STATUS detail_no, void *user_data)
{
    (void) user_data;
    fprintf (stderr, "libharu error: error_no=0x%04lX detail_no=%ld\n",
            (unsigned long) error_no, (long) detail_no);
}

/* Fills an RGB gradient buffer: red ramps left->right, blue ramps
 * top->bottom (row 0 = top, matching raw-image row order), green held at
 * a mid constant -- a real, computed image (not a decorative pattern),
 * so its /Alt text below can describe it exactly and accurately. */
static void
fill_rgb_gradient (HPDF_BYTE *buf)
{
    int x, y;

    for (y = 0; y < GRADIENT_H; y++) {
        for (x = 0; x < GRADIENT_W; x++) {
            HPDF_BYTE *p = buf + (y * GRADIENT_W + x) * 3;

            p[0] = (HPDF_BYTE) (x * 255 / (GRADIENT_W - 1));       /* R */
            p[1] = 96;                                              /* G */
            p[2] = (HPDF_BYTE) (y * 255 / (GRADIENT_H - 1));       /* B */
        }
    }
}

/* Fills a grayscale ramp: black (left) to white (right), constant down
 * each column. */
static void
fill_gray_ramp (HPDF_BYTE *buf)
{
    int x, y;

    for (y = 0; y < RAMP_H; y++) {
        for (x = 0; x < RAMP_W; x++) {
            buf[y * RAMP_W + x] = (HPDF_BYTE) (x * 255 / (RAMP_W - 1));
        }
    }
}

int
main (void)
{
    HPDF_Doc pdf;
    HPDF_Page page;
    HPDF_Font font;
    const char *font_name;
    HPDF_UA_Context ctx;
    HPDF_UA_StructElem doc_elem, figure1_elem, caption1_elem;
    HPDF_UA_StructElem figure2_elem, caption2_elem;
    HPDF_STATUS status;
    HPDF_Image gradient_image, ramp_image;
    HPDF_BYTE *gradient_buf, *ramp_buf;
    HPDF_REAL y;

    gradient_buf = (HPDF_BYTE *) malloc ((size_t) GRADIENT_W * GRADIENT_H * 3);
    ramp_buf = (HPDF_BYTE *) malloc ((size_t) RAMP_W * RAMP_H);
    if (!gradient_buf || !ramp_buf) {
        fprintf (stderr, "out of memory building raw image buffers\n");
        free (gradient_buf);
        free (ramp_buf);
        return 1;
    }
    fill_rgb_gradient (gradient_buf);
    fill_gray_ramp (ramp_buf);

    pdf = HPDF_New (error_handler, NULL);
    if (!pdf) {
        fprintf (stderr, "failed to create PDF document\n");
        free (gradient_buf);
        free (ramp_buf);
        return 1;
    }

    HPDF_SetInfoAttr (pdf, HPDF_INFO_TITLE, "libharu-pdfua tagged-image demo");
    HPDF_UA_SetDocumentLanguage (pdf, "en-US");
    HPDF_UA_SetDisplayDocTitle (pdf, HPDF_TRUE);

    status = HPDF_UA_AddMetadata (pdf);
    if (status != HPDF_OK) {
        fprintf (stderr, "HPDF_UA_AddMetadata failed\n");
        goto early_fail;
    }

    ctx = HPDF_UA_NewContext (pdf);
    if (!ctx) {
        fprintf (stderr, "HPDF_UA_NewContext failed\n");
        goto early_fail;
    }

    page = HPDF_AddPage (pdf);
    HPDF_Page_SetSize (page, HPDF_PAGE_SIZE_LETTER, HPDF_PAGE_PORTRAIT);

    font_name = HPDF_LoadTTFontFromFile (pdf, HPDF_UA_DEMO_FONT_PATH, HPDF_TRUE);
    if (!font_name) {
        fprintf (stderr, "HPDF_LoadTTFontFromFile failed\n");
        HPDF_UA_FreeContext (ctx);
        goto early_fail;
    }
    font = HPDF_GetFont (pdf, font_name, "WinAnsiEncoding");

    {
        HPDF_Outline outline = HPDF_CreateOutline (pdf, NULL, "Raw images", NULL);
        HPDF_Destination dst = HPDF_Page_CreateDestination (page);

        HPDF_Destination_SetXYZ (dst, 0, HPDF_Page_GetHeight (page), 1);
        HPDF_Outline_SetDestination (outline, dst);
    }

    gradient_image = HPDF_LoadRawImageFromMem (pdf, gradient_buf,
            GRADIENT_W, GRADIENT_H, HPDF_CS_DEVICE_RGB, 8);
    ramp_image = HPDF_LoadRawImageFromMem (pdf, ramp_buf,
            RAMP_W, RAMP_H, HPDF_CS_DEVICE_GRAY, 8);
    if (!gradient_image || !ramp_image) {
        fprintf (stderr, "HPDF_LoadRawImageFromMem failed\n");
        HPDF_UA_FreeContext (ctx);
        goto early_fail;
    }

    doc_elem = HPDF_UA_BeginStructureElement (ctx, NULL, HPDF_UA_ROLE_DOCUMENT);
    if (!doc_elem)
        goto fail;

    /* --- Figure 1: RGB gradient swatch --- */
    y = 730;
    figure1_elem = HPDF_UA_BeginStructureElement (ctx, doc_elem, HPDF_UA_ROLE_FIGURE);
    if (!figure1_elem)
        goto fail;
    status = HPDF_UA_SetAlternateText (ctx, figure1_elem,
            "Raster image: a 64 by 64 pixel RGB color gradient swatch, "
            "computed at runtime from raw pixel data (no external image "
            "file). Red intensity increases from left (0) to right "
            "(255); blue intensity increases from top (0) to bottom "
            "(255); green is held constant at a mid value, giving a "
            "smooth two-axis color blend from dark green in the "
            "upper-left corner to bright magenta in the lower-right.");
    if (status != HPDF_OK)
        goto fail;
    status = HPDF_UA_BeginMarkedContent (ctx, page, figure1_elem);
    if (status != HPDF_OK)
        goto fail;
    HPDF_Page_DrawImage (page, gradient_image, 50, y - GRADIENT_H,
            (HPDF_REAL) GRADIENT_W, (HPDF_REAL) GRADIENT_H);
    status = HPDF_UA_EndMarkedContent (ctx, page);
    if (status != HPDF_OK)
        goto fail;
    HPDF_UA_EndStructureElement (ctx, figure1_elem);
    y -= (GRADIENT_H + 15);

    caption1_elem = HPDF_UA_BeginStructureElement (ctx, doc_elem, HPDF_UA_ROLE_CAPTION);
    if (!caption1_elem)
        goto fail;
    status = HPDF_UA_BeginMarkedContent (ctx, page, caption1_elem);
    if (status != HPDF_OK)
        goto fail;
    HPDF_Page_SetFontAndSize (page, font, 10);
    HPDF_Page_BeginText (page);
    HPDF_Page_MoveTextPos (page, 50, y);
    HPDF_Page_ShowText (page,
            "Figure 1: computed RGB gradient (HPDF_LoadRawImageFromMem, "
            "DEVICE_RGB, 8 bpc).");
    HPDF_Page_EndText (page);
    status = HPDF_UA_EndMarkedContent (ctx, page);
    if (status != HPDF_OK)
        goto fail;
    HPDF_UA_EndStructureElement (ctx, caption1_elem);
    y -= 45;

    /* --- Figure 2: grayscale ramp --- */
    figure2_elem = HPDF_UA_BeginStructureElement (ctx, doc_elem, HPDF_UA_ROLE_FIGURE);
    if (!figure2_elem)
        goto fail;
    status = HPDF_UA_SetAlternateText (ctx, figure2_elem,
            "Raster image: a 64 by 16 pixel horizontal grayscale ramp, "
            "computed at runtime, transitioning smoothly from solid "
            "black on the left edge to solid white on the right edge, "
            "constant top to bottom.");
    if (status != HPDF_OK)
        goto fail;
    status = HPDF_UA_BeginMarkedContent (ctx, page, figure2_elem);
    if (status != HPDF_OK)
        goto fail;
    HPDF_Page_DrawImage (page, ramp_image, 50, y - RAMP_H,
            (HPDF_REAL) RAMP_W * 2, (HPDF_REAL) RAMP_H * 2);
    status = HPDF_UA_EndMarkedContent (ctx, page);
    if (status != HPDF_OK)
        goto fail;
    HPDF_UA_EndStructureElement (ctx, figure2_elem);
    y -= (RAMP_H * 2 + 15);

    caption2_elem = HPDF_UA_BeginStructureElement (ctx, doc_elem, HPDF_UA_ROLE_CAPTION);
    if (!caption2_elem)
        goto fail;
    status = HPDF_UA_BeginMarkedContent (ctx, page, caption2_elem);
    if (status != HPDF_OK)
        goto fail;
    HPDF_Page_SetFontAndSize (page, font, 10);
    HPDF_Page_BeginText (page);
    HPDF_Page_MoveTextPos (page, 50, y);
    HPDF_Page_ShowText (page,
            "Figure 2: computed grayscale ramp (HPDF_LoadRawImageFromMem, "
            "DEVICE_GRAY, 8 bpc).");
    HPDF_Page_EndText (page);
    status = HPDF_UA_EndMarkedContent (ctx, page);
    if (status != HPDF_OK)
        goto fail;
    HPDF_UA_EndStructureElement (ctx, caption2_elem);

    HPDF_UA_EndStructureElement (ctx, doc_elem);

    HPDF_SaveToFile (pdf, "tagged_image_demo.pdf");
    HPDF_UA_FreeContext (ctx);
    HPDF_Free (pdf);
    free (gradient_buf);
    free (ramp_buf);

    printf ("wrote tagged_image_demo.pdf -- two raw-pixel images (an RGB "
            "gradient and a grayscale ramp), each a tagged Figure with "
            "real /Alt text plus its own Caption, ported from libharu's "
            "original raw_image_demo.c. Validate with "
            "validate/run_verapdf.sh.\n");
    return 0;

fail:
    fprintf (stderr, "tagged_image_demo: a tagging call failed, see above\n");
    HPDF_UA_FreeContext (ctx);
    HPDF_Free (pdf);
    free (gradient_buf);
    free (ramp_buf);
    return 1;

early_fail:
    HPDF_Free (pdf);
    free (gradient_buf);
    free (ramp_buf);
    return 1;
}
