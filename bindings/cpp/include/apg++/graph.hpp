// SPDX-License-Identifier: GPL-3.0-only
// SPDX-FileCopyrightText: 2026 AnmiTaliDev <anmitalidev@nuros.org>

#pragma once

#include "error.hpp"
#include "package.hpp"

#include <apg/graph.h>

#include <memory>
#include <string>
#include <vector>

namespace apg
{

using DepError = dep_error_t;

class DependencyGraph
{
public:
    DependencyGraph() : g_(make(), &dep_graph_free)
    {
        if (!g_)
            throw Error("dep_graph_new failed");
    }

    DepError add(const PackageMetadataView &meta)
    {
        return ::dep_graph_add(
            g_.get(), const_cast<struct package_metadata *>(meta.get()));
    }

    DepError add_installed(const PackageMetadataView &meta)
    {
        return ::dep_graph_add_installed(
            g_.get(), const_cast<struct package_metadata *>(meta.get()));
    }

    bool has_cycle() { return ::dep_graph_has_cycle(g_.get()); }

    DepError resolve(const std::string &pkg_name,
                      std::vector<std::string> &order)
    {
        char **raw_order = nullptr;
        std::size_t count = 0;
        DepError err =
            ::dep_graph_resolve(g_.get(), pkg_name.c_str(), &raw_order, &count);
        order = borrowed_to_vector(raw_order, count);
        std::free(raw_order);
        return err;
    }

    DepError resolve_parallel(const std::vector<std::string> &pkg_names,
                              std::vector<std::string> &order)
    {
        std::vector<const char *> c_names;
        c_names.reserve(pkg_names.size());
        for (const auto &n : pkg_names)
            c_names.push_back(n.c_str());

        char **raw_order = nullptr;
        std::size_t order_count = 0;
        DepError err = ::dep_graph_resolve_parallel(
            g_.get(), c_names.data(), c_names.size(), &raw_order, &order_count);
        order = borrowed_to_vector(raw_order, order_count);
        std::free(raw_order);
        return err;
    }

    DepError find_breaks(const std::string &pkg_name,
                          const std::vector<std::string> &installed,
                          std::vector<std::string> &breaks)
    {
        std::vector<const char *> c_installed;
        c_installed.reserve(installed.size());
        for (const auto &n : installed)
            c_installed.push_back(n.c_str());

        char **raw_breaks = nullptr;
        std::size_t break_count = 0;
        DepError err = ::dep_graph_find_breaks(
            g_.get(), pkg_name.c_str(), c_installed.data(), c_installed.size(),
            &raw_breaks, &break_count);
        breaks = borrowed_to_vector(raw_breaks, break_count);
        std::free(raw_breaks);
        return err;
    }

private:
    static struct dep_graph *make() { return ::dep_graph_new(); }

    static std::vector<std::string> borrowed_to_vector(char **arr,
                                                        std::size_t count)
    {
        std::vector<std::string> result;
        if (!arr)
            return result;
        result.reserve(count);
        for (std::size_t i = 0; i < count; ++i)
            result.emplace_back(arr[i] ? arr[i] : "");
        return result;
    }

    std::unique_ptr<struct dep_graph, void (*)(struct dep_graph *)> g_;
};

} // namespace apg
