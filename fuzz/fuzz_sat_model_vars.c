// SPDX-License-Identifier: GPL-3.0-only
// SPDX-FileCopyrightText: 2026 AnmiTaliDev <anmitalidev@nuros.org>

#include <stdint.h>
#include <stdlib.h>

#include <apg/package.h>
#include "../src/graph/sat_model_priv.h"
#include "../src/graph/sat_solve_priv.h"

#define FUZZ_DUMMY_PKG_COUNT 32

int
LLVMFuzzerTestOneInput(const uint8_t *data, size_t size)
{
    struct package_metadata dummy_pkgs[FUZZ_DUMMY_PKG_COUNT] = {0};

    struct sat_model m;
    if (!sat_model_init(&m))
        return 0;

    for (size_t i = 0; i < size; i++)
    {
        uint8_t idx = data[i] % FUZZ_DUMMY_PKG_COUNT;
        if (data[i] & 0x80)
            sat_model_force(&m, &dummy_pkgs[idx]);
        else
            sat_model_var(&m, &dummy_pkgs[idx]);
    }

    int *assignment = NULL;
    char *conflict = NULL;
    enum sat_result r = sat_solve(&m, 2000, &assignment, &conflict);
    if (r == SAT_RESULT_SATISFIABLE)
        free(assignment);
    free(conflict);

    sat_model_free(&m);
    return 0;
}
