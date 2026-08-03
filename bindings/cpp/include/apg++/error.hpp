// SPDX-License-Identifier: GPL-3.0-only
// SPDX-FileCopyrightText: 2026 AnmiTaliDev <anmitalidev@nuros.org>

#pragma once

#include <stdexcept>
#include <string>

namespace apg
{

class Error : public std::runtime_error
{
public:
    explicit Error(const std::string &what) : std::runtime_error(what) {}
};

} // namespace apg
