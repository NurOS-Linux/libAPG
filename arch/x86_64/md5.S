// SPDX-License-Identifier: GPL-3.0-only
// SPDX-FileCopyrightText: 2026 AnmiTaliDev <anmitalidev@nuros.org>
// MD5 transform — optimized scalar, all state in registers.
// Syntax: Intel (.intel_syntax noprefix) — compatible with GAS and Clang IAS.
//
// int  apg_md5_hw_supported(void)
// void apg_md5_transform_hw(uint32_t state[4], const uint8_t block[64])

    .intel_syntax noprefix

    .text

    .globl apg_md5_hw_supported
    .type  apg_md5_hw_supported, @function
apg_md5_hw_supported:
    mov     eax, 1
    ret
    .size apg_md5_hw_supported, .-apg_md5_hw_supported

    .globl apg_md5_transform_hw
    .type  apg_md5_transform_hw, @function
    .align 16
apg_md5_transform_hw:
    // rdi = state[4], rsi = block[64]
    push    rbx
    push    r12
    push    r13
    push    r14
    push    r15
    push    rbp

    mov     rbp, rdi            // save state ptr
    mov     r15, rsi            // save block ptr

    // a=r8d, b=r9d, c=r10d, d=r11d
    mov     r8d,  [rdi]
    mov     r9d,  [rdi+4]
    mov     r10d, [rdi+8]
    mov     r11d, [rdi+12]

    lea     r12, [rip + .LT]
    lea     r13, [rip + .LS]
    lea     r14, [rip + .LG]
    xor     rbx, rbx            // round counter

.Lrnd:
    cmp     rbx, 48
    jge     .Lf_I
    cmp     rbx, 32
    jge     .Lf_H
    cmp     rbx, 16
    jge     .Lf_G
    // F(b,c,d) = d ^ (b & (c^d))
    mov     eax, r10d
    xor     eax, r11d
    and     eax, r9d
    xor     eax, r11d
    jmp     .Ldo_round
.Lf_G:
    // G(b,c,d) = c ^ (d & (b^c))
    mov     eax, r9d
    xor     eax, r10d
    and     eax, r11d
    xor     eax, r10d
    jmp     .Ldo_round
.Lf_H:
    // H(b,c,d) = b^c^d
    mov     eax, r9d
    xor     eax, r10d
    xor     eax, r11d
    jmp     .Ldo_round
.Lf_I:
    // I(b,c,d) = c ^ (b | ~d)
    mov     eax, r11d
    not     eax
    or      eax, r9d
    xor     eax, r10d

.Ldo_round:
    movzx   edx, byte ptr [r14 + rbx]       // g
    add     eax, r8d                         // f + a
    add     eax, dword ptr [r15 + rdx*4]    // + M[g]
    add     eax, dword ptr [r12 + rbx*4]    // + T[i]
    movzx   ecx, byte ptr [r13 + rbx]       // S[i]
    rol     eax, cl                          // ROTL(result, S[i])
    add     eax, r9d                         // + b
    // rotate: a=d, d=c, c=b, b=result
    mov     r8d,  r11d
    mov     r11d, r10d
    mov     r10d, r9d
    mov     r9d,  eax
    inc     rbx
    cmp     rbx, 64
    jl      .Lrnd

    add     dword ptr [rbp],    r8d
    add     dword ptr [rbp+4],  r9d
    add     dword ptr [rbp+8],  r10d
    add     dword ptr [rbp+12], r11d

    pop     rbp
    pop     r15
    pop     r14
    pop     r13
    pop     r12
    pop     rbx
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

    .section .note.GNU-stack,"",@progbits
