// SPDX-License-Identifier: GPL-3.0-only
// SHA-256 via Intel SHA Extensions (SHA-NI + SSE4.1).
// Syntax: AT&T, compatible with both GAS and Clang IAS.
//
// int  apg_sha256_hw_supported(void)
// void apg_sha256_transform_hw(uint32_t state[8], const uint8_t data[64])

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
    pushq   %rbx
    movl    $1, %eax
    cpuid
    movl    %ecx, %r8d
    movl    $7, %eax
    xorl    %ecx, %ecx
    cpuid
    shrl    $29, %ebx
    andl    $1, %ebx
    shrl    $19, %r8d
    andl    $1, %r8d
    andl    %r8d, %ebx
    movl    %ebx, %eax
    popq    %rbx
    retq
    .size apg_sha256_hw_supported, .-apg_sha256_hw_supported

    .globl apg_sha256_transform_hw
    .type  apg_sha256_transform_hw, @function
    .align 16
apg_sha256_transform_hw:
    subq    $96, %rsp
    movdqu  %xmm6,  0(%rsp)
    movdqu  %xmm7,  16(%rsp)
    movdqu  %xmm8,  32(%rsp)
    movdqu  %xmm9,  48(%rsp)
    movdqu  %xmm10, 64(%rsp)
    movdqu  %xmm11, 80(%rsp)

    movdqu  .Lbswap(%rip), %xmm11

    // Load state, reorder for SHA-NI: ABEF in xmm0, CDGH in xmm1
    movdqu    (%rdi), %xmm0
    movdqu  16(%rdi), %xmm1
    pshufd  $0xb1, %xmm0, %xmm0
    pshufd  $0x1b, %xmm1, %xmm1
    movdqa  %xmm0, %xmm9
    palignr $8, %xmm1, %xmm9
    pblendw $0xf0, %xmm0, %xmm1
    movdqa  %xmm9, %xmm0
    movdqa  %xmm0, %xmm8
    movdqa  %xmm1, %xmm10

    // Load and byte-swap message
    movdqu    (%rsi), %xmm4
    movdqu  16(%rsi), %xmm5
    movdqu  32(%rsi), %xmm6
    movdqu  48(%rsi), %xmm7
    pshufb  %xmm11, %xmm4
    pshufb  %xmm11, %xmm5
    pshufb  %xmm11, %xmm6
    pshufb  %xmm11, %xmm7

    // Rounds 0-3 (no msg schedule yet)
    movdqa  %xmm4, %xmm9
    paddd   .LK0(%rip), %xmm9
    sha256rnds2 %xmm0, %xmm1, %xmm9
    pshufd  $0x0e, %xmm9, %xmm9
    sha256rnds2 %xmm1, %xmm0, %xmm9

    // Rounds 4-7
    movdqa  %xmm5, %xmm9
    paddd   .LK1(%rip), %xmm9
    sha256msg1 %xmm5, %xmm4
    sha256rnds2 %xmm0, %xmm1, %xmm9
    pshufd  $0x0e, %xmm9, %xmm9
    sha256rnds2 %xmm1, %xmm0, %xmm9

    // Rounds 8-11
    movdqa  %xmm6, %xmm9
    paddd   .LK2(%rip), %xmm9
    sha256msg1 %xmm6, %xmm5
    sha256rnds2 %xmm0, %xmm1, %xmm9
    pshufd  $0x0e, %xmm9, %xmm9
    sha256rnds2 %xmm1, %xmm0, %xmm9

    // Rounds 12-15
    movdqa  %xmm7, %xmm9
    paddd   .LK3(%rip), %xmm9
    sha256msg1 %xmm7, %xmm6
    sha256msg2 %xmm7, %xmm4
    sha256rnds2 %xmm0, %xmm1, %xmm9
    pshufd  $0x0e, %xmm9, %xmm9
    sha256rnds2 %xmm1, %xmm0, %xmm9

    // Rounds 16-19
    movdqa  %xmm4, %xmm9
    paddd   .LK4(%rip), %xmm9
    sha256msg1 %xmm4, %xmm7
    sha256msg2 %xmm4, %xmm5
    sha256rnds2 %xmm0, %xmm1, %xmm9
    pshufd  $0x0e, %xmm9, %xmm9
    sha256rnds2 %xmm1, %xmm0, %xmm9

    // Rounds 20-23
    movdqa  %xmm5, %xmm9
    paddd   .LK5(%rip), %xmm9
    sha256msg1 %xmm5, %xmm4
    sha256msg2 %xmm5, %xmm6
    sha256rnds2 %xmm0, %xmm1, %xmm9
    pshufd  $0x0e, %xmm9, %xmm9
    sha256rnds2 %xmm1, %xmm0, %xmm9

    // Rounds 24-27
    movdqa  %xmm6, %xmm9
    paddd   .LK6(%rip), %xmm9
    sha256msg1 %xmm6, %xmm5
    sha256msg2 %xmm6, %xmm7
    sha256rnds2 %xmm0, %xmm1, %xmm9
    pshufd  $0x0e, %xmm9, %xmm9
    sha256rnds2 %xmm1, %xmm0, %xmm9

    // Rounds 28-31
    movdqa  %xmm7, %xmm9
    paddd   .LK7(%rip), %xmm9
    sha256msg1 %xmm7, %xmm6
    sha256msg2 %xmm7, %xmm4
    sha256rnds2 %xmm0, %xmm1, %xmm9
    pshufd  $0x0e, %xmm9, %xmm9
    sha256rnds2 %xmm1, %xmm0, %xmm9

    // Rounds 32-35
    movdqa  %xmm4, %xmm9
    paddd   .LK8(%rip), %xmm9
    sha256msg1 %xmm4, %xmm7
    sha256msg2 %xmm4, %xmm5
    sha256rnds2 %xmm0, %xmm1, %xmm9
    pshufd  $0x0e, %xmm9, %xmm9
    sha256rnds2 %xmm1, %xmm0, %xmm9

    // Rounds 36-39
    movdqa  %xmm5, %xmm9
    paddd   .LK9(%rip), %xmm9
    sha256msg1 %xmm5, %xmm4
    sha256msg2 %xmm5, %xmm6
    sha256rnds2 %xmm0, %xmm1, %xmm9
    pshufd  $0x0e, %xmm9, %xmm9
    sha256rnds2 %xmm1, %xmm0, %xmm9

    // Rounds 40-43
    movdqa  %xmm6, %xmm9
    paddd   .LK10(%rip), %xmm9
    sha256msg1 %xmm6, %xmm5
    sha256msg2 %xmm6, %xmm7
    sha256rnds2 %xmm0, %xmm1, %xmm9
    pshufd  $0x0e, %xmm9, %xmm9
    sha256rnds2 %xmm1, %xmm0, %xmm9

    // Rounds 44-47
    movdqa  %xmm7, %xmm9
    paddd   .LK11(%rip), %xmm9
    sha256msg1 %xmm7, %xmm6
    sha256msg2 %xmm7, %xmm4
    sha256rnds2 %xmm0, %xmm1, %xmm9
    pshufd  $0x0e, %xmm9, %xmm9
    sha256rnds2 %xmm1, %xmm0, %xmm9

    // Rounds 48-51 (no more msg1, only msg2)
    movdqa  %xmm4, %xmm9
    paddd   .LK12(%rip), %xmm9
    sha256msg2 %xmm4, %xmm5
    sha256rnds2 %xmm0, %xmm1, %xmm9
    pshufd  $0x0e, %xmm9, %xmm9
    sha256rnds2 %xmm1, %xmm0, %xmm9

    // Rounds 52-55
    movdqa  %xmm5, %xmm9
    paddd   .LK13(%rip), %xmm9
    sha256msg2 %xmm5, %xmm6
    sha256rnds2 %xmm0, %xmm1, %xmm9
    pshufd  $0x0e, %xmm9, %xmm9
    sha256rnds2 %xmm1, %xmm0, %xmm9

    // Rounds 56-59
    movdqa  %xmm6, %xmm9
    paddd   .LK14(%rip), %xmm9
    sha256msg2 %xmm6, %xmm7
    sha256rnds2 %xmm0, %xmm1, %xmm9
    pshufd  $0x0e, %xmm9, %xmm9
    sha256rnds2 %xmm1, %xmm0, %xmm9

    // Rounds 60-63
    movdqa  %xmm7, %xmm9
    paddd   .LK15(%rip), %xmm9
    sha256rnds2 %xmm0, %xmm1, %xmm9
    pshufd  $0x0e, %xmm9, %xmm9
    sha256rnds2 %xmm1, %xmm0, %xmm9

    // Add original state
    paddd   %xmm8,  %xmm0
    paddd   %xmm10, %xmm1

    // Reorder back to state[0..7]
    pshufd  $0x1b, %xmm0, %xmm0
    pshufd  $0xb1, %xmm1, %xmm1
    movdqa  %xmm0, %xmm9
    pblendw $0xf0, %xmm1, %xmm0
    palignr $8, %xmm1, %xmm9
    pshufd  $0x1b, %xmm0, %xmm0
    movdqu  %xmm9,   (%rdi)
    movdqu  %xmm0, 16(%rdi)

    movdqu   0(%rsp), %xmm6
    movdqu  16(%rsp), %xmm7
    movdqu  32(%rsp), %xmm8
    movdqu  48(%rsp), %xmm9
    movdqu  64(%rsp), %xmm10
    movdqu  80(%rsp), %xmm11
    addq    $96, %rsp
    retq
    .size apg_sha256_transform_hw, .-apg_sha256_transform_hw

    .section .note.GNU-stack,"",@progbits
