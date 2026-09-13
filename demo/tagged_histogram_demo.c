/*
 * tagged_histogram_demo.c -- Milestone 2: a real, fully tagged bar-chart
 * histogram figure (matching Migrate's own plot_svg.c REPORT_FIGURE_HISTOGRAM
 * renderer: one filled rectangle per bin).
 *
 * Unlike Milestone 1's table demo (which only wrapped text-showing
 * operators, BT..Tj..ET, in marked content), this demo wraps PATH-PAINTING
 * operators (re..f for the bars, m/l..S for the axis lines) inside a
 * BDC..EMC span too -- the real point of this milestone, per
 * docs/roadmap.md: confirming HPDF_UA_BeginMarkedContent()/
 * EndMarkedContent() work for non-text content, not just text.
 *
 * Structure: Document > Figure (one /Alt-described marked-content span
 * covering the whole chart: axis + bars + tick labels) and, as a sibling
 * following it in reading order, Document > Caption (its own real tagged
 * text, not folded into the Figure's /Alt).
 *
 * Validate with: validate/run_verapdf.sh build/tagged_histogram_demo.pdf
 */
#include <stdio.h>
#include "hpdf_ua/hpdf_ua.h"

#define NBINS 10

/* Fake posterior-density bin values for "Theta_1", shaped like a real
 * unimodal posterior -- not from an actual migrate-n run, just
 * representative of what plot_svg.c's histogram renderer draws. */
