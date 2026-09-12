/*
 * tagged_skyline_demo.c -- Milestone 3: a real, fully tagged multi-series
 * line plot (matching the shape of Migrate's own skyline plots: a
 * parameter's value through time, here for two populations at once).
 *
 * New content shapes this milestone exercises that Milestones 1-2 didn't:
 * - polylines (moveto/lineto chains forming connected line segments, not
 *   single closed rectangles) as the plotted data itself;
 * - a dashed stroke (HPDF_Page_SetDash()), so the two series are
 *   distinguishable without relying on color alone;
 * - a genuine reading-order DECISION, not just one monolithic Figure like
 *   Milestone 2's: the legend's two color/dash swatches are decorative and
 *   live inside the Figure's own marked-content span, but the legend's
 *   TEXT LABELS ("Population 1"/"Population 2") are real information and
 *   are pulled out as separate, real HPDF_UA_ROLE_P structure elements --
 *   the first use of the P role in this project -- placed as Document
 *   children AFTER the Figure, in a fixed top-to-bottom order matching
 *   their visual position, not left to whatever order the drawing
 *   happened to touch them in.
 *
 * Axis titles ("Time"/"Theta") are folded into the Figure's own /Alt text
 * rather than pulled out separately -- a legitimate, common PDF/UA
 * pattern (axis meaning conveyed via alt text), and keeps this milestone
 * focused on the two genuinely new things above rather than adding a
 * third structural pattern for something Milestone 2 already covered
 * conceptually (real text living outside a figure's own span).
 *
 * Validate with: validate/run_verapdf.sh build/tagged_skyline_demo.pdf
 */
#include <stdio.h>
#include "hpdf_ua/hpdf_ua.h"

#define NPOINTS 11

/* Fake "Theta through time" trajectories for two populations -- not from
 * a real migrate-n skyline run, just representative of the shape
 * (unimodal-ish curves that diverge over time). */
static const double POP1[NPOINTS] = {
    0.010, 0.014, 0.019, 0.026, 0.031, 0.033, 0.030, 0.025, 0.019, 0.014, 0.010
};
static const double POP2[NPOINTS] = {
    0.008, 0.009, 0.011, 0.015, 0.021, 0.028, 0.034, 0.038, 0.039, 0.036, 0.030
};

static void
error_handler (HPDF_STATUS error_no, HPDF_STATUS detail_no, void *user_data)
{
    (void) user_data;
    fprintf (stderr, "libharu error: error_no=0x%04lX detail_no=%ld\n",
            (unsigned long) error_no, (long) detail_no);
}

static HPDF_REAL
scale_y (double v, double max_v, HPDF_REAL baseline, HPDF_REAL top)
{
    return baseline + (HPDF_REAL) (v / max_v) * (top - baseline);
}

