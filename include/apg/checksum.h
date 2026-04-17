// SPDX-License-Identifier: GPL-3.0-only
// SPDX-FileCopyrightText: 2026 AnmiTaliDev <anmitalidev@nuros.org>
#pragma once

#include <stdbool.h>

// Finds and verifies md5sums or crc32sums inside pkg_dir.
// Tries crc32sums first, falls back to md5sums.
// Returns false if neither file exists or any checksum mismatches.
bool verify_checksums(const char *pkg_dir);
