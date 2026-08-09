// SPDX-License-Identifier: GPL-3.0-only
// SPDX-FileCopyrightText: 2026 AnmiTaliDev <anmitalidev@nuros.org>

#include <stdlib.h>
#include <string.h>

#include "db_priv.h"

bool
str_vec_push(struct str_vec *v, const char *str)
{
    if (v->count == v->cap)
    {
        int new_cap = v->cap ? v->cap * 2 : 8;
        char **tmp = realloc(v->items, new_cap * sizeof(char *));
        if (!tmp)
            return false;
        v->items = tmp;
        v->cap = new_cap;
    }

    char *copy = strdup(str);
    if (!copy)
        return false;

    v->items[v->count++] = copy;
    return true;
}
