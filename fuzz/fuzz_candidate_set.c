// SPDX-License-Identifier: GPL-3.0-only
// SPDX-FileCopyrightText: 2026 AnmiTaliDev <anmitalidev@nuros.org>

#include <stdint.h>
#include <stdlib.h>
#include <string.h>

#include <apg/package.h>
#include "../src/graph/candidates_priv.h"

#define FUZZ_DUMMY_PKG_COUNT 4

int
LLVMFuzzerTestOneInput(const uint8_t *data, size_t size)
{
    struct package_metadata dummy_pkgs[FUZZ_DUMMY_PKG_COUNT] = {0};

    struct candidate_set cs;
    if (!candidate_set_init(&cs))
        return 0;

    size_t pos = 0;
    while (pos < size)
    {
        size_t len = data[pos] % 64;
        pos++;
        if (pos + len > size)
            len = size - pos;

        char *name = malloc(len + 1);
        if (!name)
            break;
        memcpy(name, data + pos, len);
        name[len] = '\0';
        pos += len;

        uint8_t pkg_idx = pos < size ? data[pos] % FUZZ_DUMMY_PKG_COUNT : 0;
        if (pos < size)
            pos++;

        if (strlen(name) > 0)
        {
            candidate_set_add(&cs, name, &dummy_pkgs[pkg_idx]);
            candidate_set_lookup(&cs, name);
        }

        free(name);
    }

    candidate_set_free(&cs);
    return 0;
}
