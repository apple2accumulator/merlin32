*
*  main.s
*  Merlin32 Test
*
*  Created by Lane Roathe on 8/26/19.
*  Copyright © 2019 Ideas From the Deep. All rights reserved.
*

]XCODESTART     ; Keep this at the start and put your code after this

*==========================================================
* monitor addresses


TEXT	=	$FB39		;Reset text window
TABV	=	$FB5B		;Complete vtab, using contents of 'A'
FIXVEC	=	$FB6F		;fix vector's powerup byte
DOLSR	=	$F87B		;do 4 lsr's
INSTDSP	=	$F8D0		;display instruction at (pc)
PRBLK	=	$F94A		;print X blanks
PCADJ	=	$F953		;point to next instruction
MONBELL	=	$FBE4		;random bell noise!
STORADV	=	$FBF0		;store char on screen, adv cursor
VTAB	=	$FC22		;vtab to '_CV'
CLEOP	=	$FC42		;Clear to end of page
HOME    =	$FC58		;Clear text window
CLEOL	=	$FC9C		;Clear to end of line
CLEOLZ	=	$FC9E		;Clear current line from htab 'Y'
WAIT	=	$FCA8		;delay routine
GETLN1	=	$FD6F		;input routine
CROUT	=	$FD8E		;Print a CR
PRBYTE	=	$FDDA		;Print 'A' as a hex number
PRHEX	=	$FDE3		;as above, but bits 0-3 only
COUT	=	$FDED		;Monitor char out
MOVE	=	$FE2C		;memory move routine
LIST2	=	$FE63		;disasm 'A' lines of code at (pc)
INVERSE	=	$FE80		;Print in inverse
NORMAL	=	$FE84		;Normal print
OUTPORT	=	$FE95		;Set CSW to 'a' (ie, PR#x)

* Jump Vectors
CONNECT	=	$3EA		;Connect DOS
DOSWARM	=	$3D0		;exit to DOS prompt
RSTVEC	=	$3F2		;reset vector
DRVTRK	=	$478		;slot location, RWFTS

TSTADDR	=	$1000		;absolute address for testing

*==========================================================
* QDecimal is a 32bit format:
*
* Mem: D0D1D2D3 -> value: D3D2D1.D0 -> 999,999.99 max value

		DUM	0
qdFrac	ds	1			;fractional byte
qdIntL	ds	1
qdIntM	ds	1
qdIntH	ds	1			;the three integer bytes
qdSize	=	*
		DEND

*==========================================================
* zero page (all zp var names are prefixed with _)
* UPPERCASE are monitor or OS locations, lowercase are application use

		DUM	0

_ptr	ds	2
_tmp	ds	2

_num1	ds	qdSize		;first and second operand values
_num2	ds	qdSize
_rslt	ds	qdSize		;resulting value

_menPtr	ds	2			;ptr to current menu's table
_curmen	ds	1			;current menu and item selected (item is 1..max-1)
_maxitm	ds	1			;max # of items in current menu
_curitm	ds	1
_prvitm	ds	1			;previous item selected (menus are 1 deep max, so this is always for the main menu)

		ORG	$20

_LFT	ds	1			;Window edge   0..39
_WDTH	ds	1			;  ''   widht  1..40/80
_TOP	ds  1			;  ''   top    0..23
_BTM	ds  1			;  ''   bottom 1..24
_CH		ds  1			;Cursor htab
_CV		ds  1			;  ''   vtab

		ORG	$32

_INVFLG	ds  1			;Monitor inverse control byte
_SPACE	ds  3
_CSW	ds  2

		DEND

*==========================================================
* Program Entry

		org $800

START
; PUT current issue here, so it's the first thing assembled. The rest below are unit tests to make sure future changes don't break existing code!

		adc    (_tmp,x)

;adc (ZP,x)
		adc (0,x)
        adc ($80,x)
        adc	(_tmp,x)
        adc	(_tmp+0,x)
        adc	(_tmp+$10,x)
        adc	(_tmp+qdFrac,x)
        adc	(_tmp+qdIntL,x)
        adc	(_tmp+qdFrac+qdIntL,x)

        adc 0
        adc $80
		adc _tmp
        adc #0
        adc #$1111
        adc $1111

        sta TSTADDR+qdFrac
        sta TSTADDR+_rslt+qdFrac
        sta TSTADDR+_rslt+qdFrac,x

        lda _num1+qdFrac
        adc _num2+qdFrac
        sbc _num2+qdFrac
        bit _num2+qdFrac
        sta _rslt+qdFrac    ;(FIXED): can't use sta _rslt+qdFrac
		stz _rslt+qdFrac

		lda _rslt+qdFrac,x
		adc _rslt+qdFrac,x
		sbc _rslt+qdFrac,x
		bit _rslt+qdFrac,x
		sta _rslt+qdFrac,x
		stz _rslt+qdFrac,x

        lda _rslt+qdFrac,y	;these assemble to abs accesses: lda $00C0,y
		adc _rslt+qdFrac,y
		sbc _rslt+qdFrac,y
		sta _rslt+qdFrac,y

		jmp DOSWARM

]XCODEEND       ; Keep this at the end and put your code above this
