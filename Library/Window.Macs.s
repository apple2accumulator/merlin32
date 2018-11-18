* Window Manager macros
*   by Dave Klimas
;
; Copyright Apple Computer, Inc. 1986, 1987
; and Roger Wagner Publishing, Inc. 1988
; All Rights Reserved
;
_WindBootInit         MAC
                      Tool  $10E
                      <<<
~WindStartUp          MAC
                      PHW   ]1
_WindStartUp          MAC
                      Tool  $20E
                      <<<
_WindShutDown         MAC
                      Tool  $30E
                      <<<
~WindVersion          MAC
                      PHA
_WindVersion          MAC
                      Tool  $40E
                      <<<
_WindReset            MAC
                      Tool  $50E
                      <<<
~WindStatus           MAC
                      PHA
_WindStatus           MAC
                      Tool  $60E
                      <<<
~NewWindow            MAC
                      P2SL  ]1
_NewWindow            MAC
                      Tool  $90E
                      <<<
~CheckUpdate          MAC
                      P1SL  ]1
_CheckUpdate          MAC
                      Tool  $A0E
                      <<<
~CloseWindow          MAC
                      PHL   ]1
_CloseWindow          MAC
                      Tool  $B0E
                      <<<
~Desktop              MAC
                      IF    2=]1
                      IF    3=]1
                      IF    4=]1
                      PHS   2
                      FIN
                      FIN
                      FIN
                      PHWL  ]1;]2
_Desktop              MAC
                      Tool  $C0E
                      <<<
~SetWTitle            MAC
                      PxL   ]1;]2
_SetWTitle            MAC
                      Tool  $D0E
                      <<<
~GetWTitle            MAC
                      P2SL  ]1
_GetWTitle            MAC
                      Tool  $E0E
                      <<<
~SetFrameColor        MAC
                      PxL   ]1;]2
_SetFrameColor        MAC
                      Tool  $F0E
                      <<<
~GetFrameColor        MAC
                      PxL   ]1;]2
_GetFrameColor        MAC
                      Tool  $100E
                      <<<
~SelectWindow         MAC
                      PHL   ]1
_SelectWindow         MAC
                      Tool  $110E
                      <<<
~HideWindow           MAC
                      PHL   ]1
_HideWindow           MAC
                      Tool  $120E
                      <<<
~ShowWindow           MAC
                      PHL   ]1
_ShowWindow           MAC
                      Tool  $130E
                      <<<
~SendBehind           MAC
                      PxL   ]1;]2
_SendBehind           MAC
                      Tool  $140E
                      <<<
~FrontWindow          MAC
                      PHS   2
_FrontWindow          MAC
                      Tool  $150E
                      <<<
~SetInfoDraw          MAC
                      PxL   ]1;]2
_SetInfoDraw          MAC
                      Tool  $160E
                      <<<
~FindWindow           MAC
                      P1SL  ]1
                      PxW   ]2;]3
_FindWindow           MAC
                      Tool  $170E
                      <<<
~TrackGoAway          MAC
                      P1SW  ]1
                      PHWL  ]2;]3
_TrackGoAway          MAC
                      Tool  $180E
                      <<<
~MoveWindow           MAC
                      PxW   ]1;]2
                      PHL   ]3
_MoveWindow           MAC
                      Tool  $190E
                      <<<
~DragWindow           MAC
                      PxW   ]1;]2;]3;]4
                      PxL   ]5;]6
_DragWindow           MAC
                      Tool  $1A0E
                      <<<
~GrowWindow           MAC
                      PHS   2
                      PxW   ]1;]2;]3;]4
                      PHL   ]5
_GrowWindow           MAC
                      Tool  $1B0E
                      <<<
~SizeWindow           MAC
                      PxW   ]1;]2
                      PHL   ]3
_SizeWindow           MAC
                      Tool  $1C0E
                      <<<
~TaskMaster           MAC
                      PHA
                      PHWL  ]1;]2
_TaskMaster           MAC
                      Tool  $1D0E
                      <<<
~BeginUpdate          MAC
                      PHL   ]1
_BeginUpdate          MAC
                      Tool  $1E0E
                      <<<
~EndUpdate            MAC
                      PHL   ]1
_EndUpdate            MAC
                      Tool  $1F0E
                      <<<
~GetWMgrPort          MAC
                      PHS   2
_GetWMgrPort          MAC
                      Tool  $200E
                      <<<
~PinRect              MAC
                      PHS   2
                      PxW   ]1;]2
                      PHL   ]3
_PinRect              MAC
                      Tool  $210E
                      <<<
~HiliteWindow         MAC
                      PHWL  ]1;]2
_HiliteWindow         MAC
                      Tool  $220E
                      <<<
~ShowHide             MAC
                      PHWL  ]1;]2
