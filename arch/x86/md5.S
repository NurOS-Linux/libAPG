/* SPDX-License-Identifier: GPL-3.0-only */
/* SPDX-FileCopyrightText: 2026 Meigo™ Corporation (https://github.com/meigoc) */

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
apg_md5_transform_hw:
    push    ebx
    push    esi
    push    edi
    push    ebp
    sub     esp, 96
    call    __x86.get_pc_thunk.bx
    add     ebx, offset _GLOBAL_OFFSET_TABLE_

    mov     eax, [esp+116]
    mov     [esp+80], eax
    mov     esi, [esp+120]

    xor     ecx, ecx
.Lmld:
    mov     edx, [esi + ecx*4]
    mov     [esp + ecx*4], edx
    inc     ecx
    cmp     ecx, 16
    jb      .Lmld

    mov     eax, [esp+80]
    mov     edx, [eax+0]
    mov     [esp+64], edx
    mov     edx, [eax+4]
    mov     [esp+68], edx
    mov     edx, [eax+8]
    mov     [esp+72], edx
    mov     edx, [eax+12]
    mov     [esp+76], edx

    xor     ebp, ebp
.Lrnd:
    cmp     ebp, 48
    jae     .LfI
    cmp     ebp, 32
    jae     .LfH
    cmp     ebp, 16
    jae     .LfG
    mov     edx, [esp+72]
    xor     edx, [esp+76]
    and     edx, [esp+68]
    xor     edx, [esp+76]
    mov     esi, ebp
    jmp     .Lgot
.LfG:
    mov     edx, [esp+68]
    xor     edx, [esp+72]
    and     edx, [esp+76]
    xor     edx, [esp+72]
    lea     esi, [ebp + ebp*4]
    inc     esi
    and     esi, 15
    jmp     .Lgot
.LfH:
    mov     edx, [esp+68]
    xor     edx, [esp+72]
    xor     edx, [esp+76]
    lea     esi, [ebp + ebp*2]
    add     esi, 5
    and     esi, 15
    jmp     .Lgot
.LfI:
    mov     edx, [esp+76]
    not     edx
    or      edx, [esp+68]
    xor     edx, [esp+72]
    mov     esi, ebp
    shl     esi, 3
    sub     esi, ebp
    and     esi, 15
.Lgot:
    add     edx, [esp+64]
    add     edx, [esp + esi*4]
    lea     eax, [ebx + .LT@GOTOFF]
    add     edx, [eax + ebp*4]
    lea     eax, [ebx + .LS@GOTOFF]
    mov     cl, [eax + ebp]
    rol     edx, cl
    add     edx, [esp+68]

    mov     eax, [esp+76]
    mov     [esp+64], eax
    mov     eax, [esp+72]
    mov     [esp+76], eax
    mov     eax, [esp+68]
    mov     [esp+72], eax
    mov     [esp+68], edx

    inc     ebp
    cmp     ebp, 64
    jb      .Lrnd

    mov     eax, [esp+80]
    mov     edx, [esp+64]
    add     [eax+0], edx
    mov     edx, [esp+68]
    add     [eax+4], edx
    mov     edx, [esp+72]
    add     [eax+8], edx
    mov     edx, [esp+76]
    add     [eax+12], edx

    add     esp, 96
    pop     ebp
    pop     edi
    pop     esi
    pop     ebx
    ret
    .size apg_md5_transform_hw, .-apg_md5_transform_hw

    .section .text.__x86.get_pc_thunk.bx,"axG",@progbits,__x86.get_pc_thunk.bx,comdat
    .globl  __x86.get_pc_thunk.bx
    .hidden __x86.get_pc_thunk.bx
    .type   __x86.get_pc_thunk.bx, @function
__x86.get_pc_thunk.bx:
    mov     ebx, [esp]
    ret

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

    .section .note.GNU-stack,"",@progbits
