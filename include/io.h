/*
** io.h — x86 port I/O (giris/cikis) yardimcilari.
**
** x86'da bazi donanimlar (VGA cursor register'lari, PS/2 klavye controller'i)
** bellek uzerinden degil, ayri bir "I/O address space" uzerinden konusulur.
** Bu adres alanina sadece 'in' ve 'out' instruction'lari ile erisilir, bu yuzden
** inline assembly kullanmak zorundayiz.
*/

#ifndef IO_H
#define IO_H

#include "types.h"

/* Verilen port'a 1 byte yazar. "a" = al register'i, "Nd" = dx ya da sabit port */
static inline void outb(uint16_t port, uint8_t value)
{
	__asm__ volatile("outb %0, %1" : : "a"(value), "Nd"(port));
}

/* Verilen port'tan 1 byte okur ve geri dondurur. */
static inline uint8_t inb(uint16_t port)
{
	uint8_t value;

	__asm__ volatile("inb %1, %0" : "=a"(value) : "Nd"(port));
	return (value);
}

/*
** CPU'yu bir sonraki interrupt'a kadar uyutur (halt).
** Bos dongude islemciyi %100 mesgul etmemek icin kullanilir.
*/
static inline void cpu_halt(void)
{
	__asm__ volatile("hlt");
}

#endif
