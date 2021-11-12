#define TEST 1
#if !defined(TEST)
    .globl    _atari_prg_bootstrap
    .globl   _atari_prg_terminated
    .globl   _atari_tos_running_prg

 #define basepage 0
 #define stack 4
#else
/*    XDEF    _atari_prg_bootstrap
    XREF    _atari_prg_terminated
    XREF    _atari_tos_running_prg
    CARGS   basepage.l,stack.l
*/
#endif

_atari_prg_bootstrap: 
    | We're starting an Atari program. This is called by the kernel itself
    movea.l  _atari_tos_running_prg,a0      | Here is our BASEPAGE 
    move.l  4(a0),sp                    | Install or own stack
    move.l  a0,-(sp)                        | Put BASEPAGE on the stack as expected by the program 
    move.l  #trap1handler,0x80+4*4               | Install trap# handler if needed so we can handler Pterm calls
    jmp     8(a0)                           | Go! (TEXT segment address)


trap1handler:
    move.l  usp,a0
    tst.w   (a0)    | Pterm0 ?
    bne.s   .not_implemented    
    jsr     _atari_prg_terminated

.not_implemented:
    rte
