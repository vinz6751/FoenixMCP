/*
 * Support executing ATARI TOS programs.
 */

#ifndef __ATARI_PRG_LOADER_H
#define __ATARI_PRG_LOADER_H

#include "types.h"
#include "fsys.h"

/* This is the loader that is registered to the kernel */
short atari_prg_runner(short chan, long destination, long *start);

#endif