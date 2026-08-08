# SPDX-License-Identifier: GPL-3.0-only
# SPDX-FileCopyrightText: 2026 AnmiTaliDev <anmitalidev@nuros.org>

from . import _lib


def verify(pkg_path, sig_path):
    return bool(_lib.sign_verify(pkg_path.encode(), sig_path.encode(), False))


def sign(pkg_path, sig_path):
    return bool(_lib.sign_file(pkg_path.encode(), sig_path.encode()))
