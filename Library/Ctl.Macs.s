* Control Manager macros
*   by Dave Klimas
;
; Copyright Apple Computer, Inc. 1986, 1987
; and Roger Wagner Publishing, Inc. 1988
; All Rights Reserved
;
_CtlBootInit         MAC
                     Tool    $110
                     <<<
~CtlStartUp          MAC
                     PxW     ]1;]2
_CtlStartUp          MAC
_InitCtrlMgr         MAC
                     Tool    $210
                     <<<
_CtlShutDown         MAC
_CtrlShutDown        MAC
                     Tool    $310
                     <<<
~CtlVersion          MAC
                     PHA
_CtlVersion          MAC
                     Tool    $410
                     <<<
_CtlReset            MAC
                     Tool    $510
                     <<<
~CtlStatus           MAC
                     PHA
_CtlStatus           MAC
                     Tool    $610
                     <<<
_NewControl          MAC
                     Tool    $910
                     <<<
~DisposeControl      MAC
                     PHL     ]1
_DisposeControl      MAC
                     Tool    $A10
                     <<<
~KillControls        MAC
                     PHL     ]1
_KillControls        MAC
                     Tool    $B10
                     <<<
~SetCtlTitle         MAC
                     PxL     ]1;]2
_SetCtlTitle         MAC
                     Tool    $C10
                     <<<
~GetCtlTitle         MAC
                     P2SL    ]1
_GetCtlTitle         MAC
                     Tool    $D10
                     <<<
~HideControl         MAC
                     PHL     ]1
_HideControl         MAC
                     Tool    $E10
                     <<<
~ShowControl         MAC
                     PHL     ]1
_ShowControl         MAC
                     Tool    $F10
                     <<<
~DrawControls        MAC
                     PHL     ]1
_DrawControls        MAC
                     Tool    $1010
                     <<<
~HiliteControl       MAC
                     PHWL    ]1;]2
_HiliteControl       MAC
                     Tool    $1110
                     <<<
_CtlNewRes           MAC
                     Tool    $1210
                     <<<
~FindControl         MAC
                     PHA
                     PHLW    ]1;]2
                     PHWL    ]3;]4
_FindControl         MAC
                     Tool    $1310
                     <<<
~TestControl         MAC
                     P1SW    ]1
                     PHWL    ]2;]3
_TestControl         MAC
                     Tool    $1410
                     <<<
~TrackControl        MAC
                     PHA
                     PxW     ]1;]2
                     PxL     ]3;]4
_TrackControl        MAC
                     Tool    $1510
                     <<<
~MoveControl         MAC
                     PxW     ]1;]2
                     PHL     ]3
_MoveControl         MAC
                     Tool    $1610
                     <<<
~DragControl         MAC
                     PxW     ]1;]2
                     PxL     ]3;]4
                     PHWL    ]5;]6
_DragControl         MAC
                     Tool    $1710
                     <<<
~SetCtlIcons         MAC
                     P2SL    ]1
_SetCtlIcons         MAC
                     Tool    $1810
                     <<<
~SetCtlValue         MAC
                     PHWL    ]1;]2
_SetCtlValue         MAC
                     Tool    $1910
                     <<<
~GetCtlValue         MAC
                     P1SL    ]1
_GetCtlValue         MAC
                     Tool    $1A10
                     <<<
~SetCtlParams        MAC
                     PxW     ]1;]2
                     PHL     ]3
_SetCtlParams        MAC
                     Tool    $1B10
                     <<<
~GetCtlParams        MAC
                     P2SL    ]1
_GetCtlParams        MAC
                     Tool    $1C10
                     <<<
~DragRect            MAC
                     P2SL    ]1
                     PHLW    ]2;]3
                     PHWL    ]4;]5
                     PxL     ]6;]7
                     PHW     ]8
_DragRect            MAC
                     Tool    $1D10
                     <<<
~GrowSize            MAC
                     PHS     2
_GrowSize            MAC
                     Tool    $1E10
                     <<<
~GetCtlDpage         MAC
                     PHA
_GetCtlDpage         MAC
                     Tool    $1F10
                     <<<
