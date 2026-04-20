// SPDX-License-Identifier: GPL-3.0-only
// SHA-256 via RISC-V Zknh scalar crypto (sha256sig0/1, sha256sum0/1).
// Syntax: RISC-V unified assembly — compatible with GAS, Clang IAS, and LLVM.
//
// int  apg_sha256_hw_supported(void)
// void apg_sha256_transform_hw(uint32_t state[8], const uint8_t data[64])

    .option arch, +zknh
    .text

    .globl apg_sha256_hw_supported
    .type  apg_sha256_hw_supported, @function
apg_sha256_hw_supported:
    li      a0, 1
    ret
    .size apg_sha256_hw_supported, .-apg_sha256_hw_supported

// void apg_sha256_transform_hw(uint32_t state[8], const uint8_t data[64])
// a0 = state, a1 = data
//
// Register allocation:
//   s0-s7  = A..H (state words)
//   s8     = saved state pointer
//   t0-t4  = temporaries
//   a2     = K pointer
//   a3     = W ring-buffer pointer (on stack)
//   a4     = round counter
    .globl apg_sha256_transform_hw
    .type  apg_sha256_transform_hw, @function
    .align 2
apg_sha256_transform_hw:
    addi    sp, sp, -176
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
    sd      s10, 88(sp)
    sd      s11, 96(sp)
    sd      a0, 104(sp)

    lw      s0,  0(a0)
    lw      s1,  4(a0)
    lw      s2,  8(a0)
    lw      s3, 12(a0)
    lw      s4, 16(a0)
    lw      s5, 20(a0)
    lw      s6, 24(a0)
    lw      s7, 28(a0)

    // Load W[0..15] big-endian from data into stack ring buffer at sp+112..sp+172
    // Each word: 4 bytes big-endian → 32-bit little-endian
    addi    a3, sp, 112

    lbu     t0,  0(a1); lbu t1,  1(a1); lbu t2,  2(a1); lbu t3,  3(a1)
    slli    t0, t0, 24; slli t1, t1, 16; slli t2, t2, 8
    or      t0, t0, t1; or t2, t2, t3; or t0, t0, t2; sw t0,  0(a3)

    lbu     t0,  4(a1); lbu t1,  5(a1); lbu t2,  6(a1); lbu t3,  7(a1)
    slli    t0, t0, 24; slli t1, t1, 16; slli t2, t2, 8
    or      t0, t0, t1; or t2, t2, t3; or t0, t0, t2; sw t0,  4(a3)

    lbu     t0,  8(a1); lbu t1,  9(a1); lbu t2, 10(a1); lbu t3, 11(a1)
    slli    t0, t0, 24; slli t1, t1, 16; slli t2, t2, 8
    or      t0, t0, t1; or t2, t2, t3; or t0, t0, t2; sw t0,  8(a3)

    lbu     t0, 12(a1); lbu t1, 13(a1); lbu t2, 14(a1); lbu t3, 15(a1)
    slli    t0, t0, 24; slli t1, t1, 16; slli t2, t2, 8
    or      t0, t0, t1; or t2, t2, t3; or t0, t0, t2; sw t0, 12(a3)

    lbu     t0, 16(a1); lbu t1, 17(a1); lbu t2, 18(a1); lbu t3, 19(a1)
    slli    t0, t0, 24; slli t1, t1, 16; slli t2, t2, 8
    or      t0, t0, t1; or t2, t2, t3; or t0, t0, t2; sw t0, 16(a3)

    lbu     t0, 20(a1); lbu t1, 21(a1); lbu t2, 22(a1); lbu t3, 23(a1)
    slli    t0, t0, 24; slli t1, t1, 16; slli t2, t2, 8
    or      t0, t0, t1; or t2, t2, t3; or t0, t0, t2; sw t0, 20(a3)

    lbu     t0, 24(a1); lbu t1, 25(a1); lbu t2, 26(a1); lbu t3, 27(a1)
    slli    t0, t0, 24; slli t1, t1, 16; slli t2, t2, 8
    or      t0, t0, t1; or t2, t2, t3; or t0, t0, t2; sw t0, 24(a3)

    lbu     t0, 28(a1); lbu t1, 29(a1); lbu t2, 30(a1); lbu t3, 31(a1)
    slli    t0, t0, 24; slli t1, t1, 16; slli t2, t2, 8
    or      t0, t0, t1; or t2, t2, t3; or t0, t0, t2; sw t0, 28(a3)

    lbu     t0, 32(a1); lbu t1, 33(a1); lbu t2, 34(a1); lbu t3, 35(a1)
    slli    t0, t0, 24; slli t1, t1, 16; slli t2, t2, 8
    or      t0, t0, t1; or t2, t2, t3; or t0, t0, t2; sw t0, 32(a3)

    lbu     t0, 36(a1); lbu t1, 37(a1); lbu t2, 38(a1); lbu t3, 39(a1)
    slli    t0, t0, 24; slli t1, t1, 16; slli t2, t2, 8
    or      t0, t0, t1; or t2, t2, t3; or t0, t0, t2; sw t0, 36(a3)

    lbu     t0, 40(a1); lbu t1, 41(a1); lbu t2, 42(a1); lbu t3, 43(a1)
    slli    t0, t0, 24; slli t1, t1, 16; slli t2, t2, 8
    or      t0, t0, t1; or t2, t2, t3; or t0, t0, t2; sw t0, 40(a3)

    lbu     t0, 44(a1); lbu t1, 45(a1); lbu t2, 46(a1); lbu t3, 47(a1)
    slli    t0, t0, 24; slli t1, t1, 16; slli t2, t2, 8
    or      t0, t0, t1; or t2, t2, t3; or t0, t0, t2; sw t0, 44(a3)

    lbu     t0, 48(a1); lbu t1, 49(a1); lbu t2, 50(a1); lbu t3, 51(a1)
    slli    t0, t0, 24; slli t1, t1, 16; slli t2, t2, 8
    or      t0, t0, t1; or t2, t2, t3; or t0, t0, t2; sw t0, 48(a3)

    lbu     t0, 52(a1); lbu t1, 53(a1); lbu t2, 54(a1); lbu t3, 55(a1)
    slli    t0, t0, 24; slli t1, t1, 16; slli t2, t2, 8
    or      t0, t0, t1; or t2, t2, t3; or t0, t0, t2; sw t0, 52(a3)

    lbu     t0, 56(a1); lbu t1, 57(a1); lbu t2, 58(a1); lbu t3, 59(a1)
    slli    t0, t0, 24; slli t1, t1, 16; slli t2, t2, 8
    or      t0, t0, t1; or t2, t2, t3; or t0, t0, t2; sw t0, 56(a3)

    lbu     t0, 60(a1); lbu t1, 61(a1); lbu t2, 62(a1); lbu t3, 63(a1)
    slli    t0, t0, 24; slli t1, t1, 16; slli t2, t2, 8
    or      t0, t0, t1; or t2, t2, t3; or t0, t0, t2; sw t0, 60(a3)

    la      a2, .LK
    li      a4, 0

