// SPDX-License-Identifier: GPL-3.0-only
// SPDX-FileCopyrightText: 2026 AnmiTaliDev <anmitalidev@nuros.org>

#pragma once

#include "error.hpp"

#include <apg/keyring.h>

#include <memory>
#include <string>

namespace apg
{

class Keyring
{
public:
    static Keyring load(const std::string &keyring_dir)
    {
        struct keyring *k = ::keyring_load(keyring_dir.c_str());
        if (!k)
            throw Error("keyring_load failed for '" + keyring_dir + "'");
        return Keyring(k);
    }

    struct keyring *get() const noexcept { return kr_.get(); }

    bool verify(const std::string &pkg_path, const std::string &sig_path) const
    {
        return ::keyring_verify(kr_.get(), pkg_path.c_str(), sig_path.c_str());
    }

    static bool add_key(const std::string &keyring_dir,
                        const std::string &new_key_path,
                        const std::string &key_sig_path,
                        const Keyring &trusted)
    {
        return ::keyring_add_key(keyring_dir.c_str(), new_key_path.c_str(),
                                  key_sig_path.c_str(), trusted.get());
    }

private:
    explicit Keyring(struct keyring *raw) : kr_(raw, &keyring_free) {}

    std::unique_ptr<struct keyring, void (*)(struct keyring *)> kr_;
};

} // namespace apg
