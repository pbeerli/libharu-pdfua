/*
 * tagged_slide_show_demo.c -- Milestone 6: tagged port of a
 * representative subset of libharu's original slide_show_demo.c: pages
 * with a slide transition effect (HPDF_Page_SetSlideShow()) and
 * Next/Prev link annotations chaining between them. A deliberate scope
 * cut from the original (17 pages, one per HPDF_TransitionStyle value):
 * 4 pages/transitions are enough to exercise both the slide-show code
 * path and a real Next/Prev annotation chain (more pages would just
 * repeat the same two code paths with a different enum value).
 *
 * The Next/Prev links reuse tagged_annotation_demo.c's own
 * HPDF_UA_TagAnnotation() pattern (HPDF_UA_ROLE_LINK, /Alt copied to
 * /Contents), now exercised across a longer chain (4 pages x up to 2
 * links each) rather than a single page's outbound links -- confirms
 * the shared /ParentTree numbering used by both marked content and
 * annotation StructParent keys keeps working correctly as more pages
 * and more annotations accumulate in the same context.
 *
 * Structure: each page gets its own Document > H1 (the transition
 * style's name) and Document > LINK per Next/Prev annotation (mirroring
 * tagged_annotation_demo.c). The page's colored background rectangle is
 * marked as an Artifact, same precedent as tagged_table_demo.c's
 * decorative border.
 *
 * Validate with: validate/run_verapdf.sh build/tagged_slide_show_demo.pdf
 */
#include <stdio.h>
#include "hpdf_ua/hpdf_ua.h"

#define NPAGES 4

static void
error_handler (HPDF_STATUS error_no, HPDF_STATUS detail_no, void *user_data)
{
    (void) user_data;
    fprintf (stderr, "libharu error: error_no=0x%04lX detail_no=%ld\n",
            (unsigned long) error_no, (long) detail_no);
}

static HPDF_STATUS
tag_nav_link (HPDF_UA_Context ctx, HPDF_Page page, HPDF_UA_StructElem doc_elem,
        HPDF_Rect rect, HPDF_Page target, const char *alt_text)
{
    HPDF_Destination dst;
    HPDF_Annotation annot;
    HPDF_UA_StructElem link_elem;
    HPDF_STATUS status;

    dst = HPDF_Page_CreateDestination (target);
    HPDF_Destination_SetFit (dst);
    annot = HPDF_Page_CreateLinkAnnot (page, rect, dst);
    HPDF_LinkAnnot_SetBorderStyle (annot, 0, 0, 0);
    HPDF_LinkAnnot_SetHighlightMode (annot, HPDF_ANNOT_INVERT_BOX);

    link_elem = HPDF_UA_BeginStructureElement (ctx, doc_elem, HPDF_UA_ROLE_LINK);
    if (!link_elem)
        return HPDF_FAILED_TO_ALLOC_MEM;
    status = HPDF_UA_SetAlternateText (ctx, link_elem, alt_text);
    if (status != HPDF_OK)
        return status;
    status = HPDF_UA_TagAnnotation (ctx, page, link_elem, annot);
    if (status != HPDF_OK)
        return status;
    HPDF_UA_EndStructureElement (ctx, link_elem);

    return HPDF_OK;
}

