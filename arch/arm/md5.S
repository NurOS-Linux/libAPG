/* SPDX-License-Identifier: GPL-3.0-only */
/* SPDX-FileCopyrightText: 2026 Meigo™ Corporation (https://github.com/meigoc) */

    .text
    .syntax unified
    .arm

    .globl  apg_md5_hw_supported
    .type   apg_md5_hw_supported, %function
apg_md5_hw_supported:
    mov     r0, #1
    bx      lr
    .size   apg_md5_hw_supported, .-apg_md5_hw_supported

    .globl  apg_md5_transform_hw
    .type   apg_md5_transform_hw, %function
apg_md5_transform_hw:
    push    {r4-r11, lr}
    sub     sp, sp, #64

    mov     r8, r0

    mov     r2, #0
.Lmld:
    add     r3, r1, r2, lsl #2
    ldrb    r0, [r3, #0]
    ldrb    r12, [r3, #1]
    orr     r0, r0, r12, lsl #8
    ldrb    r12, [r3, #2]
    orr     r0, r0, r12, lsl #16
    ldrb    r12, [r3, #3]
    orr     r0, r0, r12, lsl #24
    str     r0, [sp, r2, lsl #2]
    add     r2, r2, #1
    cmp     r2, #16
    blo     .Lmld

    ldr     r4, [r8, #0]
    ldr     r5, [r8, #4]
    ldr     r6, [r8, #8]
    ldr     r7, [r8, #12]

    movw    r10, #:lower16:(.LT - (1f + 8))
    movt    r10, #:upper16:(.LT - (1f + 8))
1:  add     r10, pc, r10
    movw    r11, #:lower16:(.LS - (2f + 8))
    movt    r11, #:upper16:(.LS - (2f + 8))
2:  add     r11, pc, r11
    mov     r9, #0

.Lrnd:
    cmp     r9, #48
    bhs     .LfI
    cmp     r9, #32
    bhs     .LfH
    cmp     r9, #16
    bhs     .LfG
    eor     r2, r6, r7
    and     r2, r2, r5
    eor     r2, r2, r7
    mov     r3, r9
    b       .Lgot
.LfG:
    eor     r2, r5, r6
    and     r2, r2, r7
    eor     r2, r2, r6
    add     r3, r9, r9, lsl #2
    add     r3, r3, #1
    and     r3, r3, #15
    b       .Lgot
.LfH:
    eor     r2, r5, r6
    eor     r2, r2, r7
    add     r3, r9, r9, lsl #1
    add     r3, r3, #5
    and     r3, r3, #15
    b       .Lgot
.LfI:
    mvn     r3, r7
    orr     r3, r3, r5
    eor     r2, r3, r6
    lsl     r3, r9, #3
    sub     r3, r3, r9
    and     r3, r3, #15
.Lgot:
    ldr     r1, [sp, r3, lsl #2]
    ldr     r0, [r10, r9, lsl #2]
    add     r2, r2, r4
    add     r2, r2, r1
    add     r2, r2, r0
    ldrb    r12, [r11, r9]
    rsb     r0, r12, #32
    lsl     r1, r2, r12
    lsr     r0, r2, r0
    orr     r2, r1, r0
    add     r2, r2, r5

    mov     r4, r7
    mov     r7, r6
    mov     r6, r5
    mov     r5, r2

    add     r9, r9, #1
    cmp     r9, #64
    blo     .Lrnd

    ldr     r0, [r8, #0]
    add     r0, r0, r4
    str     r0, [r8, #0]
    ldr     r0, [r8, #4]
    add     r0, r0, r5
    str     r0, [r8, #4]
    ldr     r0, [r8, #8]
    add     r0, r0, r6
    str     r0, [r8, #8]
    ldr     r0, [r8, #12]
    add     r0, r0, r7
    str     r0, [r8, #12]

    add     sp, sp, #64
    pop     {r4-r11, lr}
    bx      lr
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

    .section .note.GNU-stack,"",%progbits
