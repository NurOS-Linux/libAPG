# SPDX-License-Identifier: GPL-3.0-only
# SPDX-FileCopyrightText: 2026 AnmiTaliDev <anmitalidev@nuros.org>

from . import _lib
from ._util import take_str


def run_script(pkg_dir, name, root_path):
    return bool(
        _lib.run_script(
            pkg_dir.encode(),
            name.encode(),
            root_path.encode() if root_path else None,
        )
    )


def store_path(root_path, pkg_name):
    return take_str(_lib.scripts_store_path(root_path.encode(), pkg_name.encode()))


def persist(pkg_dir, root_path, pkg_name):
    return bool(
        _lib.scripts_persist(
            pkg_dir.encode(), root_path.encode(), pkg_name.encode()
        )
    )


def persist_remove(root_path, pkg_name):
    _lib.scripts_persist_remove(root_path.encode(), pkg_name.encode())
