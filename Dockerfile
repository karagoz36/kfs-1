# macOS (Apple Silicon) uzerinde gelistirirken kullanilan build ortami.
# Mac'te gcc -m32, ld -m elf_i386 ve grub-mkrescue yok; bu container
# Linux/x86_64 icinde okuldaki Fedora ile ayni araclari sunar.
#
# Kullanim:  make docker   (Makefile bu imaji kurup 'make re' calistirir)

FROM debian:bookworm-slim

# Not: -o Acquire::Check-Date=false -> container ile host arasindaki saat
# farkinda apt'nin "Release file is not valid yet" hatasi vermesini onler.
RUN apt-get -o Acquire::Check-Date=false update && \
	apt-get install -y --no-install-recommends \
	build-essential \
	gcc-multilib \
	nasm \
	binutils \
	make \
	grub-common \
	grub-pc-bin \
	xorriso \
	qemu-system-x86 \
	&& rm -rf /var/lib/apt/lists/*

WORKDIR /kfs
