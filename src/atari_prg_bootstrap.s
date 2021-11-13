#define TEST 1
#if defined(TEST)
    | Imported
    .globl   _atari_prg_terminated
    .globl   _atari_tos_running_prg
    
    | Exported
    .globl    _atari_prg_bootstrap
    .globl _atari_trap1handler
 #define basepage 0
 #define stack 4
.equ p_reserved,40
.equ p_tbase,8
#else
/*
    XDEF    _atari_prg_bootstrap
    XREF    _atari_prg_terminated
    XREF    _atari_tos_running_prg
    CARGS   basepage.l,stack.l
*/
#endif

_atari_prg_bootstrap:
    | We're starting an Atari program. This is called by the kernel itself
    movea.l  _atari_tos_running_prg,a0      | Here the BASEPAGE of the program we have to run 
    movea.l p_reserved(a0),sp               | Setup stack
    move.l  p_tbase(a0),-(sp)               | Beginning of user program
    | Clear registers
    lea     .nulls,a6
    movem.l (a6),d0-a5
    suba.l  a6,a6
    rts                                     | Go! (TEXT segment address)
.nulls: .dc.l 0,0,0,0,0,0,0,0,0,0,0,0,0,0    | 14 nulls are we clear 15 regs

_atari_trap1handler:
    move.l  usp,a0
    move.w  (a0),d0 | Function number
    bne.s   .Pterm
.Pterm0:
    clr.w   -(sp)
    jsr     _atari_prg_terminated   | We should never return from here !
.Pterm:
    cmp.w   #76,d0
    bne.s   .not_implemented
    move.l  usp,a0
    move.w  (a0),-(sp)
    jsr     _atari_prg_terminated   | We should never return from here !
.not_implemented:
    rte
