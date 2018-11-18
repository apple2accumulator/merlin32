*===============================
*  ProDOS-8 macro and equates
*
*  Use with syntax:
*
*       command PARMADR
*
* where "command" is one of the
* following labels and "PARMADR"
* is the label of the parameters
* table to use.
*
*-------------------------------

;
; Copyright Apple Computer, Inc. 1986, 1987
; All Rights Reserved
;
* MLI call codes:

ALLOC_INTERRUPT    MAC
                   DOS8  $40;]1
                   <<<

DEALLOC_INTERRUPT  MAC
                   DOS8  $41;]1
                   <<<

APPLE_TALK         MAC
                   DOS8  $42;]1
                   <<<

SPECIAL_OPEN_FORK  MAC
                   DOS8  $43;]1
                   <<<

BYTE_RANGE_LOCK    MAC
                   DOS8  $44;]1
                   <<<

QUIT               MAC
                   DOS8  $65;]1
                   <<<

READ_BLOCK         MAC
                   DOS8  $80;]1
                   <<<

WRITE_BLOCK        MAC
                   DOS8  $81;]1
                   <<<

GET_TIME           MAC
                   DOS8  $82;]1
                   <<<

CREATE             MAC
                   DOS8  $C0;]1
                   <<<

DESTROY            MAC
                   DOS8  $C1;]1
                   <<<

RENAME             MAC
                   DOS8  $C2;]1
                   <<<

SET_FILE_INFO      MAC
                   DOS8  $C3;]1
                   <<<

GET_FILE_INFO      MAC
                   DOS8  $C4;]1
                   <<<

ON_LINE            MAC
                   DOS8  $C5;]1
                   <<<

SET_PREFIX         MAC
                   DOS8  $C6;]1
                   <<<

GET_PREFIX         MAC
                   DOS8  $C7;]1
                   <<<

OPEN               MAC
                   DOS8  $C8;]1
                   <<<

NEWLINE            MAC
                   DOS8  $C9;]1
                   <<<

READ               MAC
                   DOS8  $CA;]1
                   <<<

WRITE              MAC
                   DOS8  $CB;]1
                   <<<

CLOSE              MAC
                   DOS8  $CC;]1
                   <<<

FLUSH              MAC
                   DOS8  $CD;]1
                   <<<

SET_MARK           MAC
                   DOS8  $CE;]1
                   <<<

GET_MARK           MAC
                   DOS8  $CF;]1
                   <<<

SET_EOF            MAC
                   DOS8  $D0;]1
                   <<<

GET_EOF            MAC
                   DOS8  $D1;]1
                   <<<

SET_BUF            MAC
                   DOS8  $D2;]1
                   <<<

GET_BUF            MAC
                   DOS8  $D3;]1
                   <<<

DOS8               MAC
                   JSR   $BF00
                   DFB   ]1
                   DA    ]2
                   <<<

