// SPDX-License-Identifier: GPL-3.0-only
// SPDX-FileCopyrightText: 2026 AnmiTaliDev <anmitalidev@nuros.org>

#pragma once

#include "config.hpp"
#include "db.hpp"
#include "error.hpp"
#include "package.hpp"

#include <apg/transaction.h>

#include <memory>
#include <string>
#include <vector>

namespace apg
{

using TransOp = trans_op_t;
using TransError = trans_error_t;

class TransStepView
{
public:
    explicit TransStepView(const struct trans_step *ptr) : ptr_(ptr) {}
    TransOp op() const { return ::trans_step_op(ptr_); }
    std::string pkg_name() const { return ::trans_step_pkg_name(ptr_); }
    std::string pkg_version() const { return ::trans_step_pkg_version(ptr_); }
    bool is_explicit() const { return ::trans_step_explicit(ptr_); }

private:
    const struct trans_step *ptr_;
};

class TransConflictView
{
public:
    explicit TransConflictView(const struct trans_conflict *ptr) : ptr_(ptr) {}
    std::string pkg_name() const { return ::trans_conflict_pkg_name(ptr_); }
    std::string conflicts_with() const
    {
        return ::trans_conflict_conflicts_with(ptr_);
    }

private:
    const struct trans_conflict *ptr_;
};

class TransFileConflictView
{
public:
    explicit TransFileConflictView(const struct trans_file_conflict *ptr)
        : ptr_(ptr)
    {
    }
    std::string path() const { return ::trans_file_conflict_path(ptr_); }
    std::string requested_by() const
    {
        return ::trans_file_conflict_requested_by(ptr_);
    }
    std::string owned_by() const { return ::trans_file_conflict_owned_by(ptr_); }

private:
    const struct trans_file_conflict *ptr_;
};

class TransHeldPkgView
{
public:
    explicit TransHeldPkgView(const struct trans_held_pkg *ptr) : ptr_(ptr) {}
    std::string pkg_name() const { return ::trans_held_pkg_name(ptr_); }
    TransOp op() const { return ::trans_held_pkg_op(ptr_); }

private:
    const struct trans_held_pkg *ptr_;
};

class TransBlockedRemoveView
{
public:
    explicit TransBlockedRemoveView(const struct trans_blocked_remove *ptr)
        : ptr_(ptr)
    {
    }
    std::string pkg_name() const { return ::trans_blocked_remove_pkg_name(ptr_); }
    std::vector<std::string> dependents() const
    {
        int count = ::trans_blocked_remove_dependent_count(ptr_);
        std::vector<std::string> result;
        result.reserve(static_cast<std::size_t>(count));
        for (int i = 0; i < count; ++i)
            result.emplace_back(::trans_blocked_remove_dependent_at(ptr_, i));
        return result;
    }

private:
    const struct trans_blocked_remove *ptr_;
};

class Transaction
{
public:
    explicit Transaction(Database &db) : trans_(make(db), &trans_free)
    {
        if (!trans_)
            throw Error("trans_new failed");
    }

    struct apg_trans *get() const noexcept { return trans_.get(); }

    void set_policy(const InstallPolicy &policy)
    {
        install_policy raw = policy.to_c();
        ::trans_set_policy(trans_.get(), &raw);
    }

    void clear_policy() { ::trans_set_policy(trans_.get(), nullptr); }

    TransError add_install(Package &pkg)
    {
        return ::trans_add_install(trans_.get(), pkg.get());
    }

    TransError add_remove(const std::string &pkg_name)
    {
        return ::trans_add_remove(trans_.get(), pkg_name.c_str());
    }

    TransError add_upgrade(Package &pkg)
    {
        return ::trans_add_upgrade(trans_.get(), pkg.get());
    }

    TransError prepare() { return ::trans_prepare(trans_.get()); }

    std::vector<TransStepView> plan() const
    {
        return snapshot<TransStepView>(::trans_plan_count(trans_.get()),
                                        ::trans_plan_at, trans_.get());
    }

    std::vector<TransConflictView> conflicts() const
    {
        return snapshot<TransConflictView>(
            ::trans_conflict_count(trans_.get()), ::trans_conflict_at,
            trans_.get());
    }

    std::vector<TransBlockedRemoveView> blocked_removes() const
    {
        return snapshot<TransBlockedRemoveView>(
            ::trans_blocked_remove_count(trans_.get()),
            ::trans_blocked_remove_at, trans_.get());
    }

    std::vector<TransFileConflictView> file_conflicts() const
    {
        return snapshot<TransFileConflictView>(
            ::trans_file_conflict_count(trans_.get()),
            ::trans_file_conflict_at, trans_.get());
    }

    std::vector<TransHeldPkgView> held_pkgs() const
    {
        return snapshot<TransHeldPkgView>(::trans_held_pkg_count(trans_.get()),
                                           ::trans_held_pkg_at, trans_.get());
    }

    TransError commit(const std::string &root_path)
    {
        return ::trans_commit(trans_.get(), root_path.c_str());
    }

private:
    static struct apg_trans *make(Database &db) { return ::trans_new(db.get()); }

    template <typename View, typename CountType, typename Fn>
    static std::vector<View> snapshot(CountType count, Fn fn,
                                      const struct apg_trans *trans)
    {
        std::vector<View> result;
        result.reserve(static_cast<std::size_t>(count));
        for (CountType i = 0; i < count; ++i)
            result.emplace_back(fn(trans, i));
        return result;
    }

    std::unique_ptr<struct apg_trans, void (*)(struct apg_trans *)> trans_;
};

} // namespace apg
