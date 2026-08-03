// SPDX-License-Identifier: GPL-3.0-only
// SPDX-FileCopyrightText: 2026 AnmiTaliDev <anmitalidev@nuros.org>

#pragma once

#include <apg/config.h>

#include <string>

namespace apg
{

struct InstallPolicy
{
    bool require_signature = false;
    std::string keyring_dir;

    install_policy to_c() const
    {
        install_policy raw{};
        raw.require_signature = require_signature;
        raw.keyring_dir =
            keyring_dir.empty() ? nullptr : const_cast<char *>(keyring_dir.c_str());
        return raw;
    }
};

} // namespace apg
