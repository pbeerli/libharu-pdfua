/*
 * tagged_multipage_table_demo.c -- a tagged table that runs over several
 * pages, drawn with the tag stack (HPDF_UA_Stack, 0.9.0).
 *
 * Structure tree:
 *   Document > H1, P,
 *              Table > TR > TH x4, TR > TD x4 (one TR per data row),
 *              Sect > H2, P, P..., Figure, P...
 *
 * What it shows (see docs/api.md, "Multi-page documents and tables"):
 * - Running header, page number and rules are Pagination/Layout
 *   artifacts, drawn while nothing is tagged on the new page yet.
 * - The table header row is real TH content once, on the first page;
 *   on every continuation page it is redrawn as a Pagination artifact,
 *   so the table keeps one header row in the structure tree.
 * - Each row is pushed, drawn and popped before the code advances to the
 *   next line, so a page break never happens inside an open TR. A cell
 *   split across pages would add an extra cell to its row (elements are
 *   linked into their parent's /K when pushed), which veraPDF reports as
 *   7.2/42.
 * - The notes paragraph is drawn line by line and DOES cross a page:
 *   HPDF_UA_Stack_SuspendForNewPage()/ResumeAfterNewPage() end the P on
 *   the old page and continue in a new sibling P on the new one.
 * - A figure in the middle of the notes uses the same-page pair
 *   (HPDF_UA_Stack_SuspendContent()/ResumeContent()) to close the
 *   paragraph around it, and may itself start a new page in between.
 * - Numbers are right-aligned by measuring them with
 *   HPDF_Page_TextWidth(), never by padding with spaces, and no string
 *   drawn with HPDF_Page_TextOut() carries a trailing newline.
 *
 * Validate with:
 *   validate/run_verapdf.sh build/tagged_multipage_table_demo.pdf
 */
#include <stdio.h>
#include <string.h>
#include <math.h>
#include "hpdf_ua/hpdf_ua.h"

#define ROWS  96
#define COLS  4

#define PAGE_TOP     720.0f
#define PAGE_BOTTOM   72.0f
#define LEFT          72.0f
#define TEXT_WIDTH   468.0f
#define ROW_HEIGHT    16.0f
#define LINE_HEIGHT   14.0f

static const char *HEADERS[COLS] = { "Locus", "Theta", "M", "log L" };
static const HPDF_REAL COL_RIGHT[COLS] = { 120, 230, 340, 460 };  /* right
                                                  * edge of each column,
                                                  * relative to LEFT */

static const char *NOTES =
    "This table was drawn with the tag stack of libharu-pdfua. Every row is "
    "pushed as a TR element, its four cells are pushed, drawn and popped, "
    "and the row is popped before the drawing position moves down. Because "
    "a structure element is linked into its parent when it is pushed, a "
    "page break inside an open row would leave that row with one extra "
    "cell, and a checker would report that the rows of the table do not "
    "have the same number of columns. Closing the row before advancing "
    "avoids this without any special case. The header row of the table is "
    "tagged once, on the first page. On each continuation page it is drawn "
    "again for the sighted reader, but marked as a pagination artifact, so "
    "a screen reader does not announce it again in the middle of the data. "
    "Running headers, page numbers and the rules under the header are "
    "artifacts as well. "
    "This paragraph itself is long enough to run onto the next page. The "
    "stack closes the paragraph at the bottom of one page and continues "
    "the text in a new paragraph element on the next page, because the "
    "content of one structure element may live on one page only. The small "
    "figure below interrupts the paragraph in the same way: the paragraph "
    "is suspended, the figure is tagged with its own alternative text, and "
    "a new paragraph element continues the text after it. The two kinds of "
    "interruption keep separate saved state, since the figure may itself "
    "be pushed onto a new page between suspend and resume. "
    "Numbers in the table are right-aligned by measuring their width, "
    "never by drawing padding spaces, and no drawn string ends in a "
    "newline character, which would otherwise be drawn as a glyph without "
    "a Unicode meaning. All text uses an embedded TrueType font, since the "
    "Standard 14 fonts are not embedded and fail the PDF/UA font rules. "
    "When an existing layout was tuned for Helvetica or Courier, a "
    "metric-compatible font such as Liberation Sans or Liberation Mono "
    "keeps every line where it was.";

