/* Smoke test for tests/test_install.sh: touches both halves of the
 * installed package (hpdf_ua's own API, plus a plain libharu call) to
 * confirm both libhpdf.a and libhpdf_ua.a resolve and link, and that
 * hpdf_ua.h's own `#include "hpdf.h"` finds the installed headers. */
#include <stdio.h>
#include <hpdf_ua/hpdf_ua.h>

int
main(void)
{
    HPDF_Doc pdf = HPDF_New(NULL, NULL);
    if (!pdf) {
        fprintf(stderr, "HPDF_New failed\n");
        return 1;
    }

    if (HPDF_UA_SetDocumentLanguage(pdf, "en-US") != HPDF_OK) {
        fprintf(stderr, "HPDF_UA_SetDocumentLanguage failed\n");
        HPDF_Free(pdf);
        return 1;
    }

    HPDF_UA_Context ctx = HPDF_UA_NewContext(pdf);
    if (!ctx) {
        fprintf(stderr, "HPDF_UA_NewContext failed\n");
        HPDF_Free(pdf);
        return 1;
    }

    HPDF_UA_FreeContext(ctx);
    HPDF_Free(pdf);
    printf("consume_test: OK\n");
    return 0;
}
