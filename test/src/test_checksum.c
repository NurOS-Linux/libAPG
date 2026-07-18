// SPDX-License-Identifier: GPL-3.0-only
// SPDX-FileCopyrightText: 2026 AnmiTaliDev <anmitalidev@nuros.org>

#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <apg/crc32.h>
#include <apg/md5.h>
#include <apg/sha256.h>

static bool
digest_matches_hex(const uint8_t *digest, size_t len, const char *hex)
{
    if (strlen(hex) != len * 2)
        return false;
    for (size_t i = 0; i < len; i++)
    {
        unsigned int byte;
        if (sscanf(hex + i * 2, "%2x", &byte) != 1)
            return false;
        if (digest[i] != (uint8_t)byte)
            return false;
    }
    return true;
}

void
test_crc32_known_vectors(void)
{
    assert(crc32((const unsigned char *)"", 0) == 0x00000000u);
    assert(crc32((const unsigned char *)"a", 1) == 0xE8B7BE43u);
    assert(crc32((const unsigned char *)"123456789", 9) == 0xCBF43926u);
    printf("test_crc32_known_vectors: PASS\n");
}

void
test_crc32_matches_simple_reference(void)
{
    unsigned int seed = 0xC0FFEEu;
    unsigned char buf[4096];

    for (int iter = 0; iter < 2000; iter++)
    {
        unsigned int len = (unsigned int)(rand_r(&seed) % sizeof(buf));
        for (unsigned int i = 0; i < len; i++)
            buf[i] = (unsigned char)(rand_r(&seed) & 0xff);

        unsigned int a = crc32(buf, len);
        unsigned int b = crc32_simple(buf, len);
        assert(a == b);
    }
    printf("test_crc32_matches_simple_reference: PASS\n");
}

void
test_md5_known_vectors(void)
{
    md5_ctx ctx;
    uint8_t digest[16];

    md5_init(&ctx);
    md5_final(digest, &ctx);
    assert(digest_matches_hex(digest, 16, "d41d8cd98f00b204e9800998ecf8427e"));

    md5_init(&ctx);
    md5_update(&ctx, (const uint8_t *)"abc", 3);
    md5_final(digest, &ctx);
    assert(digest_matches_hex(digest, 16, "900150983cd24fb0d6963f7d28e17f72"));

    printf("test_md5_known_vectors: PASS\n");
}

void
test_md5_chunking_invariant(void)
{
    unsigned int seed = 0x5EED5u;
    uint8_t data[8192];
    for (size_t i = 0; i < sizeof(data); i++)
        data[i] = (uint8_t)(rand_r(&seed) & 0xff);

    for (int iter = 0; iter < 200; iter++)
    {
        size_t len = (size_t)(rand_r(&seed) % sizeof(data));

        md5_ctx whole;
        uint8_t whole_digest[16];
        md5_init(&whole);
        md5_update(&whole, data, len);
        md5_final(whole_digest, &whole);

        md5_ctx chunked;
        uint8_t chunked_digest[16];
        md5_init(&chunked);
        size_t off = 0;
        while (off < len)
        {
            size_t remaining = len - off;
            size_t chunk = 1 + (size_t)(rand_r(&seed) % remaining);
            md5_update(&chunked, data + off, chunk);
            off += chunk;
        }
        md5_final(chunked_digest, &chunked);

        assert(memcmp(whole_digest, chunked_digest, 16) == 0);
    }
    printf("test_md5_chunking_invariant: PASS\n");
}

void
test_sha256_known_vectors(void)
{
    sha256_ctx ctx;
    uint8_t digest[32];

    sha256_init(&ctx);
    sha256_final(&ctx, digest);
    assert(digest_matches_hex(
        digest, 32,
        "e3b0c44298fc1c149afbf4c8996fb92427ae41e4649b934ca495991b7852b855"));

    sha256_init(&ctx);
    sha256_update(&ctx, (const uint8_t *)"abc", 3);
    sha256_final(&ctx, digest);
    assert(digest_matches_hex(
        digest, 32,
        "ba7816bf8f01cfea414140de5dae2223b00361a396177a9cb410ff61f20015ad"));

    printf("test_sha256_known_vectors: PASS\n");
}

void
test_sha256_chunking_invariant(void)
{
    unsigned int seed = 0xFEED5u;
    uint8_t data[8192];
    for (size_t i = 0; i < sizeof(data); i++)
        data[i] = (uint8_t)(rand_r(&seed) & 0xff);

    for (int iter = 0; iter < 200; iter++)
    {
        size_t len = (size_t)(rand_r(&seed) % sizeof(data));

        sha256_ctx whole;
        uint8_t whole_digest[32];
        sha256_init(&whole);
        sha256_update(&whole, data, len);
        sha256_final(&whole, whole_digest);

        sha256_ctx chunked;
        uint8_t chunked_digest[32];
        sha256_init(&chunked);
        size_t off = 0;
        while (off < len)
        {
            size_t remaining = len - off;
            size_t chunk = 1 + (size_t)(rand_r(&seed) % remaining);
            sha256_update(&chunked, data + off, chunk);
            off += chunk;
        }
        sha256_final(&chunked, chunked_digest);

        assert(memcmp(whole_digest, chunked_digest, 32) == 0);
    }
    printf("test_sha256_chunking_invariant: PASS\n");
}
