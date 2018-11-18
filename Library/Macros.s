*-------------------------
*      Macro library
*-------------------------

INCD        MAC                 ;Two byte INC
            INC   ]1
            IF    MX>1
            BNE   NC
            INC   ]1+1
NC          FIN
            <<<

DECD        MAC                 ;Two byte DEC
            IF    MX>1
            LDA   ]1
            BNE   NC
            DEC   ]1+1
NC          FIN
            DEC   ]1
            <<<

MOV         MAC
            LDA   ]1
            STA   ]2
            <<<

MOVD        MAC
            MOV   ]1;]2
            IF    MX>1          ;If A is short
            IF    (=]1          ;Syntax MOVD (ADR1),Y;????
            INY
            IF    (=]2          ; MOVD (ADR1),Y;(ADR2),Y
            MOV   ]1;]2
            ELSE                ; MOVD (ADR1),Y;ADR2
            MOV   ]1;]2+1
            FIN
            ELSE
            IF    (=]2          ;Syntax MOVD ????;(ADR2),Y
            INY
            IF    #=]1          ; MOVD #ADR1;(ADR2),Y
            MOV   ]1/$100;]2
            ELSE                ; MOVD ADR1;(ADR2),Y
            MOV   ]1+1;]2
            FIN
            ELSE                ;Syntax MOVD ????;ADR2
            IF    #=]1          ; MOVD #ADR1;ADR2
            MOV   ]1/$100;]2+1
            ELSE                ; MOVD ADR1;ADR2
            MOV   ]1+1;]2+1
            FIN
            FIN
            FIN
            FIN
            <<<

LDHI        MAC                 ;For calls from other macs
            IF    #=]1
            LDA   ]1/$100
            ELSE
            LDA   ]1+1
            FIN
            <<<

ADD         MAC
            IF    #=]2
            IF    #=]1
            ERR   1             ;Error if ADD #lab1;#lab2..
            FIN
            FIN
            CLC
            LDA   ]1            ;Syntax ADD lab1;lab2;lab3
            ADC   ]2            ; or ADD #lab1;lab2;lab3 etc
            DO    ]0/3
            STA   ]3            ;If 3 parms
            ELSE                ;2 parm cases:
            IF    #=]2
            STA   ]1            ;Syntax ADD lab1;#lab2
            ELSE                ;Syntax ADD lab1;lab2
            STA   ]2            ; or ADD #lab1;lab2 -> lab2
            FIN
            FIN
            IF    MX>1          ;Following ignored if M long
            LDA   ]1+1
            IF    #=]2
            ADC   ]2/$100
            ELSE
            ADC   ]2+1
            FIN
            DO    ]0/3
            STA   ]3+1          ;If 3 parms
            ELSE                ;Two parm cases:
            IF    #=]2
            STA   ]1+1          ;Syntax ADD lab1;#lab2
            ELSE                ; -> lab1
            STA   ]2+1          ;Syntax ADD lab1;lab2 -> lab2
            FIN                 ; or ADD #lab1;lab2 -> lab2
            FIN
            FIN
            <<<

SUB         MAC
            IF    #=]2
            IF    #=]1
            ERR   1             ;Error if SUB #lab1;#lab2..
            FIN
            FIN
            SEC
            LDA   ]1            ;Syntax SUB lab1;lab2;lab3
            SBC   ]2            ; or SUB #lab1;lab2;lab3 etc
            DO    ]0/3
            STA   ]3            ;If 3 parms
            ELSE                ;Two parm cases:
            IF    #=]2
            STA   ]1            ;Syntax SUB lab1;#lab2
            ELSE                ;Syntax SUB lab1;lab2
            STA   ]2            ; or SUB #lab1;lab2 -> lab2
            FIN
            FIN                 ;Of 2 parm cases
            IF    MX>1          ;Rest ignored if M long
            LDHI  ]1
            IF    #=]2
            SBC   ]2/$100       ;Case #lab2
            ELSE
            SBC   ]2+1          ;Case lab2
            FIN
            DO    ]0/3
            STA   ]3+1          ;If 3 parms
            ELSE                ;Two parm cases:
            IF    #=]2
            STA   ]1+1          ;Syntax SUB lab1;#lab2
            ELSE                ; -> lab1
            STA   ]2+1          ;Syntax SUB lab1;lab2 -> lab2
            FIN                 ; or SUB #lab1;lab2 -> lab2
            FIN                 ;Of 2 parm cases
            FIN                 ;Of M short
            <<<

ADDX        MAC
            TXA
ADDA        MAC
            CLC
            ADC   ]1
            STA   ]2
            IF    MX>1
            LDHI  ]1
            ADC   #0
            STA   ]2+1
            FIN
            <<<

ADDY        MAC
            TYA
            ADDA  ]1;]2
            <<<

ADDNUM      MAC
            LDA   #]1
            CLC
            ADC   ]2
            STA   ]2
            IF    MX>1
            BCC   NC
            INC   ]2+1
NC          FIN
            <<<

SWAP        MAC
            LDA   ]1
            PHA
            LDA   ]2
            STA   ]1
            PLA
            STA   ]2
            <<<

