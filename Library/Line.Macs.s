* LineEdit macros
*   by Dave Klimas
;
; Copyright Apple Computer, Inc. 1986, 1987
; and Roger Wagner Publishing, Inc. 1988
; All Rights Reserved
;
_LEBootInit     MAC
                Tool    $114
                <<<
~LEStartUp      MAC
                PxW     ]1;]2
_LEStartUp      MAC
                Tool    $214
                <<<
_LEShutDown     MAC
                Tool    $314
                <<<
~LEVersion      MAC
                PHA
_LEVersion      MAC
                Tool    $414
                <<<
_LEReset        MAC
                Tool    $514
                <<<
~LEStatus       MAC
                PHA
_LEStatus       MAC
                Tool    $614
                <<<
~LENew          MAC
                P2SL    ]1
                PHLW    ]2;]3
_LENew          MAC
                Tool    $914
                <<<
~LEDispose      MAC
                PHL     ]1
_LEDispose      MAC
                Tool    $A14
                <<<
~LESetText      MAC
                PHL     ]1
                PHWL    ]2;]3
_LESetText      MAC
                Tool    $B14
                <<<
~LEIdle         MAC
                PHL     ]1
_LEIdle         MAC
                Tool    $C14
                <<<
~LEClick        MAC
                PxL     ]1;]2
_LEClick        MAC
                Tool    $D14
                <<<
~LESetSelect    MAC
                PxW     ]1;]2
                PHL     ]3
_LESetSelect    MAC
                Tool    $E14
                <<<
~LEActivate     MAC
                PHL     ]1
_LEActivate     MAC
                Tool    $F14
                <<<
~LEDeactivate   MAC
                PHL     ]1
_LEDeactivate   MAC
                Tool    $1014
                <<<
~LEKey          MAC
                PxW     ]1;]2
                PHL     ]3
_LEKey          MAC
                Tool    $1114
                <<<
~LECut          MAC
                PHL     ]1
_LECut          MAC
                Tool    $1214
                <<<
~LECopy         MAC
                PHL     ]1
_LECopy         MAC
                Tool    $1314
                <<<
~LEPaste        MAC
                PHL     ]1
_LEPaste        MAC
                Tool    $1414
                <<<
~LEDelete       MAC
                PHL     ]1
_LEDelete       MAC
                Tool    $1514
                <<<
~LEInsert       MAC
                PHL     ]1
                PHWL    ]2;]3
_LEInsert       MAC
                Tool    $1614
                <<<
~LEUpdate       MAC
                PHL     ]1
_LEUpdate       MAC
                Tool    $1714
                <<<
~LETextBox      MAC
                PHLW    ]1;]2
                PHLW    ]3;]4
_LETextBox      MAC
                Tool    $1814
                <<<
_LEFromScrap    MAC
                Tool    $1914
                <<<
_LEToScrap      MAC
                Tool    $1A14
                <<<
~LEScrapHandle  MAC
                PHS     2
_LEScrapHandle  MAC
                Tool    $1B14
                <<<
~LEGetScrapLen  MAC
                PHA
_LEGetScrapLen  MAC
                Tool    $1C14
                <<<
~LESetScrapLen  MAC
                PHW     ]1
_LESetScrapLen  MAC
                Tool    $1D14
                <<<
~LESetHilite    MAC
                PxL     ]1;]2
_LESetHilite    MAC
                Tool    $1E14
                <<<
~LESetCaret     MAC
                PxL     ]1;]2
_LESetCaret     MAC
                Tool    $1F14
                <<<
~LETextBox2     MAC
                PHLW    ]1;]2
                PHLW    ]3;]4
_LETextBox2     MAC
                Tool    $2014
                <<<
~LESetJust      MAC
                PHWL    ]1;]2
_LESetJust      MAC
                Tool    $2114
                <<<
~LEGetTextHand  MAC
                P2SL    ]1
_LEGetTextHand  MAC
                Tool    $2214
                <<<
~LEGetTextLen   MAC
                P1SL    ]1
_LEGetTextLen   MAC
                Tool    $2314
                <<<
~GetLEDefProc   MAC
                PHS     2
_GetLEDefProc   MAC
                Tool    $2414
                <<<
~qLEStartUp     MAC
                PHW     ]1
                NextDP  ]2;$100
                Tool    $214
                <<<
_LEClassifyKey  MAC
                Tool    $2514
                <<<

