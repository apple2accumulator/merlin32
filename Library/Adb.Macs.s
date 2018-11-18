* Desktop Bus tool macros
*   by Dave Klimas
;
; Copyright Apple Computer, Inc. 1986, 1987
; and Roger Wagner Publishing, Inc. 1988
; All Rights Reserved
;
_ADBBootInit       MAC
                   Tool  $109
                   <<<
_ADBStartUp        MAC
                   Tool  $209
                   <<<
_ADBShutDown       MAC
                   Tool  $309
                   <<<
_ADBVersion        MAC
                   Tool  $409
                   <<<
_ADBReset          MAC
                   Tool  $509
                   <<<
~ADBStatus         MAC
                   PHA
_ADBStatus         MAC
                   Tool  $609
                   <<<
~SendInfo          MAC
                   PHW   ]1
                   PHLW  ]2;]3
_SendInfo          MAC
                   Tool  $909
                   <<<
~ReadKeyMicroData  MAC
                   PHW   ]1
                   PHLW  ]2;]3
_ReadKeyMicroData  MAC
                   Tool  $A09
                   <<<
~ReadKeyMicroMem   MAC
                   PxL   ]1;]2
                   PHW   ]3
_ReadKeyMicroMem   MAC
                   Tool  $B09
                   <<<
~AsyncADBReceive   MAC
                   PHLW  ]1;]2
_AsyncADBReceive   MAC
                   Tool  $D09
                   <<<
~SyncADBReceive    MAC
                   PHW   ]1
                   PHLW  ]2;]3
_SyncADBReceive    MAC
                   Tool  $E09
                   <<<
_AbsOn             MAC
                   Tool  $F09
                   <<<
_AbsOff            MAC
                   Tool  $1009
                   <<<
~ReadAbs           MAC
                   PHA
_ReadAbs           MAC
                   Tool  $1109
                   <<<
~GetAbsScale       MAC
                   PHL   ]1
_GetAbsScale       MAC
                   Tool  $1209
                   <<<
~SetAbsScale       MAC
                   PHL   ]1
_SetAbsScale       MAC
                   Tool  $1309
                   <<<
~SRQPoll           MAC
                   PHLW  ]1;]2
_SRQPoll           MAC
                   Tool  $1409
                   <<<
~SRQRemove         MAC
                   PHW   ]1
_SRQRemove         MAC
                   Tool  $1509
                   <<<
_ClearSRQTable     MAC
                   Tool  $1609
                   <<<

