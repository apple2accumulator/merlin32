* ACE tool macros
*   by Dave Klimas
;
; Copyright Apple Computer, Inc. 1986, 1987
; and Roger Wagner Publishing, Inc. 1988
; All Rights Reserved
;

_ACEBootInit     MAC
                 Tool  $11D
                 <<<
~ACEStartUp      MAC
                 PHW   ]1
_ACEStartUp      MAC
                 Tool  $21D
                 <<<
_ACEShutDown     MAC
                 Tool  $31D
                 <<<
~ACEVersion      MAC
                 PHA
_ACEVersion      MAC
                 Tool  $41D
                 <<<
_ACEReset        MAC
                 Tool  $51D
                 <<<
~ACEStatus       MAC
                 PHA
_ACEStatus       MAC
                 Tool  $61D
                 <<<
~ACEInfo         MAC
                 P2SW  ]1
_ACEInfo         MAC
                 Tool  $71D
                 <<<
~ACECompress     MAC
                 PxL   ]1;]2;]3;]4
                 PxW   ]5;]6
_ACECompress     MAC
                 Tool  $91D
                 <<<
~ACEExpand       MAC
                 PxL   ]1;]2;]3;]4
                 PxW   ]5;]6
_ACEExpand       MAC
                 Tool  $A1D
                 <<<
_ACECompBegin    MAC
                 Tool  $B1D
                 <<<
_ACEExpBegin     MAC
                 Tool  $C1D
                 <<<
_GetACEExpState  MAC
                 Tool  $D1D
                 <<<
_SetACEExpState  MAC
                 Tool  $E1D
                 <<<

