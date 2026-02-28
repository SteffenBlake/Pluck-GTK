/**
 * search.h — Fuzzy-match highlighting helpers.
 *
 * Provides a single public function that, given a file-path string and the
 * user's current query, returns a Pango markup string with matching
 * characters highlighted in bold gold.
 */

#ifndef PLUCK_SEARCH_H
#define PLUCK_SEARCH_H

#include <glib.h>

/**
 * create_highlighted_markup:
 * @text:  The plain-text string to annotate (e.g. a file path).
 * @query: The search string typed by the user.
 *
 * Returns a newly-allocated Pango markup string.  Where @query appears as
 * a contiguous substring of @text (case-insensitive) the matching characters
 * are wrapped in a bold, gold-coloured &lt;span&gt;.  When no exact run is
 * found the function falls back to highlighting every character of @text
 * that also appears anywhere in @query.
 *
 * The caller is responsible for freeing the returned string with g_free().
 */
char *create_highlighted_markup(const char *text, const char *query);

#endif /* PLUCK_SEARCH_H */
