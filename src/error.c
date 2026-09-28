// SPDX-License-Identifier: GPL-3.0-only
// SPDX-FileCopyrightText: 2026 AnmiTaliDev <anmitalidev@nuros.org>

#include <stdarg.h>
#include <stdio.h>

#include "error_priv.h"

static _Thread_local char g_error[1024];

void
apg_set_error(const char *fmt, ...)
{
    va_list ap;
    va_start(ap, fmt);
    // NOLINTNEXTLINE(clang-analyzer-valist.Uninitialized)
    (void)vsnprintf(g_error, sizeof(g_error), fmt, ap);
    va_end(ap);
}

void
apg_clear_error(void)
{
    g_error[0] = '\0';
}

const char *
// NOLINTNEXTLINE(misc-use-internal-linkage)
apg_last_error(void)
{
    return g_error[0] ? g_error : NULL;
}