~SetCtlAction        MAC
                     PxL     ]1;]2
_SetCtlAction        MAC
                     Tool    $2010
                     <<<
~GetCtlAction        MAC
                     P2SL    ]1
_GetCtlAction        MAC
                     Tool    $2110
                     <<<
~SetCtlRefCon        MAC
                     PxL     ]1;]2
_SetCtlRefCon        MAC
                     Tool    $2210
                     <<<
~GetCtlRefCon        MAC
                     P2SL    ]1
_GetCtlRefCon        MAC
                     Tool    $2310
                     <<<
~EraseControl        MAC
                     PHL     ]1
_EraseControl        MAC
                     Tool    $2410
                     <<<
~DrawOneCtl          MAC
                     PHL     ]1
_DrawOneCtl          MAC
                     Tool    $2510
                     <<<
~FindTargetCtl       MAC
                     PHS     2
_FindTargetCtl       MAC
                     Tool    $2610
                     <<<
~MakeNextCtlTarget   MAC
                     PHS     2
_MakeNextCtlTarget   MAC
                     Tool    $2710
                     <<<
~MakeThisCtlTarget   MAC
                     PHL     ]1
_MakeThisCtlTarget   MAC
                     Tool    $2810
                     <<<
~SendEventToCtl      MAC
                     P1SW    ]1
                     PxL     ]2;]3
_SendEventToCtl      MAC
                     Tool    $2910
                     <<<
~GetCtlID            MAC
                     P2SL    ]1
_GetCtlID            MAC
                     Tool    $2A10
                     <<<
~SetCtlID            MAC
                     PxL     ]1;]2
_SetCtlID            MAC
                     Tool    $2B10
                     <<<
~CallCtlDefProc      MAC
                     PHLW    ]1;]2
                     PHL     ]3
_CallCtlDefProc      MAC
                     Tool    $2C10
                     <<<
~NotifyCtls          MAC
                     PxW     ]1;]2
                     PxL     ]3;]4
_NotifyCtls          MAC
                     Tool    $2D10
                     <<<
~GetCtlMoreFlags     MAC
                     P1SL    ]1
_GetCtlMoreFlags     MAC
                     Tool    $2E10
                     <<<
~SetCtlMoreFlags     MAC
                     PHWL    ]1;]2
_SetCtlMoreFlags     MAC
                     Tool    $2F10
                     <<<
~GetCtlHandleFromID  MAC
                     PHS     2
                     PxL     ]1;]2
_GetCtlHandleFromID  MAC
                     Tool    $3010
                     <<<
~NewControl2         MAC
                     P2SL    ]1
                     PHWL    ]2;]3
_NewControl2         MAC
                     Tool    $3110
                     <<<
~CMLoadResource      MAC
                     P2SW    ]1
                     PHL     ]2
_CMLoadResource      MAC
                     Tool    $3210
                     <<<
~CMReleaseResource   MAC
                     PHWL    ]1;]2
_CMReleaseResource   MAC
                     Tool    $3310
                     <<<
~SetCtlParamPtr      MAC
                     PHL     ]1
_SetCtlParamPtr      MAC
                     Tool    $3410
                     <<<
~GetCtlParamPtr      MAC
                     PHS     2
_GetCtlParamPtr      MAC
                     Tool    $3510
                     <<<
~InvalCtls           MAC
                     PHL     ]1
_InvalCtls           MAC
                     Tool    $3710
                     <<<
~qCtlStartUp         MAC
                     PHW     ]1
                     NextDP  ]2;$100
                     Tool    $210
                     <<<
_FindRadioButton     MAC
                     Tool    $3910
                     <<<
_SetLETextByID       MAC
                     Tool    $3A10
                     <<<
_GetLETextByID       MAC
                     Tool    $3B10
                     <<<
_SetCtlValueByID     MAC
                     Tool    $3C10
                     <<<
_GetCtlValueByID     MAC
                     Tool    $3D10
                     <<<
_InvalOneCtlByID     MAC
                     Tool    $3E10
                     <<<
_HiliteCtlByID       MAC
                     Tool    $3F10
                     <<<

