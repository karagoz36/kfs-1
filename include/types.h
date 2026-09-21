/*
** types.h — Kernel'in kendi temel tipleri.
**
** Kernel freestanding (-nostdlib) derlendigi icin <stdint.h> gibi standart
** header'lari kullanmiyoruz. i386 (32-bit protected mode) icin boyutlar sabit:
** char 1, short 2, int 4 byte.
*/

#ifndef TYPES_H
#define TYPES_H

typedef unsigned char      uint8_t;
typedef unsigned short     uint16_t;
typedef unsigned int       uint32_t;
typedef signed int         int32_t;

/* i386'da pointer 4 byte oldugu icin size_t = unsigned int */
typedef unsigned int       size_t;

/* Basit boolean; C99 <stdbool.h> yok */
typedef uint8_t            bool_t;
#define TRUE  1
#define FALSE 0

#define NULL ((void *)0)

#endif
