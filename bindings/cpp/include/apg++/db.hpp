// SPDX-License-Identifier: GPL-3.0-only
// SPDX-FileCopyrightText: 2026 AnmiTaliDev <anmitalidev@nuros.org>

#pragma once

#include "detail.hpp"
#include "error.hpp"
#include "package.hpp"

#include <apg/db.h>

#include <memory>
#include <optional>
#include <string>
#include <vector>

namespace apg
{

using DbStats = struct db_stats;

struct DbVerifyIssue
{
    std::string pkg_name;
    std::vector<std::string> missing_files;
};

class Database
{
public:
    static Database open(const std::string &path)
    {
        struct db_handle *h = ::db_open(path.c_str());
        if (!h)
            throw Error("db_open failed for '" + path + "'");
        return Database(h);
    }

    static Database open_readonly(const std::string &path)
    {
        struct db_handle *h = ::db_open_readonly(path.c_str());
        if (!h)
            throw Error("db_open_readonly failed for '" + path + "'");
        return Database(h);
    }

    struct db_handle *get() const noexcept { return db_.get(); }

    void set_hooks(const struct db_hooks *hooks)
    {
        ::db_set_hooks(db_.get(), hooks);
    }

    bool add(Package &pkg) { return ::db_add(db_.get(), pkg.get()); }

    bool remove(const std::string &pkg_name)
    {
        return ::db_remove(db_.get(), pkg_name.c_str());
    }

    bool set_hold(const std::string &pkg_name, bool held)
    {
        return ::db_set_hold(db_.get(), pkg_name.c_str(), held);
    }

    std::optional<Package> get(const std::string &name)
    {
        struct package *p = ::db_get(db_.get(), name.c_str());
        if (!p)
            return std::nullopt;
        return Package(p);
    }

    std::vector<Package> list()
    {
        int count = 0;
        struct package **raw = ::db_list(db_.get(), &count);
        return take_package_array(raw, count);
    }

    std::optional<std::string> owner(const std::string &path)
    {
        char *s = ::db_owner(db_.get(), path.c_str());
        if (!s)
            return std::nullopt;
        return detail::take_c_string(s);
    }

    DbStats stats()
    {
        DbStats out{};
        if (!::db_stats(db_.get(), &out))
            throw Error("db_stats failed");
        return out;
    }

    std::vector<std::string> get_orphans()
    {
        int count = 0;
        char **raw = ::db_get_orphans(db_.get(), &count);
        return detail::take_c_string_array(raw, count);
    }

    std::vector<Package> search(const std::string &query)
    {
        int count = 0;
        struct package **raw = ::db_search(db_.get(), query.c_str(), &count);
        return take_package_array(raw, count);
    }

    std::vector<std::string> get_dependents(const std::string &pkg_name)
    {
        int count = 0;
        char **raw = ::db_get_dependents(db_.get(), pkg_name.c_str(), &count);
        return detail::take_c_string_array(raw, count);
    }

    std::vector<DbVerifyIssue> verify(const std::string &root_path)
    {
        int count = 0;
        struct db_verify_issue *issues =
            ::db_verify(db_.get(), root_path.c_str(), &count);

        std::vector<DbVerifyIssue> result;
        result.reserve(static_cast<std::size_t>(count));
        for (int i = 0; i < count; ++i)
        {
            const struct db_verify_issue *issue =
                ::db_verify_issue_at(issues, count, i);
            DbVerifyIssue out;
            out.pkg_name = ::db_verify_issue_pkg_name(issue);
            int missing_count = ::db_verify_issue_missing_count(issue);
            out.missing_files.reserve(static_cast<std::size_t>(missing_count));
            for (int j = 0; j < missing_count; ++j)
                out.missing_files.emplace_back(
                    ::db_verify_issue_missing_file_at(issue, j));
            result.push_back(std::move(out));
        }
        ::db_verify_free(issues, count);
        return result;
    }

private:
    explicit Database(struct db_handle *raw) : db_(raw, &db_close) {}

    static std::vector<Package> take_package_array(struct package **raw,
                                                    int count)
    {
        std::vector<Package> result;
        if (!raw)
            return result;
        result.reserve(static_cast<std::size_t>(count));
        for (int i = 0; i < count; ++i)
            result.emplace_back(raw[i]);
        std::free(raw);
        return result;
    }

    std::unique_ptr<struct db_handle, void (*)(struct db_handle *)> db_;
};

} // namespace apg
