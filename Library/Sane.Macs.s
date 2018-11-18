* Sane Toolset macros.
*   by Dave Klimas
;
; Copyright Apple Computer, Inc. 1986, 1987
; and Roger Wagner Publishing, Inc. 1988
; All Rights Reserved
;
_SANEBootInit   MAC
                Tool    $10A
                <<<
~SANEStartUp    MAC
                PHW     ]1
_SANEStartUp    MAC
                Tool    $20A
                <<<
_SANEShutDown   MAC
                Tool    $30A
                <<<
~SANEVersion    MAC
                PHA
_SANEVersion    MAC
                Tool    $40A
                <<<
_SANEReset      MAC
                Tool    $50A
                <<<
~SANEStatus     MAC
                PHA
_SANEStatus     MAC
                Tool    $60A
                <<<
_SANEFP816      MAC
                Tool    $90A
                <<<
_SANEDecStr816  MAC
                Tool    $A0A
                <<<
_SANEElems816   MAC
                Tool    $B0A
                <<<
~qSANEStartUp   MAC
                NextDP  ]1;$100
                Tool    $20A
                <<<
* Auxiliary macros

FOPRF           MAC                 ;call FP
                PEA     ]1
                _SANEFP816
                <<<
FOPRD           MAC                 ;call DecStr
                PEA     ]1
                _SANEDecStr816
                <<<
FOPRE           MAC                 ;call Elems
                PEA     ]1
                _SANEElems816
                <<<
* Addition

FADDX           MAC
                FOPRF   0
                <<<
FADDD           MAC
                FOPRF   $100
                <<<
FADDS           MAC
                FOPRF   $200
                <<<
FADDC           MAC
                FOPRF   $500
                <<<
FADDI           MAC
                FOPRF   $400
                <<<
FADDL           MAC
                FOPRF   $300
                <<<
* Subtraction

FSUBX           MAC
                FOPRF   $002
                <<<
FSUBD           MAC
                FOPRF   $102
                <<<
FSUBS           MAC
                FOPRF   $202
                <<<
FSUBC           MAC
                FOPRF   $502
                <<<
FSUBI           MAC
                FOPRF   $402
                <<<
FSUBL           MAC
                FOPRF   $302
                <<<
* Multiplication

FMULX           MAC
                FOPRF   $004
                <<<
FMULD           MAC
                FOPRF   $104
                <<<
FMULS           MAC
                FOPRF   $204
                <<<
FMULC           MAC
                FOPRF   $504
                <<<
FMULI           MAC
                FOPRF   $404
                <<<
FMULL           MAC
                FOPRF   $304
                <<<
* Division

FDIVX           MAC
                FOPRF   $006
                <<<
FDIVD           MAC
                FOPRF   $106
                <<<
FDIVS           MAC
                FOPRF   $206
                <<<
FDIVC           MAC
                FOPRF   $506
                <<<
FDIVI           MAC
                FOPRF   $406
                <<<
FDIVL           MAC
                FOPRF   $306
                <<<
* Square root

FSQRTX          MAC
                FOPRF   $12
                <<<

* Round to integer, according to the current rounding mode

FRINTX          MAC
                FOPRF   $14
                <<<

* Truncate to integer, using round toward zero.

FTINTX          MAC
                FOPRF   $16
                <<<
* Remainder

FREMX           MAC
                FOPRF   $00C
                <<<
FREMD           MAC
                FOPRF   $10C
                <<<
FREMS           MAC
                FOPRF   $20C
                <<<
FREMC           MAC
                FOPRF   $50C
                <<<
FREMI           MAC
                FOPRF   $40C
                <<<
FREML           MAC
                FOPRF   $30C
                <<<
* Logb

FLOGBX          MAC
                FOPRF   $1A
                <<<
* Scalb

FSCALBX         MAC
                FOPRF   $18
                <<<
* Copy-sign

FCPYSGNX        MAC
                FOPRF   $011
                <<<
FCPYSGND        MAC
                FOPRF   $111
                <<<
FCPYSGNS        MAC
                FOPRF   $211
                <<<
FCPYSGNC        MAC
                FOPRF   $511
                <<<
FCPYSGNI        MAC
                FOPRF   $411
                <<<
FCPYSGNL        MAC
                FOPRF   $311
                <<<
* Negate

FNEGX           MAC
                FOPRF   $0D
                <<<

* Absolute value

