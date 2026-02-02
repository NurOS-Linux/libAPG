// NurOS Ruzen42 2025 apg/crc32.h
// Last change: Dec 25

#include "../include/apg/crc32.h"

unsigned int
crc32(const unsigned char *buffer, unsigned int len)
{
    unsigned int crc = 0;
    crc = crc ^ 0xffffffffL;
    while(len >= 8) {
        DO8(buffer);
        len -= 8;
    }
    if(len) do {
        DO1(buffer);
    } while(--len);
    return crc ^ 0xffffffffL;
}

unsigned int
crc32_simple(const unsigned char *message, unsigned int len)
{
    int i, j;
    unsigned int byte, crc, mask;

    i = 0;
    crc = 0xFFFFFFFF;
    while (i < len) {
        byte = message[i];
        crc = crc ^ byte;
        for (j = 7; j >= 0; j--) {
            mask = -(crc & 1);
            crc = (crc >> 1) ^ (0xEDB88320 & mask);
        }
        i = i + 1;
    }
    return ~crc;
}