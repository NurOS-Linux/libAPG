// SPDX-License-Identifier: GPL-3.0-only
// SPDX-FileCopyrightText: 2026 Meigo™ Corporation (https://github.com/meigoc)

    .set    reorder
    .set    mips32r2
    .text

    .globl  apg_md5_hw_supported
    .type   apg_md5_hw_supported, @function
apg_md5_hw_supported:
    li      $v0, 1
    jr      $ra
    .size   apg_md5_hw_supported, .-apg_md5_hw_supported

    .globl  apg_md5_transform_hw
    .type   apg_md5_transform_hw, @function
    .align  2
apg_md5_transform_hw:
    addiu   $sp, $sp, -96
    sw      $s0, 0($sp)
    sw      $s1, 4($sp)
    sw      $s2, 8($sp)
    sw      $s3, 12($sp)
    sw      $s4, 16($sp)
    sw      $s5, 20($sp)
    sw      $s6, 24($sp)
    sw      $s7, 28($sp)

    move    $s4, $a0

    li      $s5, 0
.Lmld:
    sll     $t0, $s5, 2
    addu    $t0, $a1, $t0
    lbu     $t1, 0($t0)
    lbu     $t2, 1($t0)
    sll     $t2, $t2, 8
    or      $t1, $t1, $t2
    lbu     $t2, 2($t0)
    sll     $t2, $t2, 16
    or      $t1, $t1, $t2
    lbu     $t2, 3($t0)
    sll     $t2, $t2, 24
    or      $t1, $t1, $t2
    sll     $t0, $s5, 2
    addiu   $t3, $sp, 32
    addu    $t0, $t3, $t0
    sw      $t1, 0($t0)
    addiu   $s5, $s5, 1
    li      $t0, 16
    blt     $s5, $t0, .Lmld

    lw      $s0, 0($s4)
    lw      $s1, 4($s4)
    lw      $s2, 8($s4)
    lw      $s3, 12($s4)

    la      $s6, .LT
    la      $s7, .LS
    li      $s5, 0

.Lrnd:
    li      $t0, 48
    bge     $s5, $t0, .LfI
    li      $t0, 32
    bge     $s5, $t0, .LfH
    li      $t0, 16
    bge     $s5, $t0, .LfG
    xor     $t1, $s2, $s3
    and     $t1, $t1, $s1
    xor     $t1, $t1, $s3
    move    $t2, $s5
    b       .Lgot
.LfG:
    xor     $t1, $s1, $s2
    and     $t1, $t1, $s3
    xor     $t1, $t1, $s2
    sll     $t2, $s5, 2
    addu    $t2, $t2, $s5
    addiu   $t2, $t2, 1
    andi    $t2, $t2, 15
    b       .Lgot
.LfH:
    xor     $t1, $s1, $s2
    xor     $t1, $t1, $s3
    sll     $t2, $s5, 1
    addu    $t2, $t2, $s5
    addiu   $t2, $t2, 5
    andi    $t2, $t2, 15
    b       .Lgot
.LfI:
    nor     $t2, $s3, $zero
    or      $t2, $t2, $s1
    xor     $t1, $t2, $s2
    sll     $t2, $s5, 3
    subu    $t2, $t2, $s5
    andi    $t2, $t2, 15
.Lgot:
    sll     $t3, $t2, 2
    addiu   $t4, $sp, 32
    addu    $t3, $t4, $t3
    lw      $t3, 0($t3)
    sll     $t4, $s5, 2
    addu    $t4, $s6, $t4
    lw      $t4, 0($t4)
    addu    $t1, $t1, $s0
    addu    $t1, $t1, $t3
    addu    $t1, $t1, $t4
    addu    $t5, $s7, $s5
    lbu     $t5, 0($t5)
    li      $t6, 32
    subu    $t6, $t6, $t5
    rotrv   $t1, $t1, $t6
    addu    $t1, $t1, $s1

    move    $s0, $s3
    move    $s3, $s2
    move    $s2, $s1
    move    $s1, $t1

    addiu   $s5, $s5, 1
    li      $t0, 64
    blt     $s5, $t0, .Lrnd

    lw      $t0, 0($s4)
    addu    $s0, $s0, $t0
    sw      $s0, 0($s4)
    lw      $t0, 4($s4)
    addu    $s1, $s1, $t0
    sw      $s1, 4($s4)
    lw      $t0, 8($s4)
    addu    $s2, $s2, $t0
    sw      $s2, 8($s4)
    lw      $t0, 12($s4)
    addu    $s3, $s3, $t0
    sw      $s3, 12($s4)

    lw      $s0, 0($sp)
    lw      $s1, 4($sp)
    lw      $s2, 8($sp)
    lw      $s3, 12($sp)
    lw      $s4, 16($sp)
    lw      $s5, 20($sp)
    lw      $s6, 24($sp)
    lw      $s7, 28($sp)
    addiu   $sp, $sp, 96
    jr      $ra
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
