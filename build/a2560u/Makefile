VPATH = ../../src:../../src/cli:../../src/dev:../../src/fatfs:../../src/m68k:../../src/snd:../../module/Calypsi-remote-debug/src

ASM_SRCS = low_level_68000.s startup_m68k.s
C_SRCS_DEBUGGER = agent.c a2560-uart.c
C_SRCS_CLI = cli.c dis68k.c dos_cmds.c dos_copy.c mem_cmds.c settings.c sound_cmds.c test_cmd2.c
C_SRCS_DEV = block.c channel.c console.c fdc.c fsys.c kbd_mo.c lpt.c midi.c pata.c ps2.c rtc.c sdc.c \
             txt_a2560u.c txt_screen.c uart.c
C_SRCS_FATFS = c256_diskio.c ff.c ffsystem.c ffunicode.c
C_SRCS_SND = codec.c psg.c sid.c yamaha.c
C_SRCS_M68K = bios_m68k.c
C_SRCS = $(C_SRCS_DEBUGGER) $(C_SRCS_CLI) $(C_SRCS_DEV) $(C_SRCS_FATFS) $(C_SRCS_SND) $(C_SRCS_M68K) \
         boot.c foenixmcp.c indicators.c interrupt.c log.c memory.c proc.c ring_buffer.c \
         simpleio.c sys_general.c syscalls.c timers.c utilities.c variables.c

OBJS = $(ASM_SRCS:%.s=obj/%.o) $(C_SRCS:%.c=obj/%.o)

CFLAGS = -DMODEL=9 -DA2560U -D_CALYPSI_MCP_BUILD --data-model=far-only --code-model=large --debug

obj/%.o: %.s
	as68k $(CFLAGS) --list-file=$(@:%.o=%.lst) -o $@ $<

obj/%.o: %.c
	cc68k $(CFLAGS) -I../../src/include -I../../src --list-file=$(@:%.o=%.lst) -o $@ $<

a2560u-mcp.s68: $(OBJS) $(cpu) dev fatfs snd cli
	ln68k -o a2560u-mcp.elf $(OBJS) \
		--sstack-size=2000 --stack-size=1000 --heap-size=1000 -l --cross-reference --rom-code  \
		--debug --rtattr printf=medium --output-format=s28 --semi-hosted mcp-ram.scm clib-68000-lc-fod.a

a2560u-mcp.raw: $(OBJS) $(cpu) dev fatfs snd cli
	ln68k -o a2560u-mcp.elf $(OBJS) \
		--sstack-size=2000 --stack-size=1000 --heap-size=1000 -l --cross-reference --rom-code  \
		--debug --rtattr printf=medium --output-format=raw \
		--semi-hosted mcp-flash.scm clib-68000-lc-fod.a

clean:
	-rm $(OBJS) obj/*lst a2560u-mcp.s68 a2560u-mcp.elf a2560u-mcp.raw ln68k.lst
