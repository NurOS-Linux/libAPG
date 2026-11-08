// SPDX-License-Identifier: GPL-3.0-only
// SPDX-FileCopyrightText: 2026 AnmiTaliDev <anmitalidev@nuros.org>

#pragma once

#include "db.hpp"
#include "journal.hpp"

#include <apg/audit.h>

#include <cstdlib>
#include <vector>

namespace apg
{

inline std::vector<JournalEntry> audit_read_all(Database &db)
{
    int count = 0;
    struct journal_entry **raw = ::audit_read_all(db.get(), &count);

    std::vector<JournalEntry> result;
    if (!raw)
        return result;
    result.reserve(static_cast<std::size_t>(count));
    for (int i = 0; i < count; ++i)
        result.emplace_back(raw[i]);
    std::free(raw);
    return result;
}

} // namespace apg
