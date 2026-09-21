# KFS-1 — Grub, boot and screen

The first project of 42's Kernel From Scratch series: a minimal i386 kernel,
written from scratch, booted by GRUB and able to write on the screen.

- **Language:** C (gnu99, freestanding) + NASM
- **Architecture:** i386 (x86, 32-bit protected mode)
- **Bootloader:** GRUB, multiboot 1 protocol
- **Output:** `kfs.iso` (far below the 10 MB limit)

---

## 1. Quick start

### Linux (Fedora at school)

```bash
sudo dnf install -y gcc nasm binutils make xorriso grub2-tools-extra grub2-pc-modules qemu-system-x86
make          # produces kernel.bin + kfs.iso
make run      # boots it in QEMU  (for KVM: make run KVM=1)
```

> `grub2-pc-modules` is required: without it `grub2-mkrescue` cannot produce an
> ISO that boots through BIOS. The Makefile detects the `grub-*` / `grub2-*`
> naming difference (`mkrescue`, `file`) itself.

### macOS (development environment)

macOS has no `gcc -m32`, no `ld -m elf_i386` and no `grub-mkrescue`, so the
build runs inside a Linux container while QEMU runs natively:

```bash
brew install qemu
make docker   # clean build inside Docker (linux/amd64) -> kfs.iso
make run      # start it with the Mac's qemu-system-i386
```

`make docker-shell` opens a shell in the container for manual work.

---

## 2. Boot chain

```
BIOS
 └─> GRUB (inside the ISO, in the BIOS area)
      └─> finds the multiboot header in kernel.bin (first 8 KB, 4-byte aligned)
           └─> loads the kernel at 1 MB, switches to 32-bit protected mode
                └─> jumps to _start, the ENTRY of linker.ld   (boot/boot.asm)
                     └─> esp = stack_top  (our own stack)
                          └─> call kernel_main                 (kernel/main.c)
```

Details:

1. **Multiboot header** (`boot/boot.asm`): the `0x1BADB002` magic, `flags = 0`
   and a checksum chosen so that `magic + flags + checksum == 0`. Flags are
   zero because we ask GRUB for nothing extra: no modules are loaded and the
   multiboot info struct is never read, so loading the kernel from its ELF
   headers is all we need.
2. **Stack**: the multiboot spec does not guarantee the value of `esp`, so the
   stack left by GRUB cannot be trusted. C code cannot run without a stack, so
   16 KB are reserved in `.bss` and `esp` is pointed at the top of it (the
   stack grows downwards on x86 and the System V ABI wants 16-byte alignment).
3. **`kernel_main` never returns**: the bootloader is no longer in memory. If
   it did return anyway, `cli; hlt` halts the machine.

---

## 3. File layout

| File | Purpose |
|---|---|
| `boot/boot.asm` | Multiboot header, stack, `_start` → `kernel_main` |
| `linker.ld` | The kernel's memory layout (1 MB, section order) |
| `grub/grub.cfg` | GRUB menu entry: `multiboot /boot/kernel.bin` |
| `kernel/main.c` | `kernel_main`: sets up the screen, prints "42", polls the keyboard |
| `kernel/console.c` | Virtual screens, writing, scrolling, cursor sync |
| `kernel/printk.c` | `printk` — `%c %s %d %i %u %x %p %%` |
| `drivers/vga.c` | Writing into the 0xB8000 framebuffer + hardware cursor |
| `drivers/keyboard.c` | PS/2 keyboard (polling), scancode → ASCII, shortcuts |
| `lib/string.c` | `k_strlen`, `k_strcmp`, `k_memcpy` |
| `include/*.h` | Our own types (`types.h`), port I/O (`io.h`), interfaces |
| `Makefile` | Two-language build + link + ISO + QEMU |
| `Dockerfile` | The Linux build environment used on macOS |

---

## 4. The screen (VGA text mode)

GRUB leaves us in **80x25 VGA text mode**. Screen memory starts at `0xB8000`
and every cell is 2 bytes:

```
 15            8 7            0
+---------------+---------------+
|   attribute   |     ASCII     |
+---------------+---------------+
   bg(4) fg(4)
```

- `vga_entry(c, color)` builds that 16-bit cell.
- `vga_color(fg, bg)` builds the attribute byte (**bonus: colors**, 16 of them).
- Cell index: `row * 80 + col`.

### Hardware cursor (bonus)

The cursor does not live in memory but in the CRT controller's registers, which
are only reachable through port I/O:

- `0x3D4` = index port (which register), `0x3D5` = data port (the value)
- `0x0A/0x0B`: cursor visibility and the scanlines it spans inside a cell
- `0x0E/0x0F`: high/low byte of the cursor position

This is why `include/io.h` implements `inb`/`outb` in inline assembly — that
address space cannot be reached from C any other way.

### Scroll (bonus)

