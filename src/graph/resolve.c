// SPDX-License-Identifier: GPL-3.0-only
// SPDX-FileCopyrightText: 2026 AnmiTaliDev <anmitalidev@nuros.org>

#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <pthread.h>

#include "graph_priv.h"
#include "../../include/apg/version.h"

#define STATE_UNVISITED 0
#define STATE_VISITING 1
#define STATE_VISITED 2

struct dfs_ctx
{
    const struct dep_graph *g;
    uint8_t *state;
    size_t *order;
    size_t order_count;
};

static dep_error_t
dfs(struct dfs_ctx *ctx, size_t idx)
{
    if (ctx->state[idx] == STATE_VISITED)
        return DEP_OK;
    if (ctx->state[idx] == STATE_VISITING)
        return DEP_ERR_CYCLE;

    ctx->state[idx] = STATE_VISITING;

    const struct package_metadata *pkg = ctx->g->nodes[idx]->pkg;
    for (int i = 0; i < pkg->dependencies.count; i++)
    {
        const struct dep_constraint *c = &pkg->dependencies.items[i];
        size_t dep_idx = dep_graph_lookup(ctx->g, c->name);
        if (dep_idx == SIZE_MAX)
            return DEP_ERR_MISSING;

        if (c->op != VER_OP_ANY)
        {
            const char *dep_ver = ctx->g->nodes[dep_idx]->pkg->version;
            if (!ver_satisfies(dep_ver, c->op, c->version))
                return DEP_ERR_VERSION;
        }

        dep_error_t err = dfs(ctx, dep_idx);
        if (err != DEP_OK)
            return err;
    }

    ctx->state[idx] = STATE_VISITED;
    ctx->order[ctx->order_count++] = idx;
    return DEP_OK;
}

dep_error_t
dep_graph_resolve(struct dep_graph *g, const char *pkg_name, char ***order,
                  size_t *count)
{
    if (!g || !pkg_name || !order || !count)
        return DEP_ERR_NOMEM;

    size_t root = dep_graph_lookup(g, pkg_name);
    if (root == SIZE_MAX)
        return DEP_ERR_MISSING;

    uint8_t *state = calloc(g->count, sizeof(*state));
    if (!state)
        return DEP_ERR_NOMEM;

    size_t *idx_order = malloc(g->count * sizeof(*idx_order));
    if (!idx_order)
    {
        free(state);
        return DEP_ERR_NOMEM;
    }

    struct dfs_ctx ctx = {g, state, idx_order, 0};
    dep_error_t err = dfs(&ctx, root);

    if (err == DEP_OK)
    {
        char **names = malloc(ctx.order_count * sizeof(*names));
        if (!names)
        {
            err = DEP_ERR_NOMEM;
        }
        else
        {
            for (size_t i = 0; i < ctx.order_count; i++)
                names[i] = g->nodes[ctx.order[i]]->name;
            *order = names;
            *count = ctx.order_count;
        }
    }

    free(state);
    free(idx_order);
    return err;
}

struct resolve_task
{
    const struct dep_graph *g;
    const char *pkg_name;
    char **order;
    size_t order_count;
    dep_error_t err;
};

static void *
resolve_worker(void *arg)
{
    struct resolve_task *t = arg;
    t->err = dep_graph_resolve((struct dep_graph *)t->g, t->pkg_name, &t->order,
                               &t->order_count);
    return NULL;
}

dep_error_t
dep_graph_resolve_parallel(const struct dep_graph *g, const char **pkg_names,
                           size_t count, char ***order, size_t *order_count)
{
    if (!g || !order || !order_count || (count > 0 && !pkg_names))
        return DEP_ERR_NOMEM;

    if (count == 0)
    {
        *order = NULL;
        *order_count = 0;
        return DEP_OK;
    }

    if (count == 1)
    {
        return dep_graph_resolve((struct dep_graph *)g, pkg_names[0], order,
                                 order_count);
    }

    struct resolve_task *tasks = calloc(count, sizeof(*tasks));
    pthread_t *threads = malloc(count * sizeof(*threads));
    if (!tasks || !threads)
    {
        free(tasks);
        free(threads);
        return DEP_ERR_NOMEM;
    }

    for (size_t i = 0; i < count; i++)
    {
        tasks[i].g = g;
        tasks[i].pkg_name = pkg_names[i];
        if (pthread_create(&threads[i], NULL, resolve_worker, &tasks[i]) != 0)
        {
            tasks[i].err =
                dep_graph_resolve((struct dep_graph *)g, pkg_names[i],
                                  &tasks[i].order, &tasks[i].order_count);
            threads[i] = 0;
        }
    }

    dep_error_t first_err = DEP_OK;
    for (size_t i = 0; i < count; i++)
    {
        if (threads[i] != 0)
            pthread_join(threads[i], NULL);
        if (first_err == DEP_OK && tasks[i].err != DEP_OK)
            first_err = tasks[i].err;
    }

    free(threads);

    if (first_err != DEP_OK)
    {
        for (size_t i = 0; i < count; i++)
            free(tasks[i].order);
        free(tasks);
        return first_err;
    }

    size_t total_max = 0;
    for (size_t i = 0; i < count; i++)
        total_max += tasks[i].order_count;

    char **merged = malloc(total_max * sizeof(*merged));
    if (!merged)
    {
        for (size_t i = 0; i < count; i++)
            free(tasks[i].order);
        free(tasks);
        return DEP_ERR_NOMEM;
    }

    struct str_map seen = {0};
    if (!str_map_init(&seen))
    {
        for (size_t i = 0; i < count; i++)
            free(tasks[i].order);
        free(tasks);
        free(merged);
        return DEP_ERR_NOMEM;
    }

    size_t merged_count = 0;
    for (size_t i = 0; i < count; i++)
    {
        for (size_t j = 0; j < tasks[i].order_count; j++)
        {
            const char *name = tasks[i].order[j];
            size_t unused;
            if (!str_map_get(&seen, name, &unused))
            {
                if (!str_map_set(&seen, name, 1))
                {
                    str_map_free(&seen);
                    for (size_t k = i; k < count; k++)
                        free(tasks[k].order);
                    free(tasks);
                    free(merged);
                    return DEP_ERR_NOMEM;
                }
                merged[merged_count++] = (char *)name;
            }
        }
        free(tasks[i].order);
    }

    str_map_free(&seen);
    free(tasks);
    *order = merged;
    *order_count = merged_count;
    return DEP_OK;
}

bool
dep_graph_has_cycle(struct dep_graph *g)
{
    if (!g || g->count == 0)
        return false;

    uint8_t *state = calloc(g->count, sizeof(*state));
    if (!state)
        return false;

    size_t *dummy = malloc(g->count * sizeof(*dummy));
    if (!dummy)
    {
        free(state);
        return false;
    }

    struct dfs_ctx ctx = {g, state, dummy, 0};
    bool cycle = false;

    for (size_t i = 0; i < g->count && !cycle; i++)
    {
        if (state[i] == STATE_UNVISITED && dfs(&ctx, i) == DEP_ERR_CYCLE)
            cycle = true;
    }

    free(state);
    free(dummy);
    return cycle;
}
