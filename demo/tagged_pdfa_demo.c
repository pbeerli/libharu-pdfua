/*
 * tagged_pdfa_demo.c -- Milestone 6: tagged port of a representative
 * slice of libharu's original pdf_a_conformance.c, combining real
 * PDF/A-3B machinery (an embedded-file attachment, a real ICC
 * /OutputIntents entry, PDF/A XMP identification) with this project's
 * own real PDF/UA-1 tagging in ONE document. Deliberately does NOT
 * attempt the original demo's own Factur-X e-invoicing XMP extension
 * schema (that is normative, invoice-specific business logic, not
 * fundamental to "can PDF/A and PDF/UA-1 coexist") -- the attached file
 * here is a small, original, non-normative sample XML
 * (demo/pdf_a/sample-attachment.xml), not a Factur-X document, and this
 * project makes no Factur-X conformance claim.
 *
 * The real question this demo answers, per docs/roadmap.md's own
 * framing: PDF/A and PDF/UA-1 are independent, simultaneously
 * satisfiable conformance levels for the SAME document, not a
 * "combine a non-compliant PDF into a compliant one" question -- but
 * libharu's own PDF/A support and this project's own PDF/UA-1 tagging
 * each independently want to own /MarkInfo, /StructTreeRoot, and the
 * XMP /Metadata stream, so combining them for real needs those three
 * objects written ONCE, correctly, not twice.
 *
 * Concretely: this demo does NOT call `HPDF_SetPDFAConformance()` --
 * that function sets an internal flag that makes `HPDF_SaveToFile()`
 * automatically invoke libharu's own `HPDF_PDFA_AddXmpMetadata()` at
 * save time, unconditionally (re)creating a fresh, EMPTY
 * `/StructTreeRoot` (confirmed by reading `hpdf_pdfa.c` directly, not
 * assumed) -- which would silently discard this project's real,
 * already-populated structure tree the moment the file is saved. Two
 * other real libharu PDF/A building blocks -- `HPDF_AppendOutputIntents()`
 * and `HPDF_LoadIccProfileFromFile()` -- turned out to be independent of
 * that flag (confirmed by reading `hpdf_pdfa.c` directly: neither one
 * even reads `pdf->pdfa_type`) and so are used directly, unmodified.
 * The one genuinely missing piece -- an XMP block declaring PDF/A
 * conformance in the SAME packet as this project's own PDF/UA-1
 * declaration -- is `HPDF_UA_AddMetadataWithPDFA()`, a new, real
 * function added to this project's own API for exactly this demo (see
 * its own doc comment in hpdf_ua.h).
 *
 * The ICC profile (demo/pdf_a/sRGB2014.icc) is the International Color
 * Consortium's own official, freely redistributable sRGB profile (see
 * demo/pdf_a/sRGB2014-LICENSE.txt) -- not upstream libharu's own
 * pdf_a/device_rgb.icc, whose license (if any) is unstated.
 *
 * Verification note: this demo is checked against this project's own
 * established bar, veraPDF's PDF/UA-1 (ua1) flavour, like every other
 * demo -- it reaches full compliance. It was ALSO checked against
 * veraPDF's PDF/A-3B (3b) flavour as a bonus, real check (not required
 * by this project's own stated scope, see README.md's "Terminology
 * note"); see docs/roadmap.md for that result, honestly reported either
 * way.
 *
 * Structure: Document > H1, Document > P (explains what the document
 * demonstrates).
 *
 * Validate with:
 *   validate/run_verapdf.sh build/tagged_pdfa_demo.pdf
 *   verapdf --flavour 3b build/tagged_pdfa_demo.pdf   # bonus PDF/A-3B check
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
    HPDF_UA_StructElem doc_elem, h1_elem, p_elem;
    HPDF_STATUS status;
    HPDF_OutputIntent output_intent;
    HPDF_EmbeddedFile embedded_file;

    pdf = HPDF_New (error_handler, NULL);
    if (!pdf) {
        fprintf (stderr, "failed to create PDF document\n");
        return 1;
    }

    HPDF_SetInfoAttr (pdf, HPDF_INFO_TITLE, "libharu-pdfua tagged-pdfa demo");
    HPDF_UA_SetDocumentLanguage (pdf, "en-US");
    HPDF_UA_SetDisplayDocTitle (pdf, HPDF_TRUE);

    /* Real PDF/UA-1 + PDF/A-3B XMP, in one packet -- see this file's
     * own top comment and hpdf_ua.h's doc comment on this function for
     * why this replaces the usual HPDF_UA_AddMetadata() call here. */
    status = HPDF_UA_AddMetadataWithPDFA (pdf, HPDF_PDFA_3B);
    if (status != HPDF_OK) {
        fprintf (stderr, "HPDF_UA_AddMetadataWithPDFA failed\n");
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
        HPDF_UA_FreeContext (ctx);
        HPDF_Free (pdf);
        return 1;
    }
    font = HPDF_GetFont (pdf, font_name, "WinAnsiEncoding");

    {
        HPDF_Outline outline = HPDF_CreateOutline (pdf, NULL, "PDF/A + PDF/UA", NULL);
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
    HPDF_Page_ShowText (page, "PDF/A-3B + PDF/UA-1 Demo");
    HPDF_Page_EndText (page);
    status = HPDF_UA_EndMarkedContent (ctx, page);
    if (status != HPDF_OK)
        goto fail;
    HPDF_UA_EndStructureElement (ctx, h1_elem);

    p_elem = HPDF_UA_BeginStructureElement (ctx, doc_elem, HPDF_UA_ROLE_P);
    if (!p_elem)
        goto fail;
    status = HPDF_UA_BeginMarkedContent (ctx, page, p_elem);
    if (status != HPDF_OK)
        goto fail;
    HPDF_Page_SetFontAndSize (page, font, 12);
    HPDF_Page_SetTextLeading (page, 18);
    HPDF_Page_BeginText (page);
    HPDF_Page_MoveTextPos (page, 50, 700);
    HPDF_Page_ShowTextNextLine (page,
            "This document declares both PDF/A-3B and PDF/UA-1");
    HPDF_Page_ShowTextNextLine (page,
            "conformance in one XMP packet, carries a real sRGB");
    HPDF_Page_ShowTextNextLine (page,
            "/OutputIntents entry, and has a real, tagged structure");
    HPDF_Page_ShowTextNextLine (page,
            "tree -- plus a small embedded XML file attachment.");
    HPDF_Page_EndText (page);
    status = HPDF_UA_EndMarkedContent (ctx, page);
    if (status != HPDF_OK)
        goto fail;
    HPDF_UA_EndStructureElement (ctx, p_elem);

    HPDF_UA_EndStructureElement (ctx, doc_elem);

    /* --- real PDF/A building blocks, independent of
     * HPDF_SetPDFAConformance() (deliberately not called -- see this
     * file's own top comment) --- */
    output_intent = HPDF_LoadIccProfileFromFile (pdf, HPDF_UA_DEMO_ICC_PATH, 3);
    if (!output_intent) {
        fprintf (stderr, "HPDF_LoadIccProfileFromFile failed\n");
        goto fail;
    }
    if (HPDF_AppendOutputIntents (pdf, "sRGB", output_intent) != HPDF_OK) {
        fprintf (stderr, "HPDF_AppendOutputIntents failed\n");
        goto fail;
    }

    embedded_file = HPDF_AttachFile (pdf, HPDF_UA_DEMO_XML_PATH);
    if (!embedded_file) {
        fprintf (stderr, "HPDF_AttachFile failed\n");
        goto fail;
    }
    HPDF_EmbeddedFile_SetAFRelationship (embedded_file, HPDF_AFRELATIONSHIP_DATA);
    HPDF_EmbeddedFile_SetDescription (embedded_file, "Sample metadata (not Factur-X)");
    HPDF_EmbeddedFile_SetSubtype (embedded_file, "text/xml");

    HPDF_SaveToFile (pdf, "tagged_pdfa_demo.pdf");
    HPDF_UA_FreeContext (ctx);
    HPDF_Free (pdf);

    printf ("wrote tagged_pdfa_demo.pdf -- a real, tagged PDF/A-3B + "
            "PDF/UA-1 document with an ICC output intent and an "
            "embedded XML attachment (pdf_a_conformance.c port). "
            "Validate with validate/run_verapdf.sh.\n");
    return 0;

fail:
    fprintf (stderr, "tagged_pdfa_demo: a tagging call failed, see above\n");
    HPDF_UA_FreeContext (ctx);
    HPDF_Free (pdf);
    return 1;
}
