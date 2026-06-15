// SPDX-License-Identifier: GPL-3.0-only
// SPDX-FileCopyrightText: 2026 AnmiTaliDev <anmitalidev@nuros.org>
// SHA-256 — optimized scalar MIPS64 assembly (MIPS64r2, N64 ABI).
// Uses rotr for bit-rotates; no hardware crypto extension required.
// Syntax: MIPS unified assembly — compatible with GAS and Clang IAS.
//
// int  apg_sha256_hw_supported(void)
// void apg_sha256_transform_hw(uint32_t state[8], const uint8_t data[64])

    .set    reorder
    .set    mips64r2
    .text

    .globl  apg_sha256_hw_supported
    .type   apg_sha256_hw_supported, @function
apg_sha256_hw_supported:
    li      $v0, 1
    jr      $ra
    .size   apg_sha256_hw_supported, .-apg_sha256_hw_supported

// void apg_sha256_transform_hw(uint32_t state[8], const uint8_t data[64])
// $a0 = state, $a1 = data
//
// Register allocation:
//   $s0-$s7  = A..H (state words)
//   $s8      = W ring-buffer pointer ($sp+96)
//   $t8      = saved state pointer
//   $t9      = K[] pointer
//   $a2      = round counter
    .globl  apg_sha256_transform_hw
    .type   apg_sha256_transform_hw, @function
    .align  3
apg_sha256_transform_hw:
    // Frame: ra s0-s7 s8 a0 pad W[16]
    // 11*8 + 8 + 64 = 160 bytes (16-byte aligned)
    daddiu  $sp, $sp, -160
    sd      $ra,   0($sp)
    sd      $s0,   8($sp)
    sd      $s1,  16($sp)
    sd      $s2,  24($sp)
    sd      $s3,  32($sp)
    sd      $s4,  40($sp)
    sd      $s5,  48($sp)
    sd      $s6,  56($sp)
    sd      $s7,  64($sp)
    sd      $s8,  72($sp)
    sd      $a0,  80($sp)

    lw      $s0,  0($a0)
    lw      $s1,  4($a0)
    lw      $s2,  8($a0)
    lw      $s3, 12($a0)
    lw      $s4, 16($a0)
    lw      $s5, 20($a0)
    lw      $s6, 24($a0)
    lw      $s7, 28($a0)

    daddiu  $s8, $sp, 96           // W ring buffer

    // Load W[0..15] big-endian from data into ring buffer
    li      $a2, 0
.Lload_w:
    dsll    $t0, $a2, 2
    daddu   $t0, $t0, $a1          // &data[i*4]
    lbu     $t2, 0($t0)
    lbu     $t3, 1($t0)
    lbu     $t4, 2($t0)
    lbu     $t5, 3($t0)
    sll     $t2, $t2, 24
    sll     $t3, $t3, 16
    sll     $t4, $t4, 8
    or      $t2, $t2, $t3
    or      $t4, $t4, $t5
    or      $t2, $t2, $t4
    dsll    $t3, $a2, 2
    daddu   $t3, $t3, $s8
    sw      $t2, 0($t3)
    addiu   $a2, $a2, 1
    li      $t0, 16
    blt     $a2, $t0, .Lload_w

    dla     $t9, .LK
    li      $a2, 0

.Lmainloop:
    li      $t0, 16
    blt     $a2, $t0, .Luse_w

    // sigma1(W[i-2]) = ROTR(x,17) ^ ROTR(x,19) ^ (x >> 10)
    addiu   $t0, $a2, -2
    andi    $t0, $t0, 15
    sll     $t0, $t0, 2
    daddu   $t0, $t0, $s8
    lw      $t1, 0($t0)
    rotr    $t2, $t1, 17
    rotr    $t3, $t1, 19
    srl     $t4, $t1, 10
    xor     $t2, $t2, $t3
    xor     $t1, $t2, $t4

    // W[i-7]
    addiu   $t0, $a2, -7
    andi    $t0, $t0, 15
    sll     $t0, $t0, 2
    daddu   $t0, $t0, $s8
    lw      $t2, 0($t0)
    addu    $t1, $t1, $t2

    // sigma0(W[i-15]) = ROTR(x,7) ^ ROTR(x,18) ^ (x >> 3)
    addiu   $t0, $a2, -15
    andi    $t0, $t0, 15
    sll     $t0, $t0, 2
    daddu   $t0, $t0, $s8
    lw      $t2, 0($t0)
    rotr    $t3, $t2, 7
    rotr    $t4, $t2, 18
    srl     $t5, $t2, 3
    xor     $t3, $t3, $t4
    xor     $t2, $t3, $t5
    addu    $t1, $t1, $t2

    // W[i-16]
    addiu   $t0, $a2, -16
    andi    $t0, $t0, 15
    sll     $t0, $t0, 2
    daddu   $t0, $t0, $s8
    lw      $t2, 0($t0)
    addu    $t1, $t1, $t2

    andi    $t0, $a2, 15
    sll     $t0, $t0, 2
    daddu   $t0, $t0, $s8
    sw      $t1, 0($t0)
    b       .Lround

