// SPDX-License-Identifier: GPL-3.0-only
// SPDX-FileCopyrightText: 2026 AnmiTaliDev <anmitalidev@nuros.org>

#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <stdio.h>

#include "../../include/apg/version.h"

static char *
trim_strndup(const char *s, size_t n)
{
    while (n > 0 && (*s == ' ' || *s == '\t'))
    {
        s++;
        n--;
    }
    while (n > 0 && (s[n - 1] == ' ' || s[n - 1] == '\t'))
        n--;
    return strndup(s, n);
}

static char *
trim_strdup(const char *s)
{
    return trim_strndup(s, strlen(s));
}

struct dep_constraint
dep_constraint_parse(const char *str)
{
    struct dep_constraint c = {NULL, VER_OP_ANY, NULL};
    if (!str)
        return c;

    static const struct
    {
        const char *sym;
        ver_op_t op;
    } ops[] = {
        {">=", VER_OP_GE},  {"<=", VER_OP_LE}, {"==", VER_OP_EQ},
        {"!=", VER_OP_NEQ}, {">", VER_OP_GT},  {"<", VER_OP_LT},
    };

    const char *op_start = NULL;
    ver_op_t op = VER_OP_ANY;
    size_t op_len = 0;

    for (size_t i = 0; i < sizeof(ops) / sizeof(ops[0]); i++)
    {
        const char *p = strstr(str, ops[i].sym);
        if (p)
        {
            op_start = p;
            op = ops[i].op;
            op_len = strlen(ops[i].sym);
            break;
        }
    }

    if (!op_start)
    {
        c.name = trim_strdup(str);
        return c;
    }

    c.name = trim_strndup(str, (size_t)(op_start - str));
    c.op = op;
    c.version = trim_strdup(op_start + op_len);
    return c;
}

char *
dep_constraint_to_str(const struct dep_constraint *c)
{
    if (!c || !c->name)
        return NULL;
    if (c->op == VER_OP_ANY || !c->version)
        return strdup(c->name);

    static const char *op_strs[] = {
        [VER_OP_EQ] = "==", [VER_OP_NEQ] = "!=", [VER_OP_LT] = "<",
        [VER_OP_LE] = "<=", [VER_OP_GT] = ">",   [VER_OP_GE] = ">=",
    };

    const char *op_str = (c->op < (int)(sizeof(op_strs) / sizeof(op_strs[0])))
                             ? op_strs[c->op]
                             : "?";

    size_t len = strlen(c->name) + strlen(op_str) + strlen(c->version) + 5;
    char *buf = malloc(len);
    if (buf)
        snprintf(buf, len, "%s %s %s", c->name, op_str, c->version);
    return buf;
}

void
dep_constraint_free(struct dep_constraint *c)
{
    if (!c)
        return;
    free(c->name);
    free(c->version);
    c->name = NULL;
    c->version = NULL;
}

void
dep_constraint_list_free(struct dep_constraint_list *list)
{
    if (!list)
        return;
    for (int i = 0; i < list->count; i++)
        dep_constraint_free(&list->items[i]);
    free(list->items);
    list->items = NULL;
    list->count = 0;
}

int
ver_compare(const char *a, const char *b)
{
    if (!a && !b)
        return 0;
    if (!a)
        return -1;
    if (!b)
        return 1;

    long ea = 0, eb = 0;
    const char *ca = strchr(a, ':');
    const char *cb = strchr(b, ':');
    if (ca)
    {
        ea = strtol(a, NULL, 10);
        a = ca + 1;
    }
    if (cb)
    {
        eb = strtol(b, NULL, 10);
        b = cb + 1;
    }
    if (ea != eb)
        return (ea > eb) ? 1 : -1;

    while (*a || *b)
    {
        while (*a == '.')
            a++;
        while (*b == '.')
            b++;

        if (!*a && !*b)
            break;

        char *ae = (char *)a, *be = (char *)b;
        long va = *a ? strtol(a, &ae, 10) : 0;
        long vb = *b ? strtol(b, &be, 10) : 0;

        if (ae == a && be == b)
        {
            if (*a != *b)
                return (unsigned char)*a - (unsigned char)*b;
            if (*a)
            {
                a++;
                b++;
            }
            continue;
        }

        if (ae == a)
            va = 0;
        else
            a = ae;
        if (be == b)
            vb = 0;
        else
            b = be;

        if (va != vb)
            return (va > vb) ? 1 : -1;
    }
    return 0;
}

bool
ver_satisfies(const char *pkg_version, ver_op_t op,
              const char *constraint_version)
{
    if (op == VER_OP_ANY)
        return true;
    if (!pkg_version || !constraint_version)
        return false;

    int cmp = ver_compare(pkg_version, constraint_version);
    switch (op)
    {
    case VER_OP_EQ:
        return cmp == 0;
    case VER_OP_NEQ:
        return cmp != 0;
    case VER_OP_LT:
        return cmp < 0;
    case VER_OP_LE:
        return cmp <= 0;
    case VER_OP_GT:
        return cmp > 0;
    case VER_OP_GE:
        return cmp >= 0;
    default:
        return true;
    }
}
