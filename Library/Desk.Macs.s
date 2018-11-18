* Desk Manager macros
*   by Dave Klimas
;
; Copyright Apple Computer, Inc. 1986, 1987
; and Roger Wagner Publishing, Inc. 1988
; All Rights Reserved
;
_DeskBootInit      MAC
                   Tool  $105
                   <<<
_DeskStartUp       MAC
                   Tool  $205
                   <<<
_DeskShutDown      MAC
                   Tool  $305
                   <<<
~DeskVersion       MAC
                   PHA
_DeskVersion       MAC
                   Tool  $405
                   <<<
_DeskReset         MAC
                   Tool  $505
                   <<<
~DeskStatus        MAC
                   PHA
_DeskStatus        MAC
                   Tool  $605
                   <<<
_SaveScrn          MAC
                   Tool  $905
                   <<<
_RestScrn          MAC
                   Tool  $A05
                   <<<
_SaveAll           MAC
                   Tool  $B05
                   <<<
_RestAll           MAC
                   Tool  $C05
                   <<<
~InstallNDA        MAC
                   PHL   ]1
_InstallNDA        MAC
                   Tool  $E05
                   <<<
~InstallCDA        MAC
                   PHL   ]1
_InstallCDA        MAC
                   Tool  $F05
                   <<<
_ChooseCDA         MAC
                   Tool  $1105
                   <<<
~SetDAStrPtr       MAC
                   PxL   ]1;]2
_SetDAStrPtr       MAC
                   Tool  $1305
                   <<<
~GetDAStrPtr       MAC
                   PHS   2
_GetDAStrPtr       MAC
                   Tool  $1405
                   <<<
~OpenNDA           MAC
                   P1SW  ]1
_OpenNDA           MAC
                   Tool  $1505
                   <<<
~CloseNDA          MAC
                   PHW   ]1
_CloseNDA          MAC
                   Tool  $1605
                   <<<
~SystemClick       MAC
                   PxL   ]1;]2
                   PHW   ]3
_SystemClick       MAC
                   Tool  $1705
                   <<<
~SystemEdit        MAC
                   P1SW  ]1
_SystemEdit        MAC
                   Tool  $1805
                   <<<
_SystemTask        MAC
                   Tool  $1905
                   <<<
~SystemEvent       MAC
                   P1SW  ]1
                   PxL   ]2;]3;]4
                   PHW   ]5
_SystemEvent       MAC
                   Tool  $1A05
                   <<<
~GetNumNDAs        MAC
                   PHA
_GetNumNDAs        MAC
                   Tool  $1B05
                   <<<
~CloseNDAbyWinPtr  MAC
                   PHL   ]1
_CloseNDAbyWinPtr  MAC
                   Tool  $1C05
                   <<<
_CloseAllNDAs      MAC
                   Tool  $1D05
                   <<<
~FixAppleMenu      MAC
                   PHW   ]1
_FixAppleMenu      MAC
                   Tool  $1E05
                   <<<
~AddToRunQ         MAC
                   PHL   ]1
_AddToRunQ         MAC
                   Tool  $1F05
                   <<<
~RemoveFromRunQ    MAC
                   PHL   ]1
_RemoveFromRunQ    MAC
                   Tool  $2005
                   <<<
~RemoveCDA         MAC
                   PHL   ]1
_RemoveCDA         MAC
                   Tool  $2105
                   <<<
~RemoveNDA         MAC
                   PHL   ]1
_RemoveNDA         MAC
                   Tool  $2205
                   <<<
_GetDeskAccInfo    MAC
                   Tool  $2305
                   <<<
_CallDeskAcc       MAC
                   Tool  $2405
                   <<<
_GetDeskGlobal     MAC
                   Tool  $2505
                   <<<

