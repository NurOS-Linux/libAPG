/* SPDX-License-Identifier: GPL-3.0-only */
/* SPDX-FileCopyrightText: 2026 Meigo™ Corporation (https://github.com/meigoc) */

    .text
    .syntax unified
    .arm

    .globl  apg_sha256_hw_supported
    .type   apg_sha256_hw_supported, %function
apg_sha256_hw_supported:
    mov     r0, #1
    bx      lr
    .size   apg_sha256_hw_supported, .-apg_sha256_hw_supported

    .globl  apg_sha256_transform_hw
    .type   apg_sha256_transform_hw, %function
apg_sha256_transform_hw:
    push    {r4-r11, lr}
    sub     sp, sp, #264
    str     r0, [sp, #256]

    mov     r2, #0
.Lldw:
    add     r3, r1, r2, lsl #2
    ldrb    r0, [r3, #0]
    ldrb    r12, [r3, #1]
    lsl     r0, r0, #8
    orr     r0, r0, r12
    ldrb    r12, [r3, #2]
    lsl     r0, r0, #8
    orr     r0, r0, r12
    ldrb    r12, [r3, #3]
    lsl     r0, r0, #8
    orr     r0, r0, r12
    str     r0, [sp, r2, lsl #2]
    add     r2, r2, #1
    cmp     r2, #16
    blo     .Lldw

    mov     r2, #16
.Lexp:
    sub     r3, r2, #2
    ldr     r0, [sp, r3, lsl #2]
    ror     r1, r0, #17
    eor     r1, r1, r0, ror #19
    eor     r1, r1, r0, lsr #10
    sub     r3, r2, #7
    ldr     r0, [sp, r3, lsl #2]
    add     r1, r1, r0
    sub     r3, r2, #15
    ldr     r0, [sp, r3, lsl #2]
    ror     r12, r0, #7
    eor     r12, r12, r0, ror #18
    eor     r12, r12, r0, lsr #3
    add     r1, r1, r12
    sub     r3, r2, #16
    ldr     r0, [sp, r3, lsl #2]
    add     r1, r1, r0
    str     r1, [sp, r2, lsl #2]
    add     r2, r2, #1
    cmp     r2, #64
    blo     .Lexp

    ldr     r0, [sp, #256]
    ldr     r4, [r0, #0]
    ldr     r5, [r0, #4]
    ldr     r6, [r0, #8]
    ldr     r7, [r0, #12]
    ldr     r8, [r0, #16]
    ldr     r9, [r0, #20]
    ldr     r10, [r0, #24]
    ldr     r11, [r0, #28]

    movw    r12, #:lower16:(.LK - (3f + 8))
    movt    r12, #:upper16:(.LK - (3f + 8))
3:  add     r12, pc, r12
    mov     lr, sp

.Lround:
    ldr     r0, [r12], #4
    ldr     r1, [lr], #4
    ror     r2, r8, #6
    eor     r2, r2, r8, ror #11
    eor     r2, r2, r8, ror #25
    add     r0, r0, r1
    add     r0, r0, r11
    add     r0, r0, r2
    and     r2, r8, r9
    bic     r3, r10, r8
    eor     r2, r2, r3
    add     r0, r0, r2
    ror     r2, r4, #2
    eor     r2, r2, r4, ror #13
    eor     r2, r2, r4, ror #22
    and     r1, r4, r5
    and     r3, r4, r6
    eor     r1, r1, r3
    and     r3, r5, r6
    eor     r1, r1, r3
    add     r2, r2, r1

    mov     r11, r10
    mov     r10, r9
    mov     r9, r8
    add     r8, r7, r0
    mov     r7, r6
    mov     r6, r5
    mov     r5, r4
    add     r4, r0, r2

    add     r1, sp, #256
    cmp     lr, r1
    blo     .Lround

    ldr     r0, [sp, #256]
    ldr     r1, [r0, #0]
    add     r1, r1, r4
    str     r1, [r0, #0]
    ldr     r1, [r0, #4]
    add     r1, r1, r5
    str     r1, [r0, #4]
    ldr     r1, [r0, #8]
    add     r1, r1, r6
    str     r1, [r0, #8]
    ldr     r1, [r0, #12]
    add     r1, r1, r7
    str     r1, [r0, #12]
    ldr     r1, [r0, #16]
    add     r1, r1, r8
    str     r1, [r0, #16]
    ldr     r1, [r0, #20]
    add     r1, r1, r9
    str     r1, [r0, #20]
    ldr     r1, [r0, #24]
    add     r1, r1, r10
    str     r1, [r0, #24]
    ldr     r1, [r0, #28]
    add     r1, r1, r11
    str     r1, [r0, #28]

    add     sp, sp, #264
    pop     {r4-r11, lr}
    bx      lr
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

    .section .note.GNU-stack,"",%progbits