typedef struct {
    HPDF_Doc      pdf;
    HPDF_Font     font;
    HPDF_Page     page;
    HPDF_UA_Stack stack;
    HPDF_REAL     y;
    int           page_number;
} Demo;

static void
error_handler (HPDF_STATUS error_no, HPDF_STATUS detail_no, void *user_data)
{
    (void) user_data;
    fprintf (stderr, "libharu error: error_no=0x%04lX detail_no=%ld\n",
            (unsigned long) error_no, (long) detail_no);
}

static void
text_at (Demo *d, HPDF_REAL size, HPDF_REAL x, HPDF_REAL y, const char *s)
{
    HPDF_Page_SetFontAndSize (d->page, d->font, size);
    HPDF_Page_BeginText (d->page);
    HPDF_Page_TextOut (d->page, x, y, s);
    HPDF_Page_EndText (d->page);
}

static void
text_right (Demo *d, HPDF_REAL size, HPDF_REAL right, HPDF_REAL y,
            const char *s)
{
    HPDF_Page_SetFontAndSize (d->page, d->font, size);
    text_at (d, size, right - HPDF_Page_TextWidth (d->page, s), y, s);
}

/* The header row as it is drawn; tagging is the caller's business. */
static void
draw_header_cells (Demo *d, HPDF_REAL y, int col)
{
    text_right (d, 10, LEFT + COL_RIGHT[col], y, HEADERS[col]);
}

static HPDF_STATUS
draw_header_rule (Demo *d, HPDF_REAL y)
{
    HPDF_STATUS status = HPDF_UA_Stack_BeginArtifact (d->stack, d->page,
            HPDF_UA_ARTIFACT_LAYOUT);
    if (status != HPDF_OK)
        return status;
    HPDF_Page_SetLineWidth (d->page, 0.5);
    HPDF_Page_MoveTo (d->page, LEFT, y - 4);
    HPDF_Page_LineTo (d->page, LEFT + COL_RIGHT[COLS - 1], y - 4);
    HPDF_Page_Stroke (d->page);
    return HPDF_UA_Stack_EndArtifact (d->stack);
}

/* Starts a new page: whatever is open is suspended, the running header,
 * page number and rule are drawn as artifacts, then the suspended
 * content (if any) resumes as a new sibling on the new page. */
static HPDF_STATUS
new_page (Demo *d)
{
    char number[32];
    HPDF_STATUS status;

    status = HPDF_UA_Stack_SuspendForNewPage (d->stack);
    if (status != HPDF_OK)
        return status;

    d->page = HPDF_AddPage (d->pdf);
    HPDF_Page_SetSize (d->page, HPDF_PAGE_SIZE_LETTER, HPDF_PAGE_PORTRAIT);
    d->page_number++;

    status = HPDF_UA_Stack_BeginArtifact (d->stack, d->page,
            HPDF_UA_ARTIFACT_PAGINATION);
    if (status != HPDF_OK)
        return status;
    text_at (d, 8, LEFT, 752, "libharu-pdfua: multi-page table demo");
    snprintf (number, sizeof number, "Page %d", d->page_number);
    text_right (d, 8, LEFT + TEXT_WIDTH, 40, number);
    status = HPDF_UA_Stack_EndArtifact (d->stack);
    if (status != HPDF_OK)
        return status;

    status = HPDF_UA_Stack_BeginArtifact (d->stack, d->page,
            HPDF_UA_ARTIFACT_LAYOUT);
    if (status != HPDF_OK)
        return status;
    HPDF_Page_SetLineWidth (d->page, 0.5);
    HPDF_Page_MoveTo (d->page, LEFT, 746);
    HPDF_Page_LineTo (d->page, LEFT + TEXT_WIDTH, 746);
    HPDF_Page_Stroke (d->page);
    status = HPDF_UA_Stack_EndArtifact (d->stack);
    if (status != HPDF_OK)
        return status;

    d->y = PAGE_TOP;
    return HPDF_UA_Stack_ResumeAfterNewPage (d->stack, d->page);
}

/* TRUE if `height` more points do not fit on the current page. */
static HPDF_BOOL
would_break (Demo *d, HPDF_REAL height)
{
    return (HPDF_BOOL) (d->y - height < PAGE_BOTTOM);
}

/* Moves down by `height`, starting a new page first if needed. */
static HPDF_STATUS
make_room (Demo *d, HPDF_REAL height)
{
    if (would_break (d, height))
        return new_page (d);
    return HPDF_OK;
}

