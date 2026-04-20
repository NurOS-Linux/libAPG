// SPDX-License-Identifier: GPL-3.0-only
// SHA-256 via Intel SHA Extensions (SHA-NI + SSE4.1).
// Syntax: Intel (.intel_syntax noprefix) — compatible with GAS and Clang IAS.
//         Assemble with: clang -masm=intel  or  as -masm=intel
//
// int  apg_sha256_hw_supported(void)
// void apg_sha256_transform_hw(uint32_t state[8], const uint8_t data[64])

    .intel_syntax noprefix

    .section .rodata
    .align 16
.Lbswap:
    .byte 3,2,1,0, 7,6,5,4, 11,10,9,8, 15,14,13,12
.LK0:  .long 0x428a2f98,0x71374491,0xb5c0fbcf,0xe9b5dba5
.LK1:  .long 0x3956c25b,0x59f111f1,0x923f82a4,0xab1c5ed5
.LK2:  .long 0xd807aa98,0x12835b01,0x243185be,0x550c7dc3
.LK3:  .long 0x72be5d74,0x80deb1fe,0x9bdc06a7,0xc19bf174
.LK4:  .long 0xe49b69c1,0xefbe4786,0x0fc19dc6,0x240ca1cc
.LK5:  .long 0x2de92c6f,0x4a7484aa,0x5cb0a9dc,0x76f988da
.LK6:  .long 0x983e5152,0xa831c66d,0xb00327c8,0xbf597fc7
.LK7:  .long 0xc6e00bf3,0xd5a79147,0x06ca6351,0x14292967
.LK8:  .long 0x27b70a85,0x2e1b2138,0x4d2c6dfc,0x53380d13
.LK9:  .long 0x650a7354,0x766a0abb,0x81c2c92e,0x92722c85
.LK10: .long 0xa2bfe8a1,0xa81a664b,0xc24b8b70,0xc76c51a3
.LK11: .long 0xd192e819,0xd6990624,0xf40e3585,0x106aa070
.LK12: .long 0x19a4c116,0x1e376c08,0x2748774c,0x34b0bcb5
.LK13: .long 0x391c0cb3,0x4ed8aa4a,0x5b9cca4f,0x682e6ff3
.LK14: .long 0x748f82ee,0x78a5636f,0x84c87814,0x8cc70208
.LK15: .long 0x90befffa,0xa4506ceb,0xbef9a3f7,0xc67178f2

    .text

    .globl apg_sha256_hw_supported
    .type  apg_sha256_hw_supported, @function
apg_sha256_hw_supported:
    push    rbx
    mov     eax, 1
    cpuid
    mov     r8d, ecx
    mov     eax, 7
    xor     ecx, ecx
    cpuid
    shr     ebx, 29
    and     ebx, 1
    shr     r8d, 19
    and     r8d, 1
    and     ebx, r8d
    mov     eax, ebx
    pop     rbx
    ret
    .size apg_sha256_hw_supported, .-apg_sha256_hw_supported

    .globl apg_sha256_transform_hw
    .type  apg_sha256_transform_hw, @function
    .align 16
