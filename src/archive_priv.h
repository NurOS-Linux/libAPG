// SPDX-License-Identifier: GPL-3.0-only
// SPDX-FileCopyrightText: 2026 AnmiTaliDev <anmitalidev@nuros.org>
#pragma once
#include <stdbool.h>
#include <stdint.h>

#define APG_SPACE_BLOCK 4096

struct archive;

struct archive *apg_archive_open_read(const char *archive_path);
bool apg_archive_is_metadata(const char *name);
const char *apg_archive_data_relative(const char *name);
bool apg_archive_is_safe_relative(const char *path);

bool apg_archive_measure(const char *archive_path, uint64_t *extracted,
                         uint64_t *installed);
