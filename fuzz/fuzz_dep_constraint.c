// SPDX-License-Identifier: GPL-3.0-only
// SPDX-FileCopyrightText: 2026 AnmiTaliDev <anmitalidev@nuros.org>

#include <stdint.h>
#include <stdlib.h>
#include <string.h>

#include <apg/version.h>

int
LLVMFuzzerTestOneInput(const uint8_t *data, size_t size)
{
    char *str = malloc(size + 1);
    if (!str)
        return 0;
    memcpy(str, data, size);
    str[size] = '\0';

    struct dep_constraint c = dep_constraint_parse(str);
    dep_constraint_free(&c);

    free(str);
    return 0;
}
