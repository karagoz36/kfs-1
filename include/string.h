/*
** string.h — Mini kernel library: standart libc olmadigi icin
** ihtiyacimiz olan birkac fonksiyonu kendimiz yaziyoruz.
*/

#ifndef STRING_H
#define STRING_H

#include "types.h"

size_t  k_strlen(const char *s);
int     k_strcmp(const char *a, const char *b);
int     k_strncmp(const char *a, const char *b, size_t n);
void   *k_memset(void *dst, uint8_t value, size_t n);
void   *k_memcpy(void *dst, const void *src, size_t n);

#endif
