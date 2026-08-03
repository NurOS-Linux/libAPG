// SPDX-License-Identifier: GPL-3.0-only
// SPDX-FileCopyrightText: 2026 AnmiTaliDev <anmitalidev@nuros.org>

#pragma once

#include "detail.hpp"

#include <apg/version.h>

#include <string>

namespace apg
{

using VerOp = ver_op_t;

struct DepConstraint
{
    std::string name;
    VerOp op = VER_OP_ANY;
    std::string version;
};

inline DepConstraint dep_constraint_parse(const std::string &str)
{
    struct dep_constraint c = ::dep_constraint_parse(str.c_str());
    DepConstraint result{c.name ? c.name : "", c.op, c.version ? c.version : ""};
    ::dep_constraint_free(&c);
    return result;
}

inline std::string dep_constraint_to_str(const DepConstraint &c)
{
    struct dep_constraint raw{};
    raw.name = const_cast<char *>(c.name.c_str());
    raw.op = c.op;
    raw.version =
        c.version.empty() ? nullptr : const_cast<char *>(c.version.c_str());
    return detail::take_c_string(::dep_constraint_to_str(&raw));
}

inline int ver_compare(const std::string &a, const std::string &b)
{
    return ::ver_compare(a.c_str(), b.c_str());
}

inline bool ver_satisfies(const std::string &pkg_version, VerOp op,
                           const std::string &constraint_version)
{
    return ::ver_satisfies(pkg_version.c_str(), op,
                            constraint_version.empty()
                                ? nullptr
                                : constraint_version.c_str());
}

} // namespace apg
