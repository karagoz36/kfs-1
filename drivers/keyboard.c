/*
** keyboard.c — PS/2 klavye surucusu (bonus).
**
** KFS_1'de henuz IDT/interrupt altyapisi yok, bu yuzden klavyeyi "polling"
** ile okuyoruz: PS/2 controller'in status port'una (0x64) bakip output
** buffer dolu ise data port'undan (0x60) scancode aliyoruz.
**
** Klavye "scancode set 1" gonderir: tusa basildiginda make code, birakildiginda
** ayni kodun 0x80 eklenmis hali (break code) gelir.
*/

#include "keyboard.h"
#include "console.h"
#include "printk.h"
#include "io.h"

#define PS2_DATA_PORT   0x60
#define PS2_STATUS_PORT 0x64
#define PS2_OUTPUT_FULL 0x01  /* status register bit 0: okunacak veri var mi */

#define SC_RELEASE_FLAG 0x80  /* break code'lari ayirt eden bit */
#define SC_EXTENDED     0xE0  /* bazi tuslar iki byte gonderir */

#define SC_LSHIFT       0x2A
#define SC_RSHIFT       0x36
#define SC_CTRL         0x1D
#define SC_ALT          0x38
#define SC_F1           0x3B  /* F1..F4 -> 0x3B, 0x3C, 0x3D, 0x3E */

/* Modifier tuslarin anlik durumu */
static bool_t g_shift = FALSE;
static bool_t g_ctrl  = FALSE;
static bool_t g_alt   = FALSE;

/* Bir sonraki scancode'un extended (0xE0 sonrasi) olup olmadigi */
static bool_t g_extended = FALSE;

/*
** Scancode -> ASCII tablosu (US QWERTY, set 1).
** 0 = bu tusun yazdirilacak bir karsiligi yok (F tuslari, modifier'lar vs.)
*/
static const char g_keymap[SC_RELEASE_FLAG] = {
	0,    27,  '1', '2', '3', '4', '5', '6', '7', '8', '9', '0', '-', '=', '\b',
	'\t', 'q', 'w', 'e', 'r', 't', 'y', 'u', 'i', 'o', 'p', '[', ']', '\n',
	0,    'a', 's', 'd', 'f', 'g', 'h', 'j', 'k', 'l', ';', '\'', '`',
	0,    '\\', 'z', 'x', 'c', 'v', 'b', 'n', 'm', ',', '.', '/',
	0,    '*', 0,   ' '
};

/* Shift basiliyken ayni scancode'larin verdigi karakterler */
static const char g_keymap_shift[SC_RELEASE_FLAG] = {
	0,    27,  '!', '@', '#', '$', '%', '^', '&', '*', '(', ')', '_', '+', '\b',
	'\t', 'Q', 'W', 'E', 'R', 'T', 'Y', 'U', 'I', 'O', 'P', '{', '}', '\n',
	0,    'A', 'S', 'D', 'F', 'G', 'H', 'J', 'K', 'L', ':', '"', '~',
	0,    '|', 'Z', 'X', 'C', 'V', 'B', 'N', 'M', '<', '>', '?',
	0,    '*', 0,   ' '
};

/* Modifier tuslarini isler; isledi ise TRUE doner (karakter basilmaz). */
static bool_t handle_modifier(uint8_t code, bool_t pressed)
{
	if (code == SC_LSHIFT || code == SC_RSHIFT)
		g_shift = pressed;
	else if (code == SC_CTRL)
		g_ctrl = pressed;
	else if (code == SC_ALT)
		g_alt = pressed;
	else
		return (FALSE);
	return (TRUE);
}

/*
** Ekran degistirme kisayolu (bonus): Alt + F1..F4 ya da Ctrl + F1..F4.
** Islenirse TRUE doner.
*/
static bool_t handle_shortcut(uint8_t code)
{
	size_t index;

	if (code < SC_F1 || code > SC_F1 + CONSOLE_COUNT - 1)
		return (FALSE);
	if (!g_alt && !g_ctrl)
		return (FALSE);
	index = (size_t)(code - SC_F1);
	console_switch(index);
	return (TRUE);
}

/*
** Bekleyen scancode varsa okur ve isler.
** kernel_main icindeki ana dongude surekli cagrilir.
*/
void keyboard_poll(void)
{
	uint8_t status;
	uint8_t scancode;
	uint8_t code;
	bool_t  pressed;
	char    c;

	status = inb(PS2_STATUS_PORT);
	if (!(status & PS2_OUTPUT_FULL))
		return ;
	scancode = inb(PS2_DATA_PORT);

	/* 0xE0 bir prefix'tir: asil kod bir sonraki okumada gelir */
	if (scancode == SC_EXTENDED)
	{
		g_extended = TRUE;
		return ;
	}

	pressed = (scancode & SC_RELEASE_FLAG) ? FALSE : TRUE;
	code = scancode & (uint8_t)(SC_RELEASE_FLAG - 1);

	if (handle_modifier(code, pressed))
	{
		g_extended = FALSE;
		return ;
	}

	/* Sadece tusa basma anini isliyoruz, birakmayi yok sayiyoruz */
	if (!pressed || g_extended)
	{
		g_extended = FALSE;
		return ;
	}

	if (handle_shortcut(code))
		return ;

	c = g_shift ? g_keymap_shift[code] : g_keymap[code];
	if (c != 0)
		console_putchar(c);
}
