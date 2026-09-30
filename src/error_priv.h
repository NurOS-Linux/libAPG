// SPDX-License-Identifier: GPL-3.0-only
// SPDX-FileCopyrightText: 2026 AnmiTaliDev <anmitalidev@nuros.org>

#pragma once

#include "../include/apg/error.h"

#if defined(__GNUC__) || defined(__clang__)
#define APG_PRINTF_FORMAT __attribute__((format(printf, 1, 2)))
#else
#define APG_PRINTF_FORMAT
#endif

void apg_set_error(const char *fmt, ...) APG_PRINTF_FORMAT;

void apg_clear_error(void);
