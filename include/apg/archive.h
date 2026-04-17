// SPDX-License-Identifier: GPL-3.0-only
// SPDX-FileCopyrightText: 2026 Ruzen42
// SPDX-FileCopyrightText: 2026 AnmiTaliDev <anmitalidev@nuros.org>

#pragma once

#include "package.h"

#include <stdbool.h>

bool unarchive_package(const struct package *pkg);
bool unarchive_package_in_root(const struct package *pkg, const char *root);

