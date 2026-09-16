/*
** vga.h — VGA text mode (80x25) donaniminin dusuk seviyeli arayuzu.
**
** BIOS/GRUB bizi ekran "text mode"dayken birakir. Bu modda ekran,
** 0xB8000 adresinden baslayan bir framebuffer'dir: her hucre 2 byte,
** dusuk byte = ASCII karakter, yuksek byte = renk (attribute).
*/

#ifndef VGA_H
#define VGA_H

#include "types.h"

#define VGA_WIDTH   80
#define VGA_HEIGHT  25
#define VGA_MEMORY  ((volatile uint16_t *)0xB8000)

/* Standart VGA 16 renk paleti (bonus: renk destegi) */
enum e_vga_color
{
	VGA_BLACK = 0,
	VGA_BLUE,
	VGA_GREEN,
	VGA_CYAN,
	VGA_RED,
	VGA_MAGENTA,
	VGA_BROWN,
	VGA_LIGHT_GREY,
	VGA_DARK_GREY,
	VGA_LIGHT_BLUE,
	VGA_LIGHT_GREEN,
	VGA_LIGHT_CYAN,
	VGA_LIGHT_RED,
	VGA_LIGHT_MAGENTA,
	VGA_YELLOW,
	VGA_WHITE
};

/* Arka plan (yuksek 4 bit) + on plan (dusuk 4 bit) -> attribute byte */
static inline uint8_t vga_color(uint8_t fg, uint8_t bg)
{
	return ((uint8_t)(fg | (bg << 4)));
}

/* Karakter + renk -> framebuffer'a yazilacak 16-bit hucre */
static inline uint16_t vga_entry(char c, uint8_t color)
{
	return ((uint16_t)(uint8_t)c | ((uint16_t)color << 8));
}

void vga_write_cell(size_t index, uint16_t cell);
void vga_blit(const uint16_t *buffer);
void vga_enable_cursor(void);
void vga_move_cursor(size_t row, size_t col);

#endif
