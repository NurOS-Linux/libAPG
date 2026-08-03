// SPDX-License-Identifier: GPL-3.0-only
// SPDX-FileCopyrightText: 2026 AnmiTaliDev <anmitalidev@nuros.org>

#pragma once

#include <apg/sign.h>

#include <string>

namespace apg
{

inline bool sign_verify(const std::string &pkg_path,
                         const std::string &sig_path)
{
    return ::sign_verify(pkg_path.c_str(), sig_path.c_str(), false);
}

inline bool sign_file(const std::string &pkg_path, const std::string &sig_path)
{
    return ::sign_file(pkg_path.c_str(), sig_path.c_str());
}

} // namespace apg
