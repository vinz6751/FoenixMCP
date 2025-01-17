#
# User may over-ride the UNIT and MEMORY parameters to specify target machine
# and where the MCP will run (ram or flash)
#
UNIT := C256U_PLUS
MEMORY := ram

# The script expects the following environment variables to be set:
# VBCC: path to VBCC distribution 

# Define OS-dependent tools ---------------------------------------------------
ifeq ("$(PAD_FLASH_SIZE)","")
	# When compiling an image for flash, size to which the image must be padded
	PAD_FLASH_SIZE = 524288
endif
ifeq ($(OS),Windows_NT)
	RM = del /F/Q
	PAD_CMD = fsutil file seteof $@ $(PAD_FLASH_SIZE)
	SEP= \$ # Leave this as is, it's a trick
	PYTHON3=python
else
	RM = rm -f
	PAD_CMD = truncate -s $(PAD_FLASH_SIZE) $@
	SEP = /
	PYTHON3=python3
endif


# The below doesn't seem true !
# CPU_WDC65816                0x16  /* CPU code for the Western Design Center 65816 */
# CPU_M68000                  0x20  /* CPU code for the Motorola 68000 */
# CPU_M68010                  0x21  /* CPU code for the Motorola 68010 */
# CPU_M68020                  0x22  /* CPU code for the Motorola 68020 */
# CPU_M68030                  0x23  /* CPU code for the Motorola 68030 */
# CPU_M68040                  0x24  /* CPU code for the Motorola 68040 */
# CPU_I486DX                  0x34  /* CPU code for the Intel 486DX */

# MODEL_FOENIX_FMX            0
# MODEL_FOENIX_C256U          1
# MODEL_FOENIX_GENX           4
# MODEL_FOENIX_C256U_PLUS     5
# MODEL_FOENIX_A2560U_PLUS    6
# MODEL_FOENIX_A2560X         8
# MODEL_FOENIX_A2560U         9
# MODEL_FOENIX_A2560K         11

# Unit-specific properties ----------------------------------------------------

ifeq ($(UNIT),a2560k)
	MODEL_NUMBER = 11
	CPU = m68040
	MODEL_FAMILY = GENX32

else ifeq ($(UNIT),genx)
	MODEL_NUMBER = 4
	CPU = m68040
	MODEL_FAMILY = GENX32

else ifeq ($(UNIT),a2560x)
	MODEL_NUMBER = 8
	CPU = m68040
	MODEL_FAMILY = GENX32

else ifeq ($(UNIT),a2560u)
	MODEL_NUMBER = 9
	CPU = m68k
else ifeq ($(UNIT),a2560u_plus)
	MODEL_NUMBER = 6
	CPU = m68k
else ifeq ($(UNIT),C256U)
	CPU = w65816
	MODEL_NUMBER = 1
	SRCS_FOR_UNIT =
	LDFLAGS_FOR_UNIT = C256/ld_lc_c256_u.scm clib-lc-ld.a

else ifeq ($(UNIT),C256U_PLUS)
	MODEL_NUMBER = 5
	CPU = w65816
	SRCS_FOR_UNIT = C256/jumptable.s C256/syscalls.s C256/syscalls_c256.c C256/io_stubs.c
	LDFLAGS_FOR_UNIT = C256/ld_lc_c256_fmx.scm clib-lc-ld.a --rtattr printf=medium

else ifeq ($(UNIT),C256_FMX)
	CPU = w65816
	MODEL_NUMBER = 0
	SRCS_FOR_UNIT =
	LDFLAGS_FOR_UNIT = C256/ld_lc_c256_fmx.scm clib-lc-ld.a
endif


# Log settings ----------------------------------------------------------------
# FATAL   0   Log a critical or fatal event
# ERROR   1   Log an error
# INFO    2   Log miscellaneous information
# DEBUG   3   Log a debugging message
# TRACE   4   Log tracing information (like entry into a subroutine)
# VERBOSE 5   Log a truly verbose message... the sort you almost never want to bother with
# Log message above DEFAULT_LOG_LEVEL level will not even be compiled, 
# ie they will have NO chance of being output even if log_level is high enough.
DEFAULT_LOG_LEVEL=1


# Common defines and includes -------------------------------------------------
DEFINES = -DCPU=$(CPU_NUMBER) -DMODEL=$(MODEL_NUMBER) # -DKBD_POLLED 
ifeq ("$(CPU)","w65816")
	CPU_NUMBER = 255
else ifeq ("$(CPU)","m68040") # M68040V or 68040
	CPU_NUMBER = 6
