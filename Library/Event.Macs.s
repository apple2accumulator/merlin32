* Event Manager macros
*   by Dave Klimas
;
; Copyright Apple Computer, Inc. 1986, 1987
; and Roger Wagner Publishing, Inc. 1988
; All Rights Reserved
;
_EMBootInit         MAC
                    Tool    $106
                    <<<
~EMStartUp          MAC
                    PxW     ]1;]2;]3;]4
                    PxW     ]5;]6;]7
_EMStartUp          MAC
                    Tool    $206
                    <<<
_EMShutDown         MAC
                    Tool    $306
                    <<<
~EMVersion          MAC
                    PHA
_EMVersion          MAC
                    Tool    $406
                    <<<
_EMReset            MAC
                    Tool    $506
                    <<<
~EMStatus           MAC
                    PHA
_EMStatus           MAC
                    Tool    $606
                    <<<
~DoWindows          MAC
                    PHA
_DoWindows          MAC
                    Tool    $906
                    <<<
~GetNextEvent       MAC
                    PHA
                    PHWL    ]1;]2
_GetNextEvent       MAC
                    Tool    $A06
                    <<<
~EventAvail         MAC
                    PHA
                    PHWL    ]1;]2
_EventAvail         MAC
                    Tool    $B06
                    <<<
~GetMouse           MAC
                    PHL     ]1
_GetMouse           MAC
                    Tool    $C06
                    <<<
~Button             MAC
                    P1SW    ]1
_Button             MAC
                    Tool    $D06
                    <<<
~StillDown          MAC
                    P1SW    ]1
_StillDown          MAC
                    Tool    $E06
                    <<<
~WaitMouseUp        MAC
                    P1SW    ]1
_WaitMouseUp        MAC
                    Tool    $F06
                    <<<
~TickCount          MAC
                    PHS     2
_TickCount          MAC
                    Tool    $1006
                    <<<
~GetDblTime         MAC
                    PHS     2
_GetDblTime         MAC
                    Tool    $1106
                    <<<
~GetCaretTime       MAC
                    PHS     2
_GetCaretTime       MAC
                    Tool    $1206
                    <<<
_SetSwitch          MAC
                    Tool    $1306
                    <<<
~PostEvent          MAC
                    PHA
                    PHWL    ]1;]2
_PostEvent          MAC
                    Tool    $1406
                    <<<
~FlushEvents        MAC
                    PHA
                    PxW     ]1;]2
_FlushEvents        MAC
                    Tool    $1506
                    <<<
~GetOSEvent         MAC
                    PHA
                    PHWL    ]1;]2
_GetOSEvent         MAC
                    Tool    $1606
                    <<<
~OSEventAvail       MAC
                    PHA
                    PHWL    ]1;]2
_OSEventAvail       MAC
                    Tool    $1706
                    <<<
~SetEventMask       MAC
                    PHW     ]1
_SetEventMask       MAC
                    Tool    $1806
                    <<<
~FakeMouse          MAC
                    PxW     ]1;]2;]3;]4
                    PHW     ]5
_FakeMouse          MAC
                    Tool    $1906
                    <<<
~SetAutoKeyLimit    MAC
                    PHW     ]1
_SetAutoKeyLimit    MAC
                    Tool    $1A06
                    <<<
~GetKeyTranslation  MAC
                    P1SW    ]1
_GetKeyTranslation  MAC
                    Tool    $1B06
                    <<<
~SetKeyTranslation  MAC
                    PHW     ]1
_SetKeyTranslation  MAC
                    Tool    $1C06
                    <<<
~qEMStartUp         MAC
                    NextDP  ]1;$100
                    PxW     ]2;]3;]4;]5
                    PxW     ]6;]7
                    Tool    $206
                    <<<