int
main (void)
{
    HPDF_Doc pdf;
    HPDF_Page pages[NPAGES];
    HPDF_Font font;
    const char *font_name;
    HPDF_UA_Context ctx;
    HPDF_UA_StructElem doc_elem;
    HPDF_STATUS status;
    int i;

    static const struct { HPDF_TransitionStyle style; const char *name; }
    TRANSITIONS[NPAGES] = {
        { HPDF_TS_WIPE_RIGHT, "HPDF_TS_WIPE_RIGHT" },
        { HPDF_TS_BOX_OUT, "HPDF_TS_BOX_OUT" },
        { HPDF_TS_DISSOLVE, "HPDF_TS_DISSOLVE" },
        { HPDF_TS_REPLACE, "HPDF_TS_REPLACE" },
    };
    static const float COLORS[NPAGES][3] = {
        { 0.85f, 0.30f, 0.25f }, { 0.25f, 0.60f, 0.85f },
        { 0.35f, 0.75f, 0.35f }, { 0.80f, 0.70f, 0.20f },
    };

    pdf = HPDF_New (error_handler, NULL);
    if (!pdf) {
        fprintf (stderr, "failed to create PDF document\n");
        return 1;
    }

    HPDF_SetInfoAttr (pdf, HPDF_INFO_TITLE, "libharu-pdfua tagged-slide-show demo");
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

    font_name = HPDF_LoadTTFontFromFile (pdf, HPDF_UA_DEMO_FONT_PATH, HPDF_TRUE);
    if (!font_name) {
        fprintf (stderr, "HPDF_LoadTTFontFromFile failed\n");
        HPDF_UA_FreeContext (ctx);
        HPDF_Free (pdf);
        return 1;
    }
    font = HPDF_GetFont (pdf, font_name, "WinAnsiEncoding");

    doc_elem = HPDF_UA_BeginStructureElement (ctx, NULL, HPDF_UA_ROLE_DOCUMENT);
    if (!doc_elem)
        goto fail;

    for (i = 0; i < NPAGES; i++) {
        HPDF_UA_StructElem h1_elem;
        HPDF_Rect border = { 0, 0, 800, 600 };

        pages[i] = HPDF_AddPage (pdf);
        HPDF_Page_SetWidth (pages[i], 800);
        HPDF_Page_SetHeight (pages[i], 600);
        HPDF_Page_SetSlideShow (pages[i], TRANSITIONS[i].style, 5.0, 1.0);

        /* decorative colored background -- an Artifact, same precedent
         * as tagged_table_demo.c's decorative border. */
        status = HPDF_UA_BeginArtifact (ctx, pages[i]);
        if (status != HPDF_OK)
            goto fail;
        HPDF_Page_SetRGBFill (pages[i], COLORS[i][0], COLORS[i][1], COLORS[i][2]);
        HPDF_Page_Rectangle (pages[i], border.left, border.bottom,
                border.right - border.left, border.top - border.bottom);
        HPDF_Page_Fill (pages[i]);
        status = HPDF_UA_EndArtifact (ctx, pages[i]);
        if (status != HPDF_OK)
            goto fail;

        h1_elem = HPDF_UA_BeginStructureElement (ctx, doc_elem, HPDF_UA_ROLE_H1);
        if (!h1_elem)
            goto fail;
        status = HPDF_UA_BeginMarkedContent (ctx, pages[i], h1_elem);
        if (status != HPDF_OK)
            goto fail;
        HPDF_Page_SetRGBFill (pages[i], 0, 0, 0);
        HPDF_Page_SetFontAndSize (pages[i], font, 24);
        HPDF_Page_BeginText (pages[i]);
        HPDF_Page_MoveTextPos (pages[i], 50, 530);
        HPDF_Page_ShowText (pages[i], TRANSITIONS[i].name);
        HPDF_Page_EndText (pages[i]);
        status = HPDF_UA_EndMarkedContent (ctx, pages[i]);
        if (status != HPDF_OK)
            goto fail;
        HPDF_UA_EndStructureElement (ctx, h1_elem);
    }

    /* --- Next/Prev link chain, added only once every page exists --- */
    for (i = 0; i < NPAGES; i++) {
        if (i + 1 < NPAGES) {
            HPDF_Rect next_rect = { 680, 50, 750, 70 };
            status = tag_nav_link (ctx, pages[i], doc_elem, next_rect,
                    pages[i + 1], "Go to next slide");
            if (status != HPDF_OK)
                goto fail;
        }
        if (i > 0) {
            HPDF_Rect prev_rect = { 50, 50, 110, 70 };
            status = tag_nav_link (ctx, pages[i], doc_elem, prev_rect,
                    pages[i - 1], "Go to previous slide");
            if (status != HPDF_OK)
                goto fail;
        }
    }

    HPDF_UA_EndStructureElement (ctx, doc_elem);

    HPDF_SetPageMode (pdf, HPDF_PAGE_MODE_FULL_SCREEN);

    HPDF_SaveToFile (pdf, "tagged_slide_show_demo.pdf");
    HPDF_UA_FreeContext (ctx);
    HPDF_Free (pdf);

    printf ("wrote tagged_slide_show_demo.pdf -- a tagged 4-page slide "
            "show with real Next/Prev link annotations "
            "(slide_show_demo.c port). Validate with "
            "validate/run_verapdf.sh.\n");
    return 0;

fail:
    fprintf (stderr, "tagged_slide_show_demo: a tagging call failed, see above\n");
    HPDF_UA_FreeContext (ctx);
    HPDF_Free (pdf);
    return 1;
}
