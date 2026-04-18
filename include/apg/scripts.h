// SPDX-License-Identifier: GPL-3.0-only
// SPDX-FileCopyrightText: 2026 AnmiTaliDev <anmitalidev@nuros.org>
#pragma once

#include <stdbool.h>

// Runs pkg_dir/scripts/<name> if it exists and is executable.
// Returns true if the script succeeded or does not exist.
// Returns false if the script exists but exited with a non-zero code.
bool run_script(const char *pkg_dir, const char *name);
