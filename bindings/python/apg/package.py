# SPDX-License-Identifier: GPL-3.0-only
# SPDX-FileCopyrightText: 2026 AnmiTaliDev <anmitalidev@nuros.org>

import ctypes

from . import _lib
from .error import ApgError
from ._util import (
    alloc_dep_constraint_list,
    alloc_str_list,
    decode_ptr,
    dep_constraint_list_to_list,
    str_list_to_list,
    take_str,
)


class PackageMetadataView:
    def __init__(self, ptr):
        self._ptr = ptr

    @property
    def raw(self):
        return self._ptr

    @property
    def _struct(self):
        return self._ptr.contents

    @property
    def name(self):
        return decode_ptr(self._struct.name)

    @property
    def version(self):
        return decode_ptr(self._struct.version)

    @property
    def type(self):
        return decode_ptr(self._struct.type)

    @property
    def architecture(self):
        return decode_ptr(self._struct.architecture)

    @property
    def description(self):
        return decode_ptr(self._struct.description)

    @property
    def maintainer(self):
        return decode_ptr(self._struct.maintainer)

    @property
    def license(self):
        return decode_ptr(self._struct.license)

    @property
    def homepage(self):
        return decode_ptr(self._struct.homepage)

    @property
    def tags(self):
        return str_list_to_list(self._struct.tags)

    @property
    def dependencies(self):
        return dep_constraint_list_to_list(self._struct.dependencies)

    @property
    def conflicts(self):
        return str_list_to_list(self._struct.conflicts)

    @property
    def provides(self):
        return str_list_to_list(self._struct.provides)

    @property
    def replaces(self):
        return str_list_to_list(self._struct.replaces)

    @property
    def conf(self):
        return str_list_to_list(self._struct.conf)

    def set_name(self, name):
        old = self._struct.name
        self._struct.name = _lib.c_strdup(name)
        _lib.free(old)

    def set_version(self, version):
        old = self._struct.version
        self._struct.version = _lib.c_strdup(version)
        _lib.free(old)

    def set_type(self, value):
        old = self._struct.type
        self._struct.type = _lib.c_strdup(value)
        _lib.free(old)

    def set_architecture(self, value):
        old = self._struct.architecture
        self._struct.architecture = _lib.c_strdup(value)
        _lib.free(old)

    def set_description(self, value):
        old = self._struct.description
        self._struct.description = _lib.c_strdup(value)
        _lib.free(old)

    def set_maintainer(self, value):
        old = self._struct.maintainer
        self._struct.maintainer = _lib.c_strdup(value)
        _lib.free(old)

    def set_license(self, value):
        old = self._struct.license
        self._struct.license = _lib.c_strdup(value)
        _lib.free(old)

    def set_homepage(self, value):
        old = self._struct.homepage
        self._struct.homepage = _lib.c_strdup(value)
        _lib.free(old)

    def set_tags(self, values):
        self._set_str_list("tags", values)

    def set_conflicts(self, values):
        self._set_str_list("conflicts", values)

    def set_provides(self, values):
        self._set_str_list("provides", values)

    def set_replaces(self, values):
        self._set_str_list("replaces", values)

    def set_conf(self, values):
        self._set_str_list("conf", values)

    def set_dependencies(self, constraints):
        old = self._struct.dependencies
        _lib.dep_constraint_list_free(ctypes.byref(old))
        self._struct.dependencies = alloc_dep_constraint_list(constraints)

    def _set_str_list(self, field_name, values):
        old = getattr(self._struct, field_name)
        _lib.str_list_free(ctypes.byref(old))
        setattr(self._struct, field_name, alloc_str_list(values))


class PackageMetadata(PackageMetadataView):
    def __init__(self, ptr):
        super().__init__(ptr)
        self._owned = True

    @classmethod
    def create(cls):
        ptr = _lib.package_metadata_new()
        if not ptr:
            raise ApgError("package_metadata_new failed")
        return cls(ptr)

    @classmethod
    def from_file(cls, path):
        ptr = _lib.package_metadata_from_file(path.encode())
        if not ptr:
            raise ApgError(f"package_metadata_from_file failed for {path!r}")
        return cls(ptr)

    @classmethod
    def from_json(cls, data):
        if isinstance(data, str):
            data = data.encode()
        ptr = _lib.package_metadata_from_json(data, len(data))
        if not ptr:
            raise ApgError("package_metadata_from_json failed to parse input")
        return cls(ptr)

    def close(self):
        if self._owned and self._ptr:
            _lib.package_metadata_free(self._ptr)
            self._ptr = None
            self._owned = False

    def __del__(self):
        self.close()

    def __enter__(self):
        return self

    def __exit__(self, *exc_info):
        self.close()


class Package:
    def __init__(self, ptr):
        self._ptr = ptr

    @classmethod
    def create(cls):
        ptr = _lib.package_new()
        if not ptr:
            raise ApgError("package_new failed")
        return cls(ptr)

    @classmethod
    def parse(cls, path, root_path):
        ptr = _lib.parse_package(path.encode(), root_path.encode())
        if not ptr:
            raise ApgError(f"parse_package failed for {path!r}")
        return cls(ptr)

    @classmethod
    def from_json(cls, data):
        if isinstance(data, str):
            data = data.encode()
        ptr = _lib.package_from_json(data, len(data))
        if not ptr:
            raise ApgError("package_from_json failed to parse input")
        return cls(ptr)

    @property
    def raw(self):
        return self._ptr

    @property
    def metadata(self):
        return PackageMetadataView(self._ptr.contents.meta)

    @property
    def path(self):
        return decode_ptr(self._ptr.contents.pkg_path)

    @path.setter
    def path(self, value):
        old = self._ptr.contents.pkg_path
        self._ptr.contents.pkg_path = _lib.c_strdup(value)
        _lib.free(old)

    @property
    def files(self):
        return str_list_to_list(self._ptr.contents.package_files)

    @property
    def installed_by_hand(self):
        return self._ptr.contents.installed_by_hand

    @installed_by_hand.setter
    def installed_by_hand(self, value):
        self._ptr.contents.installed_by_hand = bool(value)

    @property
    def held(self):
        return self._ptr.contents.held

    @held.setter
    def held(self, value):
        self._ptr.contents.held = bool(value)

    def to_json(self):
        return take_str(_lib.package_to_json(self._ptr))

    def install(self):
        return bool(_lib.install_package(self._ptr))

    def install_in_root(self, root_path):
        return bool(
            _lib.install_package_in_root(self._ptr, root_path.encode())
        )

    def collect_files(self, root_path):
        return bool(
            _lib.package_collect_files(self._ptr, root_path.encode())
        )

    def close(self):
        if self._ptr:
            _lib.package_free(self._ptr)
            self._ptr = None

    def __del__(self):
        self.close()

    def __enter__(self):
        return self

    def __exit__(self, *exc_info):
        self.close()
