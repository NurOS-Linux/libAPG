/* SPDX-License-Identifier: GPL-3.0-only */
/* SPDX-FileCopyrightText: 2026 Meigo™ Corporation (https://github.com/meigoc) */

    .intel_syntax noprefix
    .text

    .globl apg_sha256_hw_supported
    .type  apg_sha256_hw_supported, @function
apg_sha256_hw_supported:
    mov     eax, 1
    ret
    .size apg_sha256_hw_supported, .-apg_sha256_hw_supported

    .globl apg_sha256_transform_hw
    .type  apg_sha256_transform_hw, @function
apg_sha256_transform_hw:
    push    ebx
    push    esi
    push    edi
    push    ebp
    sub     esp, 304
    call    __x86.get_pc_thunk.bx
    add     ebx, offset _GLOBAL_OFFSET_TABLE_

    mov     eax, [esp+324]
    mov     [esp+288], eax
    mov     esi, [esp+328]

    xor     ecx, ecx
.Lldw:
    mov     edx, [esi + ecx*4]
    bswap   edx
    mov     [esp + ecx*4], edx
    inc     ecx
    cmp     ecx, 16
    jb      .Lldw

    mov     ecx, 16
.Lexp:
    mov     eax, [esp + ecx*4 - 8]
    mov     edx, eax
    ror     edx, 17
    mov     edi, eax
    ror     edi, 19
    xor     edx, edi
    shr     eax, 10
    xor     edx, eax
    add     edx, [esp + ecx*4 - 28]
    mov     eax, [esp + ecx*4 - 60]
    mov     edi, eax
    ror     edi, 7
    mov     esi, eax
    ror     esi, 18
    xor     edi, esi
    shr     eax, 3
    xor     edi, eax
    add     edx, edi
    add     edx, [esp + ecx*4 - 64]
    mov     [esp + ecx*4], edx
    inc     ecx
    cmp     ecx, 64
    jb      .Lexp

    mov     eax, [esp+288]
    mov     edx, [eax+0]
    mov     [esp+256], edx
    mov     edx, [eax+4]
    mov     [esp+260], edx
    mov     edx, [eax+8]
    mov     [esp+264], edx
    mov     edx, [eax+12]
    mov     [esp+268], edx
    mov     edx, [eax+16]
    mov     [esp+272], edx
    mov     edx, [eax+20]
    mov     [esp+276], edx
    mov     edx, [eax+24]
    mov     [esp+280], edx
    mov     edx, [eax+28]
    mov     [esp+284], edx

    xor     ebp, ebp
.Lround:
    mov     eax, [esp+272]
    mov     edx, eax
    ror     edx, 6
    mov     esi, eax
    ror     esi, 11
    xor     edx, esi
    mov     esi, eax
    ror     esi, 25
    xor     edx, esi
    mov     esi, eax
    and     esi, [esp+276]
    not     eax
    and     eax, [esp+280]
    xor     esi, eax
    add     edx, esi
    add     edx, [esp+284]
    add     edx, [esp + ebp*4]
    lea     eax, [ebx + .LK@GOTOFF]
    add     edx, [eax + ebp*4]
    mov     edi, edx

    mov     eax, [esp+256]
    mov     edx, eax
    ror     edx, 2
    mov     esi, eax
    ror     esi, 13
    xor     edx, esi
    mov     esi, eax
    ror     esi, 22
    xor     edx, esi
    mov     esi, eax
    and     esi, [esp+260]
    mov     ecx, eax
    and     ecx, [esp+264]
    xor     esi, ecx
    mov     ecx, [esp+260]
    and     ecx, [esp+264]
    xor     esi, ecx
    add     edx, esi

    mov     eax, [esp+280]
    mov     [esp+284], eax
    mov     eax, [esp+276]
    mov     [esp+280], eax
    mov     eax, [esp+272]
    mov     [esp+276], eax
    mov     eax, [esp+268]
    add     eax, edi
    mov     [esp+272], eax
    mov     eax, [esp+264]
    mov     [esp+268], eax
    mov     eax, [esp+260]
    mov     [esp+264], eax
    mov     eax, [esp+256]
    mov     [esp+260], eax
    add     edi, edx
    mov     [esp+256], edi

    inc     ebp
    cmp     ebp, 64
    jb      .Lround

    mov     eax, [esp+288]
    mov     edx, [esp+256]
    add     [eax+0], edx
    mov     edx, [esp+260]
    add     [eax+4], edx
    mov     edx, [esp+264]
    add     [eax+8], edx
    mov     edx, [esp+268]
    add     [eax+12], edx
    mov     edx, [esp+272]
    add     [eax+16], edx
    mov     edx, [esp+276]
    add     [eax+20], edx
    mov     edx, [esp+280]
    add     [eax+24], edx
    mov     edx, [esp+284]
    add     [eax+28], edx

    add     esp, 304
    pop     ebp
    pop     edi
    pop     esi
    pop     ebx
    ret
    .size apg_sha256_transform_hw, .-apg_sha256_transform_hw

    .section .text.__x86.get_pc_thunk.bx,"axG",@progbits,__x86.get_pc_thunk.bx,comdat
    .globl  __x86.get_pc_thunk.bx
    .hidden __x86.get_pc_thunk.bx
    .type   __x86.get_pc_thunk.bx, @function
__x86.get_pc_thunk.bx:
    mov     ebx, [esp]
    ret

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

    .section .note.GNU-stack,"",@progbits
