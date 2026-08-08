# SPDX-License-Identifier: GPL-3.0-only
# SPDX-FileCopyrightText: 2026 AnmiTaliDev <anmitalidev@nuros.org>

from . import _lib


class InstallPolicy:
    def __init__(self, require_signature=False, keyring_dir=None):
        self.require_signature = require_signature
        self.keyring_dir = keyring_dir

    def to_c(self):
        return _lib.InstallPolicy(
            self.require_signature,
            self.keyring_dir.encode() if self.keyring_dir else None,
        )
