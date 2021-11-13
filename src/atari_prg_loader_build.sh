#!/bin/sh
m68k-atari-mint-gcc $LIBCMINI/crt0.o atari_prg_loader.c atari_prg_bootstrap.s -fomit-frame-pointer -L$LIBCMINI -lcmini -lgcc -o prgload.tos -I./include -nostdlib
