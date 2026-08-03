// SPDX-License-Identifier: GPL-3.0-only
// SPDX-FileCopyrightText: 2026 AnmiTaliDev <anmitalidev@nuros.org>

#pragma once

#include <cstdlib>
#include <string>
#include <vector>

namespace apg::detail
{

inline std::string take_c_string(char *s)
{
    if (!s)
        return {};
    std::string result(s);
    std::free(s);
    return result;
}

inline std::vector<std::string> take_c_string_array(char **arr, int count)
{
    std::vector<std::string> result;
    if (!arr)
        return result;
    result.reserve(static_cast<std::size_t>(count));
    for (int i = 0; i < count; ++i)
    {
        result.emplace_back(arr[i] ? arr[i] : "");
        std::free(arr[i]);
    }
    std::free(arr);
    return result;
}

} // namespace apg::detail