.Luse_w:
    andi    $t0, $a2, 15
    sll     $t0, $t0, 2
    daddu   $t0, $t0, $s8
    lw      $t1, 0($t0)

.Lround:
    lw      $t0, 0($t9)
    daddiu  $t9, $t9, 4

    // Sigma1(E) = ROTR(E,6) ^ ROTR(E,11) ^ ROTR(E,25)
    rotr    $t2, $s4, 6
    rotr    $t3, $s4, 11
    rotr    $t4, $s4, 25
    xor     $t2, $t2, $t3
    xor     $t2, $t2, $t4

    // Ch(E,F,G) = (E & F) ^ (~E & G)
    and     $t3, $s4, $s5
    nor     $t4, $s4, $zero
    and     $t4, $t4, $s6
    xor     $t3, $t3, $t4

    // T1 = H + Sigma1(E) + Ch(E,F,G) + K[i] + W[i]
    addu    $t2, $t2, $t3
    addu    $t2, $t2, $s7
    addu    $t2, $t2, $t0
    addu    $t2, $t2, $t1

    // Sigma0(A) = ROTR(A,2) ^ ROTR(A,13) ^ ROTR(A,22)
    rotr    $t3, $s0, 2
    rotr    $t4, $s0, 13
    rotr    $t5, $s0, 22
    xor     $t3, $t3, $t4
    xor     $t3, $t3, $t5

    // Maj(A,B,C) = (A&B) ^ (A&C) ^ (B&C)
    and     $t4, $s0, $s1
    and     $t5, $s0, $s2
    and     $t6, $s1, $s2
    xor     $t4, $t4, $t5
    xor     $t4, $t4, $t6

    // T2 = Sigma0(A) + Maj(A,B,C)
    addu    $t3, $t3, $t4

    move    $s7, $s6
    move    $s6, $s5
    move    $s5, $s4
    addu    $s4, $s3, $t2
    move    $s3, $s2
    move    $s2, $s1
    move    $s1, $s0
    addu    $s0, $t2, $t3

    addiu   $a2, $a2, 1
    li      $t0, 64
    blt     $a2, $t0, .Lmainloop

    ld      $a0,  80($sp)
    lw      $t0,  0($a0); addu $s0, $s0, $t0; sw $s0,  0($a0)
    lw      $t0,  4($a0); addu $s1, $s1, $t0; sw $s1,  4($a0)
    lw      $t0,  8($a0); addu $s2, $s2, $t0; sw $s2,  8($a0)
    lw      $t0, 12($a0); addu $s3, $s3, $t0; sw $s3, 12($a0)
    lw      $t0, 16($a0); addu $s4, $s4, $t0; sw $s4, 16($a0)
    lw      $t0, 20($a0); addu $s5, $s5, $t0; sw $s5, 20($a0)
    lw      $t0, 24($a0); addu $s6, $s6, $t0; sw $s6, 24($a0)
    lw      $t0, 28($a0); addu $s7, $s7, $t0; sw $s7, 28($a0)

    ld      $ra,  0($sp)
    ld      $s0,  8($sp)
    ld      $s1, 16($sp)
    ld      $s2, 24($sp)
    ld      $s3, 32($sp)
    ld      $s4, 40($sp)
    ld      $s5, 48($sp)
    ld      $s6, 56($sp)
    ld      $s7, 64($sp)
    ld      $s8, 72($sp)
    daddiu  $sp, $sp, 160
    jr      $ra
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