else ifeq ("$(CPU)","m68k") # M68000 or 68SEC000
	CPU_NUMBER = 0
endif

# Maximum working logging level to compile
#define LOG_FATAL   0   /* Log a critical or fatal event */
#define LOG_ERROR   1   /* Log an error */
#define LOG_INFO    2   /* Log miscellaneous information */
#define LOG_DEBUG   3   /* Log a debugging message */
#define LOG_TRACE   4   /* Log tracing information (like entry into a subroutine) */
#define LOG_VERBOSE 5   /* Log a truly verbose message... the sort you almost never want to bother with */
DEFAULT_LOG_LEVEL=0

#define LOG_CHANNEL_CHANNEL_A 0
#define LOG_CHANNEL_CHANNEL_B 1
#define LOG_CHANNEL_CHANNEL_A_LOW_LEVEL 2 // low-level routines (doesn't use MCP's console stuff)
#define LOG_CHANNEL_COM1 10
#define LOG_CHANNEL_COM2 11
LOG_CHANNEL=11

ifeq ("$(DEFAULT_LOG_LEVEL)","")
else
 DEFINES := $(DEFINES) -DDEFAULT_LOG_LEVEL=$(DEFAULT_LOG_LEVEL)
endif
ifeq ("$(LOG_CHANNEL)","")
else
 DEFINES := $(DEFINES) -DLOG_CHANNEL=$(LOG_CHANNEL)
endif

INCLUDES = -I. -I$(CURDIR)/include -I$(CURDIR)
ifeq ($(OS),Windows_NT)
 INCLUDES := $(subst /,$(SEP),$(INCLUDES))
endif


# Toolchain selection ---------------------------------------------------------
ifeq ($(CPU),w65816)
	TOOLCHAIN = calypsi
else
	TOOLCHAIN = vbcc
endif


# Toolchain specific settings -------------------------------------------------
ifeq ($(TOOLCHAIN),calypsi)
	CC = cc65816
	AS = as65816
	LD = ln65816
	AR = nlib

	# Determine the correct configuration file (barring OS)
	ifeq ($(MEMORY),ram)
		LD_OUTPUT_TYPE = s37
	else
		LD_OUTPUT_TYPE = # TODO
	endif

	CFLAGS = -l --target Foenix --code-model large --data-model large
	LDFLAGS=--target foenix --output-format $(LD_OUTPUT_TYPE) --list-file foenixmcp.map
else
	CC = vc
	AS = vasmm68k_mot

	# VBCC
	ifeq ("$(CPU)","m68k")
		VASM_CPU = -m68000
		VBCC_CPU = 68000
	else ifeq ("$(CPU)","m68040")
		VASM_CPU = -m68040
		VBCC_CPU = 68040
	endif

	# Determine the correct configuration file (barring OS)
	ifeq ($(MEMORY),ram)
		CFG_FILE = $(VBCC)/config/$(UNIT)_ram
	else
		CFG_FILE = $(VBCC)/config/$(UNIT)_flash
	endif
	ifneq ($(OS),Windows_NT)
		CFG_FILE := $(CFG_FILE)_linux
	else
		CFG_FILE := $(subst /,$(SEP),$(CFG_FILE))
	endif

	ASFLAGS = -Fvobj -nowarn=62 $(VASM_CPU) -quiet
	ifneq ($(MODEL_FAMILY),GENX32)
		# Inline functions cause problems (at least on the X)
		CFLAGS_EXTRA=-D__NOINLINE__
	else
	endif	
	CFLAGS =-c -S -cpu=$(VBCC_CPU) +$(CFG_FILE) $(CFLAGS_EXTRA)
	LDFLAGS = +$(CFG_FILE)
endif


# Flags (are automatically used by make) --------------------------------------
ASFLAGS := $(ASFLAGS) $(DEFINES) $(INCLUDES)
CFLAGS  := $(CFLAGS) $(DEFINES) $(INCLUDES)
LDFLAGS := $(LDFLAGS) $(LDFLAGS_FOR_UNIT)

# New make file (Calypsi / 1_makefile per folder). This needs adaptation (removal?)
SRCS = foenixmcp.c log.c memory.c ring_buffer.c simpleio.c sys_general.c variables.c utilities.c $(SRCS_FOR_UNIT)
OBJS = $(patsubst %.s,%.o,$(patsubst %.c,%.o,$(SRCS)))
OBJS_TO_CLEAN = $(subst /,\\,$(OBJS))
LIBS = dev/devices.a snd/sound.a fatfs/fatfs.a

