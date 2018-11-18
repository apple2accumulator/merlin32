* Menu Manager macros
*   by Dave Klimas
;
; Copyright Apple Computer, Inc. 1986, 1987
; and Roger Wagner Publishing, Inc. 1988
; All Rights Reserved
;
_MenuBootInit       MAC
                    Tool      $10F
                    <<<
~MenuStartUp        MAC
                    PxW       ]1;]2
_MenuStartUp        MAC
                    Tool      $20F
                    <<<
_MenuShutDown       MAC
                    Tool      $30F
                    <<<
~MenuVersion        MAC
                    PHA
_MenuVersion        MAC
                    Tool      $40F
                    <<<
_MenuReset          MAC
                    Tool      $50F
                    <<<
~MenuStatus         MAC
                    PHA
_MenuStatus         MAC
                    Tool      $60F
                    <<<
~MenuKey            MAC
                    PxL       ]1;]2
_MenuKey            MAC
                    Tool      $90F
                    <<<
~GetMenuBar         MAC
                    PHS       2
_GetMenuBar         MAC
                    Tool      $A0F
                    <<<
~MenuRefresh        MAC
                    PHL       ]1
_MenuRefresh        MAC
                    Tool      $B0F
                    <<<
_FlashMenuBar       MAC
                    Tool      $C0F
                    <<<
~InsertMenu         MAC
                    PHLW      ]1;]2
_InsertMenu         MAC
                    Tool      $D0F
                    <<<
~DeleteMenu         MAC
                    PHW       ]1
_DeleteMenu         MAC
                    Tool      $E0F
                    <<<
~InsertMItem        MAC
                    PHL       ]1
                    PxW       ]2;]3
_InsertMItem        MAC
                    Tool      $F0F
                    <<<
~DeleteMItem        MAC
                    PHW       ]1
_DeleteMItem        MAC
                    Tool      $100F
                    <<<
~GetSysBar          MAC
                    PHS       2
_GetSysBar          MAC
                    Tool      $110F
                    <<<
~SetSysBar          MAC
                    PHL       ]1
_SetSysBar          MAC
                    Tool      $120F
                    <<<
~FixMenuBar         MAC
                    PHA
_FixMenuBar         MAC
                    Tool      $130F
                    <<<
~CountMItems        MAC
                    P1SW      ]1
_CountMItems        MAC
                    Tool      $140F
                    <<<
~NewMenuBar         MAC
                    P2SL      ]1
_NewMenuBar         MAC
                    Tool      $150F
                    <<<
~GetMHandle         MAC
                    P2SW      ]1
_GetMHandle         MAC
                    Tool      $160F
                    <<<
~SetBarColors       MAC
                    PxW       ]1;]2;]3
_SetBarColors       MAC
                    Tool      $170F
                    <<<
~GetBarColors       MAC
                    PHS       2
_GetBarColors       MAC
                    Tool      $180F
                    <<<
~SetMTitleStart     MAC
                    PHW       ]1
_SetMTitleStart     MAC
                    Tool      $190F
                    <<<
~GetMTitleStart     MAC
                    PHA
_GetMTitleStart     MAC
                    Tool      $1A0F
                    <<<
~GetMenuMgrPort     MAC
                    PHS       2
_GetMenuMgrPort     MAC
                    Tool      $1B0F
                    <<<
~CalcMenuSize       MAC
                    PxW       ]1;]2;]3
_CalcMenuSize       MAC
                    Tool      $1C0F
                    <<<
~SetMTitleWidth     MAC
                    PxW       ]1;]2
_SetMTitleWidth     MAC
                    Tool      $1D0F
                    <<<
~GetMTitleWidth     MAC
                    P1SW      ]1
_GetMTitleWidth     MAC
                    Tool      $1E0F
                    <<<
~SetMenuFlag        MAC
                    PxW       ]1;]2
_SetMenuFlag        MAC
                    Tool      $1F0F
                    <<<
~GetMenuFlag        MAC
                    P1SW      ]1
_GetMenuFlag        MAC
                    Tool      $200F
                    <<<
~SetMenuTitle       MAC
                    PHLW      ]1;]2
_SetMenuTitle       MAC
                    Tool      $210F
                    <<<
~GetMenuTitle       MAC
                    P2SW      ]1
_GetMenuTitle       MAC
                    Tool      $220F
                    <<<
~MenuGlobal         MAC
                    P1SW      ]1
_MenuGlobal         MAC
                    Tool      $230F
                    <<<
~SetMItem           MAC
                    PHLW      ]1;]2
_SetMItem           MAC
                    Tool      $240F
                    <<<
~GetMItem           MAC
                    PHLW      ]1;]2
_GetMItem           MAC
                    Tool      $250F
                    <<<
~SetMItemFlag       MAC
                    PxW       ]1;]2
_SetMItemFlag       MAC
                    Tool      $260F
                    <<<
~GetMItemFlag       MAC
                    P1SW      ]1
_GetMItemFlag       MAC
                    Tool      $270F
                    <<<
~SetMItemBlink      MAC
                    PHW       ]1
