// NurOS Ruzen42 2025 apg/util.c
// Last change: Dec 21

#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <time.h>

char *
concat(const char *s1, const char *s2)
{
    int len1 = 0;
    int len2 = 0;
    int i, j;

    while (s1[len1] != '\0') {
        len1++;
    }

    while (s2[len2] != '\0') {
        len2++;
    }

    char *result = malloc(len1 + len2 + 1);
    if (result == NULL) {
        return NULL;
    }

    for (i = 0; i < len1; i++) {
        result[i] = s1[i];
    }

    for (j = 0; j < len2; j++, i++) {
        result[i] = s2[j];
    }

    result[i] = '\0';

    return result;
    return result;
}

