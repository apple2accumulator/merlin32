* Note synthesizer macros
*   by Dave Klimas
;
; Copyright Apple Computer, Inc. 1986, 1987
; and Roger Wagner Publishing, Inc. 1988
; All Rights Reserved
;
_NSBootInit          MAC
                     Tool  $119
                     <<<
~NSStartUp           MAC
                     PHWL  ]1;]2
_NSStartUp           MAC
                     Tool  $219
                     <<<
_NSShutDown          MAC
                     Tool  $319
                     <<<
~NSVersion           MAC
                     PHA
_NSVersion           MAC
                     Tool  $419
                     <<<
_NSReset             MAC
                     Tool  $519
                     <<<
~NSStatus            MAC
                     PHA
_NSStatus            MAC
                     Tool  $619
                     <<<
~AllocGen            MAC
                     P1SW  ]1
_AllocGen            MAC
                     Tool  $919
                     <<<
~DeallocGen          MAC
                     PHW   ]1
_DeallocGen          MAC
                     Tool  $A19
                     <<<
~NoteOn              MAC
                     PxW   ]1;]2;]3
                     PHL   ]4
_NoteOn              MAC
                     Tool  $B19
                     <<<
~NoteOff             MAC
                     PxW   ]1;]2
_NoteOff             MAC
                     Tool  $C19
                     <<<
_AllNotesOff         MAC
                     Tool  $D19
                     <<<
~NSSetUpdateRate     MAC
                     P1SW  ]1
_NSSetUpdateRate     MAC
                     Tool  $E19
                     <<<
~NSSetUserUpdateRtn  MAC
                     P2SL  ]1
_NSSetUserUpdateRtn  MAC
                     Tool  $F19
                     <<<

