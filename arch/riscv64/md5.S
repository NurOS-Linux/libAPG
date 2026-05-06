// SPDX-License-Identifier: GPL-3.0-only
// SPDX-FileCopyrightText: 2026 AnmiTaliDev <anmitalidev@nuros.org>
// MD5 transform — optimized scalar RISC-V assembly.
// Syntax: RISC-V unified assembly — compatible with GAS, Clang IAS, and LLVM.
//
// int  apg_md5_hw_supported(void)
// void apg_md5_transform_hw(uint32_t state[4], const uint8_t block[64])

    .text

    .globl apg_md5_hw_supported
    .type  apg_md5_hw_supported, @function
apg_md5_hw_supported:
    li      a0, 1
    ret
    .size apg_md5_hw_supported, .-apg_md5_hw_supported

    .globl apg_md5_transform_hw
    .type  apg_md5_transform_hw, @function
    .align 2
apg_md5_transform_hw:
    // a0 = state[4], a1 = block[64]
    addi    sp, sp, -112
    sd      ra,   0(sp)
    sd      s0,   8(sp)
    sd      s1,  16(sp)
    sd      s2,  24(sp)
    sd      s3,  32(sp)
    sd      s4,  40(sp)
    sd      s5,  48(sp)
    sd      s6,  56(sp)
    sd      s7,  64(sp)
    sd      s8,  72(sp)
    sd      s9,  80(sp)
    sd      a0,  88(sp)
    sd      a1,  96(sp)

    lw      s0,  0(a0)          // a
    lw      s1,  4(a0)          // b
    lw      s2,  8(a0)          // c
    lw      s3, 12(a0)          // d

    la      s5, .LT
    la      s6, .LS
    la      s7, .LG

    li      s4, 0               // round counter

.Lrnd:
    li      t0, 48
    bge     s4, t0, .Lf_I
    li      t0, 32
    bge     s4, t0, .Lf_H
    li      t0, 16
    bge     s4, t0, .Lf_G
    // F(b,c,d) = d ^ (b & (c^d))
    xor     t1, s2, s3
    and     t1, t1, s1
    xor     t1, t1, s3
    j       .Ldo_round
.Lf_G:
    // G(b,c,d) = c ^ (d & (b^c))
    xor     t1, s1, s2
    and     t1, t1, s3
    xor     t1, t1, s2
    j       .Ldo_round
.Lf_H:
    // H(b,c,d) = b^c^d
    xor     t1, s1, s2
    xor     t1, t1, s3
    j       .Ldo_round
.Lf_I:
    // I(b,c,d) = c ^ (b | ~d)
    not     t1, s3
    or      t1, t1, s1
    xor     t1, t1, s2

.Ldo_round:
    // t1 = f
    add     t0, s7, s4          // &G[i]
    lbu     t0, 0(t0)           // g
    slli    t0, t0, 2           // g*4
    ld      s8, 96(sp)          // block ptr
    add     t0, t0, s8
    lw      t2, 0(t0)           // M[g]

    slli    t0, s4, 2           // i*4
    add     t0, t0, s5
    lw      t3, 0(t0)           // T[i]

    add     t0, s6, s4
    lbu     t4, 0(t0)           // S[i]

    addw    t1, t1, s0          // f + a
    addw    t1, t1, t2          // + M[g]
    addw    t1, t1, t3          // + T[i]

    // ROTL32(t1, t4): sllw t5 + srlw t6
    li      t0, 32
    subw    t5, t0, t4          // 32 - S[i]
    sllw    t6, t1, t4
    srlw    t1, t1, t5
    or      t1, t6, t1          // ROTL result

    addw    t1, t1, s1          // + b

    mv      s0, s3              // a = old d
    mv      s3, s2              // d = old c
    mv      s2, s1              // c = old b
    mv      s1, t1              // b = result

    addi    s4, s4, 1
    li      t0, 64
    blt     s4, t0, .Lrnd

    ld      a0, 88(sp)
    lw      t0,  0(a0); addw s0, s0, t0; sw s0,  0(a0)
    lw      t0,  4(a0); addw s1, s1, t0; sw s1,  4(a0)
    lw      t0,  8(a0); addw s2, s2, t0; sw s2,  8(a0)
    lw      t0, 12(a0); addw s3, s3, t0; sw s3, 12(a0)

    ld      ra,  0(sp)
    ld      s0,  8(sp)
    ld      s1, 16(sp)
    ld      s2, 24(sp)
    ld      s3, 32(sp)
    ld      s4, 40(sp)
    ld      s5, 48(sp)
    ld      s6, 56(sp)
    ld      s7, 64(sp)
    ld      s8, 72(sp)
    ld      s9, 80(sp)
    addi    sp, sp, 112
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
