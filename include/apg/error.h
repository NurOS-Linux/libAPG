// SPDX-License-Identifier: GPL-3.0-only
// SPDX-FileCopyrightText: 2026 AnmiTaliDev <anmitalidev@nuros.org>

#pragma once

/**
 * @file error.h
 * @brief Human-readable detail for the most recent libapg failure.
 */

#include "export.h"

#ifdef __cplusplus
extern "C"
{
#endif

    /**
     * @brief Describe the most recent libapg failure on this thread.
     *
     * Functions that fail because of an operating system, database, archive,
     * or script error record a message here that names the failing object
     * and the underlying cause (for example the @c strerror() text for
     * @c ENOSPC). Only meaningful immediately after a libapg function
     * reports failure; a successful call does not necessarily clear it. The
     * returned pointer is valid until the next libapg call on the same
     * thread.
     *
     * @return Error message, or NULL if no failure has been recorded.
     */
    APG_API const char *apg_last_error(void);

#ifdef __cplusplus
}
#endif