_ShowHide             MAC
                      Tool  $230E
                      <<<
~BringToFront         MAC
                      PHL   ]1
_BringToFront         MAC
                      Tool  $240E
                      <<<
_WindNewRes           MAC
                      Tool  $250E
                      <<<
~TrackZoom            MAC
                      P1SW  ]1
                      PHWL  ]2;]3
_TrackZoom            MAC
                      Tool  $260E
                      <<<
~ZoomWindow           MAC
                      PHL   ]1
_ZoomWindow           MAC
                      Tool  $270E
                      <<<
~SetWRefCon           MAC
                      PxL   ]1;]2
_SetWRefCon           MAC
                      Tool  $280E
                      <<<
~GetWRefCon           MAC
                      P2SL  ]1
_GetWRefCon           MAC
                      Tool  $290E
                      <<<
~GetNextWindow        MAC
                      P2SL  ]1
_GetNextWindow        MAC
                      Tool  $2A0E
                      <<<
~GetWKind             MAC
                      P1SL  ]1
_GetWKind             MAC
                      Tool  $2B0E
                      <<<
~SetWFrame            MAC
                      PHWL  ]1;]2
_SetWFrame            MAC
                      Tool  $2D0E
                      <<<
~GetWFrame            MAC
                      P1SL  ]1
_GetWFrame            MAC
                      Tool  $2C0E
                      <<<
~GetStructRgn         MAC
                      P2SL  ]1
_GetStructRgn         MAC
                      Tool  $2E0E
                      <<<
~GetContentRgn        MAC
                      P2SL  ]1
_GetContentRgn        MAC
                      Tool  $2F0E
                      <<<
~GetUpdateRgn         MAC
                      P2SL  ]1
_GetUpdateRgn         MAC
                      Tool  $300E
                      <<<
~GetDefProc           MAC
                      P2SL  ]1
_GetDefProc           MAC
                      Tool  $310E
                      <<<
~SetDefProc           MAC
                      PxL   ]1;]2
_SetDefProc           MAC
                      Tool  $320E
                      <<<
~GetWControls         MAC
                      P2SL  ]1
_GetWControls         MAC
                      Tool  $330E
                      <<<
~SetOriginMask        MAC
                      PHWL  ]1;]2
_SetOriginMask        MAC
                      Tool  $340E
                      <<<
~GetInfoRefCon        MAC
                      P2SL  ]1
_GetInfoRefCon        MAC
                      Tool  $350E
                      <<<
~SetInfoRefCon        MAC
                      PxL   ]1;]2
_SetInfoRefCon        MAC
                      Tool  $360E
                      <<<
~GetZoomRect          MAC
                      P2SL  ]1
_GetZoomRect          MAC
                      Tool  $370E
                      <<<
~SetZoomRect          MAC
                      PxL   ]1;]2
_SetZoomRect          MAC
                      Tool  $380E
                      <<<
~RefreshDesktop       MAC
                      PHL   ]1
_RefreshDesktop       MAC
                      Tool  $390E
                      <<<
~InvalRect            MAC
                      PHL   ]1
_InvalRect            MAC
                      Tool  $3A0E
                      <<<
~InvalRgn             MAC
                      PHL   ]1
_InvalRgn             MAC
                      Tool  $3B0E
                      <<<
~ValidRect            MAC
                      PHL   ]1
_ValidRect            MAC
                      Tool  $3C0E
                      <<<
~ValidRgn             MAC
                      PHL   ]1
_ValidRgn             MAC
                      Tool  $3D0E
                      <<<
~GetContentOrigin     MAC
                      P2SL  ]1
_GetContentOrigin     MAC
                      Tool  $3E0E
                      <<<
~SetContentOrigin     MAC
                      PxW   ]1;]2
                      PHL   ]3
_SetContentOrigin     MAC
                      Tool  $3F0E
                      <<<
~GetDataSize          MAC
                      P2SL  ]1
_GetDataSize          MAC
                      Tool  $400E
                      <<<
~SetDataSize          MAC
                      PxW   ]1;]2
                      PHL   ]3
_SetDataSize          MAC
                      Tool  $410E
                      <<<
~GetMaxGrow           MAC
                      P2SL  ]1
_GetMaxGrow           MAC
                      Tool  $420E
                      <<<
~SetMaxGrow           MAC
                      PxW   ]1;]2
                      PHL   ]3
_SetMaxGrow           MAC
                      Tool  $430E
                      <<<
~GetScroll            MAC
                      P2SL  ]1
_GetScroll            MAC
                      Tool  $440E
                      <<<
~SetScroll            MAC
                      PxW   ]1;]2
                      PHL   ]3
_SetScroll            MAC
                      Tool  $450E
                      <<<
~GetPage              MAC
                      P2SL  ]1
_GetPage              MAC
                      Tool  $460E
                      <<<
