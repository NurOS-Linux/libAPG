// SPDX-License-Identifier: GPL-3.0-only
// SHA-256 via ARMv8 Crypto Extensions (sha2).
// Syntax: AArch64 unified assembly — compatible with GAS, Clang IAS, and LLVM.
//
// int  apg_sha256_hw_supported(void)
// void apg_sha256_transform_hw(uint32_t state[8], const uint8_t data[64])

    .arch armv8-a+crypto
    .text

    .globl apg_sha256_hw_supported
    .type  apg_sha256_hw_supported, @function
apg_sha256_hw_supported:
    mov     w0, #1
    ret
    .size apg_sha256_hw_supported, .-apg_sha256_hw_supported

// void apg_sha256_transform_hw(uint32_t state[8], const uint8_t data[64])
// x0 = state, x1 = data
    .globl apg_sha256_transform_hw
    .type  apg_sha256_transform_hw, @function
    .align 4
apg_sha256_transform_hw:
    ld1     {v0.4s, v1.4s}, [x0]
    mov     v16.16b, v0.16b
    mov     v17.16b, v1.16b

    ld1     {v4.16b, v5.16b, v6.16b, v7.16b}, [x1]
    rev32   v4.16b, v4.16b
    rev32   v5.16b, v5.16b
    rev32   v6.16b, v6.16b
    rev32   v7.16b, v7.16b

    adrp    x2, .LK
    add     x2, x2, :lo12:.LK

    // Rounds 0-3
    ld1     {v18.4s}, [x2], #16
    add     v19.4s, v4.4s, v18.4s
    sha256h   q0, q1, v19.4s
    sha256h2  q1, q0, v19.4s

    // Rounds 4-7
    ld1     {v18.4s}, [x2], #16
    add     v19.4s, v5.4s, v18.4s
    sha256su0 v4.4s, v5.4s
    sha256h   q0, q1, v19.4s
    sha256h2  q1, q0, v19.4s

    // Rounds 8-11
    ld1     {v18.4s}, [x2], #16
    add     v19.4s, v6.4s, v18.4s
    sha256su0 v5.4s, v6.4s
    sha256su1 v4.4s, v6.4s, v7.4s
    sha256h   q0, q1, v19.4s
    sha256h2  q1, q0, v19.4s

    // Rounds 12-15
    ld1     {v18.4s}, [x2], #16
    add     v19.4s, v7.4s, v18.4s
    sha256su0 v6.4s, v7.4s
    sha256su1 v5.4s, v7.4s, v4.4s
    sha256h   q0, q1, v19.4s
    sha256h2  q1, q0, v19.4s

    // Rounds 16-19
    ld1     {v18.4s}, [x2], #16
    add     v19.4s, v4.4s, v18.4s
    sha256su0 v7.4s, v4.4s
    sha256su1 v6.4s, v4.4s, v5.4s
    sha256h   q0, q1, v19.4s
    sha256h2  q1, q0, v19.4s

    // Rounds 20-23
    ld1     {v18.4s}, [x2], #16
    add     v19.4s, v5.4s, v18.4s
    sha256su0 v4.4s, v5.4s
    sha256su1 v7.4s, v5.4s, v6.4s
    sha256h   q0, q1, v19.4s
    sha256h2  q1, q0, v19.4s

    // Rounds 24-27
    ld1     {v18.4s}, [x2], #16
    add     v19.4s, v6.4s, v18.4s
    sha256su0 v5.4s, v6.4s
    sha256su1 v4.4s, v6.4s, v7.4s
    sha256h   q0, q1, v19.4s
    sha256h2  q1, q0, v19.4s

    // Rounds 28-31
    ld1     {v18.4s}, [x2], #16
    add     v19.4s, v7.4s, v18.4s
    sha256su0 v6.4s, v7.4s
    sha256su1 v5.4s, v7.4s, v4.4s
    sha256h   q0, q1, v19.4s
    sha256h2  q1, q0, v19.4s

    // Rounds 32-35
    ld1     {v18.4s}, [x2], #16
    add     v19.4s, v4.4s, v18.4s
    sha256su0 v7.4s, v4.4s
    sha256su1 v6.4s, v4.4s, v5.4s
    sha256h   q0, q1, v19.4s
    sha256h2  q1, q0, v19.4s

    // Rounds 36-39
    ld1     {v18.4s}, [x2], #16
    add     v19.4s, v5.4s, v18.4s
    sha256su0 v4.4s, v5.4s
    sha256su1 v7.4s, v5.4s, v6.4s
    sha256h   q0, q1, v19.4s
    sha256h2  q1, q0, v19.4s

    // Rounds 40-43
    ld1     {v18.4s}, [x2], #16
    add     v19.4s, v6.4s, v18.4s
    sha256su0 v5.4s, v6.4s
    sha256su1 v4.4s, v6.4s, v7.4s
    sha256h   q0, q1, v19.4s
    sha256h2  q1, q0, v19.4s

    // Rounds 44-47
    ld1     {v18.4s}, [x2], #16
    add     v19.4s, v7.4s, v18.4s
    sha256su0 v6.4s, v7.4s
    sha256su1 v5.4s, v7.4s, v4.4s
    sha256h   q0, q1, v19.4s
    sha256h2  q1, q0, v19.4s

    // Rounds 48-51
    ld1     {v18.4s}, [x2], #16
    add     v19.4s, v4.4s, v18.4s
    sha256su1 v6.4s, v4.4s, v5.4s
    sha256h   q0, q1, v19.4s
    sha256h2  q1, q0, v19.4s

    // Rounds 52-55
    ld1     {v18.4s}, [x2], #16
    add     v19.4s, v5.4s, v18.4s
    sha256su1 v7.4s, v5.4s, v6.4s
    sha256h   q0, q1, v19.4s
    sha256h2  q1, q0, v19.4s

    // Rounds 56-59
    ld1     {v18.4s}, [x2], #16
    add     v19.4s, v6.4s, v18.4s
    sha256h   q0, q1, v19.4s
    sha256h2  q1, q0, v19.4s

    // Rounds 60-63
    ld1     {v18.4s}, [x2], #16
    add     v19.4s, v7.4s, v18.4s
    sha256h   q0, q1, v19.4s
    sha256h2  q1, q0, v19.4s

    add     v0.4s, v0.4s, v16.4s
    add     v1.4s, v1.4s, v17.4s
    st1     {v0.4s, v1.4s}, [x0]
    ret
    .size apg_sha256_transform_hw, .-apg_sha256_transform_hw

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
