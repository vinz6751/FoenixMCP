/*
 * Support loading of Atari TOS programs.
 *
 * This is written according to https://freemint.github.io/tos.hyp/en/gemdos_programs.html
 * The kernel doesn't have memory management so we load the programs at
 * a block we reserve.
 * 
 * Author(s):
 *   Vincent Barrilliot
 *
 * TODO:
 *   * command line support ?
 *   * environment support ?
 * Execution of a program happens like this:
 * call_user
 *   (jumps to ) atari_prg_bootstrap
 *     program
 *       Pterm0 (trap handler)
 *       the Pterm trap handlers needs to 
 * 
 */

#include <limits.h>
#include "types.h"
#include "errors.h"
#include "memory.h"
#include "dev/channel.h"
#include "proc.h"

#define TEST 1


#define DEFAULT_STACK_SIZE 256 /* Stack we create for user programs. This is deliberately tiny
                                 * as the user process is supposed to create its own stack in
                                 * its BSS segment */
#define PID 0x601a /* TODO: This is hardcoded because the proc library doesn't create PIDs yet. 
                    * As value we use the "magic" number identifying Atari PRGs,
                    * it's as good as anything else. */
#define MEM_TAG 1  /* Tag for the memory block. Whatever value is ok. */

/* This is the header of on-disk programs */
typedef struct
{
   uint16_t ph_branch;        /* Branch to start of the program,  (must be 0x601a!) */
   uint32_t ph_tlen;          /* Length of the TEXT segment               */
   uint32_t ph_dlen;          /* Length of the DATA segment               */
   uint32_t ph_blen;          /* Length of the BSS segment                */
   uint32_t ph_slen;          /* Length of the symbol table               */
   uint32_t ph_res1;          /* Reserved, should be 0, required by PureC */
   uint32_t ph_prgflags;      /* Program flags                            */
   uint16_t ph_absflag;       /* 0 = Relocation info present              */
} PROGRAM_HEADER;

#if defined(TEST)
 #include <osbind.h> /* This defines BASEPAGE */
 #include <stdio.h>
 #include <stddef.h>
#else
/* This is what a process receives as information about itself.
 * Programs expected this to be at 4(sp) when they receive control */
typedef struct basep {
    uint8_t *p_lowtpa;      /* pointer to self (bottom of TPA) */
    uint8_t *p_hitpa;       /* pointer to top of TPA + 1 */
    uint8_t *p_tbase;       /* base of text segment */
    int32_t  p_tlen;         /* length of text segment */
    uint8_t *p_dbase;       /* base of data segment */
    int32_t  p_dlen;         /* length of data segment */
    uint8_t *p_bbase;       /* base of BSS segment */
    int32_t  p_blen;         /* length of BSS segment */
    uint8_t *p_dta;         /* (UNOFFICIAL, DON'T USE) */
    struct basep *p_parent;     /* pointer to parent's basepage */
    uint8_t *p_reserved;    /* 40: were we put the stack top of the stack we reserve for the process */
    uint8_t *p_env;         /* pointer to environment string */
    uint8_t  p_junk[8];
    int32_t  p_undef[18];    /* scratch area... don't touch */
    uint8_t  p_cmdlin[128];  /* command line image */
} BASEPAGE;

#endif

/* We don't tell the kernel to execute  the program directly. Instead, we
 * call a "TOS-emulation" which sets up our own stack. */
extern atari_prg_bootstrap(BASEPAGE *pd, uint16_t *ssp);

static const char default_environment[2] = { 0, 0 };

/* BASEPAGE of the currently running program. We to know this so upon 
 * program termination we can free the DTA */
BASEPAGE *atari_tos_running_prg;

/* Prototypes */
static uint32_t get_environment_size(char *env);
static uint32_t compute_process_memory_requirement(uint32_t tpa_size, short env_size);
static short    relocate(BASEPAGE *basepage, short channel);


/* Load an Atari TOS-compatible program.
 * channel_handle: file handle
 * destination: unsupported
 * start: return address for the entry point of the program
 */
