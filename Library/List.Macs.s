* List Mgr. macros
*   by Dave Klimas
;
; Copyright Apple Computer, Inc. 1986, 1987
; and Roger Wagner Publishing, Inc. 1988
; All Rights Reserved
;
_ListBootInit    MAC
                 Tool  $011C
                 <<<
_ListStartup     MAC
                 Tool  $021C
                 <<<
_ListShutDown    MAC
                 Tool  $031C
                 <<<
~ListVersion     MAC
                 PHA
_ListVersion     MAC
                 Tool  $041C
                 <<<
_ListReset       MAC
                 Tool  $051C
                 <<<
~ListStatus      MAC
                 PHA
_ListStatus      MAC
                 Tool  $061C
                 <<<
~CreateList      MAC
                 PHS   2
                 PxL   ]1;]2
_CreateList      MAC
                 Tool  $091C
                 <<<
~SortList        MAC
                 PxL   ]1;]2
_SortList        MAC
                 Tool  $0A1C
                 <<<
~NextMember      MAC
                 PHS   2
                 PxL   ]1;]2
_NextMember      MAC
                 Tool  $0B1C
                 <<<
~DrawMember      MAC
                 PxL   ]1;]2
_DrawMember      MAC
                 Tool  $0C1C
                 <<<
~SelectMember    MAC
                 PxL   ]1;]2
_SelectMember    MAC
                 Tool  $0D1C
                 <<<
~GetListDefProc  MAC
                 PHS   2
_GetListDefProc  MAC
                 Tool  $0E1C
                 <<<
~ResetMember     MAC
                 P2SL  ]1
_ResetMember     MAC
                 Tool  $0F1C
                 <<<
~NewList         MAC
                 PxL   ]1;]2
_NewList         MAC
                 Tool  $101C
                 <<<
~DrawMember2     MAC
                 PHWL  ]1;]2
_DrawMember2     MAC
                 Tool  $111C
                 <<<
~NextMember2     MAC
                 PHA
                 PHWL  ]1;]2
_NextMember2     MAC
                 Tool  $121C
                 <<<
~ResetMember2    MAC
                 P1SL  ]1
_ResetMember2    MAC
                 Tool  $131C
                 <<<
~SelectMember2   MAC
                 PHWL  ]1;]2
_SelectMember2   MAC
                 Tool  $141C
                 <<<
~SortList2       MAC
                 PxL   ]1;]2
_SortList2       MAC
                 Tool  $151C
                 <<<
~NewList2        MAC
                 PHL   ]1
                 PxW   ]2;]3;]4;]5
                 PHL   ]6
_NewList2        MAC
                 Tool  $161C
                 <<<
_ListKey         MAC
                 Tool  $171C
                 <<<
_CompareStrings  MAC
                 Tool  $181C
                 <<<

