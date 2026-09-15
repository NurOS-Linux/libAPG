# SPDX-License-Identifier: GPL-3.0-only
# SPDX-FileCopyrightText: 2026 AnmiTaliDev <anmitalidev@nuros.org>

from . import _lib


class InstallPolicy:
    def __init__(
        self,
        require_signature=False,
        keyring_dir=None,
        skip_dependency_check=False,
    ):
        self.require_signature = require_signature
        self.keyring_dir = keyring_dir
        self.skip_dependency_check = skip_dependency_check

    def to_c(self):
        return _lib.InstallPolicy(
            self.require_signature,
            self.keyring_dir.encode() if self.keyring_dir else None,
            self.skip_dependency_check,
        )
