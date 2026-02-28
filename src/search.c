/**
 * search.c — Fuzzy-match highlighting implementation.
 *
 * Two-pass algorithm:
 *   1. Try to find every contiguous run of @query inside @text
 *      (case-insensitive).  If at least one run is found, mark exactly those
 *      character positions for highlighting.
 *   2. If no contiguous run exists (pure fuzzy match), fall back to
 *      highlighting every character in @text whose lowercase form also
 *      appears anywhere in @query.
 *
 * The result is a Pango markup string ready to hand to gtk_label_set_markup().
 */

#include "search.h"

#include <string.h>
#include <ctype.h>
#include <glib.h>

/* -------------------------------------------------------------------------
 * Internal helpers
 * ---------------------------------------------------------------------- */

/**
 * char_in_query:
 * @c:     A character from the text being annotated.
 * @query: The user's search string (already lower-cased at call site).
 *
 * Returns TRUE when the lower-case form of @c appears somewhere in @query.
 * Used only in the fuzzy-fallback path.
 */
static gboolean char_in_query(char c, const char *query)
{
    char lc = (char)tolower((unsigned char)c);
    for (const char *q = query; *q; q++) {
        if ((char)tolower((unsigned char)*q) == lc)
            return TRUE;
    }
    return FALSE;
}

/**
 * find_exact_matches:
 * @text:      The string being searched.
 * @query:     The substring to locate (matched case-insensitively).
 * @matches:   Caller-allocated boolean array of length @text_len.  Each
 *             position that belongs to at least one occurrence of @query is
 *             set to TRUE.
 * @text_len:  Number of characters in @text (not counting NUL).
 *
 * Returns TRUE when at least one occurrence of @query was found in @text.
 */
static gboolean find_exact_matches(const char *text,
                                   const char *query,
                                   gboolean   *matches,
                                   int         text_len)
{
    int query_len = (int)strlen(query);
    if (query_len == 0)
        return FALSE;

    gboolean found_any = FALSE;

    for (int i = 0; i <= text_len - query_len; i++) {
        if (strncasecmp(&text[i], query, (size_t)query_len) == 0) {
            for (int j = 0; j < query_len; j++)
                matches[i + j] = TRUE;
            found_any = TRUE;
        }
    }

    return found_any;
}

/* -------------------------------------------------------------------------
 * Public API
 * ---------------------------------------------------------------------- */

char *create_highlighted_markup(const char *text, const char *query)
{
    GString *markup   = g_string_new("");
    int      text_len = (int)strlen(text);

    /* Allocate a per-character highlight map. */
    gboolean *exact_matches = g_new0(gboolean, text_len);
    gboolean  has_exact     = find_exact_matches(text, query,
                                                 exact_matches, text_len);

    for (int i = 0; i < text_len; i++) {
        char  ch      = text[i];
        char *escaped = g_markup_escape_text(&ch, 1);

        gboolean highlight = has_exact
            ? exact_matches[i]
            : char_in_query(ch, query);

        if (highlight) {
            g_string_append_printf(markup,
                "<span weight='bold' foreground='#FFD700'>%s</span>",
                escaped);
        } else {
            g_string_append(markup, escaped);
        }

        g_free(escaped);
    }

    g_free(exact_matches);
    return g_string_free(markup, FALSE);
}