static HPDF_STATUS
draw_title (Demo *d)
{
    HPDF_STATUS status;

    if (!HPDF_UA_Stack_Push (d->stack, HPDF_UA_ROLE_H1))
        return HPDF_INVALID_OPERATION;
    status = HPDF_UA_Stack_BeginContent (d->stack, d->page);
    if (status != HPDF_OK)
        return status;
    text_at (d, 18, LEFT, d->y, "Multi-page Table Demo");
    status = HPDF_UA_Stack_Pop (d->stack);
    d->y -= 28;
    if (status != HPDF_OK)
        return status;

    if (!HPDF_UA_Stack_Push (d->stack, HPDF_UA_ROLE_P))
        return HPDF_INVALID_OPERATION;
    status = HPDF_UA_Stack_BeginContent (d->stack, d->page);
    if (status != HPDF_OK)
        return status;
    text_at (d, 10, LEFT, d->y,
             "Simulated per-locus estimates; the table continues over "
             "several pages.");
    status = HPDF_UA_Stack_Pop (d->stack);
    d->y -= 2 * LINE_HEIGHT;
    return status;
}

static HPDF_STATUS
draw_table (Demo *d)
{
    HPDF_STATUS status;
    char cell[COLS][32];
    int row, col;

    if (!HPDF_UA_Stack_Push (d->stack, HPDF_UA_ROLE_TABLE))
        return HPDF_INVALID_OPERATION;

    /* the one real header row */
    if (!HPDF_UA_Stack_Push (d->stack, HPDF_UA_ROLE_TR))
        return HPDF_INVALID_OPERATION;
    for (col = 0; col < COLS; col++) {
        if (!HPDF_UA_Stack_Push (d->stack, HPDF_UA_ROLE_TH))
            return HPDF_INVALID_OPERATION;
        status = HPDF_UA_Stack_SetTableHeaderScope (d->stack,
                HPDF_UA_SCOPE_COLUMN);
        if (status == HPDF_OK)
            status = HPDF_UA_Stack_BeginContent (d->stack, d->page);
        if (status != HPDF_OK)
            return status;
        draw_header_cells (d, d->y, col);
        status = HPDF_UA_Stack_Pop (d->stack);   /* ends the span too */
        if (status != HPDF_OK)
            return status;
    }
    status = HPDF_UA_Stack_Pop (d->stack);      /* TR */
    if (status == HPDF_OK)
        status = draw_header_rule (d, d->y);
    if (status != HPDF_OK)
        return status;
    d->y -= ROW_HEIGHT + 4;

    for (row = 0; row < ROWS; row++) {
        /* Nothing is open below the Table here, so a page break now
         * splits nothing. */
        if (would_break (d, ROW_HEIGHT)) {
            status = new_page (d);
            if (status != HPDF_OK)
                return status;
            status = HPDF_UA_Stack_BeginArtifact (d->stack, d->page,
                    HPDF_UA_ARTIFACT_PAGINATION);
            if (status != HPDF_OK)
                return status;
            for (col = 0; col < COLS; col++)
                draw_header_cells (d, d->y, col);
            status = HPDF_UA_Stack_EndArtifact (d->stack);
            if (status == HPDF_OK)
                status = draw_header_rule (d, d->y);
            if (status != HPDF_OK)
                return status;
            d->y -= ROW_HEIGHT + 4;
        }

        snprintf (cell[0], sizeof cell[0], "%d", row + 1);
        snprintf (cell[1], sizeof cell[1], "%.5f",
                  0.01 + 0.004 * sin (0.7 * row));
        snprintf (cell[2], sizeof cell[2], "%.2f",
                  1.5 + 0.6 * cos (0.3 * row));
        snprintf (cell[3], sizeof cell[3], "%.2f",
                  -1234.5 - 37.25 * (row % 11) - 3.5 * row);

        if (row % 2 == 1) {
            status = HPDF_UA_Stack_BeginArtifact (d->stack, d->page,
                    HPDF_UA_ARTIFACT_LAYOUT);
            if (status != HPDF_OK)
                return status;
            HPDF_Page_SetGrayFill (d->page, 0.92);
            HPDF_Page_Rectangle (d->page, LEFT, d->y - 4,
                    COL_RIGHT[COLS - 1] + 4, ROW_HEIGHT);
            HPDF_Page_Fill (d->page);
            HPDF_Page_SetGrayFill (d->page, 0.0);
            status = HPDF_UA_Stack_EndArtifact (d->stack);
            if (status != HPDF_OK)
                return status;
        }

        if (!HPDF_UA_Stack_Push (d->stack, HPDF_UA_ROLE_TR))
            return HPDF_INVALID_OPERATION;
        for (col = 0; col < COLS; col++) {
            if (!HPDF_UA_Stack_Push (d->stack, HPDF_UA_ROLE_TD))
                return HPDF_INVALID_OPERATION;
            status = HPDF_UA_Stack_BeginContent (d->stack, d->page);
            if (status != HPDF_OK)
                return status;
            text_right (d, 10, LEFT + COL_RIGHT[col], d->y, cell[col]);
            status = HPDF_UA_Stack_Pop (d->stack);
            if (status != HPDF_OK)
                return status;
        }
        status = HPDF_UA_Stack_Pop (d->stack);  /* TR closed ... */
        if (status != HPDF_OK)
            return status;
        d->y -= ROW_HEIGHT;                     /* ... before advancing */
    }

    status = HPDF_UA_Stack_Pop (d->stack);      /* Table */
    d->y -= ROW_HEIGHT;
    return status;
}