short atari_prg_loader(short channel_handle, long destination, long *start)
{
    PROGRAM_HEADER header;
    BASEPAGE *basepage;
    uint8_t  *tpa;
    uint32_t  file_length;
    uint32_t  tpa_size;
    uint32_t  textdata_size; /* Size of TEXT+DATA */
    short     env_size;
    uint8_t  *stacktop;
    short     n;
    uint8_t  *writer; 
    char     *env;
    int       i;

    env = (char*)default_environment;

    file_length = chan_seek(channel_handle, 0L, CDEV_SEEK_END);
    chan_seek(channel_handle, 0L , CDEV_SEEK_ABSOLUTE);

    /* Read header, check that format is supported */
    n = chan_read(channel_handle, (uint8_t*)&header, sizeof(PROGRAM_HEADER));
    if (n != sizeof(PROGRAM_HEADER) && header.ph_branch != 0x601a)
        return ERR_BAD_BINARY;
    file_length -= n;

    /* Allocate TPA (= Transient Program Area, the memory area holding the program)
     * and BASEPAGE (the structure given to the program so it knows about itself) */
    textdata_size = header.ph_tlen + header.ph_dlen;
    tpa_size = textdata_size + header.ph_blen;
    env_size = get_environment_size(env);
    basepage = (BASEPAGE*)mem_alloc(PID, MEM_TAG, compute_process_memory_requirement(tpa_size, env_size));
    if (basepage == 0L)
        return ERR_OUT_OF_MEMORY;

    tpa = (int8_t*)(basepage + sizeof(BASEPAGE));
    stacktop = tpa + tpa_size + DEFAULT_STACK_SIZE;

    /* Setup BASEPAGE */
    basepage->p_lowtpa = tpa;
    basepage->p_hitpa = tpa + tpa_size + 1; /* Yeah, hitpa should point after the TPA */
    /* TEXT */
    basepage->p_tbase = tpa;
    basepage->p_tlen = header.ph_tlen;

    /* DATA */
    basepage->p_dbase = basepage->p_tbase + basepage->p_tlen;
    basepage->p_dlen = header.ph_dlen;
    /* BSS */
    basepage->p_bbase = basepage->p_dbase + basepage->p_dlen;
    basepage->p_blen = header.ph_blen;
    /* Clear BSS. TODO should use bzero when it becomes available */
    for (i = 0; i < basepage->p_blen; i++)
        basepage->p_bbase[i] = 0;
    basepage->p_parent = atari_tos_running_prg; // This is set when (if!) the process actually starts in atari_prg_bootstrap
    basepage->p_reserved = stacktop;
    basepage->p_cmdlin[0] = '\0'; // We don't support command line (yet ?)
    /* Environment */
    basepage->p_env = (char*)stacktop;
    writer = basepage->p_env;
    for (i = 0 ; i < env_size ; i++)
        *writer++ = env[i];
    
    // TODO ? the following is done by emutos
    // p->p_hitpa  = (uint32_t*)p  +  max;      /*  M01.01.06   */
    // p->p_xdta = (DTA *) p->p_cmdlin;       /* default p_xdta is p_cmdlin */

    // Read file into TEXT + DATA
    writer = basepage->p_tbase;
    while (textdata_size > 0)
    {
        /* We can only read 32K at once because the size to read is passed as short */
        n = textdata_size > SHRT_MAX ? SHRT_MAX : textdata_size;
        n = chan_read(channel_handle, writer, n);
        if (n != textdata_size)
            goto read_error;

        textdata_size -= n;
        writer += n;
    }


    /* Skip symbols, so to point on the relocation table */
    chan_seek(channel_handle, header.ph_slen, CDEV_SEEK_RELATIVE);

    // If relocation is needed, do it
    if (header.ph_absflag == 0)
        if (relocate(basepage, channel_handle) != 0)
            goto read_error; /* We assume it was a read error */

    atari_tos_running_prg = basepage;

    *start = (long)atari_prg_bootstrap;
    //printf("start=%p  basepage=%p  text=%p  stack=%p\r\n", *start, basepage, basepage->p_tbase, basepage->p_reserved);
    //printf("BASEPAGE.p_tbase: %ld\r\n", offsetof(BASEPAGE,p_tbase));
    //printf("BASEPAGE.p_reserved: %ld\r\n", offsetof(BASEPAGE,p_reserved));
    /*printf("BASEPAGE: %p\r\n", basepage);
    for (i = 0; i < sizeof(BASEPAGE); i++)
        printf("%X", ((uint8_t*)basepage)[i]);
    printf("\r\n"); */
    printf("TEXT (%p)  length: %ld\r\n", basepage->p_tbase, basepage->p_tlen);
    for (i = 0; i < basepage->p_tlen/2; i++)
        printf("%04X ", ((uint16_t*)(basepage->p_tbase))[i]);
    printf("\r\n");

    return 0; /* OK */

read_error:
    if (basepage)
       mem_free(PID, (uint32_t)basepage);
    start = 0L;
    return DEV_CANNOT_READ;
}


