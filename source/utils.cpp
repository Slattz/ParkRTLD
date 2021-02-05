#include "utils.hpp"

extern "C" {

void* __attribute__((optimize("Os"))) memset(void *dest, int val, size_t len) {
    unsigned char *ptr = (unsigned char *)dest;
    while (len-- > 0)
        *ptr++ = val;
    return dest;
}

int strcmp(const char *s1, const char *s2) {
    while (*s1 && (*s1 == *s2)) {
        s1++;
        s2++;
    }
    return *(const unsigned char *)s1 - *(const unsigned char *)s2;
}

size_t strlen(const char *str) {
    const char *s;

    for (s = str; *s; ++s);
    return (s - str);
}

} //extern "C"