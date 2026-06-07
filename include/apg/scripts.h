// SPDX-License-Identifier: GPL-3.0-only
// SPDX-FileCopyrightText: 2026 AnmiTaliDev <anmitalidev@nuros.org>

#pragma once

/**
 * @file scripts.h
 * @brief Pre/post-install script execution.
 */

#include <stdbool.h>

/**
 * @brief Run a lifecycle script from a package's @c scripts/ directory.
 *
 * Executes @c pkg_dir/scripts/name if the file exists and is executable.
 * Common script names are @c pre-install, @c post-install, @c pre-remove,
 * and @c post-remove.
 *
 * @param pkg_dir Path to the extracted package directory.
 * @param name    Script filename (without path) to execute.
 * @return true if the script succeeded or does not exist.
 *         false if the script exists but exited with a non-zero status.
 */
bool run_script(const char *pkg_dir, const char *name);
