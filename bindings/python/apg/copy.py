# SPDX-License-Identifier: GPL-3.0-only
# SPDX-FileCopyrightText: 2026 AnmiTaliDev <anmitalidev@nuros.org>

from . import _lib


def copy_dir(src, dst):
    return bool(_lib.copy_dir(src.encode(), dst.encode()))
