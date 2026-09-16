/*
** printk.c — printf benzeri yazdirma fonksiyonu.
**
** <stdarg.h> kullanamadigimiz icin degisken argumanlari GCC'nin built-in
** mekanizmasi (__builtin_va_list) ile okuyoruz; bu, header degil derleyicinin
** kendi ozelligi oldugu icin freestanding ortamda serbesttir.
*/

#include "printk.h"
#include "console.h"
#include "string.h"

typedef __builtin_va_list va_list_t;
#define VA_START(ap, last) __builtin_va_start(ap, last)
#define VA_ARG(ap, type)   __builtin_va_arg(ap, type)
#define VA_END(ap)         __builtin_va_end(ap)

/*
** Isaretsiz sayiyi istenen base'de (10 veya 16) yazar.
** Basamaklari ters sirada uretip gecici buffer'a koyar, sonra ters cevirip basar.
*/
static void print_uint(uint32_t value, uint32_t base, bool_t upper)
{
	const char *digits_lower = "0123456789abcdef";
	const char *digits_upper = "0123456789ABCDEF";
	const char *digits = upper ? digits_upper : digits_lower;
	char        tmp[32];
	size_t      len = 0;

	if (value == 0)
		tmp[len++] = '0';
	while (value > 0)
	{
		tmp[len++] = digits[value % base];
		value /= base;
	}
	while (len > 0)
		console_putchar(tmp[--len]);
}

/* Isaretli sayi: negatifse once '-' basar, sonra mutlak degerini yazar. */
static void print_int(int32_t value)
{
	uint32_t magnitude;

	if (value < 0)
	{
		console_putchar('-');
		/*
		** INT_MIN (-2147483648) icin -value tasma yapar; bu yuzden isareti
		** unsigned aritmetikle (two's complement) ceviriyoruz.
		*/
		magnitude = (uint32_t)(~(uint32_t)value + 1u);
	}
	else
		magnitude = (uint32_t)value;
	print_uint(magnitude, 10, FALSE);
}

void printk(const char *format, ...)
{
	va_list_t ap;
	size_t    i = 0;

	VA_START(ap, format);
	while (format[i] != '\0')
	{
		/* Normal karakterler dogrudan ekrana gider */
		if (format[i] != '%')
		{
			console_putchar(format[i++]);
			continue ;
		}
		i++;
		if (format[i] == 'c')
			console_putchar((char)VA_ARG(ap, int));
		else if (format[i] == 's')
		{
			const char *s = VA_ARG(ap, const char *);

			console_write(s ? s : "(null)");
		}
		else if (format[i] == 'd' || format[i] == 'i')
			print_int(VA_ARG(ap, int32_t));
		else if (format[i] == 'u')
			print_uint(VA_ARG(ap, uint32_t), 10, FALSE);
		else if (format[i] == 'x')
			print_uint(VA_ARG(ap, uint32_t), 16, FALSE);
		else if (format[i] == 'X')
			print_uint(VA_ARG(ap, uint32_t), 16, TRUE);
		else if (format[i] == 'p')
		{
			/* Pointer'i 0x... seklinde hexadecimal basar */
			console_write("0x");
			print_uint((uint32_t)VA_ARG(ap, void *), 16, FALSE);
		}
		else if (format[i] == '%')
			console_putchar('%');
		else
		{
			/* Taninmayan specifier: oldugu gibi yazdir (%q -> "%q") */
			console_putchar('%');
			if (format[i] != '\0')
				console_putchar(format[i]);
		}
		if (format[i] != '\0')
			i++;
	}
	VA_END(ap);
}
