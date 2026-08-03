// SPDX-License-Identifier: GPL-3.0-only
// SPDX-FileCopyrightText: 2026 AnmiTaliDev <anmitalidev@nuros.org>

#pragma once

#include "detail.hpp"

#include <apg/scripts.h>

#include <string>

namespace apg
{

inline bool run_script(const std::string &pkg_dir, const std::string &name,
                        const std::string &root_path)
{
    return ::run_script(pkg_dir.c_str(), name.c_str(), root_path.c_str());
}

inline std::string scripts_store_path(const std::string &root_path,
                                       const std::string &pkg_name)
{
    return detail::take_c_string(
        ::scripts_store_path(root_path.c_str(), pkg_name.c_str()));
}

inline bool scripts_persist(const std::string &pkg_dir,
                             const std::string &root_path,
                             const std::string &pkg_name)
{
    return ::scripts_persist(pkg_dir.c_str(), root_path.c_str(),
                              pkg_name.c_str());
}

inline void scripts_persist_remove(const std::string &root_path,
                                    const std::string &pkg_name)
{
    ::scripts_persist_remove(root_path.c_str(), pkg_name.c_str());
}

} // namespace apg
