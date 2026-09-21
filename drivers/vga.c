/*
** vga.c — VGA text mode driver.
**
** It does two things:
**  1) write cells into the framebuffer at 0xB8000,
**  2) drive the hardware cursor through the CRT controller registers (bonus).
*/

#include "vga.h"
#include "io.h"

/*
** CRT controller: first select the register to write through 0x3D4 (index
** port), then write the value through 0x3D5 (data port).
*/
#define CRTC_INDEX_PORT 0x3D4
#define CRTC_DATA_PORT  0x3D5

#define CRTC_CURSOR_START 0x0A  /* top scanline of the cursor + hide bit */
#define CRTC_CURSOR_END   0x0B  /* bottom scanline of the cursor */
#define CRTC_CURSOR_HIGH  0x0E  /* high byte of the cursor position */
#define CRTC_CURSOR_LOW   0x0F  /* low byte of the cursor position */

/* Updates a single cell of the framebuffer. */
void vga_write_cell(size_t index, uint16_t cell)
{
	if (index < VGA_WIDTH * VGA_HEIGHT)
		VGA_MEMORY[index] = cell;
}

/*
** Copies a whole 80x25 buffer onto the screen.
** Used when switching between virtual screens (bonus).
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
** Makes the hardware cursor visible.
** Bit 5 of the cursor start register hides the cursor; we clear it and set
** the scanlines (14-15) the cursor occupies inside a cell.
*/
void vga_enable_cursor(void)
{
	outb(CRTC_INDEX_PORT, CRTC_CURSOR_START);
	outb(CRTC_DATA_PORT, 14);
	outb(CRTC_INDEX_PORT, CRTC_CURSOR_END);
	outb(CRTC_DATA_PORT, 15);
}

/*
** Moves the cursor to (row, col).
** The hardware expects a linear offset, so we compute row * 80 + col and split
** the 16-bit value across two registers (high/low).
*/
void vga_move_cursor(size_t row, size_t col)
{
	uint16_t offset = (uint16_t)(row * VGA_WIDTH + col);

	outb(CRTC_INDEX_PORT, CRTC_CURSOR_HIGH);
	outb(CRTC_DATA_PORT, (uint8_t)((offset >> 8) & 0xFF));
	outb(CRTC_INDEX_PORT, CRTC_CURSOR_LOW);
	outb(CRTC_DATA_PORT, (uint8_t)(offset & 0xFF));
}
