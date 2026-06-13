// SPDX-License-Identifier: GPL-3.0-only
// SPDX-FileCopyrightText: 2025 Ruzen42
// SPDX-FileCopyrightText: 2026 AnmiTaliDev <anmitalidev@nuros.org>

#include <stdint.h>
#include <stddef.h>
#include "../../include/apg/crc32.h"

#ifdef APG_CRC32_HW_AVAILABLE
extern int apg_crc32_hw_supported(void);
extern uint32_t apg_crc32_update_hw(uint32_t crc, const uint8_t *buf,
                                    size_t len);

static int hw_ok = -1;

static inline int
use_hw(void)
{
    if (__builtin_expect(hw_ok < 0, 0))
        hw_ok = apg_crc32_hw_supported() ? 1 : 0;
    return hw_ok;
}
#endif

unsigned int
crc32(const unsigned char *buffer, unsigned int len)
{
    unsigned int crc = 0xffffffffU;
#ifdef APG_CRC32_HW_AVAILABLE
    if (use_hw())
        return apg_crc32_update_hw(crc, buffer, len) ^ 0xffffffffU;
#endif
    while (len >= 8)
    {
        DO8(buffer);
        len -= 8;
    }
    if (len)
        do
        {
            DO1(buffer);
        } while (--len);
    return crc ^ 0xffffffffU;
}

unsigned int
crc32_simple(const unsigned char *message, unsigned int len)
{
    int i, j;
    unsigned int byte, crc, mask;

    i = 0;
    crc = 0xFFFFFFFF;
    while (i < len)
    {
        byte = message[i];
        crc = crc ^ byte;
        for (j = 7; j >= 0; j--)
        {
            mask = -(crc & 1);
            crc = (crc >> 1) ^ (0xEDB88320 & mask);
        }
        i = i + 1;
    }
    return ~crc;
}
