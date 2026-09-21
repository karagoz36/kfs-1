/*
** linker.ld — Kernel'in bellek yerlesimini tarif eden linker script.
**
** Host sisteminin hazir .ld dosyasini kullanamayiz (konu bunu yasakliyor ve
** zaten bootable bir binary uretmez), bu yuzden yerlesimi kendimiz yaziyoruz.
*/

/* Bootloader'in atlayacagi giris noktasi (boot.asm icinde tanimli) */
ENTRY(_start)

SECTIONS
{
	/*
	** Kernel 1 MB adresine yerlesir: ilk 1 MB'da BIOS, VGA framebuffer ve
	** diger donanim bolgeleri bulunur, oraya yazmak guvenli degildir.
	*/
	. = 1M;

	/* Multiboot header ilk 8 KB icinde olmali -> en basa koyuyoruz */
	.text ALIGN(4K) :
	{
		*(.multiboot)
		*(.text)
	}

	/* Salt okunur veriler (string literal'ler, keymap tablolari) */
	.rodata ALIGN(4K) :
	{
		*(.rodata)
	}

	/* Ilk degeri olan global/static degiskenler */
	.data ALIGN(4K) :
	{
		*(.data)
	}

	/*
	** Sifirlanmis degiskenler ve stack. Binary'de yer kaplamaz,
	** sadece calisma aninda bellekte ayrilir.
	*/
	.bss ALIGN(4K) :
	{
		*(COMMON)
		*(.bss)
	}
}
