/* SPDX-License-Identifier: GPL-3.0-only */
/* SPDX-FileCopyrightText: 2026 AnmiTaliDev <anmitalidev@nuros.org> */
/*
 * SHA-256 (FIPS 180-4) single-block transform, table-based.
 * PowerPC64 ELFv2 ABI. Content is identical to arch/ppc64le/sha256.S: the
 * message block is assembled byte-wise (lbz only), so the code is
 * endian-independent — only the target ABI/triple passed to the assembler
 * differs between the two directories.
 *
 * All 32-bit intermediate values (state words, message schedule) live in
 * the low 32 bits of 64-bit GPRs; rlwinm zero-extends the upper 32 bits per
 * the Power ISA 64-bit-mode definition, so no explicit widening is needed.
 * Saved nonvolatile GPRs (r14-r25) are spilled/reloaded as full 64-bit
 * values (std/ld) to preserve the caller's register state.
 *
 * int  apg_sha256_hw_supported(void)
 * void apg_sha256_transform_hw(uint32_t state[8], const uint8_t data[64])
 */

    .abiversion 2
    .text

    .globl  apg_sha256_hw_supported
    .type   apg_sha256_hw_supported, @function
apg_sha256_hw_supported:
    .localentry apg_sha256_hw_supported, 1
    li      3, 1
    blr
    .size   apg_sha256_hw_supported, .-apg_sha256_hw_supported

    .globl  apg_sha256_transform_hw
    .type   apg_sha256_transform_hw, @function
apg_sha256_transform_hw:
    .localentry apg_sha256_transform_hw, 1
    mflr    0
    std     0, 16(1)
    stdu    1, -352(1)
    std     14, 0(1)
    std     15, 8(1)
    std     16, 16(1)
    std     17, 24(1)
    std     18, 32(1)
    std     19, 40(1)
    std     20, 48(1)
    std     21, 56(1)
    std     22, 64(1)
    std     23, 72(1)
    std     24, 80(1)
    std     25, 88(1)

    mr      22, 3
    addi    25, 1, 96

    li      3, 0
.Lldw:
    slwi    7, 3, 2
    add     7, 4, 7
    lbz     8, 0(7)
    lbz     9, 1(7)
    slwi    8, 8, 8
    or      8, 8, 9
    lbz     9, 2(7)
    slwi    8, 8, 8
    or      8, 8, 9
    lbz     9, 3(7)
    slwi    8, 8, 8
    or      8, 8, 9
    slwi    7, 3, 2
    add     7, 25, 7
    stw     8, 0(7)
    addi    3, 3, 1
    cmpwi   3, 16
    blt     .Lldw

    li      3, 16
.Lexp:
    addi    7, 3, -2
    slwi    7, 7, 2
    add     7, 25, 7
    lwz     8, 0(7)
    rlwinm  9, 8, 15, 0, 31
    rlwinm  10, 8, 13, 0, 31
    xor     9, 9, 10
    srwi    10, 8, 10
    xor     9, 9, 10
    addi    7, 3, -7
    slwi    7, 7, 2
    add     7, 25, 7
    lwz     10, 0(7)
    add     9, 9, 10
    addi    7, 3, -15
    slwi    7, 7, 2
    add     7, 25, 7
    lwz     8, 0(7)
    rlwinm  10, 8, 25, 0, 31
    rlwinm  11, 8, 14, 0, 31
    xor     10, 10, 11
    srwi    11, 8, 3
    xor     10, 10, 11
    add     9, 9, 10
    addi    7, 3, -16
    slwi    7, 7, 2
    add     7, 25, 7
    lwz     10, 0(7)
    add     9, 9, 10
    slwi    7, 3, 2
    add     7, 25, 7
    stw     9, 0(7)
    addi    3, 3, 1
    cmpwi   3, 64
    blt     .Lexp

    lwz     14, 0(22)
    lwz     15, 4(22)
    lwz     16, 8(22)
    lwz     17, 12(22)
    lwz     18, 16(22)
    lwz     19, 20(22)
    lwz     20, 24(22)
    lwz     21, 28(22)

    bcl     20, 31, 0f
0:  mflr    7
    addis   24, 7, (.LK - 0b)@ha
    addi    24, 24, (.LK - 0b)@l
    li      23, 0

