// SPDX-License-Identifier: GPL-3.0-only
// SPDX-FileCopyrightText: 2026 AnmiTaliDev <anmitalidev@nuros.org>

#include <stdlib.h>
#include <stdint.h>

#include "graph_priv.h"
#include "../../include/apg/version.h"

#define STATE_UNVISITED 0
#define STATE_VISITING  1
#define STATE_VISITED   2

struct dfs_ctx {
    const struct dep_graph *g;
    uint8_t *state;
    size_t *order;
    size_t order_count;
};

static dep_error_t
dfs(struct dfs_ctx *ctx, size_t idx)
{
    if (ctx->state[idx] == STATE_VISITED) return DEP_OK;
    if (ctx->state[idx] == STATE_VISITING) return DEP_ERR_CYCLE;

    ctx->state[idx] = STATE_VISITING;

    const struct package_metadata *pkg = ctx->g->nodes[idx]->pkg;
    for (int i = 0; i < pkg->dependencies.count; i++) {
        const struct dep_constraint *c = &pkg->dependencies.items[i];
        size_t dep_idx = dep_graph_lookup(ctx->g, c->name);
        if (dep_idx == SIZE_MAX) return DEP_ERR_MISSING;

        if (c->op != VER_OP_ANY) {
            const char *dep_ver = ctx->g->nodes[dep_idx]->pkg->version;
            if (!ver_satisfies(dep_ver, c->op, c->version))
                return DEP_ERR_VERSION;
        }

        dep_error_t err = dfs(ctx, dep_idx);
        if (err != DEP_OK) return err;
    }

    ctx->state[idx] = STATE_VISITED;
    ctx->order[ctx->order_count++] = idx;
    return DEP_OK;
}

dep_error_t
dep_graph_resolve(struct dep_graph *g, const char *pkg_name,
                  char ***order, size_t *count)
{
    if (!g || !pkg_name || !order || !count) return DEP_ERR_NOMEM;

    size_t root = dep_graph_lookup(g, pkg_name);
    if (root == SIZE_MAX) return DEP_ERR_MISSING;

    uint8_t *state = calloc(g->count, sizeof(*state));
    if (!state) return DEP_ERR_NOMEM;

    size_t *idx_order = malloc(g->count * sizeof(*idx_order));
    if (!idx_order) { free(state); return DEP_ERR_NOMEM; }

    struct dfs_ctx ctx = { g, state, idx_order, 0 };
    dep_error_t err = dfs(&ctx, root);

    if (err == DEP_OK) {
        char **names = malloc(ctx.order_count * sizeof(*names));
        if (!names) {
            err = DEP_ERR_NOMEM;
        } else {
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

bool
dep_graph_has_cycle(struct dep_graph *g)
{
    if (!g || g->count == 0) return false;

    uint8_t *state = calloc(g->count, sizeof(*state));
    if (!state) return false;

    size_t *dummy = malloc(g->count * sizeof(*dummy));
    if (!dummy) { free(state); return false; }

    struct dfs_ctx ctx = { g, state, dummy, 0 };
    bool cycle = false;

    for (size_t i = 0; i < g->count && !cycle; i++) {
        if (state[i] == STATE_UNVISITED && dfs(&ctx, i) == DEP_ERR_CYCLE)
            cycle = true;
    }

    free(state);
    free(dummy);
    return cycle;
}
