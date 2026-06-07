// SPDX-License-Identifier: GPL-3.0-only
// SPDX-FileCopyrightText: 2026 Ruzen42
// SPDX-FileCopyrightText: 2026 AnmiTaliDev <anmitalidev@nuros.org>

#pragma once

/**
 * @file util.h
 * @brief General-purpose string and filesystem utilities.
 */

/**
 * @brief Concatenate two strings into a new heap-allocated string.
 *
 * @param str1 First string.
 * @param str2 Second string.
 * @return Heap-allocated result; caller must free(). Returns NULL on allocation
 *         failure.
 */
char *concat(const char *str1, const char *str2);

/**
 * @brief Create a directory, including any missing parent components.
 *
 * Equivalent to @c mkdir -p. Does nothing if the directory already exists.
 *
 * @param path Directory path to create.
 */
void create_dir(const char *path);

/**
 * @brief Join two path components with a directory separator.
 *
 * Inserts a @c '/' between @p path1 and @p path2 if not already present.
 *
 * @param path1 First path component.
 * @param path2 Second path component.
 * @return Heap-allocated joined path; caller must free(). Returns NULL on
 *         allocation failure.
 */
char *concat_dirs(const char *path1, const char *path2);

/**
 * @brief Collect all regular file paths rooted at a directory.
 *
 * @param base  Root directory to walk.
 * @param count Output parameter set to the number of collected paths.
 * @return Heap-allocated array of heap-allocated path strings, or NULL on
 *         failure. Caller must free each string and the array itself.
 */
char **collect_files(const char *base, int *count);
