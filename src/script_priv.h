// SPDX-License-Identifier: GPL-3.0-only
// SPDX-FileCopyrightText: 2026 AnmiTaliDev <anmitalidev@nuros.org>
#pragma once
#include <stdbool.h>

struct package;

bool apg_run_script(const char *pkg_dir, const char *name,
                    const char *root_path, bool strict);
bool apg_install_package_in_root(struct package *pkg, const char *root_path,
                                 bool strict);
