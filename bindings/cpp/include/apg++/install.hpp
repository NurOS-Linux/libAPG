// SPDX-License-Identifier: GPL-3.0-only
// SPDX-FileCopyrightText: 2026 AnmiTaliDev <anmitalidev@nuros.org>

#pragma once

#include <apg/install.h>

#include <string>

namespace apg
{

inline bool install_data_dir(const std::string &pkg_dir,
                              const std::string &root_path)
{
    return ::install_data_dir(pkg_dir.c_str(), root_path.c_str());
}

inline bool install_home_dir(const std::string &pkg_dir)
{
    return ::install_home_dir(pkg_dir.c_str());
}

inline void rollback_install(const std::string &pkg_dir,
                              const std::string &root_path)
{
    ::rollback_install(pkg_dir.c_str(), root_path.c_str());
}

} // namespace apg
