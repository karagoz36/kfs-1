/*
** console.h — Kernel ile ekran arasindaki arayuz.
**
** Birden fazla "virtual screen" (bonus) tutariz: her screen'in kendi
** 80x25 buffer'i, kendi cursor pozisyonu ve kendi rengi vardir.
** Sadece aktif olan screen VGA framebuffer'ina yansitilir (blit edilir).
*/

#ifndef CONSOLE_H
#define CONSOLE_H

#include "types.h"
#include "vga.h"

#define CONSOLE_COUNT 4

void   console_init(void);
void   console_switch(size_t index);
size_t console_current(void);
void   console_set_color(uint8_t fg, uint8_t bg);
void   console_clear(void);
void   console_putchar(char c);
void   console_write(const char *s);

#endif
