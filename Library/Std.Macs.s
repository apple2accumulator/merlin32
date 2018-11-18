* Standard File Operations macros
*   by Dave Klimas
;
; Copyright Apple Computer, Inc. 1986, 1987
; and Roger Wagner Publishing, Inc. 1988
; All Rights Reserved
;
_SFBootInit       MAC
                  Tool    $117
                  <<<
~SFStartUp        MAC
                  PxW     ]1;]2
_SFStartUp        MAC
                  Tool    $217
                  <<<
_SFShutDown       MAC
                  Tool    $317
                  <<<
~SFVersion        MAC
                  PHA
_SFVersion        MAC
                  Tool    $417
                  <<<
_SFReset          MAC
                  Tool    $517
                  <<<
~SFStatus         MAC
                  PHA
_SFStatus         MAC
                  Tool    $617
                  <<<
~SFGetFile        MAC
                  PxW     ]1;]2
                  PxL     ]3;]4;]5;]6
_SFGetFile        MAC
                  Tool    $917
                  <<<
~SFPutFile        MAC
                  PxW     ]1;]2
                  PxL     ]3;]4
                  PHWL    ]5;]6
_SFPutFile        MAC
                  Tool    $A17
                  <<<
~SFPGetFile       MAC
                  PxW     ]1;]2
                  PxL     ]3;]4;]5;]6
                  PxL     ]7;]8
_SFPGetFile       MAC
                  Tool    $B17
                  <<<
~SFPPutFile       MAC
                  PxW     ]1;]2
                  PxL     ]3;]4
                  PHWL    ]5;]6
                  PxL     ]7;]8
_SFPPutFile       MAC
                  Tool    $C17
                  <<<
~SFAllCaps        MAC
                  PHW     ]1
_SFAllCaps        MAC
                  Tool    $D17
                  <<<
~SFGetFile2       MAC
                  PxW     ]1;]2;]3
                  PxL     ]4;]5;]6;]7
_SFGetFile2       MAC
                  Tool    $E17
                  <<<
~SFPutFile2       MAC
                  PxW     ]1;]2;]3
                  PHLW    ]4;]5
                  PxL     ]6;]7
_SFPutFile2       MAC
                  Tool    $F17
                  <<<
_SFPGetFile2      MAC
                  Tool    $1017
                  <<<
_SFPPutFile2      MAC
                  Tool    $1117
                  <<<
~SFShowInvisible  MAC
                  P1SW    ]1
_SFShowInvisible  MAC
                  Tool    $1217
                  <<<
~SFReScan         MAC
                  PxL     ]1;]2
_SFReScan         MAC
                  Tool    $1317
                  <<<
~SFMultiGet2      MAC
                  PxW     ]1;]2;]3
                  PxL     ]4;]5;]6;]7
_SFMultiGet2      MAC
                  Tool    $1417
                  <<<
_SFPMultiGet2     MAC
                  Tool    $1517
                  <<<
~qSFStartUp       MAC
                  PHW     ]1
                  NextDP  ]2;$100
                  Tool    $217
                  <<<

