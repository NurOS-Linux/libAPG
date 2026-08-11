// SPDX-License-Identifier: GPL-3.0-only
// SPDX-FileCopyrightText: 2026 AnmiTaliDev <anmitalidev@nuros.org>

#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "graph_priv.h"
#include "../../include/apg/version.h"

struct dot_buf
{
    char *data;
    size_t len;
    size_t cap;
};

static bool
dot_reserve(struct dot_buf *buf, size_t extra)
{
    if (buf->len + extra + 1 <= buf->cap)
        return true;
    size_t new_cap = buf->cap == 0 ? 256 : buf->cap;
    while (new_cap < buf->len + extra + 1)
        new_cap *= 2;
    char *tmp = realloc(buf->data, new_cap);
    if (!tmp)
        return false;
    buf->data = tmp;
    buf->cap = new_cap;
    return true;
}

static bool
dot_append(struct dot_buf *buf, const char *s)
{
    size_t n = strlen(s);
    if (!dot_reserve(buf, n))
        return false;
    memcpy(buf->data + buf->len, s, n);
    buf->len += n;
    buf->data[buf->len] = '\0';
    return true;
}

static bool
dot_appendf(struct dot_buf *buf, const char *fmt, ...)
{
    va_list args;
    va_start(args, fmt);
    va_list args_copy;
    va_copy(args_copy, args);
    // NOLINTNEXTLINE(clang-analyzer-valist.Uninitialized)
    int needed = vsnprintf(NULL, 0, fmt, args_copy);
    va_end(args_copy);
    if (needed < 0)
    {
        va_end(args);
        return false;
    }

    bool ok = dot_reserve(buf, (size_t)needed);
    if (ok)
    {
        (void)vsnprintf(buf->data + buf->len, (size_t)needed + 1, fmt, args);
        buf->len += (size_t)needed;
    }
    va_end(args);
    return ok;
}

static char *
dot_escape(const char *s)
{
    size_t extra = 0;
    for (const char *p = s; *p; p++)
        if (*p == '"' || *p == '\\')
            extra++;

    char *out = malloc(strlen(s) + extra + 1);
    if (!out)
        return NULL;

    char *o = out;
    for (const char *p = s; *p; p++)
    {
        if (*p == '"' || *p == '\\')
            *o++ = '\\';
        *o++ = *p;
    }
    *o = '\0';
    return out;
}

static bool
emit_nodes(struct dot_buf *buf, const struct dep_graph *g)
{
    for (size_t i = 0; i < g->count; i++)
    {
        char *name = dot_escape(g->nodes[i]->name);
        if (!name)
            return false;
        bool ok =
            dot_appendf(buf, "    \"%s\" [style=filled, fillcolor=\"%s\"];\n",
                        name, g->nodes[i]->installed ? "lightgreen" : "white");
        free(name);
        if (!ok)
            return false;
    }
    return true;
}

static bool
emit_dependency_edges(struct dot_buf *buf, const struct dep_graph *g)
{
    for (size_t i = 0; i < g->count; i++)
    {
        const struct dep_node *node = g->nodes[i];
        const struct package_metadata *pkg = node->pkg;
        char *from = dot_escape(node->name);
        if (!from)
            return false;

        for (int j = 0; j < pkg->dependencies.count; j++)
        {
            const struct dep_constraint *c = &pkg->dependencies.items[j];
            size_t dep_idx = dep_graph_lookup(g, c->name);

            if (dep_idx == SIZE_MAX)
            {
                char *to = dot_escape(c->name);
                bool ok =
                    to && dot_appendf(buf,
                                      "    \"%s\" -> \"%s\" [color=red, "
                                      "style=dashed, label=\"missing\"];\n",
                                      from, to);
                free(to);
                if (!ok)
                {
                    free(from);
                    return false;
                }
                continue;
            }

            char *to = dot_escape(g->nodes[dep_idx]->name);
            if (!to)
            {
                free(from);
                return false;
            }

            bool ok;
            if (c->op != VER_OP_ANY)
            {
                char *constraint = dep_constraint_to_str(c);
                char *label = constraint ? dot_escape(constraint) : NULL;
                free(constraint);
                ok = label &&
                     dot_appendf(buf, "    \"%s\" -> \"%s\" [label=\"%s\"];\n",
                                 from, to, label);
                free(label);
            }
            else
            {
                ok = dot_appendf(buf, "    \"%s\" -> \"%s\";\n", from, to);
            }
            free(to);
            if (!ok)
            {
                free(from);
                return false;
            }
        }

        free(from);
    }
    return true;
}

static bool
emit_providers(struct dot_buf *buf, const struct dep_graph *g)
{
    for (size_t i = 0; i < g->alias_group_count; i++)
    {
        const struct alias_group *group = &g->alias_groups[i];

        char *alias = dot_escape(group->alias);
        if (!alias)
            return false;
        if (!dot_appendf(buf, "    \"%s\" [shape=diamond, style=dashed];\n",
                         alias))
        {
            free(alias);
            return false;
        }

        size_t resolved = dep_graph_lookup(g, group->alias);
        for (size_t k = 0; k < group->provider_count; k++)
        {
            size_t node_idx = group->providers[k];
            char *provider = dot_escape(g->nodes[node_idx]->name);
            bool ok =
                provider &&
                dot_appendf(buf, "    \"%s\" -> \"%s\" [style=%s];\n", alias,
                            provider, node_idx == resolved ? "bold" : "dashed");
            free(provider);
            if (!ok)
            {
                free(alias);
                return false;
            }
        }

        free(alias);
    }
    return true;
}

char *
dep_graph_export_dot(const struct dep_graph *g)
{
    if (!g)
        return NULL;

    struct dot_buf buf = {0};

    if (!dot_append(&buf, "digraph libapg_deps\n{\n") ||
        !dot_append(&buf, "    rankdir=LR;\n") ||
        !dot_append(&buf, "    node [shape=box, fontname=\"sans-serif\"];\n\n"))
        goto fail;

    if (!emit_nodes(&buf, g))
        goto fail;
    if (!dot_append(&buf, "\n"))
        goto fail;

    if (!emit_dependency_edges(&buf, g))
        goto fail;
    if (!dot_append(&buf, "\n"))
        goto fail;

    if (!emit_providers(&buf, g))
        goto fail;

    if (!dot_append(&buf, "}\n"))
        goto fail;

    return buf.data;

fail:
    free(buf.data);
    return NULL;
}
