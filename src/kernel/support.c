/**
 * support.c - Freestanding C library functions
 *
 * When building a freestanding kernel, we can't link against the standard
 * C library (libc). This file provides minimal implementations of functions
 * that GCC/Clang might emit calls to (memcpy, memset, etc.).
 *
 * These are intentionally simple and unoptimized. Production kernels would
 * use optimized assembly versions.
 */

#include <stddef.h>
#include <stdint.h>

/**
 * memcpy - Copy memory area (non-overlapping)
 * @dest: Destination buffer
 * @src: Source buffer
 * @n: Number of bytes to copy
 *
 * Returns: dest
 *
 * Note: Assumes non-overlapping regions. For overlapping, use memmove.
 */
void *memcpy(void *restrict dest, const void *restrict src, size_t n) {
    uint8_t *d = dest;
    const uint8_t *s = src;

    for (size_t i = 0; i < n; i++) {
        d[i] = s[i];
    }

    return dest;
}

/**
 * memset - Fill memory with a constant byte
 * @dest: Destination buffer
 * @c: Byte value to fill with
 * @n: Number of bytes to fill
 *
 * Returns: dest
 */
void *memset(void *dest, int c, size_t n) {
    uint8_t *d = dest;
    uint8_t val = (uint8_t)c;

    for (size_t i = 0; i < n; i++) {
        d[i] = val;
    }

    return dest;
}

/**
 * memmove - Copy memory area (handles overlapping)
 * @dest: Destination buffer
 * @src: Source buffer
 * @n: Number of bytes to copy
 *
 * Returns: dest
 *
 * Unlike memcpy, this handles overlapping memory regions correctly.
 */
void *memmove(void *dest, const void *src, size_t n) {
    uint8_t *d = dest;
    const uint8_t *s = src;

    if (d == s || n == 0) {
        return dest;
    }

    // If source is before dest, copy backwards to avoid overwriting
    if (s < d && d < s + n) {
        for (size_t i = n; i > 0; i--) {
            d[i - 1] = s[i - 1];
        }
    } else {
        // Copy forwards
        for (size_t i = 0; i < n; i++) {
            d[i] = s[i];
        }
    }

    return dest;
}

/**
 * memcmp - Compare memory areas
 * @s1: First buffer
 * @s2: Second buffer
 * @n: Number of bytes to compare
 *
 * Returns: 0 if equal, <0 if s1 < s2, >0 if s1 > s2
 */
int memcmp(const void *s1, const void *s2, size_t n) {
    const uint8_t *a = s1;
    const uint8_t *b = s2;

    for (size_t i = 0; i < n; i++) {
        if (a[i] != b[i]) {
            return (a[i] < b[i]) ? -1 : 1;
        }
    }

    return 0;
}

/**
 * strlen - Calculate length of string
 * @s: String to measure
 *
 * Returns: Length of string (excluding null terminator)
 */
size_t strlen(const char *s) {
    size_t len = 0;
    while (s[len] != '\0') {
        len++;
    }
    return len;
}

/**
 * strcpy - Copy string
 * @dest: Destination buffer (must be large enough)
 * @src: Source string
 *
 * Returns: dest
 */
char *strcpy(char *restrict dest, const char *restrict src) {
    size_t i = 0;
    while ((dest[i] = src[i]) != '\0') {
        i++;
    }
    return dest;
}

/**
 * strcmp - Compare strings
 * @s1: First string
 * @s2: Second string
 *
 * Returns: 0 if equal, <0 if s1 < s2, >0 if s1 > s2
 */
int strcmp(const char *s1, const char *s2) {
    while (*s1 && (*s1 == *s2)) {
        s1++;
        s2++;
    }
    return *(const unsigned char *)s1 - *(const unsigned char *)s2;
}

/**
 * strncmp - Compare strings up to n characters
 * @s1: First string
 * @s2: Second string
 * @n: Maximum number of characters to compare
 *
 * Returns: 0 if equal, <0 if s1 < s2, >0 if s1 > s2
 */
int strncmp(const char *s1, const char *s2, size_t n) {
    for (size_t i = 0; i < n; i++) {
        if (s1[i] != s2[i] || s1[i] == '\0') {
            return (unsigned char)s1[i] - (unsigned char)s2[i];
        }
    }
    return 0;
}
