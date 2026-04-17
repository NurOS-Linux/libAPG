// SPDX-License-Identifier: GPL-3.0-only
// SPDX-FileCopyrightText: 2026 Ruzen42
// SPDX-FileCopyrightText: 2026 AnmiTaliDev <anmitalidev@nuros.org>
#pragma once

#include "package.h"

// Returns a heap-allocated JSON string. Caller must free().
char *package_to_json(struct package *pkg);

struct package_metadata *package_metadata_from_file(const char *path);
