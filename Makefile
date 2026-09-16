# ==============================================================================
# KFS-1 Makefile
#
# Iki farkli dil derlenir: NASM (boot kodu) ve C (kernel).
# Sonunda butun object file'lar kendi linker script'imizle birlestirilip
# multiboot uyumlu bir ELF binary'ye, o da GRUB ile bootable bir ISO'ya donusur.
#
# Linux (Fedora/Debian) uzerinde:  make && make run
# macOS uzerinde (native toolchain yok): make docker  -> ISO uretir, sonra make run
# ==============================================================================

NAME      := kernel.bin
ISO       := kfs.iso

BUILD_DIR := build
ISO_DIR   := $(BUILD_DIR)/isodir

CC := gcc
AS := nasm
LD := ld

# Kaynaklar
ASM_SRCS := boot/boot.asm
C_SRCS   := kernel/main.c \
            kernel/console.c \
            kernel/printk.c \
            drivers/vga.c \
            drivers/keyboard.c \
            lib/string.c

ASM_OBJS := $(ASM_SRCS:%.asm=$(BUILD_DIR)/%.o)
C_OBJS   := $(C_SRCS:%.c=$(BUILD_DIR)/%.o)
OBJS     := $(ASM_OBJS) $(C_OBJS)
DEPS     := $(C_OBJS:.o=.d)

# ------------------------------------------------------------------------------
# Flags
#   -m32                 : i386 (32-bit) kod uret; konu x86 zorunlu kiliyor
#   -ffreestanding       : standart kutuphane/runtime yok, main ozel degil
#   -fno-builtin         : gcc kendi memcpy/strlen gibi optimizasyonlarini koymasin
#   -fno-stack-protector : stack canary libc'den gelir, bizde yok
#   -nostdlib/-nodefaultlibs : host kutuphanelerine link etme (yoksa boot etmez)
#   -fno-pie / -fno-pic  : sabit adreste (1 MB) calisan, yer degistirmeyen kod
# Not: Konudaki -fno-exception ve -fno-rtti C++ flag'leridir; C tarafinda
#      karsiliklari yoktur (gcc uyari verir), bu yuzden dile adapte edildiler.
# ------------------------------------------------------------------------------
CFLAGS := -m32 -std=gnu99 -O2 \
          -ffreestanding -fno-builtin -fno-stack-protector \
          -fno-pie -fno-pic -nostdlib -nodefaultlibs \
          -Wall -Wextra -Werror \
          -Iinclude -MMD -MP

ASFLAGS := -f elf32

# Host'un .ld dosyasi yerine kendi linker script'imiz kullanilir.
LDFLAGS := -m elf_i386 -T linker.ld -nostdlib

# ------------------------------------------------------------------------------
# Dagitima gore degisen arac isimleri (Fedora'da grub2-*, Debian'da grub-*)
# ------------------------------------------------------------------------------
GRUB_MKRESCUE := $(shell command -v grub-mkrescue 2>/dev/null || command -v grub2-mkrescue 2>/dev/null)
GRUB_I386_DIR := $(firstword $(wildcard /usr/lib/grub/i386-pc /usr/lib/grub2/i386-pc /usr/share/grub2/i386-pc))

# ISO'yu kucuk tutmak icin: sadece BIOS (i386-pc) hedefi, gereksiz
# font/locale/tema yok, sadece ihtiyacimiz olan GRUB modulleri.
GRUB_FLAGS := --fonts= --locales= --themes= --compress=xz \
              --install-modules="multiboot normal biosdisk iso9660"
ifneq ($(GRUB_I386_DIR),)
GRUB_FLAGS += -d $(GRUB_I386_DIR)
endif

# QEMU: Linux'ta KVM=1 ile donanim hizlandirmasi acilabilir
QEMU  := qemu-system-i386
QEMUFLAGS := -cdrom $(ISO) -m 64
ifeq ($(KVM),1)
QEMUFLAGS += -enable-kvm
endif

# macOS'ta native toolchain olmadigi icin build'i container icinde yapariz
DOCKER_IMAGE := kfs1-build

.PHONY: all iso run clean fclean re docker docker-image docker-shell check

all: $(ISO)

# --- Derleme kurallari ---
$(BUILD_DIR)/%.o: %.c
	@mkdir -p $(dir $@)
	$(CC) $(CFLAGS) -c $< -o $@

$(BUILD_DIR)/%.o: %.asm
	@mkdir -p $(dir $@)
	$(AS) $(ASFLAGS) $< -o $@

# --- Link: butun object'ler tek bir multiboot ELF'e ---
$(ISO_DIR)/boot/$(NAME): $(OBJS) linker.ld
	@mkdir -p $(dir $@)
	$(LD) $(LDFLAGS) -o $@ $(OBJS)
	@echo "==> $(NAME) linklendi"

# --- ISO: GRUB'i kernel ile birlikte bootable bir imaja koyar ---
$(ISO): $(ISO_DIR)/boot/$(NAME) grub/grub.cfg
	@if [ -z "$(GRUB_MKRESCUE)" ]; then \
		echo "HATA: grub-mkrescue bulunamadi. macOS'ta 'make docker' kullan."; \
		exit 1; \
	fi
	@mkdir -p $(ISO_DIR)/boot/grub
	cp grub/grub.cfg $(ISO_DIR)/boot/grub/grub.cfg
	$(GRUB_MKRESCUE) $(GRUB_FLAGS) -o $(ISO) $(ISO_DIR)
	@echo "==> $(ISO) hazir ($$(du -h $(ISO) | cut -f1))"

iso: $(ISO)

# Kernel gercekten multiboot uyumlu mu ve ISO 10 MB sinirinda mi?
check: $(ISO)
	grub-file --is-x86-multiboot $(ISO_DIR)/boot/$(NAME) && echo "multiboot: OK"
	@test $$(stat -c %s $(ISO) 2>/dev/null || stat -f %z $(ISO)) -lt 10485760 \
		&& echo "boyut: 10 MB altinda OK"

run: $(ISO)
	$(QEMU) $(QEMUFLAGS)

# --- macOS icin: Linux container'inda derleme ---
docker-image:
	docker build --platform linux/amd64 -t $(DOCKER_IMAGE) .

docker: docker-image
	docker run --rm --platform linux/amd64 -v "$(PWD)":/kfs -w /kfs $(DOCKER_IMAGE) make re

docker-shell: docker-image
	docker run --rm -it --platform linux/amd64 -v "$(PWD)":/kfs -w /kfs $(DOCKER_IMAGE) bash

clean:
	rm -rf $(BUILD_DIR)

fclean: clean
	rm -f $(ISO)

re: fclean all

# Header bagimliliklari (-MMD ile uretilir): header degisince .c yeniden derlenir
-include $(DEPS)
