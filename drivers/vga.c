/*
** vga.c — VGA text mode surucusu.
**
** Iki is yapar:
**  1) 0xB8000'daki framebuffer'a hucre yazmak,
**  2) hardware cursor'u CRT controller register'lari uzerinden yonetmek (bonus).
*/

#include "vga.h"
#include "io.h"

/*
** CRT controller: once 0x3D4 (index port) ile hangi register'i yazacagimizi
** secilir, sonra 0x3D5 (data port) ile deger yazilir.
*/
#define CRTC_INDEX_PORT 0x3D4
#define CRTC_DATA_PORT  0x3D5

#define CRTC_CURSOR_START 0x0A  /* cursor'un ust tarama satiri + gizleme biti */
#define CRTC_CURSOR_END   0x0B  /* cursor'un alt tarama satiri */
#define CRTC_CURSOR_HIGH  0x0E  /* cursor pozisyonunun yuksek byte'i */
#define CRTC_CURSOR_LOW   0x0F  /* cursor pozisyonunun dusuk byte'i */

/* Framebuffer'daki tek bir hucreyi gunceller. */
void vga_write_cell(size_t index, uint16_t cell)
{
	if (index < VGA_WIDTH * VGA_HEIGHT)
		VGA_MEMORY[index] = cell;
}

/*
** Verilen 80x25'lik buffer'in tamamini ekrana kopyalar.
** Virtual screen'ler arasi gecis yaparken (bonus) kullanilir.
*/
void vga_blit(const uint16_t *buffer)
{
	size_t i = 0;

	while (i < VGA_WIDTH * VGA_HEIGHT)
	{
		VGA_MEMORY[i] = buffer[i];
		i++;
	}
}

/*
** Hardware cursor'u gorunur yapar.
** Cursor start register'inin 5. biti 1 ise cursor gizlidir; onu temizleyip
** cursor'un hucre icinde hangi tarama satirlarini kaplayacagini (14-15) veriyoruz.
*/
void vga_enable_cursor(void)
{
	outb(CRTC_INDEX_PORT, CRTC_CURSOR_START);
	outb(CRTC_DATA_PORT, 14);
	outb(CRTC_INDEX_PORT, CRTC_CURSOR_END);
	outb(CRTC_DATA_PORT, 15);
}

/*
** Cursor'u (row, col) konumuna tasir.
** Donanim tek boyutlu bir offset bekledigi icin row * 80 + col hesaplanir ve
** 16-bit deger iki ayri register'a (high/low) bolunerek yazilir.
*/
void vga_move_cursor(size_t row, size_t col)
{
	uint16_t offset = (uint16_t)(row * VGA_WIDTH + col);

	outb(CRTC_INDEX_PORT, CRTC_CURSOR_HIGH);
	outb(CRTC_DATA_PORT, (uint8_t)((offset >> 8) & 0xFF));
	outb(CRTC_INDEX_PORT, CRTC_CURSOR_LOW);
	outb(CRTC_DATA_PORT, (uint8_t)(offset & 0xFF));
}