FABSX           MAC
                FOPRF   $0F
                <<<

* Next-after.  NOTE:  both operands are of the
* the same format, as specified by the usual suffix.

FNEXTS          MAC
                FOPRF   $21E
                <<<
FNEXTD          MAC
                FOPRF   $11E
                <<<
FNEXTX          MAC
                FOPRF   $01E
                <<<

* Conversion to extended

FX2X            MAC
                FOPRF   $00E
                <<<
FD2X            MAC
                FOPRF   $10E
                <<<
FS2X            MAC
                FOPRF   $20E
                <<<

* 16-bit integer, by address

FI2X            MAC
                FOPRF   $40E
                <<<

* 32-bit integer, by address

FL2X            MAC
                FOPRF   $30E
                <<<
FC2X            MAC
                FOPRF   $50E
                <<<

* Conversion from extended

FX2D            MAC
                FOPRF   $110
                <<<
FX2S            MAC
                FOPRF   $210
                <<<
FX2I            MAC
                FOPRF   $410
                <<<
FX2L            MAC
                FOPRF   $310
                <<<
FX2C            MAC
                FOPRF   $510
                <<<

* Binary to decimal conversion

FX2DEC          MAC
                FOPRF   $00B
                <<<
FD2DEC          MAC
                FOPRF   $10B
                <<<
FS2DEC          MAC
                FOPRF   $20B
                <<<
FC2DEC          MAC
                FOPRF   $50B
                <<<
FI2DEC          MAC
                FOPRF   $40B
                <<<
FL2DEC          MAC
                FOPRF   $30B
                <<<

* Decimal to binary conversion

FDEC2X          MAC
                FOPRF   $009
                <<<
FDEC2D          MAC
                FOPRF   $109
                <<<
FDEC2S          MAC
                FOPRF   $209
                <<<
FDEC2C          MAC
                FOPRF   $509
                <<<
FDEC2I          MAC
                FOPRF   $409
                <<<
FDEC2L          MAC
                FOPRF   $309
                <<<

* Compare, not signaling invalid on unordered

FCMPX           MAC
                FOPRF   $008
                <<<
FCMPD           MAC
                FOPRF   $108
                <<<
FCMPS           MAC
                FOPRF   $208
                <<<
FCMPC           MAC
                FOPRF   $508
                <<<
FCMPI           MAC
                FOPRF   $408
                <<<
FCMPL           MAC
                FOPRF   $308
                <<<

* Compare, signaling invalid on unordered

FCPXX           MAC
                FOPRF   $00A
                <<<
FCPXD           MAC
                FOPRF   $10A
                <<<
FCPXS           MAC
                FOPRF   $20A
                <<<
FCPXC           MAC
                FOPRF   $50A
                <<<
FCPXI           MAC
                FOPRF   $40A
                <<<
FCPXL           MAC
                FOPRF   $30A
                <<<

* The following macros define a set of so-called floating
* branches.  They presume that the appropriate compare
* operation, macro FCMPz or FCPXz, precedes.

FBEQ            MAC
                BEQ     ]1
                <<<
FBLT            MAC                 ;less
                BMI     ]1
                <<<
FBLE            MAC                 ;less or equal
                BMI     ]1
                BEQ     ]1
                <<<
FBGT            MAC                 ;greater
                BVS     ]1
                <<<
FBGE            MAC                 ;greater or equal
                BVS     ]1
                BEQ     ]1
                <<<
FBULT           MAC                 ;less or unordered
                BMI     ]1
                BVS     *+4
                BNE     ]1
                <<<
FBULE           MAC                 ;unordered, less, or equal
                BMI     ]1
                BEQ     ]1
                BVC     ]1
                <<<
FBUGT           MAC                 ;unordered or greater
                BVS     ]1
                BMI     *+4
                BNE     ]1
                <<<
FBUGE           MAC                 ;unordered, greater, or equal
                BVS     ]1
                BEQ     ]1
                BPL     ]1
                <<<
FBU             MAC                 ;unordered
                BVS     *+6
                BMI     *+4
                BNE     ]1
                <<<
FBO             MAC                 ;ordered
                BMI     ]1
                BVS     ]1
                BEQ     ]1
                <<<
FBNE            MAC                 ;not equal
                BMI     ]1
                BVS     ]1
                BNE     ]1
                <<<
FBUE            MAC                 ;unordered, equal
                BEQ     ]1
                BMI     *+4
                BVC     ]1
                <<<
