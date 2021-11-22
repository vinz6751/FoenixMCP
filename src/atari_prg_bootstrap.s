    ; Imported
    XREF    _atari_prg_terminated
    XREF    _atari_tos_running_prg
    
    ; Exported
    XDEF    _atari_prg_bootstrap
    XDEF    _atari_trap1handler

    ; BASEPAGE members
p_reserved EQU 40
p_tbase    EQU 8


    ; This is what is called by the kernel's call_user(), it's really a stub that
    ; gets the program's BASEPAGE to run from _atari_tos_running_prg and starts it.
    ; TODO: support argc/argv  
_atari_prg_bootstrap:
    ; We're starting an Atari program. This is called by the kernel itself.
    movea.l  _atari_tos_running_prg,a0      ; Here the BASEPAGE of the program we have to run 
    movea.l p_reserved(a0),sp               ; Setup stack
    move.l  p_tbase(a0),-(sp)               ; Beginning of user program
    ; Clear registers
    lea     .nulls,a6
    movem.l (a6),d0-a5
    suba.l  a6,a6
    rts                                     ; Go! (TEXT segment address)
.nulls: dc.l 0,0,0,0,0,0,0,0,0,0,0,0,0,0    ; 14 nulls as we clearing 15 regs


    ; Atari TOS programs terminate by calling either Pterm, Pterm0 or
    ; Ptermres (to stay resident, that's not supported yet).
    ; We install a trap #1 handler to catch these and give control to a C
    ; function atari_prg_terminated() that will terminate the process in a way
    ; the FoenixMCP expects it.
_atari_trap1handler:
    move.l  usp,a0
    move.w  (a0),d0 ; Function number
    bne.s   .Pterm
.Pterm0:
    clr.w   -(sp)
    jsr     _atari_prg_terminated   ; We should never return from here !
.Pterm:
    cmp.w   #76,d0
    bne.s   .not_implemented
    move.l  usp,a0
    move.w  (a0),-(sp)
    jsr     _atari_prg_terminated   ; We should never return from here !
.not_implemented:
    rte
