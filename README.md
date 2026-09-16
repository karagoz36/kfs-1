# KFS-1 — Grub, boot and screen

42 Kernel From Scratch serisinin ilk projesi: GRUB ile boot edilen, ekrana yazı
yazabilen, sıfırdan yazılmış minimal bir i386 kernel'i.

- **Dil:** C (gnu99, freestanding) + NASM
- **Mimari:** i386 (x86, 32-bit protected mode)
- **Bootloader:** GRUB, Multiboot 1 protokolü
- **Çıktı:** `kfs.iso` (10 MB sınırının çok altında)

---

## 1. Hızlı başlangıç

### Linux (okuldaki Fedora)

```bash
sudo dnf install -y gcc nasm binutils make xorriso grub2-tools-extra grub2-pc-modules qemu-system-x86
make          # kernel.bin + kfs.iso üretir
make run      # QEMU'da başlatır  (KVM istersen: make run KVM=1)
```

> `grub2-pc-modules` şart: onsuz `grub2-mkrescue` BIOS ile boot eden bir ISO
> üretemez. Makefile, `grub-mkrescue` / `grub2-mkrescue` isim farkını kendisi bulur.

### macOS (geliştirme ortamı)

macOS'ta `gcc -m32`, `ld -m elf_i386` ve `grub-mkrescue` yok, bu yüzden derleme
bir Linux container'ı içinde yapılır; çalıştırma ise Mac'teki QEMU ile:

```bash
brew install qemu
make docker   # Docker (linux/amd64) içinde temiz build -> kfs.iso
make run      # Mac'teki qemu-system-i386 ile başlat
```

`make docker-shell` ile container'a girip elle de derleyebilirsin.

---

## 2. Boot akışı — savunmada anlatılacak zincir

```
BIOS
 └─> GRUB (ISO'nun içinde, BIOS bölümünde)
      └─> kernel.bin içindeki Multiboot header'ı bulur (ilk 8 KB, 4 byte hizalı)
           └─> kernel'i 1 MB adresine yükler, 32-bit protected mode'a geçer
                └─> linker.ld'nin ENTRY'si olan _start'a atlar   (boot/boot.asm)
                     └─> esp = stack_top  (kendi stack'imiz)
                          └─> call kernel_main                    (kernel/main.c)
```

Detaylar:

1. **Multiboot header** (`boot/boot.asm`): `0x1BADB002` magic, flags
   (`ALIGN|MEMINFO`) ve `magic + flags + checksum == 0` olacak şekilde checksum.
   GRUB bu üçlüyü dosyanın ilk 8 KB'ında arar; bu yüzden `linker.ld` `.multiboot`
   section'ını `.text`'in en başına koyar.
2. **Stack**: Multiboot spec `esp`'nin değerini garanti etmez, yani GRUB'dan
   gelen stack'e güvenilemez. C kodu stack'siz çalışamayacağı için `.bss` içinde
   16 KB ayırıp `esp`'yi tepesine yazarız (x86'da stack aşağı doğru büyür,
   System V ABI 16 byte hizalama ister).
3. **`kernel_main` geri dönmez**: bootloader artık bellekte değildir. Yine de
   dönerse `cli; hlt` ile makine durdurulur.

---

## 3. Dosya yapısı

| Dosya | Görevi |
|---|---|
| `boot/boot.asm` | Multiboot header, stack, `_start` → `kernel_main` |
| `linker.ld` | Kernel'in bellek yerleşimi (1 MB, section sırası) |
| `grub/grub.cfg` | GRUB menüsü: `multiboot /boot/kernel.bin` |
| `kernel/main.c` | `kernel_main`: ekranı kurar, "42" basar, klavye döngüsü |
| `kernel/console.c` | Virtual screen'ler, yazma, scroll, cursor senkronu |
| `kernel/printk.c` | `printk` — `%c %s %d %i %u %x %p %%` |
| `drivers/vga.c` | 0xB8000 framebuffer'a yazma + hardware cursor |
| `drivers/keyboard.c` | PS/2 klavye (polling), scancode → ASCII, kısayollar |
| `lib/string.c` | `k_strlen`, `k_strcmp`, `k_memset`, `k_memcpy` |
| `include/*.h` | Kendi tiplerimiz (`types.h`), port I/O (`io.h`), arayüzler |
| `Makefile` | İki dilli derleme + link + ISO + QEMU |
| `Dockerfile` | macOS'ta kullanılan Linux build ortamı |

---

## 4. Ekran (VGA text mode)

GRUB bizi **80x25 VGA text mode**'da bırakır. Ekran belleği `0xB8000`'dan başlar
ve her hücre 2 byte'tır:

```
 15            8 7            0
+---------------+---------------+
|   attribute   |     ASCII     |
+---------------+---------------+
   bg(4) fg(4)
```

- `vga_entry(c, color)` bu 16-bit hücreyi üretir.
- `vga_color(fg, bg)` renk byte'ını üretir (**bonus: renk desteği**, 16 renk).
- Hücre indeksi: `row * 80 + col`.

### Hardware cursor (bonus)

Cursor bellekte değil, CRT controller register'larında tutulur ve sadece port
I/O ile yazılır:

- `0x3D4` = index port (hangi register), `0x3D5` = data port (değer)
- `0x0A/0x0B`: cursor'un görünürlüğü ve hücre içi tarama satırları
- `0x0E/0x0F`: cursor pozisyonunun yüksek/düşük byte'ı

