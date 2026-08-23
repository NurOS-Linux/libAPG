// SPDX-License-Identifier: GPL-3.0-only
// SPDX-FileCopyrightText: 2026 AnmiTaliDev <anmitalidev@nuros.org>

#pragma once

#include <stddef.h>
#include "sat_model_priv.h"

enum sat_result
{
    SAT_RESULT_SATISFIABLE,
    SAT_RESULT_UNSATISFIABLE,
    SAT_RESULT_BUDGET_EXCEEDED,
    SAT_RESULT_ERROR,
};

enum sat_result sat_solve(const struct sat_model *m, size_t decision_budget,
                          int **out_assignment, char **out_conflict);