_SetMItemBlink      MAC
                    Tool      $280F
                    <<<
_MenuNewRes         MAC
                    Tool      $290F
                    <<<
_DrawMenuBar        MAC
                    Tool      $2A0F
                    <<<
~MenuSelect         MAC
                    PxL       ]1;]2
_MenuSelect         MAC
                    Tool      $2B0F
                    <<<
~HiliteMenu         MAC
                    PxW       ]1;]2
_HiliteMenu         MAC
                    Tool      $2C0F
                    <<<
~NewMenu            MAC
                    P2SL      ]1
_NewMenu            MAC
                    Tool      $2D0F
                    <<<
~DisposeMenu        MAC
                    PHL       ]1
_DisposeMenu        MAC
                    Tool      $2E0F
                    <<<
_InitPalette        MAC
                    Tool      $2F0F
                    <<<
~EnableMItem        MAC
                    PHW       ]1
_EnableMItem        MAC
                    Tool      $300F
                    <<<
~DisableMItem       MAC
                    PHW       ]1
_DisableMItem       MAC
                    Tool      $310F
                    <<<
~CheckMItem         MAC
                    PxW       ]1;]2
_CheckMItem         MAC
                    Tool      $320F
                    <<<
~SetMItemMark       MAC
                    PxW       ]1;]2
_SetMItemMark       MAC
                    Tool      $330F
                    <<<
~GetMItemMark       MAC
                    P1SW      ]1
_GetMItemMark       MAC
                    Tool      $340F
                    <<<
~SetMItemStyle      MAC
                    PxW       ]1;]2
_SetMItemStyle      MAC
                    Tool      $350F
                    <<<
~GetMItemStyle      MAC
                    P1SW      ]1
_GetMItemStyle      MAC
                    Tool      $360F
                    <<<
~SetMenuID          MAC
                    PxW       ]1;]2
_SetMenuID          MAC
                    Tool      $370F
                    <<<
~SetMItemID         MAC
                    PxW       ]1;]2
_SetMItemID         MAC
                    Tool      $380F
                    <<<
~SetMenuBar         MAC
                    PHL       ]1
_SetMenuBar         MAC
                    Tool      $390F
                    <<<
~SetMItemName       MAC
                    PHLW      ]1;]2
_SetMItemName       MAC
                    Tool      $3A0F
                    <<<
~GetPopUpDefProc    MAC
                    PHS       2
_GetPopUpDefProc    MAC
                    Tool      $3B0F
                    <<<
~PopUpMenuSelect    MAC
                    PHA
                    PxW       ]1;]2;]3;]4
                    PHL       ]5
_PopUpMenuSelect    MAC
                    Tool      $3C0F
                    <<<
~DrawPopUp          MAC
                    PxW       ]1;]2;]3;]4
                    PxW       ]5;]6;]7
_DrawPopUp          MAC
                    Tool      $3D0F
                    <<<
~NewMenu2           MAC
                    P2SW      ]1
                    PHL       ]2
_NewMenu2           MAC
                    Tool      $3E0F
                    <<<
~InsertMItem2       MAC
                    PHWL      ]1;]2
                    PxW       ]3;]4
_InsertMItem2       MAC
                    Tool      $3F0F
                    <<<
~SetMenuTitle2      MAC
                    PHWL      ]1;]2
                    PHW       ]3
_SetMenuTitle2      MAC
                    Tool      $400F
                    <<<
~SetMItem2          MAC
                    PHWL      ]1;]2
                    PHW       ]3
_SetMItem2          MAC
                    Tool      $410F
                    <<<
~SetMItemName2      MAC
                    PHWL      ]1;]2
                    PHW       ]3
_SetMItemName2      MAC
                    Tool      $420F
                    <<<
~NewMenuBar2        MAC
                    P2SW      ]1
                    PxL       ]2;]3
_NewMenuBar2        MAC
                    Tool      $430F
                    <<<
_HideMenuBar        MAC
                    Tool      $450F
                    <<<
_ShowMenuBar        MAC
                    Tool      $460F
                    <<<
_SetMItemIcon       MAC
                    Tool      $470F
                    <<<
_GetMItemIcon       MAC
                    Tool      $480F
                    <<<
_SetMItemStruct     MAC
                    Tool      $490F
                    <<<
_GetMItemStruct     MAC
                    Tool      $4A0F
                    <<<
_RemoveMItemStruct  MAC
                    Tool      $4B0F
                    <<<
_GetMItemFlag2      MAC
                    Tool      $4C0F
                    <<<
_SetMItemFlag2      MAC
                    Tool      $4D0F
                    <<<
_GetMItemBlink      MAC
                    Tool      $4F0F
                    <<<
_InsertPathMItems   MAC
                    Tool      $500F
                    <<<
~qMenuStartUp       MAC
                    PHW       ]1
                    NextDP    ]2;$100
                    Tool      $20F
                    <<<
~qNewMenu           MAC
                    ~NewMenu  ]1
                    PHW       #0
                    _InsertMenu
                    <<<