.Lround:
    slwi    7, 23, 2
    add     7, 24, 7
    lwz     7, 0(7)
    slwi    8, 23, 2
    add     8, 25, 8
    lwz     8, 0(8)
    rlwinm  9, 18, 26, 0, 31
    rlwinm  10, 18, 21, 0, 31
    xor     9, 9, 10
    rlwinm  10, 18, 7, 0, 31
    xor     9, 9, 10
    add     7, 7, 8
    add     7, 7, 21
    add     7, 7, 9
    and     9, 18, 19
    andc    10, 20, 18
    xor     9, 9, 10
    add     7, 7, 9
    rlwinm  9, 14, 30, 0, 31
    rlwinm  10, 14, 19, 0, 31
    xor     9, 9, 10
    rlwinm  10, 14, 10, 0, 31
    xor     9, 9, 10
    and     10, 14, 15
    and     11, 14, 16
    xor     10, 10, 11
    and     11, 15, 16
    xor     10, 10, 11
    add     9, 9, 10

    mr      21, 20
    mr      20, 19
    mr      19, 18
    add     18, 17, 7
    mr      17, 16
    mr      16, 15
    mr      15, 14
    add     14, 7, 9

    addi    23, 23, 1
    cmpwi   23, 64
    blt     .Lround

    lwz     7, 0(22)
    add     14, 14, 7
    stw     14, 0(22)
    lwz     7, 4(22)
    add     15, 15, 7
    stw     15, 4(22)
    lwz     7, 8(22)
    add     16, 16, 7
    stw     16, 8(22)
    lwz     7, 12(22)
    add     17, 17, 7
    stw     17, 12(22)
    lwz     7, 16(22)
    add     18, 18, 7
    stw     18, 16(22)
    lwz     7, 20(22)
    add     19, 19, 7
    stw     19, 20(22)
    lwz     7, 24(22)
    add     20, 20, 7
    stw     20, 24(22)
    lwz     7, 28(22)
    add     21, 21, 7
    stw     21, 28(22)

    ld      14, 0(1)
    ld      15, 8(1)
    ld      16, 16(1)
    ld      17, 24(1)
    ld      18, 32(1)
    ld      19, 40(1)
    ld      20, 48(1)
    ld      21, 56(1)
    ld      22, 64(1)
    ld      23, 72(1)
    ld      24, 80(1)
    ld      25, 88(1)
    addi    1, 1, 352
    ld      0, 16(1)
    mtlr    0
    blr
    .size   apg_sha256_transform_hw, .-apg_sha256_transform_hw

    .section .rodata
    .align  4
.LK:
    .long 0x428a2f98,0x71374491,0xb5c0fbcf,0xe9b5dba5
    .long 0x3956c25b,0x59f111f1,0x923f82a4,0xab1c5ed5
    .long 0xd807aa98,0x12835b01,0x243185be,0x550c7dc3
    .long 0x72be5d74,0x80deb1fe,0x9bdc06a7,0xc19bf174
    .long 0xe49b69c1,0xefbe4786,0x0fc19dc6,0x240ca1cc
    .long 0x2de92c6f,0x4a7484aa,0x5cb0a9dc,0x76f988da
    .long 0x983e5152,0xa831c66d,0xb00327c8,0xbf597fc7
    .long 0xc6e00bf3,0xd5a79147,0x06ca6351,0x14292967
    .long 0x27b70a85,0x2e1b2138,0x4d2c6dfc,0x53380d13
    .long 0x650a7354,0x766a0abb,0x81c2c92e,0x92722c85
    .long 0xa2bfe8a1,0xa81a664b,0xc24b8b70,0xc76c51a3
    .long 0xd192e819,0xd6990624,0xf40e3585,0x106aa070
    .long 0x19a4c116,0x1e376c08,0x2748774c,0x34b0bcb5
    .long 0x391c0cb3,0x4ed8aa4a,0x5b9cca4f,0x682e6ff3
    .long 0x748f82ee,0x78a5636f,0x84c87814,0x8cc70208
    .long 0x90befffa,0xa4506ceb,0xbef9a3f7,0xc67178f2

    .section .note.GNU-stack,"",@progbits