/* Called by the emulated Pterm0 / Pterm function from the
 * trap handler when the process has completed. */
void atari_prg_terminated(uint16_t ret_code)
{
    BASEPAGE *parent = atari_tos_running_prg->p_parent; 
    mem_free(PID, (uint32_t)atari_tos_running_prg);
    atari_tos_running_prg = parent;

    proc_exit(ret_code);
}


static uint32_t compute_process_memory_requirement(uint32_t tpa_size, short env_size)
{
    return
        sizeof(BASEPAGE) 
        + tpa_size
        + DEFAULT_STACK_SIZE
        + env_size;
}


/* Calculate size of environment string.
 * Environment is composed of multiple NULL-terminated strings.
 * An ending empty string (ie 2 consecutive NULL chars) mark the end. */
static uint32_t get_environment_size(char *env)
{
    char *c;

    if (env == 0L)
        return 0L;

    c = env;    
    while (c[0] || c[1])
        c += 2;
    return (c + 2) - env;
}


static short relocate(BASEPAGE *basepage, short channel_handle)
{
    uint8_t *reloc_target;

    /* Read 32-bit initial offset */
    if (chan_read(channel_handle, (uint8_t*)&reloc_target, sizeof(reloc_target)) != sizeof(reloc_target))
        return DEV_CANNOT_READ;

    /* Beware relocation table can be present but empty */
    if (reloc_target == 0L)
        return 0; /* OK */

    /* Initial offset (index of first item to relocate) */
    reloc_target = (uint8_t*)(basepage->p_tbase + (long)reloc_target);
    printf("Initial relocation target: %p\r\n", (uint32_t*)reloc_target);

    bool first = 1;
    uint32_t p;
    /* Loop through the relocation entries */
    for (;;)
    {
        if (first)
            first = 0;
        else
        {
            uint8_t offset;
            do
            {
                if (chan_read(channel_handle, &offset, sizeof(offset)) != sizeof(offset)) 
                    return DEV_CANNOT_READ;

                reloc_target += (offset == 1) ? 254 : offset;
            } while (offset == 1);

            if (offset == 0)
                break;
        }

        *((uint32_t*)reloc_target) += (long)basepage->p_tbase;
        printf("Relocation target=%lx to %lx\r\n",p , *reloc_target);
    }

    return 0; /* OK */
}

#if defined(TEST)

#include <stdio.h>

short chan_read(short channel, uint8_t * buffer, short size)
{
    return Fread(channel, size, buffer);
}

short chan_seek(short handle, long offset, short base)
{
    return (short)Fseek(offset, handle, base);;
}

uint32_t mem_alloc(unsigned short pid, unsigned short tag, uint32_t bytes) 
{
    return Malloc(bytes);
}

void mem_free(unsigned short pid, uint32_t address)
{
    Mfree(address);
}

void proc_exit(int code)
{
    printf("proc_exit %d", code);
}

extern void atari_trap1handler(void);

void main(void) 
{
    short handle;
    long  entry_point;
    const char filename[] = "TEST.TOS";
    short r;
    long stack;

    /* We're supposed to be in the kernel, so supervisor */
    stack = Super(0L);

    /* Install our trap 5 handler for GEMDOS emulation*/
    Setexc(0x25,atari_trap1handler);

    /* Open file */
    handle = Fopen(filename, 0);
    if (handle <= 0)
    {
        printf("Failed to open %s\n", filename);
        return;
    }

    /* Invoke loader */
    r = atari_prg_loader(handle, 0, &entry_point);
    if (r != 0)
    {
        printf("r = %d\r\n",r);
        Fclose(handle);
        return;
    }

    printf("Calling entry point %p for text at %p\r\n", entry_point, atari_tos_running_prg->p_tbase);
    Super(stack);
    ((void (*)())entry_point)();
}
#endif