apg_sha256_transform_hw:
    sub     rsp, 96
    movdqu  [rsp +  0], xmm6
    movdqu  [rsp + 16], xmm7
    movdqu  [rsp + 32], xmm8
    movdqu  [rsp + 48], xmm9
    movdqu  [rsp + 64], xmm10
    movdqu  [rsp + 80], xmm11

    movdqu  xmm11, [rip + .Lbswap]

    // Load state, reorder for SHA-NI: ABEF in xmm0, CDGH in xmm1
    movdqu  xmm0, [rdi]
    movdqu  xmm1, [rdi + 16]
    pshufd  xmm0, xmm0, 0xb1
    pshufd  xmm1, xmm1, 0x1b
    movdqa  xmm9, xmm0
    palignr xmm9, xmm1, 8
    pblendw xmm1, xmm0, 0xf0
    movdqa  xmm0, xmm9
    movdqa  xmm8,  xmm0
    movdqa  xmm10, xmm1

    // Load and byte-swap message
    movdqu  xmm4, [rsi]
    movdqu  xmm5, [rsi + 16]
    movdqu  xmm6, [rsi + 32]
    movdqu  xmm7, [rsi + 48]
    pshufb  xmm4, xmm11
    pshufb  xmm5, xmm11
    pshufb  xmm6, xmm11
    pshufb  xmm7, xmm11

    // Rounds 0-3
    movdqa  xmm9, xmm4
    paddd   xmm9, [rip + .LK0]
    sha256rnds2 xmm1, xmm0, xmm9
    pshufd  xmm9, xmm9, 0x0e
    sha256rnds2 xmm0, xmm1, xmm9

    // Rounds 4-7
    movdqa  xmm9, xmm5
    paddd   xmm9, [rip + .LK1]
    sha256msg1 xmm4, xmm5
    sha256rnds2 xmm1, xmm0, xmm9
    pshufd  xmm9, xmm9, 0x0e
    sha256rnds2 xmm0, xmm1, xmm9

    // Rounds 8-11
    movdqa  xmm9, xmm6
    paddd   xmm9, [rip + .LK2]
    sha256msg1 xmm5, xmm6
    sha256rnds2 xmm1, xmm0, xmm9
    pshufd  xmm9, xmm9, 0x0e
    sha256rnds2 xmm0, xmm1, xmm9

    // Rounds 12-15
    movdqa  xmm9, xmm7
    paddd   xmm9, [rip + .LK3]
    sha256msg1 xmm6, xmm7
    sha256msg2 xmm4, xmm7
    sha256rnds2 xmm1, xmm0, xmm9
    pshufd  xmm9, xmm9, 0x0e
    sha256rnds2 xmm0, xmm1, xmm9

    // Rounds 16-19
    movdqa  xmm9, xmm4
    paddd   xmm9, [rip + .LK4]
    sha256msg1 xmm7, xmm4
    sha256msg2 xmm5, xmm4
    sha256rnds2 xmm1, xmm0, xmm9
    pshufd  xmm9, xmm9, 0x0e
    sha256rnds2 xmm0, xmm1, xmm9

    // Rounds 20-23
    movdqa  xmm9, xmm5
    paddd   xmm9, [rip + .LK5]
    sha256msg1 xmm4, xmm5
    sha256msg2 xmm6, xmm5
    sha256rnds2 xmm1, xmm0, xmm9
    pshufd  xmm9, xmm9, 0x0e
    sha256rnds2 xmm0, xmm1, xmm9

    // Rounds 24-27
    movdqa  xmm9, xmm6
    paddd   xmm9, [rip + .LK6]
    sha256msg1 xmm5, xmm6
    sha256msg2 xmm7, xmm6
    sha256rnds2 xmm1, xmm0, xmm9
    pshufd  xmm9, xmm9, 0x0e
    sha256rnds2 xmm0, xmm1, xmm9

    // Rounds 28-31
    movdqa  xmm9, xmm7
    paddd   xmm9, [rip + .LK7]
    sha256msg1 xmm6, xmm7
    sha256msg2 xmm4, xmm7
    sha256rnds2 xmm1, xmm0, xmm9
    pshufd  xmm9, xmm9, 0x0e
    sha256rnds2 xmm0, xmm1, xmm9

    // Rounds 32-35
    movdqa  xmm9, xmm4
    paddd   xmm9, [rip + .LK8]
    sha256msg1 xmm7, xmm4
    sha256msg2 xmm5, xmm4
    sha256rnds2 xmm1, xmm0, xmm9
    pshufd  xmm9, xmm9, 0x0e
    sha256rnds2 xmm0, xmm1, xmm9

    // Rounds 36-39
    movdqa  xmm9, xmm5
    paddd   xmm9, [rip + .LK9]
    sha256msg1 xmm4, xmm5
    sha256msg2 xmm6, xmm5
    sha256rnds2 xmm1, xmm0, xmm9
    pshufd  xmm9, xmm9, 0x0e
    sha256rnds2 xmm0, xmm1, xmm9

    // Rounds 40-43
    movdqa  xmm9, xmm6
    paddd   xmm9, [rip + .LK10]
    sha256msg1 xmm5, xmm6
    sha256msg2 xmm7, xmm6
    sha256rnds2 xmm1, xmm0, xmm9
    pshufd  xmm9, xmm9, 0x0e
    sha256rnds2 xmm0, xmm1, xmm9

    // Rounds 44-47
    movdqa  xmm9, xmm7
    paddd   xmm9, [rip + .LK11]
    sha256msg1 xmm6, xmm7
    sha256msg2 xmm4, xmm7
    sha256rnds2 xmm1, xmm0, xmm9
    pshufd  xmm9, xmm9, 0x0e
    sha256rnds2 xmm0, xmm1, xmm9

    // Rounds 48-51
    movdqa  xmm9, xmm4
    paddd   xmm9, [rip + .LK12]
    sha256msg2 xmm5, xmm4
    sha256rnds2 xmm1, xmm0, xmm9
    pshufd  xmm9, xmm9, 0x0e
    sha256rnds2 xmm0, xmm1, xmm9

    // Rounds 52-55
    movdqa  xmm9, xmm5
    paddd   xmm9, [rip + .LK13]
    sha256msg2 xmm6, xmm5
    sha256rnds2 xmm1, xmm0, xmm9
    pshufd  xmm9, xmm9, 0x0e
    sha256rnds2 xmm0, xmm1, xmm9

    // Rounds 56-59
    movdqa  xmm9, xmm6
    paddd   xmm9, [rip + .LK14]
    sha256msg2 xmm7, xmm6
    sha256rnds2 xmm1, xmm0, xmm9
    pshufd  xmm9, xmm9, 0x0e
    sha256rnds2 xmm0, xmm1, xmm9

    // Rounds 60-63
    movdqa  xmm9, xmm7
    paddd   xmm9, [rip + .LK15]
    sha256rnds2 xmm1, xmm0, xmm9
    pshufd  xmm9, xmm9, 0x0e
    sha256rnds2 xmm0, xmm1, xmm9

    // Add original state
    paddd   xmm0, xmm8
    paddd   xmm1, xmm10

    // Reorder back to state[0..7]
    pshufd  xmm0, xmm0, 0x1b
    pshufd  xmm1, xmm1, 0xb1
    movdqa  xmm9, xmm0
    pblendw xmm0, xmm1, 0xf0
    palignr xmm9, xmm1, 8
    pshufd  xmm0, xmm0, 0x1b
    movdqu  [rdi],      xmm9
    movdqu  [rdi + 16], xmm0

    movdqu  xmm6,  [rsp +  0]
    movdqu  xmm7,  [rsp + 16]
    movdqu  xmm8,  [rsp + 32]
    movdqu  xmm9,  [rsp + 48]
    movdqu  xmm10, [rsp + 64]
    movdqu  xmm11, [rsp + 80]
    add     rsp, 96
    ret
    .size apg_sha256_transform_hw, .-apg_sha256_transform_hw

    .section .note.GNU-stack,"",@progbits
