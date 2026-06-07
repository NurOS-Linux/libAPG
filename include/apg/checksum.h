// SPDX-License-Identifier: GPL-3.0-only
// SPDX-FileCopyrightText: 2026 AnmiTaliDev <anmitalidev@nuros.org>

#pragma once

/**
 * @file checksum.h
 * @brief High-level package checksum verification.
 */

#include <stdbool.h>

/**
 * @brief Verify the integrity of all files in an extracted package directory.
 *
 * Looks for a @c crc32sums file first; falls back to @c md5sums if
 * @c crc32sums is absent. Returns false when neither file exists or when any
 * recorded checksum does not match the corresponding file on disk.
 *
 * @param pkg_dir Path to the extracted package directory.
 * @return true if all checksums match, false on any mismatch or missing
 *         checksum file.
 */
bool verify_checksums(const char *pkg_dir);
