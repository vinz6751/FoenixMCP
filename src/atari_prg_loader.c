/*
 * Support loading of Atari TOS programs.
 *
 * This is written according to https://freemint.github.io/tos.hyp/en/gemdos_programs.html
 * The kernel doesn't have memory management so we load the programs at
 * a fixed address.
 * 
 * Author(s):
 *   Vincent Barrilliot
 *
 * TODO:
 *   * stack setting and filling with basepage
 *   * command line support ?
 *   * environment support ?
 */

#include <limits.h>
#include "types.h"
#include "errors.h"
#include "dev/channel.h"
#include "memory.h"

#define DEFAULT_STACK_SIZE 1024
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
   uint32_t ph_absflag;       /* 0 = Relocation info present              */
} PROGRAM_HEADER;

/* This is what a process receives as information about itself.
 * Programs expected this to be at 4(sp) when they receive control */
typedef struct pd
{
   uint32_t *p_lowtpa;      /* Start address of the TPA            */
   uint32_t *p_hitpa;       /* First byte after the end of the TPA */
   uint32_t *p_tbase;       /* Start address of the program code   */
   uint32_t  p_tlen;        /* Length of the program code          */
   uint32_t *p_dbase;       /* Start address of the DATA segment   */
   uint32_t  p_dlen;        /* Length of the DATA section          */
   uint32_t *p_bbase;       /* Start address of the BSS segment    */
   uint32_t  p_blen;        /* Length of the BSS section           */
   uint32_t *p_dta;         /* TODO: Pointer to the default DTA          */
                            /* Warning: Points first to the        */
                            /* command line !                      */
   struct pd *p_parent;     /* Pointer to the basepage of the      */
                            /* calling processes                   */
   int32_t   p_resrvd0;     /* Reserved                            */
   char     *p_env;         /* Address of the environment string; double-NULL-terminated */
   int8_t    p_resrvd1[80]; /* Reserved                            */
   int8_t    p_cmdlin[128]; /* Command line, length.b first, beware string may not be NULL-terminated */
} BASEPAGE; /* also known as Process Descriptor */


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
    void     *stacktop;
    short     n;
    short     result;
    uint8_t  *writer; 
    char     *env;

    start = 0L;
    env = (char*)default_environment;

    file_length = chan_seek(channel_handle, -1/*max long value*/, CDEV_SEEK_ABSOLUTE);
    chan_seek(channel_handle, 0 , CDEV_SEEK_ABSOLUTE);
 
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
    if (basepage = 0L)
        return ERR_OUT_OF_MEMORY;
    tpa = (int8_t*)(basepage + sizeof(BASEPAGE));
    stacktop = (uint16_t*)(tpa + DEFAULT_STACK_SIZE);

    /* Setup BASEPAGE */
    basepage->p_lowtpa = (uint32_t*)tpa;
    basepage->p_hitpa = (uint32_t*)tpa + tpa_size;
    /* TEXT */
    basepage->p_tbase = (uint32_t*)tpa + sizeof(PROGRAM_HEADER);
    basepage->p_tlen = header.ph_tlen;
    /* DATA */
    basepage->p_dbase = basepage->p_tbase + basepage->p_tlen;
    basepage->p_dlen = header.ph_dlen;
    /* BSS */
    basepage->p_bbase = basepage->p_tbase + basepage->p_dlen;
    basepage->p_blen = header.ph_blen;
    /* Clear BSS. TODO should use bzero when it becomes available */
    for (int i = 0; i < basepage->p_blen; i++)
        basepage->p_bbase[i] = 0;

    basepage->p_parent = atari_tos_running_prg; // This is set when (if!) the process actually starts in atari_prg_bootstrap
    basepage->p_resrvd0 = 0L;
    basepage->p_cmdlin[0] = '\0'; // We don't support command line (yet ?)
    basepage->p_env = (char*)stacktop;
    writer = basepage->p_env;
    /* Copy Environment */
    for (int i = 0 ; i < env_size ; i++)
        *writer++ = *env++;
    
    // TODO ? the following is done by emutos
    // p->p_hitpa  = (uint32_t*)p  +  max;      /*  M01.01.06   */
    // p->p_xdta = (DTA *) p->p_cmdlin;       /* default p_xdta is p_cmdlin */

    // Read file into TEXT + DATA
    writer = tpa;
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
        relocate(basepage, channel_handle);

    result = 0; /* OK */
    atari_tos_running_prg = basepage;
    start = (long*)atari_prg_bootstrap;

    return 0; /* OK */

exit_upon_error:
    if (basepage)
       mem_free(PID, (uint32_t)basepage); /* TODO */
    return result;

read_error:
    result = DEV_CANNOT_READ;
    goto exit_upon_error;
}


/* Called by the Pterm0 / Pterm trap handler when the process has completed. */
void atari_prg_terminated(void)
{
    BASEPAGE *parent = atari_tos_running_prg->p_parent; 
    mem_free(PID, (uint32_t)atari_tos_running_prg);
    atari_tos_running_prg = parent;
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
    uint32_t *reloc_target;
    uint8_t   reloc_offset;

    /* Read 32-bit initial offset */
    if (chan_read(channel_handle, (uint8_t*)&reloc_target, sizeof(reloc_target)) != sizeof(reloc_target))
        return DEV_CANNOT_READ;

    /* Beware relocation table can be present but empty */
    if (reloc_target != 0L)
    {
        bool first = 1;
        /* Initial offset (index of first item to relocate */
        reloc_target += (uint32_t)basepage->p_tbase;
        
        /* Loop through the relocation entries */
        for (;;)
        {
            if (first)
                first = 0;
            else
            {
                do
                {
                    if (chan_read(channel_handle, &reloc_offset, sizeof(reloc_offset)) != sizeof(reloc_offset)) 
                        return DEV_CANNOT_READ;

                    if (reloc_offset == 1)
                        reloc_target += 254;

                } while (reloc_offset == 1);

                if (reloc_offset == 0)
                    break;
            }

            *reloc_target += *((uint32_t*)basepage->p_tbase);
        }
    }

    return 0; /* OK */
}
