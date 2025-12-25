// NurOS Ruzen42 2025 apg/package.h
// Last change: Dec 25

#ifndef APG_PACKAGE_H
#define APG_PACKAGE_H

#include <stdbool.h>
#include <stdio.h>
#include <stddef.h>

/* items is an array of heap-allocated strings, not NULL-terminated.
 * Ownership belongs to package_metadata.
 */

struct str_list {
    char **items;
    size_t count; // I used size_t instead of int because int is unstable in sizes
};

struct package_metadata {
    char *name;
    char *version;
    char *architecture;
	char *description;
	char *maintainer;
	char *license;
	char *homepage;
	struct str_list dependencies;
	struct str_list conflicts;
	struct str_list provides;
	struct str_list replaces;
	struct str_list tags;
};

struct package {
	struct package_metadata *meta;
	char *pkg_path;
	struct str_list package_files;
	bool installed_by_hand;
};

void str_list_free(struct str_list *);

void package_metadata_free(struct package_metadata *);

void package_free(struct package *);

struct package *package_new(void);
struct package_metadata *package_metadata_new(void);
void install_package(struct package *pkg, char *path);

#endif
