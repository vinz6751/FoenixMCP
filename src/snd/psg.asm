	idnt	"psg.c"
	opt o+,ol+,op+,oc+,ot+,oj+,ob+,om+
	section	"CODE",code
	public	_psg_mute_all
	cnop	0,4
_psg_mute_all
	movem.l	l3,-(a7)
	move.b	#159,11665664
	move.b	#191,11665664
	move.b	#223,11665664
	move.b	#255,11665664
l1
l3	reg
l5	equ	0
	rts
; stacksize=0
