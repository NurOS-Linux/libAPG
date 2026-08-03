// SPDX-License-Identifier: GPL-3.0-only
// SPDX-FileCopyrightText: 2026 AnmiTaliDev <anmitalidev@nuros.org>

#pragma once

#include <apg/copy.h>

#include <string>

namespace apg
{

inline bool copy_dir(const std::string &src, const std::string &dst)
{
    return ::copy_dir(src.c_str(), dst.c_str());
}

} // namespace apg
