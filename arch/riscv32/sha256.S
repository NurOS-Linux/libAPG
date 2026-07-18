/* SPDX-License-Identifier: GPL-3.0-only */
/* SPDX-FileCopyrightText: 2026 Meigo™ Corporation (https://github.com/meigoc) */

    .text

    .globl  apg_sha256_hw_supported
    .type   apg_sha256_hw_supported, @function
apg_sha256_hw_supported:
    li      a0, 1
    ret
    .size   apg_sha256_hw_supported, .-apg_sha256_hw_supported

    .globl  apg_sha256_transform_hw
    .type   apg_sha256_transform_hw, @function
apg_sha256_transform_hw:
    addi    sp, sp, -304
    sw      s0, 0(sp)
    sw      s1, 4(sp)
    sw      s2, 8(sp)
    sw      s3, 12(sp)
    sw      s4, 16(sp)
    sw      s5, 20(sp)
    sw      s6, 24(sp)
    sw      s7, 28(sp)
    sw      s8, 32(sp)
    sw      s9, 36(sp)
    sw      s10, 40(sp)
    sw      s11, 44(sp)

    mv      s8, a0
    addi    s11, sp, 48

    li      t0, 0
.Lldw:
    slli    t1, t0, 2
    add     t1, a1, t1
    lbu     t2, 0(t1)
    lbu     t3, 1(t1)
    slli    t2, t2, 8
    or      t2, t2, t3
    lbu     t3, 2(t1)
    slli    t2, t2, 8
    or      t2, t2, t3
    lbu     t3, 3(t1)
    slli    t2, t2, 8
    or      t2, t2, t3
    slli    t1, t0, 2
    add     t1, s11, t1
    sw      t2, 0(t1)
    addi    t0, t0, 1
    li      t1, 16
    bltu    t0, t1, .Lldw

    li      t0, 16
.Lexp:
    addi    t1, t0, -2
    slli    t1, t1, 2
    add     t1, s11, t1
    lw      t2, 0(t1)
    srli    t3, t2, 17
    slli    t4, t2, 15
    or      t3, t3, t4
    srli    t4, t2, 19
    slli    t5, t2, 13
    or      t4, t4, t5
    xor     t3, t3, t4
    srli    t4, t2, 10
    xor     t3, t3, t4
    addi    t1, t0, -7
    slli    t1, t1, 2
    add     t1, s11, t1
    lw      t4, 0(t1)
    add     t3, t3, t4
    addi    t1, t0, -15
    slli    t1, t1, 2
    add     t1, s11, t1
    lw      t2, 0(t1)
    srli    t4, t2, 7
    slli    t5, t2, 25
    or      t4, t4, t5
    srli    t5, t2, 18
    slli    t6, t2, 14
    or      t5, t5, t6
    xor     t4, t4, t5
    srli    t5, t2, 3
    xor     t4, t4, t5
    add     t3, t3, t4
    addi    t1, t0, -16
    slli    t1, t1, 2
    add     t1, s11, t1
    lw      t4, 0(t1)
    add     t3, t3, t4
    slli    t1, t0, 2
    add     t1, s11, t1
    sw      t3, 0(t1)
    addi    t0, t0, 1
    li      t1, 64
    bltu    t0, t1, .Lexp

    lw      s0, 0(s8)
    lw      s1, 4(s8)
    lw      s2, 8(s8)
    lw      s3, 12(s8)
    lw      s4, 16(s8)
    lw      s5, 20(s8)
    lw      s6, 24(s8)
    lw      s7, 28(s8)

    la      s10, .LK
    li      s9, 0

.Lround:
    slli    t0, s9, 2
    add     t0, s10, t0
    lw      t0, 0(t0)
    slli    t1, s9, 2
    add     t1, s11, t1
    lw      t1, 0(t1)
    srli    t2, s4, 6
    slli    t3, s4, 26
    or      t2, t2, t3
    srli    t3, s4, 11
    slli    t4, s4, 21
    or      t3, t3, t4
    xor     t2, t2, t3
    srli    t3, s4, 25
    slli    t4, s4, 7
    or      t3, t3, t4
    xor     t2, t2, t3
    add     t0, t0, t1
    add     t0, t0, s7
    add     t0, t0, t2
    and     t2, s4, s5
    not     t3, s4
    and     t3, t3, s6
    xor     t2, t2, t3
    add     t0, t0, t2
    srli    t2, s0, 2
    slli    t3, s0, 30
    or      t2, t2, t3
    srli    t3, s0, 13
    slli    t4, s0, 19
    or      t3, t3, t4
    xor     t2, t2, t3
    srli    t3, s0, 22
    slli    t4, s0, 10
    or      t3, t3, t4
    xor     t2, t2, t3
    and     t3, s0, s1
    and     t4, s0, s2
    xor     t3, t3, t4
    and     t4, s1, s2
    xor     t3, t3, t4
    add     t2, t2, t3

    mv      s7, s6
    mv      s6, s5
    mv      s5, s4
    add     s4, s3, t0
    mv      s3, s2
    mv      s2, s1
    mv      s1, s0
    add     s0, t0, t2

    addi    s9, s9, 1
    li      t0, 64
    bltu    s9, t0, .Lround

    lw      t0, 0(s8)
    add     t0, t0, s0
    sw      t0, 0(s8)
    lw      t0, 4(s8)
    add     t0, t0, s1
    sw      t0, 4(s8)
    lw      t0, 8(s8)
    add     t0, t0, s2
    sw      t0, 8(s8)
    lw      t0, 12(s8)
    add     t0, t0, s3
    sw      t0, 12(s8)
    lw      t0, 16(s8)
    add     t0, t0, s4
    sw      t0, 16(s8)
    lw      t0, 20(s8)
    add     t0, t0, s5
    sw      t0, 20(s8)
    lw      t0, 24(s8)
    add     t0, t0, s6
    sw      t0, 24(s8)
    lw      t0, 28(s8)
    add     t0, t0, s7
    sw      t0, 28(s8)

    lw      s0, 0(sp)
    lw      s1, 4(sp)
    lw      s2, 8(sp)
    lw      s3, 12(sp)
    lw      s4, 16(sp)
    lw      s5, 20(sp)
    lw      s6, 24(sp)
    lw      s7, 28(sp)
    lw      s8, 32(sp)
    lw      s9, 36(sp)
    lw      s10, 40(sp)
    lw      s11, 44(sp)
    addi    sp, sp, 304
    ret
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