static const double DENSITY[NBINS] = {
    0.02, 0.05, 0.11, 0.19, 0.23, 0.21, 0.13, 0.07, 0.03, 0.01
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
    HPDF_UA_StructElem doc_elem, h1_elem, figure_elem, caption_elem;
    HPDF_STATUS status;
    int i;

    const HPDF_REAL chart_left = 50, chart_baseline = 400, chart_top = 600;
    const HPDF_REAL bar_width = 26, bar_gap = 4;
    const HPDF_REAL max_bar_height = chart_top - chart_baseline - 20;
    double max_density = 0.0;

    for (i = 0; i < NBINS; i++)
        if (DENSITY[i] > max_density)
            max_density = DENSITY[i];

    pdf = HPDF_New (error_handler, NULL);
    if (!pdf) {
        fprintf (stderr, "failed to create PDF document\n");
        return 1;
    }

    HPDF_SetInfoAttr (pdf, HPDF_INFO_TITLE, "libharu-pdfua tagged-histogram demo");
    HPDF_UA_SetDocumentLanguage (pdf, "en-US");
    HPDF_UA_SetDisplayDocTitle (pdf, HPDF_TRUE);

    /* Milestone 4: real XMP metadata (see tagged_table_demo.c's comment
     * on HPDF_UA_AddMetadata() for why this is separate from libharu's
     * own PDF/A path). */
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

    /* Milestone 4: an embedded font (see tagged_table_demo.c's comment). */
    font_name = HPDF_LoadTTFontFromFile (pdf, HPDF_UA_DEMO_FONT_PATH, HPDF_TRUE);
    if (!font_name) {
        fprintf (stderr, "HPDF_LoadTTFontFromFile failed\n");
        HPDF_Free (pdf);
        return 1;
    }
    font = HPDF_GetFont (pdf, font_name, "WinAnsiEncoding");

    /* Milestone 4: a document outline entry pointing at this page. */
    {
        HPDF_Outline outline = HPDF_CreateOutline (pdf, NULL, "Histogram", NULL);
        HPDF_Destination dst = HPDF_Page_CreateDestination (page);

        HPDF_Destination_SetXYZ (dst, 0, HPDF_Page_GetHeight (page), 1);
        HPDF_Outline_SetDestination (outline, dst);
    }

    doc_elem = HPDF_UA_BeginStructureElement (ctx, NULL, HPDF_UA_ROLE_DOCUMENT);
    if (!doc_elem)
        goto fail;

    /* --- H1 title -- every demo needs at least one heading (avalpdf's
     * "Document has no headings" check); this one predates that being
     * added to the later demos (font/annotation/example). --- */
    h1_elem = HPDF_UA_BeginStructureElement (ctx, doc_elem, HPDF_UA_ROLE_H1);
    if (!h1_elem)
        goto fail;
    status = HPDF_UA_BeginMarkedContent (ctx, page, h1_elem);
    if (status != HPDF_OK)
        goto fail;
    HPDF_Page_SetFontAndSize (page, font, 18);
    HPDF_Page_BeginText (page);
    HPDF_Page_MoveTextPos (page, chart_left, 750);
    HPDF_Page_ShowText (page, "Histogram Demo");
    HPDF_Page_EndText (page);
    status = HPDF_UA_EndMarkedContent (ctx, page);
    if (status != HPDF_OK)
        goto fail;
    HPDF_UA_EndStructureElement (ctx, h1_elem);

    /* --- Figure: the whole chart is one marked-content span --- */
    figure_elem = HPDF_UA_BeginStructureElement (ctx, doc_elem, HPDF_UA_ROLE_FIGURE);
    if (!figure_elem)
        goto fail;

    /* Real, meaningful alt text -- conveys the same information a sighted
     * reader gets from the chart, per PDF/UA-1 best practice (and per
     * this project's own docs/pdf_ua_requirements.md note that Migrate's
     * real report content needs this for every histogram/skyline figure).
     * A real integration would generate this from the same bin/value data
     * driving the drawing, not write it separately by hand -- done here
     * by hand only because this demo's data is itself hand-written. */
    status = HPDF_UA_SetAlternateText (ctx, figure_elem,
            "Bar chart: posterior density histogram of Theta_1 across "
            "10 bins, unimodal, peak density 0.23 at bin 5 (of 10), "
            "density near zero at both range ends.");
    if (status != HPDF_OK)
        goto fail;

    status = HPDF_UA_BeginMarkedContent (ctx, page, figure_elem);
    if (status != HPDF_OK)
        goto fail;

    /* Axis lines. */
    HPDF_Page_SetLineWidth (page, 1);
    HPDF_Page_MoveTo (page, chart_left, chart_baseline);
    HPDF_Page_LineTo (page, chart_left, chart_top);
    HPDF_Page_Stroke (page);
    HPDF_Page_MoveTo (page, chart_left, chart_baseline);
    HPDF_Page_LineTo (page, chart_left + NBINS * (bar_width + bar_gap), chart_baseline);
    HPDF_Page_Stroke (page);

    /* Bars -- filled rectangles, one per bin, scaled to max_density. This
     * is the actual new code path this milestone exists to exercise: a
     * PATH-PAINTING operator (re .. f) inside marked content, not just
     * text-showing operators like Milestone 1's table demo used. */
    HPDF_Page_SetRGBFill (page, 0.25, 0.45, 0.75);
    for (i = 0; i < NBINS; i++) {
        HPDF_REAL x = chart_left + i * (bar_width + bar_gap) + bar_gap;
        HPDF_REAL h = (HPDF_REAL) (DENSITY[i] / max_density * max_bar_height);

        HPDF_Page_Rectangle (page, x, chart_baseline, bar_width, h);
        HPDF_Page_Fill (page);
    }

    /* Tick labels under each bar (bin index) -- real text, inside the
     * same marked-content span as the bars/axis (this figure's whole
     * visual content is described together by the /Alt text above, so
     * there is no separate need to expose each tick label as its own
     * accessible text run for this milestone's scope). */
    HPDF_Page_SetFontAndSize (page, font, 8);
    HPDF_Page_SetRGBFill (page, 0, 0, 0);
    for (i = 0; i < NBINS; i++) {
        HPDF_REAL x = chart_left + i * (bar_width + bar_gap) + bar_gap;
        char label[4];

        (void) snprintf (label, sizeof (label), "%d", i + 1);

        HPDF_Page_BeginText (page);
        HPDF_Page_MoveTextPos (page, x, chart_baseline - 12);
        HPDF_Page_ShowText (page, label);
        HPDF_Page_EndText (page);
    }

    status = HPDF_UA_EndMarkedContent (ctx, page);
    if (status != HPDF_OK)
        goto fail;

    HPDF_UA_EndStructureElement (ctx, figure_elem);

    /* --- Caption: a real, separately tagged text element following the
     * figure in reading order, not folded into its /Alt. --- */
    caption_elem = HPDF_UA_BeginStructureElement (ctx, doc_elem, HPDF_UA_ROLE_CAPTION);
    if (!caption_elem)
        goto fail;

    status = HPDF_UA_BeginMarkedContent (ctx, page, caption_elem);
    if (status != HPDF_OK)
        goto fail;

    HPDF_Page_SetFontAndSize (page, font, 10);
    HPDF_Page_BeginText (page);
    HPDF_Page_MoveTextPos (page, chart_left, chart_baseline - 30);
    HPDF_Page_ShowText (page, "Figure 1: Posterior density histogram of Theta_1.");
    HPDF_Page_EndText (page);

    status = HPDF_UA_EndMarkedContent (ctx, page);
    if (status != HPDF_OK)
        goto fail;

    HPDF_UA_EndStructureElement (ctx, caption_elem);
    HPDF_UA_EndStructureElement (ctx, doc_elem);

    HPDF_SaveToFile (pdf, "tagged_histogram_demo.pdf");
    HPDF_UA_FreeContext (ctx);
    HPDF_Free (pdf);

    printf ("wrote tagged_histogram_demo.pdf -- a real tagged figure "
            "(Document > Figure with /Alt text, wrapping bars+axis+labels "
            "in one marked-content span; Document > Caption with its own "
            "tagged text). Validate with validate/run_verapdf.sh.\n");
    return 0;

fail:
    fprintf (stderr, "tagged_histogram_demo: a tagging call failed, see above\n");
    HPDF_UA_FreeContext (ctx);
    HPDF_Free (pdf);
    return 1;
}