# Splashscreen
SPLASHSCREEN = rsrc/bitmaps/splash_$(UNIT).o

# CPU-specific files
cpu_s_src := $(wildcard $(CPU)/*.s)
cpu_c_src := $(wildcard $(CPU)/*.c)
cpu_obj := $(subst .c,.o,$(cpu_c_src)) $(subst .s,.o,$(cpu_s_src))

# Device drivers (common to all Foenix)
dev_c_src = block.c channel.c console.c fsys.c pata.c ps2.c rtc.c sdc.c txt_screen.c uart.c
dev_s_src =
ifeq ($(UNIT),a2560k)
	dev_c_src := $(dev_c_src) fdc.c kbd_mo.c lpt.c midi.c txt_a2560k_a.c txt_a2560k_b.c superio.c
else ifeq ($(UNIT),genx)
	dev_c_src := $(dev_c_src) fdc.c lpt.c midi.c txt_a2560k_a.c txt_a2560k_b.c superio.c
else ifeq ($(UNIT),a2560x)
	dev_c_src := $(dev_c_src) fdc.c lpt.c midi.c txt_a2560k_a.c txt_a2560k_b.c superio.c
else ifeq ($(UNIT),a2560u)
	dev_c_src := $(dev_c_src) txt_a2560u.o
endif
dev_obj := $(addprefix dev/, $(subst .c,.o,$(dev_c_src))) $(addprefix dev/, $(subst .s,.o,$(dev_s_src)))

# Sound drivers
snd_c_src = codec.c psg.c sid.c yamaha.c
ifeq ($(UNIT),C256_FMX)
	snd_c_src = $(snd_c_src) codec_c256.c
endif
snd_obj := $(addprefix snd/, $(subst .c,.o,$(snd_c_src)))

# File system
fat_c_src := $(wildcard fatfs/*.c)
fatfs_obj := $(subst .c,.o,$(fat_c_src))

# Command Line Interface
cli_c_src := $(wildcard cli/*.c)
cli_obj := $(subst .c,.o,$(cli_c_src))

# Base MCP files
c_src :=  $(wildcard *.c)
c_obj := $(subst .c,.o,$(c_src))

.PHONY: all clean $(CPU) dev fatfs snd cli

# Target selection ------------------------------------------------------------
ifeq ($(MEMORY),ram)
#	ifeq ($(CPU),"w65816")
#all:foenixmcp.s37
#	else
all: foenixmcp.s68 $(CPU) dev snd cli
#	endif
else
all: foenixmcp.bin $(CPU) dev snd cli
endif

# Per-folder libraries (used by Calypsi)
foenixmcp.s37: $(OBJS) $(LIBS)
	$(LD) $(LDFLAGS) -o $@  $(subst /,$(SEP),$^)


DEPS = $(cpu_obj) $(c_obj) $(dev_obj) $(fatfs_obj) $(snd_obj) $(cli_obj) $(SPLASHSCREEN)

# We don't want to link:
# - the startup file (because the linker script does it)
# - the splashscreen because it's #included from somewhere (boot.c)
# So we remove them from the string of files to link.
STARTUP_OBJ=$(CPU)/startup_$(CPU).o
FILES_TO_LINK= $(subst $(STARTUP_OBJ),,$(DEPS))
FILE_TO_LINK:=$(subst /,$(SEP),$(FILES_TO_LINK))

foenixmcp.s68: $(DEPS)
	$(CC) $(LDFLAGS) -o $@ $(FILES_TO_LINK)

foenixmcp.bin: $(DEPS)
	$(CC) $(LDFLAGS) -o $@ $(FILES_TO_LINK)
# TODO Test that the file is <= PAD_FLASH_SIZE bytes otherwise'll be slashing the OS !
	$(PAD_CMD)

# Rule for generating include file containing RLE-compressed version of BMP splashscreen
# You need python3 installed obviously, and install some extra packages you can install with:
# pip3 install Image
# pip3 install pillow  
rsrc/bitmaps/splash_%.c: rsrc/bitmaps/splash_%.bmp
	$(PYTHON3) rsrc/bitmaps/bmp2c.py -i $^ -o $@

# Clean up after a build
clean:
	$(RM) $(subst /,$(SEP),$(OBJS_TO_CLEAN)) $(subst /,$(SEP),$(DEPS)) foenixmcp.s37 *.lst *.map
	$(MAKE) --directory=$(CPU) clean
	$(MAKE) --directory=cli clean
	$(MAKE) --directory=dev clean
	$(MAKE) --directory=fatfs clean
	$(MAKE) --directory=snd clean