COMPARE     MAC
            LDA   ]1
            CMP   ]2
            IF    MX>1
            LDHI  ]1            ;Syntax COMPARE lab1;lab2
            IF    #=]2          ; or COMPARE lab1;#lab2
            SBC   ]2/$100
            ELSE
            SBC   ]2+1
            FIN
            FIN
            <<<

POKE        MAC
            MOV   #]2;]1
            <<<

STADR       MAC
            POKE  ]2;]1
            IF    MX>1          ;If M is short, do high byte
            POKE  ]2+1;]1/$100
            FIN
            <<<

*=================================================
* Save and restore registers macros.  Recommended
* for use at the start and end of subroutines
* which might be called from unknown status and
* which must set up register lengths.
*-------------------------------------------------

SAVSTAT     MAC                 ;Save registers & status
            PHA
            PHY
            PHX
            PHP
            <<<

RESTORE     MAC                 ;Restore regs & status
            PLP                 ;This must come first
            PLX                 ; so register restores
            PLY                 ; have correct length.
            PLA
            <<<

*=================================================
* Conditional branches not available on the 65816:
*-------------------------------------------------

BLE         MAC                 ;Branch if less than or =
            BEQ   ]1
            BLT   ]1
            <<<

BGT         MAC                 ;Branch if greater than
            BEQ   OV
            BGE   ]1
OV          <<<

*=================================================
* Conditional long branches.  For use when out of
* range of a short branch and branching mechanism
* is wanted to be hidden on the listing.
*-------------------------------------------------

BRTL        MAC                 ;BRL if less than (long)
BCCL        MAC                 ;BRL if carry clear (long)
            BCS   OV
            BRL   ]1
OV          <<<

BGEL        MAC                 ;BRL if greater or equal (long)
BCSL        MAC                 ;BRL if carry set (long)
            BCC   OV
            BRL   ]1
OV          <<<

BPLL        MAC                 ;BRL if plus (long)
            BMI   OV
            BRL   ]1
OV          <<<

BMIL        MAC                 ;BRL if minus (long)
            BPL   OV
            BRL   ]1
OV          <<<

BEQL        MAC                 ;BRL if equal (long)
            BNE   OV
            BRL   ]1
OV          <<<

BNEL        MAC                 ;BRL if not equal (long)
            BEQ   OV
            BRL   ]1
OV          <<<

BVCL        MAC                 ;BRL if V clear (long)
            BVS   OV
            BRL   ]1
OV          <<<

BVSL        MAC                 ;BRL if V set (long)
            BVC   OV
            BRL   ]1
OV          <<<

BLEL        MAC                 ;BRL if less or equal (long)
            BEQ   BR
            BGE   OV
BR          BRL   ]1
OV          <<<

BLTL        MAC                 ;BRL if less than (long)
            BGE   OV
            BRL   ]1
OV          <<<

BGTL        MAC                 ;BRL if greater than (long)
            BEQ   OV
            BLT   OV
BR          BRL   ]1
OV          <<<

*=================================================
*  The I/O macros below expect the COUT routine
*  to be included or linked in as an external.
*  PRINT expects the same of the SENDMSG routine.
*
*  (Just declare COUT and SENDMSG as externals
*  and include the linker command LIB 5 in the
*  linker command file.  SENDMSG also requires an
*  OUTPUT routine in your source declared as an
*  entry.  It can be as simple as:
*
*   OUTPUT  JMP  COUT
*
*  SENDMSG and COUT are in the LIB directory and
*  their source files are in the SOURCE directory.
*-------------------------------------------------

PRINT       MAC                 ;Syntax PRINT "message"
            JSR   SENDMSG       ; or PRINT 8D8D   or
            ASC   ]1            ; PRINT 8D8D"text"8D"more"
            BRK                 ; etc.
            <<<

OUT         MAC
            LDA   ]1            ;Syntax OUT #"A"
            JSR   COUT          ; or OUT ADDRESS
            <<<                 ; or OUT (PNT),Y  etc.

HOME        MAC                 ;Clear 80 col screen
            OUT   #"L"&$9F
            <<<

NORMAL      MAC                 ;Set normal text
            OUT   #"N"&$9F
            <<<

INVERSE     MAC                 ;Set inverse text
            OUT   #"O"&$9F
            <<<

MOUSEON     MAC                 ;Turn mousetext on
            OUT   #"["&$9F
            INVERSE
            <<<

MOUSEOFF    MAC                 ;Turn mousetext off
            OUT   #"X"&$9F
            NORMAL
            <<<

BELL        MAC                 ;Ring bell
            OUT   #"G"&$9F
            <<<

CLEOL       MAC                 ;Clear to end of line
            OUT   #$9D
            <<<

CLEOP       MAC                 ;Clear to end of screen
            OUT   #$8B
            <<<

OUTCR       MAC                 ;Do carriage return
            OUT   #$8D
            <<<

