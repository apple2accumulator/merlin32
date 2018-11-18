* TextEdit macros
*   by Dave Klimas
;
; Copyright Apple Computer, Inc. 1986, 1987
; and Roger Wagner Publishing, Inc. 1988
; All Rights Reserved
;
_TEBootInit           MAC
                      Tool  $122
                      <<<
~TEStartUp            MAC
                      PxW   ]1;]2
_TEStartUp            MAC
                      Tool  $222
                      <<<
_TEShutDown           MAC
                      Tool  $322
                      <<<
~TEVersion            MAC
                      PHA
_TEVersion            MAC
                      Tool  $422
                      <<<
_TEReset              MAC
                      Tool  $522
                      <<<
~TEStatus             MAC
                      PHA
_TEStatus             MAC
                      Tool  $622
                      <<<
~TENew                MAC
                      P2SL  ]1
_TENew                MAC
                      Tool  $922
                      <<<
~TEKill               MAC
                      PHL   ]1
_TEKill               MAC
                      Tool  $A22
                      <<<
~TESetText            MAC
                      PHWL  ]1;]2
                      PHLW  ]3;]4
                      PxL   ]5;]6
_TESetText            MAC
                      Tool  $B22
                      <<<
~TEGetText            MAC
                      PHS   2
                      PHWL  ]1;]2
                      PHLW  ]3;]4
                      PxL   ]5;]6
_TEGetText            MAC
                      Tool  $C22
                      <<<
~TEGetTextInfo        MAC
                      PHLW  ]1;]2
                      PHL   ]3
_TEGetTextInfo        MAC
                      Tool  $D22
                      <<<
~TEIdle               MAC
                      PHL   ]1
_TEIdle               MAC
                      Tool  $E22
                      <<<
~TEActivate           MAC
                      PHL   ]1
_TEActivate           MAC
                      Tool  $F22
                      <<<
~TEDeactivate         MAC
                      PHL   ]1
_TEDeactivate         MAC
                      Tool  $1022
                      <<<
~TEClick              MAC
                      PxL   ]1;]2
_TEClick              MAC
                      Tool  $1122
                      <<<
~TEUpdate             MAC
                      PHL   ]1
_TEUpdate             MAC
                      Tool  $1222
                      <<<
~TEPaintText          MAC
                      P2SL  ]1
                      PxL   ]2;]3
                      PHWL  ]4;]5
_TEPaintText          MAC
                      Tool  $1322
                      <<<
~TEKey                MAC
                      PxL   ]1;]2
_TEKey                MAC
                      Tool  $1422
                      <<<
~TECut                MAC
                      PHL   ]1
_TECut                MAC
                      Tool  $1622
                      <<<
~TECopy               MAC
                      PHL   ]1
_TECopy               MAC
                      Tool  $1722
                      <<<
~TEPaste              MAC
                      PHL   ]1
_TEPaste              MAC
                      Tool  $1822
                      <<<
~TEClear              MAC
                      PHL   ]1
_TEClear              MAC
                      Tool  $1922
                      <<<
~TEInsert             MAC
                      PHWL  ]1;]2
                      PHLW  ]3;]4
                      PxL   ]5;]6
_TEInsert             MAC
                      Tool  $1A22
                      <<<
~TEReplace            MAC
                      PHWL  ]1;]2
                      PHLW  ]3;]4
                      PxL   ]5;]6
_TEReplace            MAC
                      Tool  $1B22
                      <<<
~TEGetSelection       MAC
                      PxL   ]1;]2;]3
_TEGetSelection       MAC
                      Tool  $1C22
                      <<<
~TESetSelection       MAC
                      PxL   ]1;]2;]3
_TESetSelection       MAC
                      Tool  $1D22
                      <<<
~TEGetSelectionStyle  MAC
                      PHA
                      PxL   ]1;]2;]3
_TEGetSelectionStyle  MAC
                      Tool  $1E22
                      <<<
~TEStyleChange        MAC
                      PHW   ]1
                      PxL   ]2;]3
_TEStyleChange        MAC
                      Tool  $1F22
                      <<<
~TEOffsetToPoint      MAC
                      PxL   ]1;]2;]3;]4
_TEOffsetToPoint      MAC
                      Tool  $2022
                      <<<
~TEPointToOffset      MAC
                      PHS   2
                      PxL   ]1;]2;]3
_TEPointToOffset      MAC
                      Tool  $2122
                      <<<
~TEGetDefProc         MAC
                      PHS   2
_TEGetDefProc         MAC
                      Tool  $2222
                      <<<
~TEGetRuler           MAC
                      PHWL  ]1;]2
                      PHL   ]3
_TEGetRuler           MAC
                      Tool  $2322
                      <<<
~TESetRuler           MAC
                      PHW   ]1
                      PxL   ]2;]3
_TESetRuler           MAC
                      Tool  $2422
                      <<<
~TEScroll             MAC
                      PHW   ]1
                      PxL   ]2;]3;]4
_TEScroll             MAC
                      Tool  $2522
                      <<<
_TEGetInternalProc    MAC
                      Tool  $2622
                      <<<
_TEGetLastError       MAC
                      Tool  $2722
                      <<<
_TECompactRecord      MAC
                      Tool  $2822
                      <<<

