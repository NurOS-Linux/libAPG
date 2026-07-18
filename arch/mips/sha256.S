// SPDX-License-Identifier: GPL-3.0-only
// SPDX-FileCopyrightText: 2026 Meigo™ Corporation (https://github.com/meigoc)

    .set    reorder
    .set    mips32r2
    .text

    .globl  apg_sha256_hw_supported
    .type   apg_sha256_hw_supported, @function
apg_sha256_hw_supported:
    li      $v0, 1
    jr      $ra
    .size   apg_sha256_hw_supported, .-apg_sha256_hw_supported

    .globl  apg_sha256_transform_hw
    .type   apg_sha256_transform_hw, @function
    .align  2
apg_sha256_transform_hw:
    addiu   $sp, $sp, -296
    sw      $s0, 0($sp)
    sw      $s1, 4($sp)
    sw      $s2, 8($sp)
    sw      $s3, 12($sp)
    sw      $s4, 16($sp)
    sw      $s5, 20($sp)
    sw      $s6, 24($sp)
    sw      $s7, 28($sp)
    sw      $fp, 32($sp)
    sw      $a0, 36($sp)

    li      $t7, 0
.Lldw:
    sll     $t0, $t7, 2
    addu    $t0, $a1, $t0
    lbu     $t1, 0($t0)
    lbu     $t2, 1($t0)
    sll     $t1, $t1, 8
    or      $t1, $t1, $t2
    lbu     $t2, 2($t0)
    sll     $t1, $t1, 8
    or      $t1, $t1, $t2
    lbu     $t2, 3($t0)
    sll     $t1, $t1, 8
    or      $t1, $t1, $t2
    sll     $t0, $t7, 2
    addiu   $t3, $sp, 40
    addu    $t0, $t3, $t0
    sw      $t1, 0($t0)
    addiu   $t7, $t7, 1
    li      $t0, 16
    blt     $t7, $t0, .Lldw

    li      $t7, 16
.Lexp:
    addiu   $t0, $t7, -2
    sll     $t0, $t0, 2
    addiu   $t1, $sp, 40
    addu    $t0, $t1, $t0
    lw      $t2, 0($t0)
    rotr    $t3, $t2, 17
    rotr    $t4, $t2, 19
    xor     $t3, $t3, $t4
    srl     $t4, $t2, 10
    xor     $t3, $t3, $t4
    addiu   $t0, $t7, -7
    sll     $t0, $t0, 2
    addiu   $t1, $sp, 40
    addu    $t0, $t1, $t0
    lw      $t4, 0($t0)
    addu    $t3, $t3, $t4
    addiu   $t0, $t7, -15
    sll     $t0, $t0, 2
    addiu   $t1, $sp, 40
    addu    $t0, $t1, $t0
    lw      $t2, 0($t0)
    rotr    $t4, $t2, 7
    rotr    $t5, $t2, 18
    xor     $t4, $t4, $t5
    srl     $t5, $t2, 3
    xor     $t4, $t4, $t5
    addu    $t3, $t3, $t4
    addiu   $t0, $t7, -16
    sll     $t0, $t0, 2
    addiu   $t1, $sp, 40
    addu    $t0, $t1, $t0
    lw      $t4, 0($t0)
    addu    $t3, $t3, $t4
    sll     $t0, $t7, 2
    addiu   $t1, $sp, 40
    addu    $t0, $t1, $t0
    sw      $t3, 0($t0)
    addiu   $t7, $t7, 1
    li      $t0, 64
    blt     $t7, $t0, .Lexp

    lw      $t8, 36($sp)
    lw      $s0, 0($t8)
    lw      $s1, 4($t8)
    lw      $s2, 8($t8)
    lw      $s3, 12($t8)
    lw      $s4, 16($t8)
    lw      $s5, 20($t8)
    lw      $s6, 24($t8)
    lw      $s7, 28($t8)

    li      $fp, 0

.Lround:
    la      $t0, .LK
    sll     $t1, $fp, 2
    addu    $t0, $t0, $t1
    lw      $t0, 0($t0)
    addiu   $t1, $sp, 40
    sll     $t2, $fp, 2
    addu    $t1, $t1, $t2
    lw      $t1, 0($t1)
    rotr    $t2, $s4, 6
    rotr    $t3, $s4, 11
    xor     $t2, $t2, $t3
    rotr    $t3, $s4, 25
    xor     $t2, $t2, $t3
    and     $t3, $s4, $s5
    nor     $t4, $s4, $zero
    and     $t4, $t4, $s6
    xor     $t3, $t3, $t4
    addu    $t2, $t2, $t3
    addu    $t2, $t2, $s7
    addu    $t2, $t2, $t0
    addu    $t2, $t2, $t1
    rotr    $t3, $s0, 2
    rotr    $t4, $s0, 13
    xor     $t3, $t3, $t4
    rotr    $t4, $s0, 22
    xor     $t3, $t3, $t4
    and     $t4, $s0, $s1
    and     $t5, $s0, $s2
    xor     $t4, $t4, $t5
    and     $t5, $s1, $s2
    xor     $t4, $t4, $t5
    addu    $t3, $t3, $t4

    move    $s7, $s6
    move    $s6, $s5
    move    $s5, $s4
    addu    $s4, $s3, $t2
    move    $s3, $s2
    move    $s2, $s1
    move    $s1, $s0
    addu    $s0, $t2, $t3

    addiu   $fp, $fp, 1
    li      $t0, 64
    blt     $fp, $t0, .Lround

    lw      $t8, 36($sp)
    lw      $t0, 0($t8)
    addu    $s0, $s0, $t0
    sw      $s0, 0($t8)
    lw      $t0, 4($t8)
    addu    $s1, $s1, $t0
    sw      $s1, 4($t8)
    lw      $t0, 8($t8)
    addu    $s2, $s2, $t0
    sw      $s2, 8($t8)
    lw      $t0, 12($t8)
    addu    $s3, $s3, $t0
    sw      $s3, 12($t8)
    lw      $t0, 16($t8)
    addu    $s4, $s4, $t0
    sw      $s4, 16($t8)
    lw      $t0, 20($t8)
    addu    $s5, $s5, $t0
    sw      $s5, 20($t8)
    lw      $t0, 24($t8)
    addu    $s6, $s6, $t0
    sw      $s6, 24($t8)
    lw      $t0, 28($t8)
    addu    $s7, $s7, $t0
    sw      $s7, 28($t8)

    lw      $s0, 0($sp)
    lw      $s1, 4($sp)
    lw      $s2, 8($sp)
    lw      $s3, 12($sp)
    lw      $s4, 16($sp)
    lw      $s5, 20($sp)
    lw      $s6, 24($sp)
    lw      $s7, 28($sp)
    lw      $fp, 32($sp)
    addiu   $sp, $sp, 296
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

    .section .note.GNU-stack,"",@progbits
