/*
 * tagged_text_annotation_demo.c -- Milestone 6: tagged port of a
 * representative subset of libharu's original text_annotation.c: popup
 * "sticky note" (Text) annotations with different icons. A deliberate
 * scope cut from the original (eight icons): four representative icons,
 * enough to exercise HPDF_UA_TagAnnotation() on a non-Link annotation
 * type for the first time.
 *
 * Text (popup/note) annotations differ from tagged_annotation_demo.c's
 * Link annotations in one important way: HPDF_Page_CreateTextAnnot()
 * already takes the note's text directly (its /Contents), so there is no
 * need for HPDF_UA_TagAnnotation()'s /Alt-to-/Contents copy (that trick
 * exists specifically because Link annotations have no such parameter --
 * see that function's own doc comment). What Text annotations still need,
 * same as Link, is a real structure-tree association (a /OBJR kid) so the
 * annotation isn't merely untagged-by-omission (ISO 14289-1:2014 7.18.1)
 * plus correct /F flags -- both of which HPDF_UA_TagAnnotation() already
 * provides regardless of annotation type.
 *
 * Role choice: HPDF_UA_ROLE_ANNOT ("Annot"), added by this same port --
 * see its own comment in hpdf_ua.h for why a generic role (HPDF_UA_ROLE_DIV
 * was tried first) isn't enough: ISO 14289-1:2014 7.18.1 requires the
 * literal name "Annot" for any non-Widget/PrinterMark/Link annotation,
 * confirmed directly with veraPDF.
 *
 * Structure: Document > H1, then Document > ANNOT per annotation (each
 * ANNOT's only child is the annotation's own /OBJR -- an "OBJR-only"
 * element, explicitly valid per HPDF_UA_TagAnnotation()'s own doc
 * comment), plus a real, separately-tagged P label next to each icon.
 *
 * Validate with: validate/run_verapdf.sh build/tagged_text_annotation_demo.pdf
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
    HPDF_UA_StructElem doc_elem, h1_elem;
    HPDF_STATUS status;
    static const struct {
        HPDF_Rect rect;
        HPDF_AnnotIcon icon;
        const char *contents;
        const char *label;
    } NOTES[] = {
        { { 50, 690, 150, 740 }, HPDF_ANNOT_ICON_COMMENT,
          "Annotation with Comment Icon.", "Comment Icon" },
        { { 210, 690, 350, 740 }, HPDF_ANNOT_ICON_KEY,
          "Annotation with Key Icon.", "Key Icon" },
        { { 50, 610, 150, 660 }, HPDF_ANNOT_ICON_NOTE,
          "Annotation with Note Icon.", "Note Icon" },
        { { 210, 610, 350, 660 }, HPDF_ANNOT_ICON_HELP,
          "Annotation with Help Icon.", "Help Icon" },
    };
    size_t i;

    pdf = HPDF_New (error_handler, NULL);
    if (!pdf) {
        fprintf (stderr, "failed to create PDF document\n");
        return 1;
    }

    HPDF_SetInfoAttr (pdf, HPDF_INFO_TITLE, "libharu-pdfua tagged-text-annotation demo");
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
        HPDF_Outline outline = HPDF_CreateOutline (pdf, NULL, "Text Annotation", NULL);
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
    HPDF_Page_ShowText (page, "Text Annotation Demo");
    HPDF_Page_EndText (page);
    status = HPDF_UA_EndMarkedContent (ctx, page);
    if (status != HPDF_OK)
        goto fail;
    HPDF_UA_EndStructureElement (ctx, h1_elem);

    for (i = 0; i < sizeof (NOTES) / sizeof (NOTES[0]); i++) {
        HPDF_Annotation annot;
        HPDF_UA_StructElem annot_elem, p_elem;

        annot = HPDF_Page_CreateTextAnnot (page, NOTES[i].rect,
                NOTES[i].contents, NULL);
        HPDF_TextAnnot_SetIcon (annot, NOTES[i].icon);

        annot_elem = HPDF_UA_BeginStructureElement (ctx, doc_elem, HPDF_UA_ROLE_ANNOT);
        if (!annot_elem)
            goto fail;
        status = HPDF_UA_TagAnnotation (ctx, page, annot_elem, annot);
        if (status != HPDF_OK)
            goto fail;
        HPDF_UA_EndStructureElement (ctx, annot_elem);

        p_elem = HPDF_UA_BeginStructureElement (ctx, doc_elem, HPDF_UA_ROLE_P);
        if (!p_elem)
            goto fail;
        status = HPDF_UA_BeginMarkedContent (ctx, page, p_elem);
        if (status != HPDF_OK)
            goto fail;
        HPDF_Page_SetFontAndSize (page, font, 11);
        HPDF_Page_BeginText (page);
        HPDF_Page_MoveTextPos (page, NOTES[i].rect.left + 40, NOTES[i].rect.top - 20);
        HPDF_Page_ShowText (page, NOTES[i].label);
        HPDF_Page_EndText (page);
        status = HPDF_UA_EndMarkedContent (ctx, page);
        if (status != HPDF_OK)
            goto fail;
        HPDF_UA_EndStructureElement (ctx, p_elem);
    }

    HPDF_UA_EndStructureElement (ctx, doc_elem);

    HPDF_SaveToFile (pdf, "tagged_text_annotation_demo.pdf");
    HPDF_UA_FreeContext (ctx);
    HPDF_Free (pdf);

    printf ("wrote tagged_text_annotation_demo.pdf -- real tagged Text "
            "(popup/note) annotations, the first non-Link use of "
            "HPDF_UA_TagAnnotation() (text_annotation.c port). Validate "
            "with validate/run_verapdf.sh.\n");
    return 0;

fail:
    fprintf (stderr, "tagged_text_annotation_demo: a tagging call failed, see above\n");
    HPDF_UA_FreeContext (ctx);
    HPDF_Free (pdf);
    return 1;
}
