// SPDX-License-Identifier: GPL-3.0-only
// SPDX-FileCopyrightText: 2026 AnmiTaliDev <anmitalidev@nuros.org>

#pragma once

#include "db.hpp"
#include "journal.hpp"

#include <apg/audit.h>

#include <cstdlib>
#include <ctime>
#include <string>
#include <vector>

namespace apg
{

namespace detail
{

inline std::vector<JournalEntry> take_journal_entries(struct journal_entry **raw,
                                                      int count)
{
    std::vector<JournalEntry> result;
    if (!raw)
        return result;
    result.reserve(static_cast<std::size_t>(count));
    for (int i = 0; i < count; ++i)
        result.emplace_back(raw[i]);
    std::free(raw);
    return result;
}

} // namespace detail

inline std::vector<JournalEntry> audit_read_since(Database &db, time_t since)
{
    int count = 0;
    struct journal_entry **raw = ::audit_read_since(db.get(), since, &count);
    return detail::take_journal_entries(raw, count);
}

inline std::vector<JournalEntry> audit_read_for_package(
    Database &db, const std::string &pkg_name)
{
    int count = 0;
    struct journal_entry **raw =
        ::audit_read_for_package(db.get(), pkg_name.c_str(), &count);
    return detail::take_journal_entries(raw, count);
}

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
