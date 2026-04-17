// SPDX-License-Identifier: GPL-3.0-only
// SPDX-FileCopyrightText: 2026 AnmiTaliDev <anmitalidev@nuros.org>
#pragma once

#include <stdbool.h>

bool install_data_dir(const char *pkg_dir, const char *root_path);
bool install_home_dir(const char *pkg_dir);
