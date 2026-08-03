// SPDX-License-Identifier: GPL-3.0-only
// SPDX-FileCopyrightText: 2026 AnmiTaliDev <anmitalidev@nuros.org>

#pragma once

#include "package.hpp"

#include <apg/archive.h>

#include <string>

namespace apg
{

inline bool unarchive(const Package &pkg)
{
    return ::unarchive_package(pkg.get());
}

inline bool unarchive_in_root(const Package &pkg, const std::string &root)
{
    return ::unarchive_package_in_root(pkg.get(), root.c_str());
}

} // namespace apg