Bu yüzden `include/io.h` içinde `inb`/`outb` inline assembly ile yazılmıştır —
bu adres alanına C'den başka türlü erişilemez.

### Scroll (bonus)

Yazma son satırı taşırınca `scroll()` çalışır: buffer bir satır yukarı
`k_memcpy` ile kaydırılır, en alt satır boşlukla doldurulur, cursor son satırda
kalır. Kopyalama her zaman **screen'in kendi buffer'ına** yapılır; screen aktifse
ekrana tek seferde `vga_blit` ile yansıtılır.

### Virtual screen'ler (bonus)

`CONSOLE_COUNT = 4` adet bağımsız ekran vardır; her birinin kendi 80x25
buffer'ı, cursor pozisyonu ve rengi bulunur. Sadece aktif olan framebuffer'a
yansıtılır, dolayısıyla arkadaki ekranlar içeriğini kaybetmez.

**Kısayol:** `Alt + 1..4` (`Ctrl + 1..4` de çalışır).

> Neden F1..F4 değil: MacBook'ta F1–F4 varsayılan olarak parlaklık/Mission Control
> tuşudur ve `Ctrl+F1..F4` macOS'un kendi klavye navigasyon kısayoludur — host
> işletim sistemi tuşu yakalayıp QEMU'ya hiç iletmez. Konu belli bir tuş
> şart koşmadığı için rakam tuşları seçildi.

---

## 5. Klavye (bonus)

KFS-1'de henüz IDT/interrupt altyapısı yok, bu yüzden klavye **polling** ile
okunur — en basit ve savunması en kolay yöntem:

1. `0x64` (status port) bit 0 → okunacak veri var mı?
2. Varsa `0x60` (data port) → scancode.

Klavye **scancode set 1** gönderir: basışta *make code*, bırakışta aynı kodun
`| 0x80` hali (*break code*). Sürücü:

- `0x80` bitine bakıp basma/bırakma ayrımı yapar,
- shift / ctrl / alt durumunu tutar,
- `0xE0` prefix'li (extended) tuşları atlar,
- `Alt/Ctrl + 1..4` ise ekran değiştirir,
- değilse scancode'u ASCII tablosundan (normal / shift) çevirip ekrana basar.
  Backspace (`\b`), Enter (`\n`) ve Tab (`\t`) `console_putchar` içinde işlenir.

---

## 6. Derleme, flag'ler ve linker

### Flag'ler (`Makefile`)

| Flag | Neden |
|---|---|
| `-m32` | i386 hedefi (konu zorunlu kılıyor) |
| `-ffreestanding` | Standart kütüphane/runtime yok, `main` özel değil |
| `-fno-builtin` | GCC kendi `memcpy`/`strlen` çağrılarını eklemesin |
| `-fno-stack-protector` | Stack canary libc'den gelir, bizde yok |
| `-nostdlib -nodefaultlibs` | Host kütüphanelerine link edilirse kernel boot etmez |
| `-fno-pie -fno-pic` | Sabit adreste (1 MB) çalışan, yer değiştirmeyen kod |
| `-Wall -Wextra -Werror` | Tek bir uyarı bile kalmasın |

> Konudaki `-fno-exception` ve `-fno-rtti` C++ flag'leridir (örnek zaten C++
> içindir); C'de karşılıkları yoktur ve GCC uyarı verir, bu yüzden dile adapte
> edildiler.

### Link

Host'un hazır linker script'i kullanılamaz (konu yasaklıyor ve zaten bootable
bir imaj üretmez), bu yüzden `linker.ld` kendimiz yazıldı:

```
ENTRY(_start)
. = 1M;            /* ilk 1 MB BIOS/VGA/donanım bölgesi, oraya yazılmaz */
.text   : { *(.multiboot) *(.text) }   /* header ilk 8 KB içinde kalsın */
.rodata : ...   .data : ...   .bss : ...
```

Link komutu: `ld -m elf_i386 -T linker.ld -nostdlib -o kernel.bin *.o`

### ISO

`grub-mkrescue` kernel'i ve `grub.cfg`'yi alıp bootable bir ISO üretir. ISO'yu
küçük tutmak için sadece BIOS (`i386-pc`) hedefi, gerekli birkaç GRUB modülü ve
`--fonts= --locales= --themes= --compress=xz` kullanılır.

`make check` hem kernel'in gerçekten multiboot uyumlu olduğunu
(`grub-file --is-x86-multiboot`) hem de ISO'nun 10 MB altında kaldığını doğrular.

---

## 7. Make hedefleri

| Hedef | Ne yapar |
|---|---|
| `make` / `make iso` | `kernel.bin` + `kfs.iso` üretir |
| `make run` | QEMU'da başlatır (`KVM=1` ile donanım hızlandırma) |
| `make check` | Multiboot uyumu + 10 MB sınırı kontrolü |
| `make clean` / `fclean` / `re` | Object'leri / ISO'yu siler, sıfırdan derler |
| `make docker` | macOS'ta Linux container'ında temiz build |
| `make docker-shell` | Build container'ında bash açar |

---

## 8. Ekranda ne göreceksin

```
 KFS-1  screen 1/4

42

Kernel From Scratch 1 - bootloader: GRUB (multiboot)
printk testi: string | -42 | 42 | 0xbeef | 0xb8000 | K | %
k_strlen("42") = 2, k_strcmp("a", "a") = 0

Klavye aktif: yazabilirsin (backspace calisir).
Ekran degistir: Alt+1..4 (Ctrl+1..4 de olur).
```