When a write overflows the last line, `scroll()` runs: the buffer is shifted up
by one line with `k_memcpy`, the last line is filled with spaces and the cursor
stays on it. The copy always happens in the **screen's own buffer**; if that
screen is active it is mirrored with a single `vga_blit`.

### Virtual screens (bonus)

There are `CONSOLE_COUNT = 4` independent screens, each with its own 80x25
buffer, cursor position and color. Only the active one is mirrored into the
framebuffer, so background screens never lose their contents.

**Shortcut:** `Alt + 1..4` (`Ctrl + 1..4` works too).

> Why not F1..F4: the host operating system or desktop can grab those keys
> (`Alt+F4` closes the QEMU window, `Ctrl+Alt+F1..F4` switch Linux virtual
> terminals) and never pass them to QEMU. The subject does not mandate a
> specific key, so digits were chosen.

---

## 5. Keyboard (bonus)

KFS-1 has no IDT/interrupt support yet, so the keyboard is read by **polling** —
the simplest approach, and the easiest one to explain:

1. `0x64` (status port), bit 0 → is there data to read?
2. If so, `0x60` (data port) → the scancode.

The keyboard sends **scancode set 1**: a *make code* when a key is pressed and
the same code `| 0x80` (a *break code*) when it is released. The driver:

- looks at bit `0x80` to tell presses from releases,
- keeps the shift / ctrl / alt state,
- skips keys with an `0xE0` (extended) prefix,
- switches screens on `Alt/Ctrl + 1..4`,
- otherwise translates the scancode through the ASCII table (plain / shifted)
  and prints it. Backspace (`\b`), Enter (`\n`) and Tab (`\t`) are handled
  inside `console_putchar`.

---

## 6. Building, flags and linking

### Flags (`Makefile`)

| Flag | Why |
|---|---|
| `-m32` | i386 target (mandated by the subject) |
| `-ffreestanding` | No standard library or runtime, `main` is not special |
| `-fno-builtin` | Stop GCC from emitting calls to libc `memcpy`/`strlen` |
| `-fno-stack-protector` | The stack canary comes from libc, which we lack |
| `-nostdlib -nodefaultlibs` | Linking host libraries would stop the kernel booting |
| `-fno-pie -fno-pic` | Fixed-address code running at 1 MB, no relocation |
| `-Wall -Wextra -Werror` | Not a single warning left behind |

> The subject's `-fno-exception` and `-fno-rtti` are C++ flags (its example is
> C++ after all); they have no C equivalent and GCC warns about them, so they
> were adapted to the language.

### Linking

The host's linker script cannot be used (the subject forbids it and it would
not produce a bootable image anyway), so `linker.ld` is written by hand:

```
ENTRY(_start)
. = 1M;            /* the first MB is BIOS/VGA/device territory */
.text   : { *(.multiboot) *(.text) }   /* keep the header in the first 8 KB */
.rodata : ...   .data : ...   .bss : ...
```

Link command: `ld -m elf_i386 -T linker.ld -nostdlib -o kernel.bin *.o`

Sections are page-aligned so that code and data land in separate LOAD segments;
without that the linker produces a single RWX segment and warns about it.

### The chain

```
boot.asm ──nasm──┐
                 ├──> .o files ──ld──> kernel.bin ──grub-mkrescue──> kfs.iso
*.c ──────gcc────┘    (no addresses)    (addressed,      (+ GRUB, bootable)
                                         multiboot)
```

Each stage feeds the next: the assembler and the compiler emit machine code
with the addresses still missing, the linker resolves the symbols and fixes
every address according to `linker.ld`, and `grub-mkrescue` wraps the result
together with GRUB into a bootable medium.

### ISO

`grub-mkrescue` takes the kernel and `grub.cfg` and produces a bootable ISO. To
keep it small it targets BIOS (`i386-pc`) only, installs just a few GRUB modules
and passes `--fonts= --locales= --themes= --compress=xz`.

`make check` verifies both that the kernel really is multiboot compliant
(`grub-file --is-x86-multiboot`) and that the ISO stays under 10 MB.

---

## 7. Make targets

| Target | What it does |
|---|---|
| `make` / `make iso` | Builds `kernel.bin` + `kfs.iso` |
| `make run` | Boots it in QEMU (`KVM=1` for hardware acceleration) |
| `make check` | Multiboot compliance + 10 MB limit |
| `make clean` / `fclean` / `re` | Removes objects / the ISO, rebuilds from scratch |
| `make docker` | Clean build inside a Linux container (macOS) |
| `make docker-shell` | Opens a shell in the build container |

---

## 8. What you will see

```
 KFS-1  screen 1/4

42

Kernel From Scratch 1 - bootloader: GRUB (multiboot)
printk test: string | -42 | 42 | 0xbeef | 0xb8000 | K | %
k_strlen("42") = 2, k_strcmp("a", "a") = 0

Keyboard is live: type away (backspace works).
Switch screen: Alt+1..4 (Ctrl+1..4 works too).
```
