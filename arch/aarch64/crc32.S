// SPDX-License-Identifier: GPL-3.0-only
// SPDX-FileCopyrightText: 2026 AnmiTaliDev <anmitalidev@nuros.org>
// CRC-32 (ISO 3309, poly 0xEDB88320) via ARMv8 CRC extension.
// Syntax: AArch64 unified assembly — compatible with GAS, Clang IAS, and LLVM.
//
// int      apg_crc32_hw_supported(void)
// uint32_t apg_crc32_update_hw(uint32_t crc, const uint8_t *buf, size_t len)

    .arch armv8-a+crc
    .text

    .globl apg_crc32_hw_supported
    .type  apg_crc32_hw_supported, @function
apg_crc32_hw_supported:
    mov     w0, #1
    ret
    .size apg_crc32_hw_supported, .-apg_crc32_hw_supported

    .globl apg_crc32_update_hw
    .type  apg_crc32_update_hw, @function
    .align 4
apg_crc32_update_hw:
    // w0 = crc, x1 = buf, x2 = len
.Lbulk:
    cmp     x2, #8
    b.lo    .Ltail4
    ldr     x3, [x1], #8
    crc32x  w0, w0, x3
    sub     x2, x2, #8
    b       .Lbulk
.Ltail4:
    tbz     x2, #2, .Ltail2
    ldr     w3, [x1], #4
    crc32w  w0, w0, w3
.Ltail2:
    tbz     x2, #1, .Ltail1
    ldrh    w3, [x1], #2
    crc32h  w0, w0, w3
.Ltail1:
    tbz     x2, #0, .Ldone
    ldrb    w3, [x1]
    crc32b  w0, w0, w3
.Ldone:
    ret
    .size apg_crc32_update_hw, .-apg_crc32_update_hw