static HPDF_STATUS
draw_figure (Demo *d)
{
    const HPDF_REAL height = 70;
    HPDF_STATUS status;
    int i;

    /* The paragraph is suspended; this may start a new page, which finds
     * nothing open to suspend -- and must not lose the paragraph's own
     * saved state, which is why the two pairs keep separate slots. */
    status = make_room (d, height + LINE_HEIGHT);
    if (status != HPDF_OK)
        return status;

    if (!HPDF_UA_Stack_Push (d->stack, HPDF_UA_ROLE_FIGURE))
        return HPDF_INVALID_OPERATION;
    status = HPDF_UA_Stack_SetAlternateText (d->stack,
            "Bar chart of Theta for loci 1 to 12: values oscillate between "
            "about 0.006 and 0.014.");
    if (status == HPDF_OK)
        status = HPDF_UA_Stack_BeginContent (d->stack, d->page);
    if (status != HPDF_OK)
        return status;
    HPDF_Page_SetRGBFill (d->page, 0.2, 0.4, 0.7);
    for (i = 0; i < 12; i++) {
        HPDF_REAL theta = 0.01 + 0.004 * sin (0.7 * i);
        HPDF_Page_Rectangle (d->page, LEFT + 20 + 22 * i, d->y - height,
                14, theta * 4000);
    }
    HPDF_Page_Fill (d->page);
    HPDF_Page_SetGrayFill (d->page, 0.0);
    status = HPDF_UA_Stack_Pop (d->stack);
    d->y -= height + LINE_HEIGHT;
    return status;
}

/* Word-wraps NOTES into lines of at most TEXT_WIDTH points and draws them
 * one by one inside a P, which is continued across page breaks by the
 * stack and interrupted once by a figure. */
static HPDF_STATUS
draw_notes (Demo *d)
{
    const char *p = NOTES;
    char line[256];
    HPDF_STATUS status;
    int lines = 0;

    status = make_room (d, 3 * LINE_HEIGHT);
    if (status != HPDF_OK)
        return status;
    if (!HPDF_UA_Stack_Push (d->stack, HPDF_UA_ROLE_SECT))
        return HPDF_INVALID_OPERATION;
    if (!HPDF_UA_Stack_Push (d->stack, HPDF_UA_ROLE_H2))
        return HPDF_INVALID_OPERATION;
    status = HPDF_UA_Stack_BeginContent (d->stack, d->page);
    if (status != HPDF_OK)
        return status;
    text_at (d, 13, LEFT, d->y, "Notes");
    status = HPDF_UA_Stack_Pop (d->stack);
    if (status != HPDF_OK)
        return status;
    d->y -= 1.5 * LINE_HEIGHT;

    if (!HPDF_UA_Stack_Push (d->stack, HPDF_UA_ROLE_P))
        return HPDF_INVALID_OPERATION;
    status = HPDF_UA_Stack_BeginContent (d->stack, d->page);
    if (status != HPDF_OK)
        return status;

    while (*p) {
        size_t len = 0, fit = 0;

        /* measure at the drawing size: a page break in between draws the
         * footer at another size */
        HPDF_Page_SetFontAndSize (d->page, d->font, 10);

        while (p[len] && len < sizeof line - 1) {
            size_t next = len;
            while (p[next] == ' ')
                next++;
            while (p[next] && p[next] != ' ' && next < sizeof line - 1)
                next++;
            memcpy (line, p, next);
            line[next] = '\0';
            if (HPDF_Page_TextWidth (d->page, line) > TEXT_WIDTH && fit > 0)
                break;
            fit = len = next;
        }
        memcpy (line, p, fit);
        line[fit] = '\0';
        p += fit;
        while (*p == ' ')
            p++;

        status = make_room (d, LINE_HEIGHT);   /* may split the P */
        if (status != HPDF_OK)
            return status;
        text_at (d, 10, LEFT, d->y, line);
        d->y -= LINE_HEIGHT;

        if (++lines == 6) {
            d->y -= LINE_HEIGHT / 2;
            status = HPDF_UA_Stack_SuspendContent (d->stack);
            if (status == HPDF_OK)
                status = draw_figure (d);
            if (status == HPDF_OK)
                status = HPDF_UA_Stack_ResumeContent (d->stack, d->page);
            if (status != HPDF_OK)
                return status;
        }
    }

    status = HPDF_UA_Stack_Pop (d->stack);      /* P */
    if (status == HPDF_OK)
        status = HPDF_UA_Stack_Pop (d->stack);  /* Sect */
    return status;
}