int
main (void)
{
    HPDF_Doc pdf;
    HPDF_Page page;
    HPDF_Font font;
    HPDF_UA_Context ctx;
    HPDF_UA_StructElem doc_elem, figure_elem, caption_elem;
    HPDF_UA_StructElem legend1_elem, legend2_elem;
    HPDF_STATUS status;
    int i;

    const HPDF_REAL chart_left = 50, chart_baseline = 400, chart_top = 600;
    const HPDF_REAL chart_width = 300;
    const HPDF_REAL dash_pattern[2] = { 4, 3 };
    double max_v = 0.0;

    for (i = 0; i < NPOINTS; i++) {
        if (POP1[i] > max_v) max_v = POP1[i];
        if (POP2[i] > max_v) max_v = POP2[i];
    }
    max_v *= 1.15; /* headroom, matching a typical auto-scaled axis */

    pdf = HPDF_New (error_handler, NULL);
    if (!pdf) {
        fprintf (stderr, "failed to create PDF document\n");
        return 1;
    }

    HPDF_SetInfoAttr (pdf, HPDF_INFO_TITLE, "libharu-pdfua tagged-skyline demo");
    HPDF_UA_SetDocumentLanguage (pdf, "en-US");
    HPDF_UA_SetDisplayDocTitle (pdf, HPDF_TRUE);

    ctx = HPDF_UA_NewContext (pdf);
    if (!ctx) {
        fprintf (stderr, "HPDF_UA_NewContext failed\n");
        HPDF_Free (pdf);
        return 1;
    }

    page = HPDF_AddPage (pdf);
    HPDF_Page_SetSize (page, HPDF_PAGE_SIZE_LETTER, HPDF_PAGE_PORTRAIT);
    font = HPDF_GetFont (pdf, "Helvetica", NULL);

    doc_elem = HPDF_UA_BeginStructureElement (ctx, NULL, HPDF_UA_ROLE_DOCUMENT);
    if (!doc_elem)
        goto fail;

    /* --- Figure: axes + both polylines + legend swatches, one span --- */
    figure_elem = HPDF_UA_BeginStructureElement (ctx, doc_elem, HPDF_UA_ROLE_FIGURE);
    if (!figure_elem)
        goto fail;

    status = HPDF_UA_SetAlternateText (ctx, figure_elem,
            "Line plot: Theta (y-axis) versus time in mutation-scaled "
            "units (x-axis) for two populations. Population 1 (solid "
            "line) rises to a peak near the middle of the time range then "
            "falls back down. Population 2 (dashed line) starts lower, "
            "rises later, and ends higher than Population 1, diverging "
            "from it over time.");
    if (status != HPDF_OK)
        goto fail;

    status = HPDF_UA_BeginMarkedContent (ctx, page, figure_elem);
    if (status != HPDF_OK)
        goto fail;

    /* Axis lines. */
    HPDF_Page_SetLineWidth (page, 1);
    HPDF_Page_SetRGBStroke (page, 0, 0, 0);
    HPDF_Page_MoveTo (page, chart_left, chart_baseline);
    HPDF_Page_LineTo (page, chart_left, chart_top);
    HPDF_Page_Stroke (page);
    HPDF_Page_MoveTo (page, chart_left, chart_baseline);
    HPDF_Page_LineTo (page, chart_left + chart_width, chart_baseline);
    HPDF_Page_Stroke (page);

    /* Population 1: solid blue polyline -- the new content shape this
     * milestone exists to exercise (a connected moveto/lineto chain
     * inside marked content, not a single closed rectangle like
     * Milestone 2's bars). */
    HPDF_Page_SetRGBStroke (page, 0.2, 0.3, 0.8);
    HPDF_Page_SetDash (page, NULL, 0, 0); /* solid */
    HPDF_Page_SetLineWidth (page, 1.5);
    HPDF_Page_MoveTo (page, chart_left,
            scale_y (POP1[0], max_v, chart_baseline, chart_top));
    for (i = 1; i < NPOINTS; i++) {
        HPDF_REAL x = chart_left + i * (chart_width / (NPOINTS - 1));
        HPDF_Page_LineTo (page, x, scale_y (POP1[i], max_v, chart_baseline, chart_top));
    }
    HPDF_Page_Stroke (page);

    /* Population 2: dashed red polyline. */
    HPDF_Page_SetRGBStroke (page, 0.8, 0.2, 0.2);
    HPDF_Page_SetDash (page, dash_pattern, 2, 0);
    HPDF_Page_MoveTo (page, chart_left,
            scale_y (POP2[0], max_v, chart_baseline, chart_top));
    for (i = 1; i < NPOINTS; i++) {
        HPDF_REAL x = chart_left + i * (chart_width / (NPOINTS - 1));
        HPDF_Page_LineTo (page, x, scale_y (POP2[i], max_v, chart_baseline, chart_top));
    }
    HPDF_Page_Stroke (page);
    HPDF_Page_SetDash (page, NULL, 0, 0); /* back to solid for anything after */

    /* Legend swatches only (short line segments) -- purely decorative
     * color/dash key marks, part of the graphic; the swatches' meaning
     * (which population each belongs to) is conveyed by the separately
     * tagged P elements below, not by text drawn here. */
    HPDF_Page_SetRGBStroke (page, 0.2, 0.3, 0.8);
    HPDF_Page_SetLineWidth (page, 1.5);
    HPDF_Page_MoveTo (page, chart_left + chart_width + 15, chart_top - 10);
    HPDF_Page_LineTo (page, chart_left + chart_width + 35, chart_top - 10);
    HPDF_Page_Stroke (page);

    HPDF_Page_SetRGBStroke (page, 0.8, 0.2, 0.2);
    HPDF_Page_SetDash (page, dash_pattern, 2, 0);
    HPDF_Page_MoveTo (page, chart_left + chart_width + 15, chart_top - 25);
    HPDF_Page_LineTo (page, chart_left + chart_width + 35, chart_top - 25);
    HPDF_Page_Stroke (page);
    HPDF_Page_SetDash (page, NULL, 0, 0);

    status = HPDF_UA_EndMarkedContent (ctx, page);
    if (status != HPDF_OK)
        goto fail;

    HPDF_UA_EndStructureElement (ctx, figure_elem);

    /* --- Legend labels: real, separately tagged text, deliberately
     * ordered top-to-bottom to match their visual position next to the
     * swatches above -- the reading-order decision this milestone is
     * meant to exercise, not left implicit. --- */
    HPDF_Page_SetFontAndSize (page, font, 9);
    HPDF_Page_SetRGBFill (page, 0, 0, 0);

    legend1_elem = HPDF_UA_BeginStructureElement (ctx, doc_elem, HPDF_UA_ROLE_P);
    if (!legend1_elem)
        goto fail;
    status = HPDF_UA_BeginMarkedContent (ctx, page, legend1_elem);
    if (status != HPDF_OK)
        goto fail;
    HPDF_Page_BeginText (page);
    HPDF_Page_MoveTextPos (page, chart_left + chart_width + 40, chart_top - 13);
    HPDF_Page_ShowText (page, "Population 1 (solid)");
    HPDF_Page_EndText (page);
    status = HPDF_UA_EndMarkedContent (ctx, page);
    if (status != HPDF_OK)
        goto fail;
    HPDF_UA_EndStructureElement (ctx, legend1_elem);

    legend2_elem = HPDF_UA_BeginStructureElement (ctx, doc_elem, HPDF_UA_ROLE_P);
    if (!legend2_elem)
        goto fail;
    status = HPDF_UA_BeginMarkedContent (ctx, page, legend2_elem);
    if (status != HPDF_OK)
        goto fail;
    HPDF_Page_BeginText (page);
    HPDF_Page_MoveTextPos (page, chart_left + chart_width + 40, chart_top - 28);
    HPDF_Page_ShowText (page, "Population 2 (dashed)");
    HPDF_Page_EndText (page);
    status = HPDF_UA_EndMarkedContent (ctx, page);
    if (status != HPDF_OK)
        goto fail;
    HPDF_UA_EndStructureElement (ctx, legend2_elem);

    /* --- Caption, same pattern as Milestone 2. --- */
    caption_elem = HPDF_UA_BeginStructureElement (ctx, doc_elem, HPDF_UA_ROLE_CAPTION);
    if (!caption_elem)
        goto fail;

    status = HPDF_UA_BeginMarkedContent (ctx, page, caption_elem);
    if (status != HPDF_OK)
        goto fail;

    HPDF_Page_SetFontAndSize (page, font, 10);
    HPDF_Page_BeginText (page);
    HPDF_Page_MoveTextPos (page, chart_left, chart_baseline - 20);
    HPDF_Page_ShowText (page,
            "Figure 2: Skyline plot of Theta through time, two populations.");
    HPDF_Page_EndText (page);

    status = HPDF_UA_EndMarkedContent (ctx, page);
    if (status != HPDF_OK)
        goto fail;

    HPDF_UA_EndStructureElement (ctx, caption_elem);
    HPDF_UA_EndStructureElement (ctx, doc_elem);

    HPDF_SaveToFile (pdf, "tagged_skyline_demo.pdf");
    HPDF_UA_FreeContext (ctx);
    HPDF_Free (pdf);

    printf ("wrote tagged_skyline_demo.pdf -- a real tagged multi-series "
            "plot (Document > Figure with /Alt text covering both series; "
            "Document > P x2 for the legend labels, in deliberate reading "
            "order; Document > Caption). Validate with "
            "validate/run_verapdf.sh.\n");
    return 0;

fail:
    fprintf (stderr, "tagged_skyline_demo: a tagging call failed, see above\n");
    HPDF_UA_FreeContext (ctx);
    HPDF_Free (pdf);
    return 1;
}
