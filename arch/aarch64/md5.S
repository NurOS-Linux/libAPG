// SPDX-License-Identifier: GPL-3.0-only
// SPDX-FileCopyrightText: 2026 AnmiTaliDev <anmitalidev@nuros.org>
// MD5 transform — optimized scalar AArch64 assembly.
// Syntax: AArch64 unified assembly — compatible with GAS, Clang IAS, and LLVM.
//
// int  apg_md5_hw_supported(void)
// void apg_md5_transform_hw(uint32_t state[4], const uint8_t block[64])

    .arch armv8-a
    .text

    .globl apg_md5_hw_supported
    .type  apg_md5_hw_supported, @function
apg_md5_hw_supported:
    mov     w0, #1
    ret
    .size apg_md5_hw_supported, .-apg_md5_hw_supported

    .globl apg_md5_transform_hw
    .type  apg_md5_transform_hw, @function
    .align 4
apg_md5_transform_hw:
    // x0 = state[4], x1 = block[64]
    stp     x19, x20, [sp, #-96]!
    stp     x21, x22, [sp, #16]
    stp     x23, x24, [sp, #32]
    stp     x25, x26, [sp, #48]
    stp     x27, x28, [sp, #64]
    str     x30, [sp, #80]

    mov     x23, x0             // state ptr
    mov     x24, x1             // block ptr

    ldr     w19, [x0]           // a
    ldr     w20, [x0, #4]       // b
    ldr     w21, [x0, #8]       // c
    ldr     w22, [x0, #12]      // d

    adrp    x25, .LT
    add     x25, x25, :lo12:.LT
    adrp    x26, .LS
    add     x26, x26, :lo12:.LS
    adrp    x27, .LG
    add     x27, x27, :lo12:.LG

    mov     x28, #0             // round counter

.Lrnd:
    cmp     x28, #48
    b.ge    .Lf_I
    cmp     x28, #32
    b.ge    .Lf_H
    cmp     x28, #16
    b.ge    .Lf_G
    // F(b,c,d) = d ^ (b & (c^d))
    eor     w0, w21, w22
    and     w0, w0, w20
    eor     w0, w0, w22
    b       .Ldo_round
.Lf_G:
    // G(b,c,d) = c ^ (d & (b^c))
    eor     w0, w20, w21
    and     w0, w0, w22
    eor     w0, w0, w21
    b       .Ldo_round
.Lf_H:
    // H(b,c,d) = b^c^d
    eor     w0, w20, w21
    eor     w0, w0, w22
    b       .Ldo_round
.Lf_I:
    // I(b,c,d) = c ^ (b | ~d)
    mvn     w0, w22
    orr     w0, w0, w20
    eor     w0, w0, w21

.Ldo_round:
    ldrb    w1, [x27, x28]              // g
    lsl     w1, w1, #2                  // g * 4
    ldr     w2, [x24, x1]              // M[g] (little-endian native load)
    ldr     w3, [x25, x28, lsl #2]     // T[i]
    ldrb    w4, [x26, x28]              // S[i]
    add     w0, w0, w19                 // f + a
    add     w0, w0, w2                  // + M[g]
    add     w0, w0, w3                  // + T[i]
    neg     w4, w4
    and     w4, w4, #31                 // 32 - S[i], giving ROTL as ROR(32-n)
    ror     w0, w0, w4
    add     w0, w0, w20                 // + b
    mov     w19, w22
    mov     w22, w21
    mov     w21, w20
    mov     w20, w0
    add     x28, x28, #1
    cmp     x28, #64
    b.lt    .Lrnd

    ldr     w0, [x23]
    add     w0, w0, w19
    str     w0, [x23]
    ldr     w0, [x23, #4]
    add     w0, w0, w20
    str     w0, [x23, #4]
    ldr     w0, [x23, #8]
    add     w0, w0, w21
    str     w0, [x23, #8]
    ldr     w0, [x23, #12]
    add     w0, w0, w22
    str     w0, [x23, #12]

    ldr     x30, [sp, #80]
    ldp     x27, x28, [sp, #64]
    ldp     x25, x26, [sp, #48]
    ldp     x23, x24, [sp, #32]
    ldp     x21, x22, [sp, #16]
    ldp     x19, x20, [sp], #96
    ret
    .size apg_md5_transform_hw, .-apg_md5_transform_hw

    .section .rodata
    .align 4

.LT:
    .long 0xd76aa478,0xe8c7b756,0x242070db,0xc1bdceee
    .long 0xf57c0faf,0x4787c62a,0xa8304613,0xfd469501
    .long 0x698098d8,0x8b44f7af,0xffff5bb1,0x895cd7be
    .long 0x6b901122,0xfd987193,0xa679438e,0x49b40821
    .long 0xf61e2562,0xc040b340,0x265e5a51,0xe9b6c7aa
    .long 0xd62f105d,0x02441453,0xd8a1e681,0xe7d3fbc8
    .long 0x21e1cde6,0xc33707d6,0xf4d50d87,0x455a14ed
    .long 0xa9e3e905,0xfcefa3f8,0x676f02d9,0x8d2a4c8a
    .long 0xfffa3942,0x8771f681,0x6d9d6122,0xfde5380c
    .long 0xa4beea44,0x4bdecfa9,0xf6bb4b60,0xbebfbc70
    .long 0x289b7ec6,0xeaa127fa,0xd4ef3085,0x04881d05
    .long 0xd9d4d039,0xe6db99e5,0x1fa27cf8,0xc4ac5665
    .long 0xf4292244,0x432aff97,0xab9423a7,0xfc93a039
    .long 0x655b59c3,0x8f0ccc92,0xffeff47d,0x85845dd1
    .long 0x6fa87e4f,0xfe2ce6e0,0xa3014314,0x4e0811a1
    .long 0xf7537e82,0xbd3af235,0x2ad7d2bb,0xeb86d391

.LS:
    .byte  7,12,17,22, 7,12,17,22, 7,12,17,22, 7,12,17,22
    .byte  5, 9,14,20, 5, 9,14,20, 5, 9,14,20, 5, 9,14,20
    .byte  4,11,16,23, 4,11,16,23, 4,11,16,23, 4,11,16,23
    .byte  6,10,15,21, 6,10,15,21, 6,10,15,21, 6,10,15,21

.LG:
    .byte  0, 1, 2, 3, 4, 5, 6, 7, 8, 9,10,11,12,13,14,15
    .byte  1, 6,11, 0, 5,10,15, 4, 9,14, 3, 8,13, 2, 7,12
    .byte  5, 8,11,14, 1, 4, 7,10,13, 0, 3, 6, 9,12,15, 2
    .byte  0, 7,14, 5,12, 3,10, 1, 8,15, 6,13, 4,11, 2, 9
