* Font Manager macros
*   by Dave Klimas
;
; Copyright Apple Computer, Inc. 1986, 1987
; and Roger Wagner Publishing, Inc. 1988
; All Rights Reserved
;
_FMBootInit        MAC
                   Tool    $11B
                   <<<
~FMStartUp         MAC
                   PxW     ]1;]2
_FMStartUp         MAC
                   Tool    $21B
                   <<<
_FMShutDown        MAC
                   Tool    $31B
                   <<<
~FMVersion         MAC
                   PHA
_FMVersion         MAC
                   Tool    $41B
                   <<<
_FMReset           MAC
                   Tool    $51B
                   <<<
~FMStatus          MAC
                   PHA
_FMStatus          MAC
                   Tool    $61B
                   <<<
~CountFamilies     MAC
                   P1SW    ]1
_CountFamilies     MAC
                   Tool    $91B
                   <<<
~FindFamily        MAC
                   P1SW    ]1
                   PHWL    ]2;]3
_FindFamily        MAC
                   Tool    $A1B
                   <<<
~GetFamInfo        MAC
                   PHA
                   PHWL    ]1;]2
_GetFamInfo        MAC
                   Tool    $B1B
                   <<<
~GetFamNum         MAC
                   P1SL    ]1
_GetFamNum         MAC
                   Tool    $C1B
                   <<<
~AddFamily         MAC
                   PHWL    ]1;]2
_AddFamily         MAC
                   Tool    $D1B
                   <<<
~InstallFont       MAC
                   PHLW    ]1;]2
_InstallFont       MAC
                   Tool    $E1B
                   <<<
~SetPurgeStat      MAC
                   PHLW    ]1;]2
_SetPurgeStat      MAC
                   Tool    $F1B
                   <<<
~CountFonts        MAC
                   P1SL    ]1
                   PHW     ]2
_CountFonts        MAC
                   Tool    $101B
                   <<<
~FindFontStats     MAC
                   PHLW    ]1;]2
                   PHWL    ]3;]4
_FindFontStats     MAC
                   Tool    $111B
                   <<<
~LoadFont          MAC
                   PHLW    ]1;]2
                   PHWL    ]3;]4
_LoadFont          MAC
                   Tool    $121B
                   <<<
_LoadSysFont       MAC
                   Tool    $131B
                   <<<
~AddFontVar        MAC
                   PHLW    ]1;]2
_AddFontVar        MAC
                   Tool    $141B
                   <<<
~FixFontMenu       MAC
                   PxW     ]1;]2;]3
_FixFontMenu       MAC
                   Tool    $151B
                   <<<
~ChooseFont        MAC
                   P2SL    ]1
                   PHW     ]2
_ChooseFont        MAC
                   Tool    $161B
                   <<<
~ItemID2FamNum     MAC
                   P1SW    ]1
_ItemID2FamNum     MAC
                   Tool    $171B
                   <<<
~FMSetSysFont      MAC
                   PHL     ]1
_FMSetSysFont      MAC
                   Tool    $181B
                   <<<
~FMGetSysFID       MAC
                   PHS     2
_FMGetSysFID       MAC
                   Tool    $191B
                   <<<
~FMGetCurFID       MAC
                   PHS     2
_FMGetCurFID       MAC
                   Tool    $1A1B
                   <<<
~FamNum2ItemID     MAC
                   P1SW    ]1
_FamNum2ItemID     MAC
                   Tool    $1B1B
                   <<<
~InstallWithStats  MAC
                   PHLW    ]1;]2
                   PHL     ]3
_InstallWithStats  MAC
                   Tool    $1C1B
                   <<<
~qFMStartUp        MAC
                   PHW     ]1
                   NextDP  ]2;$100
                   Tool    $21B
                   <<<