.Lloop:
    // Compute W[i] for i >= 16 (ring buffer, slot = i%16)
    li      t0, 16
    blt     a4, t0, .Luse_w

    // W[i-2]
    addi    t0, a4, -2
    andi    t0, t0, 15
    slli    t0, t0, 2
    add     t0, a3, t0
    lw      t1, 0(t0)
    sha256sig1 t1, t1

    // W[i-7]
    addi    t0, a4, -7
    andi    t0, t0, 15
    slli    t0, t0, 2
    add     t0, a3, t0
    lw      t2, 0(t0)
    addw    t1, t1, t2

    // W[i-15]
    addi    t0, a4, -15
    andi    t0, t0, 15
    slli    t0, t0, 2
    add     t0, a3, t0
    lw      t2, 0(t0)
    sha256sig0 t2, t2
    addw    t1, t1, t2

    // W[i-16]
    addi    t0, a4, -16
    andi    t0, t0, 15
    slli    t0, t0, 2
    add     t0, a3, t0
    lw      t2, 0(t0)
    addw    t1, t1, t2

    // Store W[i] into ring slot i%16
    andi    t0, a4, 15
    slli    t0, t0, 2
    add     t0, a3, t0
    sw      t1, 0(t0)
    j       .Lround

.Luse_w:
    andi    t0, a4, 15
    slli    t0, t0, 2
    add     t0, a3, t0
    lw      t1, 0(t0)

.Lround:
    lw      t0, 0(a2)
    addi    a2, a2, 4

    // T1 = H + Sigma1(E) + Ch(E,F,G) + K[i] + W[i]
    sha256sum1 t2, s4
    and     t3, s4, s5
    not     t4, s4
    and     t4, t4, s6
    xor     t3, t3, t4
    addw    t2, t2, t3
    addw    t2, t2, s7
    addw    t2, t2, t0
    addw    t2, t2, t1

    // T2 = Sigma0(A) + Maj(A,B,C)
    sha256sum0 t3, s0
    and     t0, s0, s1
    and     t1, s0, s2
    and     t4, s1, s2
    xor     t0, t0, t1
    xor     t0, t0, t4
    addw    t3, t3, t0

    mv      s7, s6
    mv      s6, s5
    mv      s5, s4
    addw    s4, s3, t2
    mv      s3, s2
    mv      s2, s1
    mv      s1, s0
    addw    s0, t2, t3

    addi    a4, a4, 1
    li      t0, 64
    blt     a4, t0, .Lloop

    ld      a0, 104(sp)
    lw      t0,  0(a0); addw s0, s0, t0; sw s0,  0(a0)
    lw      t0,  4(a0); addw s1, s1, t0; sw s1,  4(a0)
    lw      t0,  8(a0); addw s2, s2, t0; sw s2,  8(a0)
    lw      t0, 12(a0); addw s3, s3, t0; sw s3, 12(a0)
    lw      t0, 16(a0); addw s4, s4, t0; sw s4, 16(a0)
    lw      t0, 20(a0); addw s5, s5, t0; sw s5, 20(a0)
    lw      t0, 24(a0); addw s6, s6, t0; sw s6, 24(a0)
    lw      t0, 28(a0); addw s7, s7, t0; sw s7, 28(a0)

    ld      ra,   0(sp)
    ld      s0,   8(sp)
    ld      s1,  16(sp)
    ld      s2,  24(sp)
    ld      s3,  32(sp)
    ld      s4,  40(sp)
    ld      s5,  48(sp)
    ld      s6,  56(sp)
    ld      s7,  64(sp)
    ld      s8,  72(sp)
    ld      s9,  80(sp)
    ld      s10, 88(sp)
    ld      s11, 96(sp)
    addi    sp, sp, 176
    ret
    .size apg_sha256_transform_hw, .-apg_sha256_transform_hw

    .section .rodata
    .align 4
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
