# SPDX-License-Identifier: GPL-3.0-only
# SPDX-FileCopyrightText: 2026 AnmiTaliDev <anmitalidev@nuros.org>

from . import _lib


def install_data_dir(pkg_dir, root_path):
    return bool(_lib.install_data_dir(pkg_dir.encode(), root_path.encode()))


def install_home_dir(pkg_dir):
    return bool(_lib.install_home_dir(pkg_dir.encode()))


def rollback_install(pkg_dir, root_path):
    _lib.rollback_install(pkg_dir.encode(), root_path.encode())