~SetPage              MAC
                      PxW   ]1;]2
                      PHL   ]3
_SetPage              MAC
                      Tool  $470E
                      <<<
~GetContentDraw       MAC
                      P2SL  ]1
_GetContentDraw       MAC
                      Tool  $480E
                      <<<
~SetContentDraw       MAC
                      PxL   ]1;]2
_SetContentDraw       MAC
                      Tool  $490E
                      <<<
~GetInfoDraw          MAC
                      P2SL  ]1
_GetInfoDraw          MAC
                      Tool  $4A0E
                      <<<
~SetSysWindow         MAC
                      PHL   ]1
_SetSysWindow         MAC
                      Tool  $4B0E
                      <<<
~GetSysWFlag          MAC
                      P1SL  ]1
_GetSysWFlag          MAC
                      Tool  $4C0E
                      <<<
~StartDrawing         MAC
                      PHL   ]1
_StartDrawing         MAC
                      Tool  $4D0E
                      <<<
~SetWindowIcons       MAC
                      P2SL  ]1
_SetWindowIcons       MAC
                      Tool  $4E0E
                      <<<
~GetRectInfo          MAC
                      PxL   ]1;]2
_GetRectInfo          MAC
                      Tool  $4F0E
                      <<<
~StartInfoDrawing     MAC
                      PxL   ]1;]2
_StartInfoDrawing     MAC
                      Tool  $500E
                      <<<
_EndInfoDrawing       MAC
                      Tool  $510E
                      <<<
~GetFirstWindow       MAC
                      PHS   2
_GetFirstWindow       MAC
                      Tool  $520E
                      <<<
~WindDragRect         MAC
                      P2SL  ]1
                      PHLW  ]2;]3
                      PHWL  ]4;]5
                      PxL   ]6;]7
                      PHW   ]8
_WindDragRect         MAC
                      Tool  $530E
                      <<<
_Private01            MAC
                      Tool  $540E
                      <<<
~DrawInfoBar          MAC
                      PHL   ]1
_DrawInfoBar          MAC
                      Tool  $550E
                      <<<
~WindowGlobal         MAC
                      P1SW  ]1
_WindowGlobal         MAC
                      Tool  $560E
                      <<<
~SetContentOrigin2    MAC
                      PxW   ]1;]2;]3
                      PHL   ]4
_SetContentOrigin2    MAC
                      Tool  $570E
                      <<<
~GetWindowMgrGlobals  MAC
                      PHS   2
_GetWindowMgrGlobals  MAC
                      Tool  $580E
                      <<<
~AlertWindow          MAC
                      P1SW  ]1
                      PxL   ]2;]3
_AlertWindow          MAC
                      Tool  $590E
                      <<<
~StartFrameDrawing    MAC
                      PHL   ]1
_StartFrameDrawing    MAC
                      Tool  $5A0E
                      <<<
_EndFrameDrawing      MAC
                      Tool  $5B0E
                      <<<
~ResizeWindow         MAC
                      PHWL  ]1;]2
                      PHL   ]3
_ResizeWindow         MAC
                      Tool  $5C0E
                      <<<
_TaskMasterContent    MAC                ;private
                      Tool  $5D0E
                      <<<
_TaskMasterKey        MAC                ;private
                      Tool  $5E0E
                      <<<
~TaskMasterDA         MAC
                      P1SW  ]1
                      PHL   ]2
_TaskMasterDA         MAC
                      Tool  $5F0E
                      <<<
~CompileText          MAC
                      P2SW  ]1
                      PxL   ]2;]3
                      PHW   ]4
_CompileText          MAC
                      Tool  $600E
                      <<<
~NewWindow2           MAC
                      PHS   2
                      PxL   ]1;]2;]3;]4
                      PHWL  ]5;]6
                      PHW   ]7
_NewWindow2           MAC
                      Tool  $610E
                      <<<
~ErrorWindow          MAC
                      P1SW  ]1
                      PHLW  ]2;]3
_ErrorWindow          MAC
                      Tool  $620E
                      <<<
_GetAuxWindInfo       MAC
                      Tool  $630E
                      <<<
_DoModalWindow        MAC
                      Tool  $640E
                      <<<
_MWGetCtlPart         MAC
                      Tool  $650E
                      <<<
_MWSetMenuProc        MAC
                      Tool  $660E
                      <<<
_MWStdDrawProc        MAC
                      Tool  $670E
                      <<<
_MWSetUpEditMenu      MAC
                      Tool  $680E
                      <<<
_FindCursorCtl        MAC
                      Tool  $690E
                      <<<
_ResizeInfoBar        MAC
                      Tool  $6A0E
                      <<<
_HandleDiskInsert     MAC
                      Tool  $6B0E
                      <<<
_UpdateWindow         MAC
                      Tool  $6C0E
                      <<<

