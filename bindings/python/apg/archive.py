# SPDX-License-Identifier: GPL-3.0-only
# SPDX-FileCopyrightText: 2026 AnmiTaliDev <anmitalidev@nuros.org>

from . import _lib


def unarchive(pkg):
    return bool(_lib.unarchive_package(pkg.raw))


def unarchive_in_root(pkg, root):
    return bool(_lib.unarchive_package_in_root(pkg.raw, root.encode()))
