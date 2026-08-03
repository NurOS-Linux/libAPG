// SPDX-License-Identifier: GPL-3.0-only
// SPDX-FileCopyrightText: 2026 AnmiTaliDev <anmitalidev@nuros.org>

#pragma once

#include <apg/journal.h>

#include <memory>
#include <string>

namespace apg
{

class JournalEntry
{
public:
    explicit JournalEntry(struct journal_entry *raw)
        : ptr_(raw, &journal_entry_free)
    {
    }

    struct journal_entry *get() const noexcept { return ptr_.get(); }

    journal_op_t op() const { return ::journal_entry_op(ptr_.get()); }

    std::string pkg_name() const
    {
        const char *s = ::journal_entry_pkg_name(ptr_.get());
        return s ? s : "";
    }

    std::string pkg_version() const
    {
        const char *s = ::journal_entry_pkg_version(ptr_.get());
        return s ? s : "";
    }

    time_t timestamp() const { return ::journal_entry_timestamp(ptr_.get()); }
    journal_status_t status() const { return ::journal_entry_status(ptr_.get()); }
    uid_t uid() const { return ::journal_entry_uid(ptr_.get()); }
    bool is_explicit() const { return ::journal_entry_explicit(ptr_.get()); }

private:
    std::unique_ptr<struct journal_entry, void (*)(struct journal_entry *)>
        ptr_;
};

} // namespace apg
