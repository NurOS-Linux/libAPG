// SPDX-License-Identifier: GPL-3.0-only
// SPDX-FileCopyrightText: 2026 AnmiTaliDev <anmitalidev@nuros.org>

#pragma once

/**
 * @file export.h
 * @brief Public symbol visibility macro.
 */

#if defined(__GNUC__) || defined(__clang__)
#define APG_API __attribute__((visibility("default")))
#else
#define APG_API
#endif
