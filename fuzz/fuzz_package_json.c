// SPDX-License-Identifier: GPL-3.0-only
// SPDX-FileCopyrightText: 2026 AnmiTaliDev <anmitalidev@nuros.org>

#include <stddef.h>
#include <stdint.h>

#include <apg/json.h>
#include <apg/package.h>

int
LLVMFuzzerTestOneInput(const uint8_t *data, size_t size)
{
    struct package_metadata *meta =
        package_metadata_from_json((const char *)data, size);
    package_metadata_free(meta);
    return 0;
}
