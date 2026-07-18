/* SPDX-License-Identifier: GPL-3.0-only */
/* SPDX-FileCopyrightText: 2026 Meigo™ Corporation (https://github.com/meigoc) */

    .text

    .globl  apg_md5_hw_supported
    .type   apg_md5_hw_supported, @function
apg_md5_hw_supported:
    li      3, 1
    blr
    .size   apg_md5_hw_supported, .-apg_md5_hw_supported

    .globl  apg_md5_transform_hw
    .type   apg_md5_transform_hw, @function
apg_md5_transform_hw:
    mflr    0
    stwu    1, -112(1)
    stw     0, 116(1)
    stw     14, 8(1)
    stw     15, 12(1)
    stw     16, 16(1)
    stw     17, 20(1)
    stw     18, 24(1)
    stw     19, 28(1)
    stw     20, 32(1)
    stw     21, 36(1)

    mr      18, 3

    li      19, 0
.Lmld:
    slwi    7, 19, 2
    add     7, 4, 7
    lbz     8, 0(7)
    lbz     9, 1(7)
    slwi    9, 9, 8
    or      8, 8, 9
    lbz     9, 2(7)
    slwi    9, 9, 16
    or      8, 8, 9
    lbz     9, 3(7)
    slwi    9, 9, 24
    or      8, 8, 9
    slwi    7, 19, 2
    addi    10, 1, 40
    add     7, 10, 7
    stw     8, 0(7)
    addi    19, 19, 1
    cmpwi   19, 16
    blt     .Lmld

    lwz     14, 0(18)
    lwz     15, 4(18)
    lwz     16, 8(18)
    lwz     17, 12(18)

    bcl     20, 31, 0f
0:  mflr    7
    addis   20, 7, (.LT - 0b)@ha
    addi    20, 20, (.LT - 0b)@l
    addis   21, 7, (.LS - 0b)@ha
    addi    21, 21, (.LS - 0b)@l
    li      19, 0

.Lrnd:
    cmpwi   19, 48
    bge     .LfI
    cmpwi   19, 32
    bge     .LfH
    cmpwi   19, 16
    bge     .LfG
    xor     7, 16, 17
    and     7, 7, 15
    xor     7, 7, 17
    mr      8, 19
    b       .Lgot
.LfG:
    xor     7, 15, 16
    and     7, 7, 17
    xor     7, 7, 16
    slwi    8, 19, 2
    add     8, 8, 19
    addi    8, 8, 1
    andi.   8, 8, 15
    b       .Lgot
.LfH:
    xor     7, 15, 16
    xor     7, 7, 17
    slwi    8, 19, 1
    add     8, 8, 19
    addi    8, 8, 5
    andi.   8, 8, 15
    b       .Lgot
.LfI:
    not     8, 17
    or      8, 8, 15
    xor     7, 8, 16
    slwi    8, 19, 3
    sub     8, 8, 19
    andi.   8, 8, 15
.Lgot:
    slwi    9, 8, 2
    addi    10, 1, 40
    add     9, 10, 9
    lwz     9, 0(9)
    slwi    10, 19, 2
    add     10, 20, 10
    lwz     10, 0(10)
    add     7, 7, 14
    add     7, 7, 9
    add     7, 7, 10
    add     11, 21, 19
    lbz     11, 0(11)
    rlwnm   7, 7, 11, 0, 31
    add     7, 7, 15

    mr      14, 17
    mr      17, 16
    mr      16, 15
    mr      15, 7

    addi    19, 19, 1
    cmpwi   19, 64
    blt     .Lrnd

    lwz     7, 0(18)
    add     14, 14, 7
    stw     14, 0(18)
    lwz     7, 4(18)
    add     15, 15, 7
    stw     15, 4(18)
    lwz     7, 8(18)
    add     16, 16, 7
    stw     16, 8(18)
    lwz     7, 12(18)
    add     17, 17, 7
    stw     17, 12(18)

    lwz     14, 8(1)
    lwz     15, 12(1)
    lwz     16, 16(1)
    lwz     17, 20(1)
    lwz     18, 24(1)
    lwz     19, 28(1)
    lwz     20, 32(1)
    lwz     21, 36(1)
    lwz     0, 116(1)
    mtlr    0
    addi    1, 1, 112
    blr
    .size   apg_md5_transform_hw, .-apg_md5_transform_hw

    .section .rodata
    .align  4
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

    .section .note.GNU-stack,"",@progbits
