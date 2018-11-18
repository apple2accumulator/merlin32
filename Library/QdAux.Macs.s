* QuickDraw Aux macros
*   by Dave Klimas
;
; Copyright Apple Computer, Inc. 1986, 1987
; and Roger Wagner Publishing, Inc. 1988
; All Rights Reserved
;
_QDAuxBootInit      MAC
                    Tool  $112
                    <<<
_QDAuxStartUp       MAC
                    Tool  $212
                    <<<
_QDAuxShutDown      MAC
                    Tool  $312
                    <<<
~QDAuxVersion       MAC
                    PHA
_QDAuxVersion       MAC
                    Tool  $412
                    <<<
_QDAuxReset         MAC
                    Tool  $512
                    <<<
~QDAuxStatus        MAC
                    PHA
_QDAuxStatus        MAC
                    Tool  $612
                    <<<
~CopyPixels         MAC
                    PxL   ]1;]2;]3;]4
                    PHWL  ]5;]6
_CopyPixels         MAC
                    Tool  $912
                    <<<
_WaitCursor         MAC
                    Tool  $A12
                    <<<
~DrawIcon           MAC
                    PHLW  ]1;]2
                    PxW   ]3;]4
_DrawIcon           MAC
                    Tool  $B12
                    <<<
~SpecialRect        MAC
                    PHL   ]1
                    PxW   ]2;]3
_SpecialRect        MAC
                    Tool  $C12
                    <<<
~SeedFill           MAC
                    PxL   ]1;]2;]3;]4
                    PxW   ]5;]6;]7
                    PxL   ]8;]9
_SeedFill           MAC
                    Tool  $D12
                    <<<
~CalcMask           MAC
                    PxL   ]1;]2;]3;]4
                    PHW   ]5
                    PxL   ]6;]7
_CalcMask           MAC
                    Tool  $E12
                    <<<
_GetSysIcon         MAC
                    Tool  $F12
                    <<<
_PixelMap2Rgn       MAC
                    Tool  $1012
                    <<<
_IBeamCursor        MAC
                    Tool  $1312
                    <<<
_WhooshRect         MAC
                    Tool  $1412
                    <<<
_DrawStringWidth    MAC
                    Tool  $1512
                    <<<
_UseColorTable      MAC
                    Tool  $1612
                    <<<
_RestoreColorTable  MAC
                    Tool  $1712
                    <<<

