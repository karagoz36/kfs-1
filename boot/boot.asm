; boot.asm — Kernel'in assembly giris noktasi (NASM, 32-bit).
;
; Iki isi vardir:
;   1) GRUB'in kernel'imizi taniyabilmesi icin Multiboot header koymak,
;   2) C kodunun calisabilmesi icin bir stack hazirlayip kernel_main'i cagirmak.

bits 32

; --- Multiboot v1 header sabitleri ---
; GRUB, dosyanin ilk 8 KB'inda 4 byte hizali olarak MAGIC degerini arar.
MB_ALIGN    equ 1 << 0                   ; modulleri page boundary'ye hizala
MB_MEMINFO  equ 1 << 1                   ; bellek haritasini bize ver
MB_FLAGS    equ MB_ALIGN | MB_MEMINFO
MB_MAGIC    equ 0x1BADB002               ; multiboot 1 imzasi
MB_CHECKSUM equ -(MB_MAGIC + MB_FLAGS)   ; magic + flags + checksum = 0 olmali

; Header'i kendi section'ina koyuyoruz; linker script bunu dosyanin
; en basina yerlestirdigi icin GRUB header'i mutlaka bulur.
section .multiboot
align 4
	dd MB_MAGIC
	dd MB_FLAGS
	dd MB_CHECKSUM

; --- Stack ---
; Multiboot spec esp register'inin degerini garanti etmez, yani GRUB'dan
; gelen stack'e guvenemeyiz. C kodu stack olmadan calisamayacagi icin
; 16 KB'lik bir alani .bss'te ayiriyoruz (dosya boyutunu buyutmez).
; x86 System V ABI stack'in 16 byte hizali olmasini ister.
section .bss
align 16
stack_bottom:
	resb 16384
stack_top:

section .text
global _start
extern kernel_main

_start:
	; Stack x86'da asagi dogru buyur, bu yuzden esp tepeyi gosterir.
	mov esp, stack_top

	; C tarafina gec. Bu cagri normal sartlarda geri donmez.
	call kernel_main

	; Yine de donerse: interrupt'lari kapat ve makineyi sonsuza kadar durdur.
	cli
.hang:
	hlt
	jmp .hang

; Modern linker'lara "stack'in executable olmasina gerek yok" demek icin
; konulan bos section; olmazsa ld uyari verir.
section .note.GNU-stack noalloc noexec nowrite progbits
