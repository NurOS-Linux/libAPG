// SPDX-License-Identifier: GPL-3.0-only
// SPDX-FileCopyrightText: 2026 AnmiTaliDev <anmitalidev@nuros.org>

#pragma once

#include "detail.hpp"
#include "error.hpp"
#include "package.hpp"

#include <apg/archive.h>

#include <string>
#include <vector>

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

inline std::vector<std::string> list_archive_files(const std::string &path)
{
    char **files = nullptr;
    int count = 0;
    if (!::package_list_files(path.c_str(), &files, &count))
        throw Error("package_list_files failed for '" + path + "'");
    return detail::take_c_string_array(files, count);
}

} // namespace apg
