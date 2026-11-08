// SPDX-License-Identifier: GPL-3.0-only
// SPDX-FileCopyrightText: 2026 AnmiTaliDev <anmitalidev@nuros.org>
#pragma once
#include <stdbool.h>
#include <stdint.h>

#define APG_SPACE_BLOCK 4096

bool apg_archive_measure(const char *archive_path, uint64_t *extracted,
                         uint64_t *installed);