int
main (void)
{
    Demo d;
    HPDF_UA_Context ctx;
    const char *font_name;
    HPDF_STATUS status;

    memset (&d, 0, sizeof d);
    d.pdf = HPDF_New (error_handler, NULL);
    if (!d.pdf) {
        fprintf (stderr, "failed to create PDF document\n");
        return 1;
    }

    HPDF_SetInfoAttr (d.pdf, HPDF_INFO_TITLE,
                      "libharu-pdfua multi-page table demo");
    HPDF_UA_SetDocumentLanguage (d.pdf, "en-US");
    HPDF_UA_SetDisplayDocTitle (d.pdf, HPDF_TRUE);
    if (HPDF_UA_AddMetadata (d.pdf) != HPDF_OK) {
        fprintf (stderr, "HPDF_UA_AddMetadata failed\n");
        HPDF_Free (d.pdf);
        return 1;
    }

    ctx = HPDF_UA_NewContext (d.pdf);
    if (!ctx) {
        fprintf (stderr, "HPDF_UA_NewContext failed\n");
        HPDF_Free (d.pdf);
        return 1;
    }
    d.stack = HPDF_UA_NewStack (ctx);
    if (!d.stack) {
        fprintf (stderr, "HPDF_UA_NewStack failed\n");
        HPDF_UA_FreeContext (ctx);
        HPDF_Free (d.pdf);
        return 1;
    }

    font_name = HPDF_LoadTTFontFromFile (d.pdf, HPDF_UA_DEMO_FONT_PATH,
                                         HPDF_TRUE);
    if (!font_name) {
        fprintf (stderr, "HPDF_LoadTTFontFromFile failed\n");
        goto fail;
    }
    d.font = HPDF_GetFont (d.pdf, font_name, "WinAnsiEncoding");

    if (!HPDF_UA_Stack_Push (d.stack, HPDF_UA_ROLE_DOCUMENT))
        goto fail;

    status = new_page (&d);
    if (status == HPDF_OK)
        status = draw_title (&d);
    if (status == HPDF_OK)
        status = draw_table (&d);
    if (status == HPDF_OK)
        status = draw_notes (&d);
    if (status != HPDF_OK)
        goto fail;

    HPDF_UA_FreeStack (d.stack);                /* ends Document */
    HPDF_SaveToFile (d.pdf, "tagged_multipage_table_demo.pdf");
    HPDF_UA_FreeContext (ctx);
    HPDF_Free (d.pdf);

    printf ("wrote tagged_multipage_table_demo.pdf -- %d pages, a tagged "
            "table with repeated header artifacts and a paragraph split "
            "across pages. Validate with validate/run_verapdf.sh.\n",
            d.page_number);
    return 0;

fail:
    fprintf (stderr, "tagged_multipage_table_demo: a tagging call failed, "
             "see above\n");
    HPDF_UA_FreeStack (d.stack);
    HPDF_UA_FreeContext (ctx);
    HPDF_Free (d.pdf);
    return 1;
}
