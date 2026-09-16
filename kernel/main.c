/*
** main.c — Kernel'in C tarafindaki giris noktasi.
**
** Akis: GRUB -> boot.asm (_start) -> kernel_main().
** Buraya gelindiginde CPU 32-bit protected mode'dadir, interrupt'lar kapalidir
** ve boot.asm bize bir stack hazirlamistir.
*/

#include "console.h"
#include "printk.h"
#include "keyboard.h"
#include "string.h"
#include "io.h"

/* Her virtual screen'e kisa bir baslik yazar (bonus: coklu ekran). */
static void draw_headers(void)
{
	size_t i = 0;

	while (i < CONSOLE_COUNT)
	{
		console_switch(i);
		console_set_color(VGA_BLACK, VGA_LIGHT_GREY);
		printk(" KFS-1  screen %u/%u ", (uint32_t)(i + 1),
			(uint32_t)CONSOLE_COUNT);
		console_set_color(VGA_LIGHT_GREY, VGA_BLACK);
		printk("\n\n");
		i++;
	}
	console_switch(0);
}

/* Mandatory kisim: ekranda "42" gostermek. */
static void print_banner(void)
{
	console_set_color(VGA_LIGHT_CYAN, VGA_BLACK);
	printk("42\n\n");
	console_set_color(VGA_LIGHT_GREY, VGA_BLACK);
}

/* Bonus'lari gosteren kisa bir tanitim ve kullanim bilgisi. */
static void print_info(void)
{
	printk("Kernel From Scratch 1 - bootloader: GRUB (multiboot)\n");
	printk("printk testi: %s | %d | %u | 0x%x | %p | %c | %%\n",
		"string", -42, 42u, 48879u, (void *)0xB8000, 'K');
	printk("k_strlen(\"42\") = %d, k_strcmp(\"a\", \"a\") = %d\n",
		(int)k_strlen("42"), k_strcmp("a", "a"));
	console_set_color(VGA_LIGHT_GREEN, VGA_BLACK);
	printk("\nKlavye aktif: yazabilirsin (backspace calisir).\n");
	printk("Ekran degistir: Alt+F1..F%d (Ctrl+F1..F%d de olur).\n\n",
		CONSOLE_COUNT, CONSOLE_COUNT);
	console_set_color(VGA_LIGHT_GREY, VGA_BLACK);
}

/*
** kernel_main geri donmez: bootloader artik bellekte yok, donecek yer yok.
** Sonsuz dongude klavyeyi poll ediyoruz.
**
** Neden 'hlt' yok: GRUB bizi interrupt'lar kapali (IF = 0) birakir ve KFS_1'de
** henuz IDT yok, yani 'sti' atamayiz. IF = 0 iken 'hlt' calistiran CPU hicbir
** interrupt ile uyanamaz ve makine orada donar. Bu yuzden busy-polling yapiyoruz.
*/
void kernel_main(void)
{
	console_init();
	draw_headers();
	print_banner();
	print_info();

	while (TRUE)
		keyboard_poll();
}
