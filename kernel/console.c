/*
** console.c — Virtual screen yonetimi, yazma, scroll ve cursor.
**
** Tasarim: her virtual screen bir struct. Yazma islemleri her zaman ilgili
** screen'in kendi buffer'ina yapilir; ekranda gorunen screen ise ayrica
** VGA framebuffer'ina da yazilir. Boylece arka plandaki screen'ler icerigini
** kaybetmez ve gecis aninda tek bir blit yeterlidir.
*/

#include "console.h"
#include "string.h"

typedef struct s_console
{
	uint16_t buffer[VGA_WIDTH * VGA_HEIGHT]; /* screen'in kendi kopyasi */
	size_t   row;                            /* cursor satiri */
	size_t   col;                            /* cursor sutunu */
	uint8_t  color;                          /* aktif renk attribute'u */
}	t_console;

static t_console g_consoles[CONSOLE_COUNT];
static size_t    g_active = 0;

/* Aktif screen'e kisayol. */
static t_console *current(void)
{
	return (&g_consoles[g_active]);
}

/* Bir hucreyi buffer'a yazar; screen aktifse ekrana da yansitir. */
static void put_cell(t_console *c, size_t index, uint16_t cell)
{
	c->buffer[index] = cell;
	if (c == current())
		vga_write_cell(index, cell);
}

/* Cursor'u donanimda aktif screen'in pozisyonuna tasir. */
static void sync_cursor(void)
{
	vga_move_cursor(current()->row, current()->col);
}

/*
** Scroll (bonus): ekran dolunca butun satirlari bir yukari kaydirir,
** en alt satiri bosaltir ve cursor'u son satirda tutar.
*/
static void scroll(t_console *c)
{
	size_t i;

	/* 1. satirdan itibaren her seyi bir satir yukari tasi */
	k_memcpy(c->buffer, c->buffer + VGA_WIDTH,
		(VGA_HEIGHT - 1) * VGA_WIDTH * sizeof(uint16_t));

	/* Son satiri bosluk ile doldur */
	i = (VGA_HEIGHT - 1) * VGA_WIDTH;
	while (i < VGA_HEIGHT * VGA_WIDTH)
		c->buffer[i++] = vga_entry(' ', c->color);

	c->row = VGA_HEIGHT - 1;
	if (c == current())
		vga_blit(c->buffer);
}

/* Satir sonuna gelindiginde alt satira gec, ekran sonundaysa scroll et. */
static void newline(t_console *c)
{
	c->col = 0;
	c->row++;
	if (c->row >= VGA_HEIGHT)
		scroll(c);
}

/* Tum screen'leri sifirlar ve ilk screen'i aktif yapar. */
void console_init(void)
{
	size_t i = 0;

	while (i < CONSOLE_COUNT)
	{
		g_consoles[i].row = 0;
		g_consoles[i].col = 0;
		g_consoles[i].color = vga_color(VGA_LIGHT_GREY, VGA_BLACK);
		g_active = i;             /* console_clear aktif screen uzerinde calisir */
		console_clear();
		i++;
	}
	g_active = 0;
	vga_enable_cursor();
	vga_blit(g_consoles[0].buffer);
	sync_cursor();
}

/* Aktif screen'i degistirir ve yeni screen'i ekrana basar (bonus). */
void console_switch(size_t index)
{
	if (index >= CONSOLE_COUNT || index == g_active)
		return ;
	g_active = index;
	vga_blit(current()->buffer);
	sync_cursor();
}

size_t console_current(void)
{
	return (g_active);
}

/* Bundan sonra yazilacak karakterlerin rengini belirler (bonus). */
void console_set_color(uint8_t fg, uint8_t bg)
{
	current()->color = vga_color(fg, bg);
}

/* Aktif screen'i bosluklarla doldurur ve cursor'u basa alir. */
void console_clear(void)
{
	t_console *c = current();
	size_t     i = 0;

	while (i < VGA_WIDTH * VGA_HEIGHT)
	{
		c->buffer[i] = vga_entry(' ', c->color);
		i++;
	}
	c->row = 0;
	c->col = 0;
	vga_blit(c->buffer);
	sync_cursor();
}

/*
** Tek karakter yazar. Ozel karakterler:
**  '\n' -> yeni satir, '\r' -> satir basi,
**  '\t' -> 4'un katina hizalama, '\b' -> silme (backspace).
*/
void console_putchar(char c)
{
	t_console *con = current();

	if (c == '\n')
		newline(con);
	else if (c == '\r')
		con->col = 0;
	else if (c == '\t')
	{
		size_t spaces = 4 - (con->col % 4);

		while (spaces-- > 0)
			console_putchar(' ');
	}
	else if (c == '\b')
	{
		/* Bir karakter geri git; satir basindaysak ust satirin sonuna don */
		if (con->col > 0)
			con->col--;
		else if (con->row > 0)
		{
			con->row--;
			con->col = VGA_WIDTH - 1;
		}
		put_cell(con, con->row * VGA_WIDTH + con->col,
			vga_entry(' ', con->color));
	}
	else
	{
		put_cell(con, con->row * VGA_WIDTH + con->col,
			vga_entry(c, con->color));
		con->col++;
		if (con->col >= VGA_WIDTH)  /* satir tasti -> otomatik alt satir */
			newline(con);
	}
	sync_cursor();
}

/* Null ile biten string'i ekrana yazar. */
void console_write(const char *s)
{
	size_t i = 0;

	while (s[i] != '\0')
		console_putchar(s[i++]);
}
