// Author: AnmiTaliDev <anmitalidev@nuros.org>
#pragma once

#include <stdbool.h>

bool sign_verify(const char *pkg_path, const char *sig_path, bool allow_rsa);

bool sign_file(const char *pkg_path, const char *sig_path);
