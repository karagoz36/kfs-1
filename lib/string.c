/*
** string.c — Kernel library fonksiyonlarinin implementasyonu.
** Hepsi sade ve bagimsiz: hicbir dis fonksiyon cagrilmaz.
*/

#include "string.h"

/* String'in sonundaki '\0' haric karakter sayisini dondurur. */
size_t k_strlen(const char *s)
{
	size_t len = 0;

	while (s[len] != '\0')
		len++;
	return (len);
}

/* Iki string'i karsilastirir: esitse 0, degilse ilk farkli byte'in farki. */
int k_strcmp(const char *a, const char *b)
{
	while (*a != '\0' && *a == *b)
	{
		a++;
		b++;
	}
	return ((int)(uint8_t)*a - (int)(uint8_t)*b);
}

/* strcmp ile ayni, ama en fazla n byte karsilastirir. */
int k_strncmp(const char *a, const char *b, size_t n)
{
	size_t i = 0;

	while (i < n && a[i] != '\0' && a[i] == b[i])
		i++;
	if (i == n)
		return (0);
	return ((int)(uint8_t)a[i] - (int)(uint8_t)b[i]);
}

/* Bellek blogunu tek bir byte degeriyle doldurur (buffer temizlemede kullanilir). */
void *k_memset(void *dst, uint8_t value, size_t n)
{
	uint8_t *p = (uint8_t *)dst;
	size_t   i = 0;

	while (i < n)
		p[i++] = value;
	return (dst);
}

/* Kaynaktan hedefe n byte kopyalar (bloklarin cakismadigi varsayilir). */
void *k_memcpy(void *dst, const void *src, size_t n)
{
	uint8_t       *d = (uint8_t *)dst;
	const uint8_t *s = (const uint8_t *)src;
	size_t         i = 0;

	while (i < n)
	{
		d[i] = s[i];
		i++;
	}
	return (dst);
}
