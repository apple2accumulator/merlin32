* Dialog Manager macros
*   by Dave Klimas
;
; Copyright Apple Computer, Inc. 1986, 1987
; and Roger Wagner Publishing, Inc. 1988
; All Rights Reserved
;
_DialogBootInit     MAC
                    Tool  $115
                    <<<
~DialogStartUp      MAC
                    PHW   ]1
_DialogStartUp      MAC
                    Tool  $215
                    <<<
_DialogShutDown     MAC
                    Tool  $315
                    <<<
~DialogVersion      MAC
                    PHA
_DialogVersion      MAC
                    Tool  $415
                    <<<
_DialogReset        MAC
                    Tool  $515
                    <<<
~DialogStatus       MAC
                    PHA
_DialogStatus       MAC
                    Tool  $615
                    <<<
~ErrorSound         MAC
                    PHL   ]1
_ErrorSound         MAC
                    Tool  $915
                    <<<
~NewModalDialog     MAC
                    P2SL  ]1
                    PHWL  ]2;]3
_NewModalDialog     MAC
                    Tool  $A15
                    <<<
~NewModelessDialog  MAC
                    P2SL  ]1
                    PxL   ]2;]3
                    PHWL  ]4;]5
                    PHL   ]6
_NewModelessDialog  MAC
                    Tool  $B15
                    <<<
~CloseDialog        MAC
                    PHL   ]1
_CloseDialog        MAC
                    Tool  $C15
                    <<<
~NewDItem           MAC
                    PHLW  ]1;]2
                    PHLW  ]3;]4
                    PHLW  ]5;]6
                    PHWL  ]7;]8
_NewDItem           MAC
                    Tool  $D15
                    <<<
~RemoveDItem        MAC
                    PHLW  ]1;]2
_RemoveDItem        MAC
                    Tool  $E15
                    <<<
~ModalDialog        MAC
                    P1SL  ]1
_ModalDialog        MAC
                    Tool  $F15
                    <<<
~IsDialogEvent      MAC
                    P1SL  ]1
_IsDialogEvent      MAC
                    Tool  $1015
                    <<<
~DialogSelect       MAC
                    P1SL  ]1
                    PxL   ]2;]3
_DialogSelect       MAC
                    Tool  $1115
                    <<<
~DlgCut             MAC
                    PHL   ]1
_DlgCut             MAC
                    Tool  $1215
                    <<<
~DlgCopy            MAC
                    PHL   ]1
_DlgCopy            MAC
                    Tool  $1315
                    <<<
~DlgPaste           MAC
                    PHL   ]1
_DlgPaste           MAC
                    Tool  $1415
                    <<<
~DlgDelete          MAC
                    PHL   ]1
_DlgDelete          MAC
                    Tool  $1515
                    <<<
~DrawDialog         MAC
                    PHL   ]1
_DrawDialog         MAC
                    Tool  $1615
                    <<<
~Alert              MAC
                    PHA
                    PxL   ]1;]2
_Alert              MAC
                    Tool  $1715
                    <<<
~StopAlert          MAC
                    PHA
                    PxL   ]1;]2
_StopAlert          MAC
                    Tool  $1815
                    <<<
~NoteAlert          MAC
                    PHA
                    PxL   ]1;]2
_NoteAlert          MAC
                    Tool  $1915
                    <<<
~CautionAlert       MAC
                    PHA
                    PxL   ]1;]2
_CautionAlert       MAC
                    Tool  $1A15
                    <<<
~ParamText          MAC
                    PxL   ]1;]2;]3;]4
_ParamText          MAC
                    Tool  $1B15
                    <<<
~SetDAFont          MAC
                    PHL   ]1
_SetDAFont          MAC
                    Tool  $1C15
                    <<<
~GetControlDItem    MAC
                    PHS   2
                    PHLW  ]1;]2
_GetControlDItem    MAC
                    Tool  $1E15
                    <<<
~GetIText           MAC
                    PHL   ]1
                    PHWL  ]2;]3
_GetIText           MAC
                    Tool  $1F15
                    <<<
~SetIText           MAC
                    PHL   ]1
                    PHWL  ]2;]3
_SetIText           MAC
                    Tool  $2015
                    <<<
~SelectIText        MAC
                    PHL   ]1
                    PxW   ]2;]3;]4
_SelectIText        MAC
                    Tool  $2115
                    <<<
~HideDItem          MAC
                    PHLW  ]1;]2
_HideDItem          MAC
                    Tool  $2215
                    <<<
~ShowDItem          MAC
                    PHLW  ]1;]2
_ShowDItem          MAC
                    Tool  $2315
                    <<<
~FindDItem          MAC
                    PHA
                    PxL   ]1;]2
_FindDItem          MAC
                    Tool  $2415
                    <<<
~UpdateDialog       MAC
                    PxL   ]1;]2
_UpdateDialog       MAC
                    Tool  $2515
                    <<<
~GetDItemType       MAC
                    PHA
                    PHLW  ]1;]2
_GetDItemType       MAC
                    Tool  $2615
                    <<<
~SetDItemType       MAC
                    PHW   ]1
                    PHLW  ]2;]3
_SetDItemType       MAC
                    Tool  $2715
                    <<<
~GetDItemBox        MAC
                    PHL   ]1
                    PHWL  ]2;]3
_GetDItemBox        MAC
                    Tool  $2815
                    <<<
~SetDItemBox        MAC
                    PHL   ]1
                    PHWL  ]2;]3
_SetDItemBox        MAC
                    Tool  $2915
                    <<<
~GetFirstDItem      MAC
                    P1SL  ]1
_GetFirstDItem      MAC
                    Tool  $2A15
                    <<<
~GetNextDItem       MAC
                    PHA
                    PHLW  ]1;]2
_GetNextDItem       MAC
                    Tool  $2B15
                    <<<
~ModalDialog2       MAC
                    P2SL  ]1
_ModalDialog2       MAC
                    Tool  $2C15
                    <<<
~GetDItemValue      MAC
                    PHA
                    PHLW  ]1;]2
_GetDItemValue      MAC
                    Tool  $2E15
                    <<<
~SetDItemValue      MAC
                    PHW   ]1
                    PHLW  ]2;]3
_SetDItemValue      MAC
                    Tool  $2F15
                    <<<
~GetNewModalDialog  MAC
                    P2SL  ]1
_GetNewModalDialog  MAC
                    Tool  $3215
                    <<<
~GetNewDItem        MAC
                    PxL   ]1;]2
_GetNewDItem        MAC
                    Tool  $3315
                    <<<
~GetAlertStage      MAC
                    PHA
_GetAlertStage      MAC
                    Tool  $3415
                    <<<
_ResetAlertStage    MAC
                    Tool  $3515
                    <<<
~DefaultFilter      MAC
                    PHA
                    PxL   ]1;]2;]3
_DefaultFilter      MAC
                    Tool  $3615
                    <<<
~GetDefButton       MAC
                    P1SL  ]1
_GetDefButton       MAC
                    Tool  $3715
                    <<<
~SetDefButton       MAC
                    PHWL  ]1;]2
_SetDefButton       MAC
                    Tool  $3815
                    <<<
~DisableDItem       MAC
                    PHLW  ]1;]2
_DisableDItem       MAC
                    Tool  $3915
                    <<<
~EnableDItem        MAC
                    PHLW  ]1;]2
_EnableDItem        MAC
                    Tool  $3A15
                    <<<

