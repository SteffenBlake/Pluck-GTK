/**
 * config.h — Shared application configuration and global state.
 *
 * Declares globals that are defined in main.c and referenced by other
 * translation units (e.g. ui.c uses search_root when building the fd/fzf
 * command).
 */

#ifndef PLUCK_CONFIG_H
#define PLUCK_CONFIG_H

/** Maximum length of the search-root path (including NUL terminator). */
#define SEARCH_ROOT_MAX 1024

/**
 * Root directory passed to `fd` when scanning for files.
 * Defaults to "." (current working directory).
 * May be overridden via the first command-line argument.
 */
extern char search_root[SEARCH_ROOT_MAX];

#endif /* PLUCK_CONFIG_H */
