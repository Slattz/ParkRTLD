#pragma once
#include "types.hpp"
#include <cstddef> //size_t

extern "C" {

void* memset(void *dest, int val, size_t len);
int strcmp(const char *s1, const char *s2);
size_t strlen(const char *str);

} //extern "C"