FBLG            MAC                 ;less or greater
                BMI     ]1
                BVS     ]1
                <<<

FCLASSS         MAC
                FOPRF   $21C
                <<<
FCLASSD         MAC
                FOPRF   $11C
                <<<
FCLASSX         MAC
                FOPRF   $01C
                <<<
FCLASSC         MAC
                FOPRF   $51C
                <<<
FCLASSI         MAC
                FOPRF   $41C
                <<<
FCLASSL         MAC
                FOPRF   $31C
                <<<

* The following macros provide branches based on the
* the result of a FCLASSz macro.

FBSNAN          MAC                 ;signaling NaN
                TXA
                ASL
                CMP     #2*$FC
                BEQ     ]1
                <<<
FBQNAN          MAC                 ;quiet NaN
                TXA
                ASL
                CMP     #2*$FD
                BEQ     ]1
                <<<
FBINF           MAC                 ;infinite
                TXA
                ASL
                CMP     #2*$FE
                BEQ     ]1
                <<<
FBZERO          MAC                 ;zero
                TXA
                ASL
                CMP     #2*$FF
                BEQ     ]1
                <<<
FBNORM          MAC                 ;normal
                TXA
                ASL
                BEQ     ]1
                <<<
FBDENORM        MAC                 ;denormal
                TXA
                ASL
                CMP     #2*1
                BEQ     ]1
                <<<
FBNZENUM        MAC                 ;non-zero num (norm or denorm)
                TXA
                XBA
                ASL
                BCC     ]1
                <<<
FBNUM           MAC                 ;number (zero, norm or denorm)
                TXA
                INC     A
                XBA
                ASL
                BCC     ]1
                <<<
FBMINUS         MAC                 ;minus sign
                BMI     ]1
                <<<
FBPLUS          MAC                 ;plus sign
                BPL     ]1
                <<<

* Get and set environment

FGETENV         MAC
                FOPRF   $03
                <<<
FSETENV         MAC
                FOPRF   $01
                <<<

* Test and set exception

FTESTXCP        MAC
                FOPRF   $1B
                <<<
FSETXCP         MAC
                FOPRF   $15
                <<<

* Procedure entry and exit

FPROCENTRY      MAC
                FOPRF   $17
                <<<
FPROCEXIT       MAC
                FOPRF   $19
                <<<

* Get and set halt vector

FGETHV          MAC
                FOPRF   $07
                <<<
FSETHV          MAC
                FOPRF   $05
                <<<

* Elementary function macros

FLNX            MAC                 ;natural (base-e) log
                FOPRE   $00
                <<<
FLOG2X          MAC                 ;base-2 log
                FOPRE   $02
                <<<
FLN1X           MAC                 ;ln (1 + x)
                FOPRE   $04
                <<<
FLOG21X         MAC                 ;log2 (1 +x)
                FOPRE   $06
                <<<
FEXPX           MAC                 ;base-e exponential
                FOPRE   $08
                <<<
FEXP2X          MAC                 ;base-2 exponential
                FOPRE   $0A
                <<<
FEXP1X          MAC                 ;exp (x) - 1
                FOPRE   $0C
                <<<
FEXP21X         MAC                 ;exp2 (x) - 1
                FOPRE   $0E
                <<<
FXPWRI          MAC                 ;integer exponential
                FOPRE   $10
                <<<
FXPWRY          MAC                 ;general exponential
                FOPRE   $12
                <<<
FCOMPOUND       MAC                 ;compound
                FOPRE   $14
                <<<
FANNUITY        MAC                 ;annuity
                FOPRE   $16
                <<<
FATANX          MAC                 ;arctangent
                FOPRE   $18
                <<<
FSINX           MAC                 ;sine
                FOPRE   $1A
                <<<
FCOSX           MAC                 ;cosine
                FOPRE   $1C
                <<<
FTANX           MAC                 ;tangent
                FOPRE   $1E
                <<<
FRANDX          MAC                 ;random number generator
                FOPRE   $20
                <<<

* Scanner and formatter function macros

FPSTR2DEC       MAC                 ;pascal string to decimal record
                FOPRD   0
                <<<
FDEC2STR        MAC                 ;decimal record to pascal string
                FOPRD   1
                <<<
FCSTR2DEC       MAC                 ;C string to decimal record
                FOPRD   2
                <<<