GOTOXY      MAC                 ;Pascal output only!
            OUT   #$1E
            LDA   ]1            ;X coor (imm or location)
            CLC
            ADC   #' '
            JSR   COUT
            LDA   ]2            ;Y coor (")
            CLC
            ADC   #' '
            JSR   COUT
            <<<

CURSXY      MAC                 ;GOTOXY for BASIC output
            OUT   #"Y"&$9F      ;Home cursor
            LDX   ]2            ;Syntax (both versions):
            BEQ   H             ; CURSXY  #55;#10    or
CR          OUTCR               ; CURSXY  XLOC,YLOC  etc.
            DEX
            BNE   CR
H           IF    MX<2          ;If M is long then
            SEP   %00100000     ; must shorten it
            LDA   ]1
            STAL  $24
            REP   %00100000     ; and lengthen on exit.
            ELSE                ;If M is short then
            LDA   ]1            ; leave as is
            STAL  $24
            FIN
            <<<

**************************************************
*  APW-Equivalent Macros    1/20/89              *
*  For use with APW or ORCA/M source listings.   *
*                                                *
**************************************************

*================================================

DP          MAC
            ADRL  ]1
            <<<

ASL4        MAC
            LDA   ]1+2
            DO    ]0/2          ;If 2 parms then set
            LDX   ]2            ;  X to 2nd parm.
            FIN
]A          ASL                 ;Otherwise use X to
            ASL   ]1            ;  decide how many
            ADC   #0            ;  ASL's to do.
            DEX
            BNE   ]A
            STA   ]1+2
            <<<

LSR4        MAC
            DO    ]0/2          ;If 2 parms, then COMPLEMENT
            LDA   ]2            ; of X must be 2nd parm.
            EOR   $FFFF
            CLC
            ADC   #1
            TAX
            FIN
            LDA   ]1
]A          LSR
            LSR   ]1+2
            BCC   ]B
            ORA   #$8000
]B          INX
            BNE   ]A
            STA   ]1
            <<<

DEC4        MAC
            IF    MX/2-1        ; IF M IS SHORT
            ERR   1             ; ERR IF NOT 16-BIT MODE
            ELSE
            DEC   ]1
            BPL   ]A
            DEC   ]1+2
]A          FIN
            <<<

INC4        MAC
            IF    MX/2-1        ; IF M IS SHORT
            ERR   1             ; ERR IF NOT 16-BIT MODE
            ELSE
            INC   ]1
            BNE   ]A
            INC   ]1+2
]A          FIN
            <<<

ADD4        MAC
            CLC                 ;If 3 parms then use
            DO    ]0/3          ;  raw 4 byte addresses.

            IF    #=]1
            LDA   #<]1
            ELSE                ;Make sure we use the
            LDA   ]1            ;  right LDA for data
            FIN                 ;  or addresses.

            IF    #=]2
            ADC   #<]2
            ELSE                ;Make sure we use the
            ADC   ]2            ;  right ADC for data
            FIN                 ;  or addresses.

            STA   ]3

            IF    #=]1
            LDA   #^]1
            ELSE                ;Make sure we use the
            LDA   ]1+2          ;  right LDA for data
            FIN                 ;  or addresses.

            IF    #=]2
            ADC   #^]2
            ELSE                ;Make sure we use the
            ADC   ]2+2          ;  right ADC for data
            FIN                 ;  or addresses.

            STA   ]3+2

            ELSE                ;If 2 parms then use
;  2 byte Accumulator.


            IF    #=]1
            ADC   #<]1
            ELSE                ;Make sure we use the
            ADC   ]1            ;  right ADC for data
            FIN                 ;  or addresses.

            STA   ]2


            IF    #=]1
            LDA   #^]1
            ELSE                ;Make sure we use the
            LDA   ]1+2          ;  right LDA for data
            FIN                 ;  or addresses.

            ADC   #0
            STA   ]2+2

            FIN
            <<<

SUB4        MAC
            SEC                 ;If 3 parms then use
            DO    ]0/3          ;  raw 4 byte addresses.


            IF    #=]1
            LDA   #<]1
            ELSE                ;Make sure we use the
            LDA   ]1            ;  right LDA for data
            FIN                 ;  or addresses.

            IF    #=]2
            SBC   #<]2
            ELSE                ;Make sure we use the
            SBC   ]2            ;  right SBC for data
            FIN                 ;  or addresses.

            STA   ]3


            IF    #=]1
            LDA   #^]1
            ELSE                ;Make sure we use the
            LDA   ]1+2          ;  right LDA for data
            FIN                 ;  or addresses.

            IF    #=]2
            SBC   #^]2
            ELSE                ;Make sure we use the
            SBC   ]2+2          ;  right SBC for data
            FIN                 ;  or addresses.

            STA   ]3+2

            ELSE

            IF    #=]1
            SBC   #<]1
            ELSE                ;Make sure we use the
            SBC   ]1            ;  right SBC for data
            FIN                 ;  or addresses.

            STA   ]2

            IF    #=]1
            LDA   #^]1
            ELSE                ;Make sure we use the
            LDA   ]1+2          ;  right LDA for data
            FIN                 ;  or addresses.

            SBC   #0
            STA   ]2+2

            FIN
            <<<

