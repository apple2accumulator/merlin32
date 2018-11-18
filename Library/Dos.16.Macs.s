*===============================
*  ProDOS-16 macro and equates
*
*  Use with syntax:
*
*       _CMDNAME PARMADR
*
* where "CMDNAME" is one of the
* following labels and "PARMADR"
* is the label of the parameters
* table to use.
*
*-------------------------------

;
; Copyright Apple Computer, Inc. 1986, 1987
; All Rights Reserved
;
open                =      $10
read                =      $12
write               =      $13
close               =      $14

_CREATE             MAC
                    DOS16  $01;]1
                    <<<

_DESTROY            MAC
                    DOS16  $02;]1
                    <<<

_CHANGE_PATH        MAC
                    DOS16  $04;]1
                    <<<

_SET_FILE_INFO      MAC
                    DOS16  $05;]1
                    <<<

_GET_FILE_INFO      MAC
                    DOS16  $06;]1
                    <<<

_VOLUME             MAC
                    DOS16  $08;]1
                    <<<

_SET_PREFIX         MAC
                    DOS16  $09;]1
                    <<<

_SETPREFIX          MAC                ; alternate name
                    DOS16  $09;]1
                    <<<

_GET_PREFIX         MAC
                    DOS16  $0A;]1
                    <<<

_GETPREFIX          MAC                ; alternate name
                    DOS16  $0A;]1
                    <<<

_CLEAR_BACKUP_BIT   MAC
                    DOS16  $0B;]1
                    <<<

_OPEN               MAC
                    DOS16  $10;]1
                    <<<

_NEWLINE            MAC
                    DOS16  $11;]1
                    <<<

_READ               MAC
                    DOS16  $12;]1
                    <<<

_WRITE              MAC
                    DOS16  $13;]1
                    <<<

_CLOSE              MAC
                    DOS16  $14;]1
                    <<<

_FLUSH              MAC
                    DOS16  $15;]1
                    <<<

_SET_MARK           MAC
                    DOS16  $16;]1
                    <<<

_GET_MARK           MAC
                    DOS16  $17;]1
                    <<<

_SET_EOF            MAC
                    DOS16  $18;]1
                    <<<

_GET_EOF            MAC
                    DOS16  $19;]1
                    <<<

_SET_LEVEL          MAC
                    DOS16  $1A;]1
                    <<<

_GET_LEVEL          MAC
                    DOS16  $1B;]1
                    <<<

_GET_DIR_ENTRY      MAC
                    DOS16  $1C;]1
                    <<<

_GET_DEV_NUM        MAC
                    DOS16  $20;]1
                    <<<

_GET_LAST_DEV       MAC
                    DOS16  $21;]1
                    <<<

_READ_BLOCK         MAC
                    DOS16  $22;]1
                    <<<

_WRITE_BLOCK        MAC
                    DOS16  $23;]1
                    <<<

_FORMAT             MAC
                    DOS16  $24;]1
                    <<<

_ERASE_DISK         MAC
                    DOS16  $25;]1
                    <<<

_GET_NAME           MAC
                    DOS16  $27;]1
                    <<<

_GET_BOOT_VOL       MAC
                    DOS16  $28;]1
                    <<<

_QUIT               MAC
                    DOS16  $29;]1
                    <<<

_GET_VERSION        MAC
                    DOS16  $2A;]1
                    <<<

_D_INFO             MAC
                    DOS16  $2C;]1
                    <<<

_ALLOC_INTERRUPT    MAC
                    DOS16  $31;]1
                    <<<

_DEALLOC_INTERRUPT  MAC
                    DOS16  $32;]1
                    <<<

DOS16               MAC
                    JSL    $E100A8
                    DA     ]1          ; Change to ]1.$2000 for Class 1 P16 calls.
                    ADRL   ]2
                    <<<

