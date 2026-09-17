/*
 * tagged_encryption_demo.c -- Milestone 6: tagged port merging libharu's
 * original encryption.c (owner/user password protection) and
 * permission.c (restricting print/copy permissions). Merged into one
 * demo, same reasoning as tagged_text_demo.c: both originals exercise
 * the same real HPDF_Encrypt code path, just with different settings.
 *
 * A real, non-obvious correctness point this port had to get right (not
 * present in either original demo, which don't target PDF/UA-1 at all):
 * PDF's permission bitfield reserves several high bits, including bit 9
 * ("extract for accessibility", PDF 32000-1 Table 22) -- readers and
 * assistive technology are expected to always be able to extract content
 * for accessibility regardless of the copy-protection bit. libharu's own
 * HPDF_Encrypt_Init() defaults `permission` to (flags | HPDF_PERMISSION_PAD),
 * where HPDF_PERMISSION_PAD (0xFFFFFFC0) keeps all the reserved/high bits
 * set to 1 as the PDF spec requires -- but HPDF_SetPermission() REPLACES
 * `permission` outright (confirmed by reading hpdf_doc.c's
 * HPDF_SetPermission() directly: `e->permission = permission;`, no
 * OR-with-the-existing-value), so a caller that does what the original
 * permission.c does -- `HPDF_SetPermission(pdf, HPDF_ENABLE_READ)` --
 * silently clears those reserved bits back to 0, including the
 * accessibility-extraction bit. This port ORs in HPDF_PERMISSION_PAD
 * explicitly to avoid that; see the call below.
 *
 * Structure: Document > H1, Document > P (the visible page text). No
 * special annotation/structure-tree interaction beyond that -- encryption
 * is a document-level, not content-level, feature.
 *
 * Validate with: validate/run_verapdf.sh build/tagged_encryption_demo.pdf
 */
#include <stdio.h>
#include "hpdf_encrypt.h" /* HPDF_PERMISSION_PAD -- not pulled in by hpdf.h itself */
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

    pdf = HPDF_New (error_handler, NULL);
    if (!pdf) {
        fprintf (stderr, "failed to create PDF document\n");
        return 1;
    }

    HPDF_SetInfoAttr (pdf, HPDF_INFO_TITLE, "libharu-pdfua tagged-encryption demo");
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
        HPDF_Outline outline = HPDF_CreateOutline (pdf, NULL, "Encryption", NULL);
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
    HPDF_Page_ShowText (page, "Encryption Demo");
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
    HPDF_Page_SetFontAndSize (page, font, 14);
    HPDF_Page_BeginText (page);
    HPDF_Page_MoveTextPos (page, 50, 700);
    HPDF_Page_ShowText (page, "This document is encrypted (owner/user password)");
    HPDF_Page_MoveTextPos (page, 0, -20);
    HPDF_Page_ShowText (page, "and printing/copying are disabled for the user password.");
    HPDF_Page_EndText (page);
    status = HPDF_UA_EndMarkedContent (ctx, page);
    if (status != HPDF_OK)
        goto fail;
    HPDF_UA_EndStructureElement (ctx, p_elem);

    HPDF_UA_EndStructureElement (ctx, doc_elem);

    HPDF_SetPassword (pdf, "owner", "user");
    /* HPDF_ENABLE_READ (0) alone would silently clear the reserved
     * permission bits (see this file's own top comment) -- OR in
     * HPDF_PERMISSION_PAD explicitly so the accessibility-extraction bit
     * (and the other PDF-spec-reserved bits) stay set. */
    HPDF_SetPermission (pdf, HPDF_ENABLE_READ | HPDF_PERMISSION_PAD);
    HPDF_SetEncryptionMode (pdf, HPDF_ENCRYPT_R3, 16);

    HPDF_SaveToFile (pdf, "tagged_encryption_demo.pdf");
    HPDF_UA_FreeContext (ctx);
    HPDF_Free (pdf);

    printf ("wrote tagged_encryption_demo.pdf -- a real tagged, "
            "password-protected, permission-restricted document "
            "(encryption.c + permission.c port). Validate with "
            "validate/run_verapdf.sh.\n");
    return 0;

fail:
    fprintf (stderr, "tagged_encryption_demo: a tagging call failed, see above\n");
    HPDF_UA_FreeContext (ctx);
    HPDF_Free (pdf);
    return 1;
}
