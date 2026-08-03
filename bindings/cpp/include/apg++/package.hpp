// SPDX-License-Identifier: GPL-3.0-only
// SPDX-FileCopyrightText: 2026 AnmiTaliDev <anmitalidev@nuros.org>

#pragma once

#include "detail.hpp"
#include "error.hpp"
#include "version.hpp"

#include <apg/json.h>
#include <apg/package.h>

#include <memory>
#include <string>
#include <vector>

namespace apg
{

inline std::vector<std::string> to_vector(const struct str_list &list)
{
    std::vector<std::string> result;
    result.reserve(static_cast<std::size_t>(list.count));
    for (int i = 0; i < list.count; ++i)
        result.emplace_back(list.items[i] ? list.items[i] : "");
    return result;
}

inline std::vector<DepConstraint>
to_vector(const struct dep_constraint_list &list)
{
    std::vector<DepConstraint> result;
    result.reserve(static_cast<std::size_t>(list.count));
    for (int i = 0; i < list.count; ++i)
    {
        const struct dep_constraint &c = list.items[i];
        result.push_back(
            DepConstraint{c.name ? c.name : "", c.op, c.version ? c.version : ""});
    }
    return result;
}

class PackageMetadataView
{
public:
    explicit PackageMetadataView(const struct package_metadata *ptr) : ptr_(ptr) {}

    const struct package_metadata *get() const noexcept { return ptr_; }

    std::string name() const { return ptr_->name ? ptr_->name : ""; }
    std::string version() const { return ptr_->version ? ptr_->version : ""; }
    std::string type() const { return ptr_->type ? ptr_->type : ""; }
    std::string architecture() const
    {
        return ptr_->architecture ? ptr_->architecture : "";
    }
    std::string description() const
    {
        return ptr_->description ? ptr_->description : "";
    }
    std::string maintainer() const
    {
        return ptr_->maintainer ? ptr_->maintainer : "";
    }
    std::string license() const { return ptr_->license ? ptr_->license : ""; }
    std::string homepage() const { return ptr_->homepage ? ptr_->homepage : ""; }
    std::vector<std::string> tags() const { return to_vector(ptr_->tags); }
    std::vector<DepConstraint> dependencies() const
    {
        return to_vector(ptr_->dependencies);
    }
    std::vector<std::string> conflicts() const
    {
        return to_vector(ptr_->conflicts);
    }
    std::vector<std::string> provides() const { return to_vector(ptr_->provides); }
    std::vector<std::string> replaces() const { return to_vector(ptr_->replaces); }
    std::vector<std::string> conf() const { return to_vector(ptr_->conf); }

protected:
    const struct package_metadata *ptr_;
};

class PackageMetadata : public PackageMetadataView
{
public:
    explicit PackageMetadata(struct package_metadata *raw)
        : PackageMetadataView(raw), owned_(raw, &package_metadata_free)
    {
    }

    static PackageMetadata create()
    {
        struct package_metadata *m = ::package_metadata_new();
        if (!m)
            throw Error("package_metadata_new failed");
        return PackageMetadata(m);
    }

    static PackageMetadata from_file(const std::string &path)
    {
        struct package_metadata *m = ::package_metadata_from_file(path.c_str());
        if (!m)
            throw Error("package_metadata_from_file failed for '" + path + "'");
        return PackageMetadata(m);
    }

    static PackageMetadata from_json(const std::string &json)
    {
        struct package_metadata *m =
            ::package_metadata_from_json(json.data(), json.size());
        if (!m)
            throw Error("package_metadata_from_json failed to parse input");
        return PackageMetadata(m);
    }

    struct package_metadata *get() const noexcept { return owned_.get(); }

private:
    std::unique_ptr<struct package_metadata, void (*)(struct package_metadata *)>
        owned_;
};

class Package
{
public:
    explicit Package(struct package *raw) : raw_(raw, &package_free) {}

    static Package create()
    {
        struct package *p = ::package_new();
        if (!p)
            throw Error("package_new failed");
        return Package(p);
    }

    static Package parse(const std::string &path, const std::string &root_path)
    {
        struct package *p =
            ::parse_package(path.c_str(), root_path.c_str());
        if (!p)
            throw Error("parse_package failed for '" + path + "'");
        return Package(p);
    }

    static Package from_json(const std::string &json)
    {
        struct package *p = ::package_from_json(json.data(), json.size());
        if (!p)
            throw Error("package_from_json failed to parse input");
        return Package(p);
    }

    struct package *get() const noexcept { return raw_.get(); }

    PackageMetadataView metadata() const
    {
        return PackageMetadataView(raw_->meta);
    }

    std::string path() const { return raw_->pkg_path ? raw_->pkg_path : ""; }
    std::vector<std::string> files() const
    {
        return to_vector(raw_->package_files);
    }
    bool installed_by_hand() const { return raw_->installed_by_hand; }
    bool held() const { return raw_->held; }

    std::string to_json() const
    {
        return detail::take_c_string(::package_to_json(raw_.get()));
    }

    bool install() { return ::install_package(raw_.get()); }

    bool install_in_root(const std::string &root_path)
    {
        return ::install_package_in_root(raw_.get(), root_path.c_str());
    }

    bool collect_files(const std::string &root_path)
    {
        return ::package_collect_files(raw_.get(), root_path.c_str());
    }

private:
    std::unique_ptr<struct package, void (*)(struct package *)> raw_;
};

} // namespace apg
