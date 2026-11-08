# SPDX-License-Identifier: GPL-3.0-only
# SPDX-FileCopyrightText: 2026 AnmiTaliDev <anmitalidev@nuros.org>

import ctypes

from . import _lib
from ._util import take_str_array
from .error import ApgError


def unarchive(pkg):
    return bool(_lib.unarchive_package(pkg.raw))


def unarchive_in_root(pkg, root):
    return bool(_lib.unarchive_package_in_root(pkg.raw, root.encode()))


def list_files(archive_path):
    files = ctypes.POINTER(_lib.c_void_p)()
    count = _lib.c_int(0)
    ok = _lib.package_list_files(
        archive_path.encode(), ctypes.byref(files), ctypes.byref(count)
    )
    if not ok:
        raise ApgError(f"package_list_files failed for {archive_path!r}")
    if not files:
        return []
    return take_str_array(files, count.value)
