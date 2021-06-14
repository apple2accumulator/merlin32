*
*  linkscript.s
*  Merlin32 Test
*
*  Created by Lane Roathe on 8/21/19.
*  Copyright © 2019 Ideas From the Deep. All rights reserved.


;uncomment this section to test as a link file

    typ $06     ; Binary file at fixed address

   dsk Merlin32Test
   org $800

   asm main.s
   sna main

; uncomment this section to test as an asm file

;testEQU	=	$123
;
;	org $800
;
;	put main.s
;
;	asc "THE END"
;
;    ds    \
;
;	sav main
