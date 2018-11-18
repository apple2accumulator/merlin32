* QuickDraw II macros
*   by Dave Klimas
;
; Copyright Apple Computer, Inc. 1986, 1987
; and Roger Wagner Publishing, Inc. 1988
; All Rights Reserved
;
_QDBootInit         MAC
                    Tool    $104
                    <<<
~QDStartUp          MAC
                    PxW     ]1;]2;]3;]4
_QDStartUp          MAC
                    Tool    $204
                    <<<
_QDShutDown         MAC
                    Tool    $304
                    <<<
~QDVersion          MAC
                    PHA
_QDVersion          MAC
                    Tool    $404
                    <<<
_QDReset            MAC
                    Tool    $504
                    <<<
~QDStatus           MAC
                    PHA
_QDStatus           MAC
                    Tool    $604
                    <<<
~GetAddress         MAC
                    P2SW    ]1
_GetAddress         MAC
                    Tool    $904
                    <<<
_GrafOn             MAC
                    Tool    $A04
                    <<<
_GrafOff            MAC
                    Tool    $B04
                    <<<
~GetStandardSCB     MAC
                    PHA
_GetStandardSCB     MAC
                    Tool    $C04
                    <<<
~InitColorTable     MAC
                    PHL     ]1
_InitColorTable     MAC
                    Tool    $D04
                    <<<
~SetColorTable      MAC
                    PHWL    ]1;]2
_SetColorTable      MAC
                    Tool    $E04
                    <<<
~GetColorTable      MAC
                    PHWL    ]1;]2
_GetColorTable      MAC
                    Tool    $F04
                    <<<
~SetColorEntry      MAC
                    PxW     ]1;]2;]3
_SetColorEntry      MAC
                    Tool    $1004
                    <<<
~GetColorEntry      MAC
                    PHA
                    PxW     ]1;]2
_GetColorEntry      MAC
                    Tool    $1104
                    <<<
~SetSCB             MAC
                    PxW     ]1;]2
_SetSCB             MAC
                    Tool    $1204
                    <<<
~GetSCB             MAC
                    P1SW    ]1
_GetSCB             MAC
                    Tool    $1304
                    <<<
~SetAllSCBs         MAC
                    PHW     ]1
_SetAllSCBs         MAC
                    Tool    $1404
                    <<<
~ClearScreen        MAC
                    PHW     ]1
_ClearScreen        MAC
                    Tool    $1504
                    <<<
~SetMasterSCB       MAC
                    PHW     ]1
_SetMasterSCB       MAC
                    Tool    $1604
                    <<<
~GetMasterSCB       MAC
                    PHA
_GetMasterSCB       MAC
                    Tool    $1704
                    <<<
~OpenPort           MAC
                    PHL     ]1
_OpenPort           MAC
                    Tool    $1804
                    <<<
~InitPort           MAC
                    PHL     ]1
_InitPort           MAC
                    Tool    $1904
                    <<<
~ClosePort          MAC
                    PHL     ]1
_ClosePort          MAC
                    Tool    $1A04
                    <<<
~SetPort            MAC
                    PHL     ]1
_SetPort            MAC
                    Tool    $1B04
                    <<<
~GetPort            MAC
                    PHS     2
_GetPort            MAC
                    Tool    $1C04
                    <<<
~SetPortLoc         MAC
                    PHL     ]1
_SetPortLoc         MAC
                    Tool    $1D04
                    <<<
~GetPortLoc         MAC
                    PHL     ]1
_GetPortLoc         MAC
                    Tool    $1E04
                    <<<
~SetPortRect        MAC
                    PHL     ]1
_SetPortRect        MAC
                    Tool    $1F04
                    <<<
~GetPortRect        MAC
                    PHL     ]1
_GetPortRect        MAC
                    Tool    $2004
                    <<<
~SetPortSize        MAC
                    PxW     ]1;]2
_SetPortSize        MAC
                    Tool    $2104
                    <<<
~MovePortTo         MAC
                    PxW     ]1;]2
_MovePortTo         MAC
                    Tool    $2204
                    <<<
~SetOrigin          MAC
                    PxW     ]1;]2
_SetOrigin          MAC
                    Tool    $2304
                    <<<
~SetClip            MAC
                    PHL     ]1
_SetClip            MAC
                    Tool    $2404
                    <<<
~GetClip            MAC
                    PHL     ]1
_GetClip            MAC
                    Tool    $2504
                    <<<
~ClipRect           MAC
                    PHL     ]1
_ClipRect           MAC
                    Tool    $2604
                    <<<
_HidePen            MAC
                    Tool    $2704
                    <<<
_ShowPen            MAC
                    Tool    $2804
                    <<<
~GetPen             MAC
                    PHL     ]1
_GetPen             MAC
                    Tool    $2904
                    <<<
~SetPenState        MAC
                    PHL     ]1
_SetPenState        MAC
                    Tool    $2A04
                    <<<
~GetPenState        MAC
                    PHL     ]1
_GetPenState        MAC
                    Tool    $2B04
                    <<<
~SetPenSize         MAC
                    PxW     ]1;]2
_SetPenSize         MAC
                    Tool    $2C04
                    <<<
~GetPenSize         MAC
                    PHL     ]1
_GetPenSize         MAC
                    Tool    $2D04
                    <<<
~SetPenMode         MAC
                    PHW     ]1
_SetPenMode         MAC
                    Tool    $2E04
                    <<<
~GetPenMode         MAC
                    PHA
_GetPenMode         MAC
                    Tool    $2F04
                    <<<
~SetPenPat          MAC
                    PHL     ]1
_SetPenPat          MAC
                    Tool    $3004
                    <<<
~GetPenPat          MAC
                    PHL     ]1
_GetPenPat          MAC
                    Tool    $3104
                    <<<
~SetPenMask         MAC
                    PHL     ]1
_SetPenMask         MAC
                    Tool    $3204
                    <<<
~GetPenMask         MAC
                    PHL     ]1
_GetPenMask         MAC
                    Tool    $3304
                    <<<
~SetBackPat         MAC
                    PHL     ]1
_SetBackPat         MAC
                    Tool    $3404
                    <<<
~GetBackPat         MAC
                    PHL     ]1
_GetBackPat         MAC
                    Tool    $3504
                    <<<
_PenNormal          MAC
                    Tool    $3604
                    <<<
~SetSolidPenPat     MAC
                    PHW     ]1
_SetSolidPenPat     MAC
                    Tool    $3704
                    <<<
~SetSolidBackPat    MAC
                    PHW     ]1
_SetSolidBackPat    MAC
                    Tool    $3804
                    <<<
~SolidPattern       MAC
                    PHWL    ]1;]2
_SolidPattern       MAC
                    Tool    $3904
                    <<<
~MoveTo             MAC
GotoXY              MAC
QDGotoXY            MAC
                    PxW     ]1;]2
_MoveTo             MAC
                    Tool    $3A04
                    <<<
~Move               MAC
                    PxW     ]1;]2
_Move               MAC
                    Tool    $3B04
                    <<<
~LineTo             MAC
                    PxW     ]1;]2
_LineTo             MAC
                    Tool    $3C04
                    <<<
~Line               MAC
                    PxW     ]1;]2
_Line               MAC
                    Tool    $3D04
                    <<<
~SetPicSave         MAC
                    PHL     ]1
_SetPicSave         MAC
                    Tool    $3E04
                    <<<
~GetPicSave         MAC
                    PHS     2
_GetPicSave         MAC
                    Tool    $3F04
                    <<<
~SetRgnSave         MAC
                    PHL     ]1
_SetRgnSave         MAC
                    Tool    $4004
                    <<<
~GetRgnSave         MAC
                    PHS     2
_GetRgnSave         MAC
                    Tool    $4104
                    <<<
~SetPolySave        MAC
                    PHL     ]1
_SetPolySave        MAC
                    Tool    $4204
                    <<<
~GetPolySave        MAC
                    PHS     2
_GetPolySave        MAC
                    Tool    $4304
                    <<<
~SetGrafProcs       MAC
                    PHL     ]1
_SetGrafProcs       MAC
                    Tool    $4404
                    <<<
~GetGrafProcs       MAC
                    PHS     2
_GetGrafProcs       MAC
                    Tool    $4504
                    <<<
~SetUserField       MAC
                    PHL     ]1
_SetUserField       MAC
                    Tool    $4604
                    <<<
~GetUserField       MAC
                    PHS     2
_GetUserField       MAC
                    Tool    $4704
                    <<<
~SetSysField        MAC
                    PHL     ]1
_SetSysField        MAC
                    Tool    $4804
                    <<<
~GetSysField        MAC
                    PHS     2
_GetSysField        MAC
                    Tool    $4904
                    <<<
~SetRect            MAC
                    PHL     ]1
                    PxW     ]2;]3;]4;]5
_SetRect            MAC
                    Tool    $4A04
                    <<<
~OffsetRect         MAC
                    PHL     ]1
                    PxW     ]2;]3
_OffsetRect         MAC
                    Tool    $4B04
                    <<<
~InsetRect          MAC
                    PHL     ]1
                    PxW     ]2;]3
_InsetRect          MAC
                    Tool    $4C04
                    <<<
~SectRect           MAC
                    PHA
                    PxL     ]1;]2;]3
_SectRect           MAC
                    Tool    $4D04
                    <<<
~UnionRect          MAC
                    PxL     ]1;]2;]3
_UnionRect          MAC
                    Tool    $4E04
                    <<<
~PtInRect           MAC
                    PHA
                    PxL     ]1;]2
_PtInRect           MAC
                    Tool    $4F04
                    <<<
~Pt2Rect            MAC
                    PxL     ]1;]2;]3
_Pt2Rect            MAC
                    Tool    $5004
                    <<<
~EqualRect          MAC
                    PHA
                    PxL     ]1;]2
_EqualRect          MAC
                    Tool    $5104
                    <<<
~NotEmptyRect       MAC
                    P1SL    ]1
_NotEmptyRect       MAC
                    Tool    $5204
                    <<<
~EmptyRect          MAC
                    P1SL    ]1
_EmptyRect          MAC
                    Tool    $5204
                    <<<
~FrameRect          MAC
                    PHL     ]1
_FrameRect          MAC
                    Tool    $5304
                    <<<
~PaintRect          MAC
                    PHL     ]1
_PaintRect          MAC
                    Tool    $5404
                    <<<
~EraseRect          MAC
                    PHL     ]1
_EraseRect          MAC
                    Tool    $5504
                    <<<
~InvertRect         MAC
                    PHL     ]1
_InvertRect         MAC
                    Tool    $5604
                    <<<
~FillRect           MAC
                    PxL     ]1;]2
_FillRect           MAC
                    Tool    $5704
                    <<<
~FrameOval          MAC
                    PHL     ]1
_FrameOval          MAC
                    Tool    $5804
                    <<<
~PaintOval          MAC
                    PHL     ]1
_PaintOval          MAC
                    Tool    $5904
                    <<<
~EraseOval          MAC
                    PHL     ]1
_EraseOval          MAC
                    Tool    $5A04
                    <<<
~InvertOval         MAC
                    PHL     ]1
_InvertOval         MAC
                    Tool    $5B04
                    <<<
~FillOval           MAC
                    PxL     ]1;]2
_FillOval           MAC
                    Tool    $5C04
                    <<<
~FrameRRect         MAC
                    PHL     ]1
                    PxW     ]2;]3
_FrameRRect         MAC
                    Tool    $5D04
                    <<<
~PaintRRect         MAC
                    PHL     ]1
                    PxW     ]2;]3
_PaintRRect         MAC
                    Tool    $5E04
                    <<<
~EraseRRect         MAC
                    PHL     ]1
                    PxW     ]2;]3
_EraseRRect         MAC
                    Tool    $5F04
                    <<<
~InvertRRect        MAC
                    PHL     ]1
                    PxW     ]2;]3
_InvertRRect        MAC
                    Tool    $6004
                    <<<
~FillRRect          MAC
                    PHLW    ]1;]2
                    PHWL    ]3;]4
_FillRRect          MAC
                    Tool    $6104
                    <<<
~FrameArc           MAC
                    PHL     ]1
                    PxW     ]2;]3
_FrameArc           MAC
                    Tool    $6204
                    <<<
~PaintArc           MAC
                    PHL     ]1
                    PxW     ]2;]3
_PaintArc           MAC
                    Tool    $6304
                    <<<
~EraseArc           MAC
                    PHL     ]1
                    PxW     ]2;]3
_EraseArc           MAC
                    Tool    $6404
                    <<<
~InvertArc          MAC
                    PHL     ]1
                    PxW     ]2;]3
_InvertArc          MAC
                    Tool    $6504
                    <<<
~FillArc            MAC
                    PHLW    ]1;]2
                    PHWL    ]3;]4
_FillArc            MAC
                    Tool    $6604
                    <<<
~NewRgn             MAC
                    PHS     2
_NewRgn             MAC
                    Tool    $6704
                    <<<
~DisposeRgn         MAC
                    PHL     ]1
_DisposeRgn         MAC
                    Tool    $6804
                    <<<
~CopyRgn            MAC
                    PxL     ]1;]2
_CopyRgn            MAC
                    Tool    $6904
                    <<<
~SetEmptyRgn        MAC
                    PHL     ]1
_SetEmptyRgn        MAC
                    Tool    $6A04
                    <<<
~SetRectRgn         MAC
                    PHL     ]1
                    PxW     ]2;]3;]4;]5
_SetRectRgn         MAC
                    Tool    $6B04
                    <<<
~RectRgn            MAC
                    PxL     ]1;]2
_RectRgn            MAC
                    Tool    $6C04
                    <<<
_OpenRgn            MAC
                    Tool    $6D04
                    <<<
~CloseRgn           MAC
                    PHL     ]1
_CloseRgn           MAC
                    Tool    $6E04
                    <<<
~OffsetRgn          MAC
                    PHL     ]1
                    PxW     ]2;]3
_OffsetRgn          MAC
                    Tool    $6F04
                    <<<
~InsetRgn           MAC
                    PHL     ]1
                    PxW     ]2;]3
_InsetRgn           MAC
                    Tool    $7004
                    <<<
~SectRgn            MAC
                    PxL     ]1;]2;]3
_SectRgn            MAC
                    Tool    $7104
                    <<<
~UnionRgn           MAC
                    PxL     ]1;]2;]3
_UnionRgn           MAC
                    Tool    $7204
                    <<<
~DiffRgn            MAC
                    PxL     ]1;]2;]3
_DiffRgn            MAC
                    Tool    $7304
                    <<<
~XorRgn             MAC
                    PxL     ]1;]2;]3
_XorRgn             MAC
                    Tool    $7404
                    <<<
~PtInRgn            MAC
                    PHA
                    PxL     ]1;]2
_PtInRgn            MAC
                    Tool    $7504
                    <<<
~RectInRgn          MAC
                    PHA
                    PxL     ]1;]2
_RectInRgn          MAC
                    Tool    $7604
                    <<<
~EqualRgn           MAC
                    PHA
                    PxL     ]1;]2
_EqualRgn           MAC
                    Tool    $7704
                    <<<
~EmptyRgn           MAC
                    P1SL    ]1
_EmptyRgn           MAC
                    Tool    $7804
                    <<<
~FrameRgn           MAC
                    PHL     ]1
_FrameRgn           MAC
                    Tool    $7904
                    <<<
~PaintRgn           MAC
                    PHL     ]1
_PaintRgn           MAC
                    Tool    $7A04
                    <<<
~EraseRgn           MAC
                    PHL     ]1
_EraseRgn           MAC
                    Tool    $7B04
                    <<<
~InvertRgn          MAC
                    PHL     ]1
_InvertRgn          MAC
                    Tool    $7C04
                    <<<
~FillRgn            MAC
                    PxL     ]1;]2
_FillRgn            MAC
                    Tool    $7D04
                    <<<
~ScrollRect         MAC
                    PHLW    ]1;]2
                    PHWL    ]3;]4
_ScrollRect         MAC
                    Tool    $7E04
                    <<<
~PaintPixels        MAC
                    PHL     ]1
_PaintPixels        MAC
                    Tool    $7F04
                    <<<
~AddPt              MAC
                    PxL     ]1;]2
_AddPt              MAC
                    Tool    $8004
                    <<<
~SubPt              MAC
                    PxL     ]1;]2
_SubPt              MAC
                    Tool    $8104
                    <<<
~SetPt              MAC
                    PHL     ]1
                    PxW     ]2;]3
_SetPt              MAC
                    Tool    $8204
                    <<<
~EqualPt            MAC
                    PHA
                    PxL     ]1;]2
_EqualPt            MAC
                    Tool    $8304
                    <<<
~LocalToGlobal      MAC
                    PHL     ]1
_LocalToGlobal      MAC
                    Tool    $8404
                    <<<
~GlobalToLocal      MAC
                    PHL     ]1
_GlobalToLocal      MAC
                    Tool    $8504
                    <<<
~Random             MAC
                    PHA
_Random             MAC
                    Tool    $8604
                    <<<
~SetRandSeed        MAC
                    PHL     ]1
_SetRandSeed        MAC
                    Tool    $8704
                    <<<
~GetPixel           MAC
                    PHA
                    PxW     ]1;]2
_GetPixel           MAC
                    Tool    $8804
                    <<<
~ScalePt            MAC
                    PxL     ]1;]2;]3
_ScalePt            MAC
                    Tool    $8904
                    <<<
~MapPt              MAC
                    PxL     ]1;]2;]3
_MapPt              MAC
                    Tool    $8A04
                    <<<
~MapRect            MAC
                    PxL     ]1;]2;]3
_MapRect            MAC
                    Tool    $8B04
                    <<<
~MapRgn             MAC
                    PxL     ]1;]2;]3
_MapRgn             MAC
                    Tool    $8C04
                    <<<
~SetStdProcs        MAC
                    PHL     ]1
_SetStdProcs        MAC
                    Tool    $8D04
                    <<<
~SetCursor          MAC
                    PHL     ]1
_SetCursor          MAC
                    Tool    $8E04
                    <<<
~GetCursorAdr       MAC
                    PHS     2
_GetCursorAdr       MAC
                    Tool    $8F04
                    <<<
_HideCursor         MAC
                    Tool    $9004
                    <<<
_ShowCursor         MAC
                    Tool    $9104
                    <<<
_ObscureCursor      MAC
                    Tool    $9204
                    <<<
_SetMouseLoc        MAC
                    Tool    $9304
                    <<<
~SetFont            MAC
                    PHL     ]1
_SetFont            MAC
                    Tool    $9404
                    <<<
~GetFont            MAC
                    PHS     2
_GetFont            MAC
                    Tool    $9504
                    <<<
~GetFontInfo        MAC
                    PHL     ]1
_GetFontInfo        MAC
                    Tool    $9604
                    <<<
~GetFontGlobals     MAC
                    PHL     ]1
_GetFontGlobals     MAC
                    Tool    $9704
                    <<<
~SetFontFlags       MAC
                    PHW     ]1
_SetFontFlags       MAC
                    Tool    $9804
                    <<<
~GetFontFlags       MAC
                    PHA
_GetFontFlags       MAC
                    Tool    $9904
                    <<<
~SetTextFace        MAC
                    PHW     ]1
_SetTextFace        MAC
                    Tool    $9A04
                    <<<
~GetTextFace        MAC
                    PHA
_GetTextFace        MAC
                    Tool    $9B04
                    <<<
~SetTextMode        MAC
                    PHW     ]1
_SetTextMode        MAC
                    Tool    $9C04
                    <<<
~GetTextMode        MAC
                    PHA
_GetTextMode        MAC
                    Tool    $9D04
                    <<<
~SetSpaceExtra      MAC
                    PHL     ]1
_SetSpaceExtra      MAC
                    Tool    $9E04
                    <<<
~GetSpaceExtra      MAC
                    PHS     2
_GetSpaceExtra      MAC
                    Tool    $9F04
                    <<<
~SetForeColor       MAC
                    PHW     ]1
_SetForeColor       MAC
                    Tool    $A004
                    <<<
~GetForeColor       MAC
                    PHA
_GetForeColor       MAC
                    Tool    $A104
                    <<<
~SetBackColor       MAC
                    PHW     ]1
_SetBackColor       MAC
                    Tool    $A204
                    <<<
~GetBackColor       MAC
                    PHA
_GetBackColor       MAC
                    Tool    $A304
                    <<<
~DrawChar           MAC
                    PHW     ]1
_DrawChar           MAC
                    Tool    $A404
                    <<<
~DrawString         MAC
                    PHL     ]1
_DrawString         MAC
                    Tool    $A504
                    <<<
~DrawCString        MAC
                    PHL     ]1
_DrawCString        MAC
                    Tool    $A604
                    <<<
~DrawText           MAC
                    PHLW    ]1;]2
_DrawText           MAC
                    Tool    $A704
                    <<<
~CharWidth          MAC
                    P1SW    ]1
_CharWidth          MAC
                    Tool    $A804
                    <<<
~StringWidth        MAC
                    P1SL    ]1
_StringWidth        MAC
                    Tool    $A904
                    <<<
~CStringWidth       MAC
                    P1SL    ]1
_CStringWidth       MAC
                    Tool    $AA04
                    <<<
~TextWidth          MAC
                    P1SL    ]1
                    PHW     ]2
_TextWidth          MAC
                    Tool    $AB04
                    <<<
~CharBounds         MAC
                    PHWL    ]1;]2
_CharBounds         MAC
                    Tool    $AC04
                    <<<
~StringBounds       MAC
                    PxL     ]1;]2
_StringBounds       MAC
                    Tool    $AD04
                    <<<
~CStringBounds      MAC
                    PxL     ]1;]2
_CStringBounds      MAC
                    Tool    $AE04
                    <<<
~TextBounds         MAC
                    PHLW    ]1;]2
                    PHL     ]3
_TextBounds         MAC
                    Tool    $AF04
                    <<<
~SetArcRot          MAC
                    PHW     ]1
_SetArcRot          MAC
                    Tool    $B004
                    <<<
~GetArcRot          MAC
                    PHA
_GetArcRot          MAC
                    Tool    $B104
                    <<<
~SetSysFont         MAC
                    PHL     ]1
_SetSysFont         MAC
                    Tool    $B204
                    <<<
~GetSysFont         MAC
                    PHS     2
_GetSysFont         MAC
                    Tool    $B304
                    <<<
~SetVisRgn          MAC
                    PHL     ]1
_SetVisRgn          MAC
                    Tool    $B404
                    <<<
~GetVisRgn          MAC
                    PHL     ]1
_GetVisRgn          MAC
                    Tool    $B504
                    <<<
~SetIntUse          MAC
                    PHW     ]1
_SetIntUse          MAC
                    Tool    $B604
                    <<<
~OpenPicture        MAC
                    P2SL    ]1
_OpenPicture        MAC
                    Tool    $B704
                    <<<
~PicComment         MAC
                    PHW     ]1
                    PHWL    ]2;]3
_PicComment         MAC
                    Tool    $B804
                    <<<
_ClosePicture       MAC
                    Tool    $B904
                    <<<
~DrawPicture        MAC
                    PxL     ]1;]2
_DrawPicture        MAC
                    Tool    $BA04
                    <<<
~KillPicture        MAC
                    PHL     ]1
_KillPicture        MAC
                    Tool    $BB04
                    <<<
~FramePoly          MAC
                    PHL     ]1
_FramePoly          MAC
                    Tool    $BC04
                    <<<
~PaintPoly          MAC
                    PHL     ]1
_PaintPoly          MAC
                    Tool    $BD04
                    <<<
~ErasePoly          MAC
                    PHL     ]1
_ErasePoly          MAC
                    Tool    $BE04
                    <<<
~InvertPoly         MAC
                    PHL     ]1
_InvertPoly         MAC
                    Tool    $BF04
                    <<<
~FillPoly           MAC
                    PxL     ]1;]2
_FillPoly           MAC
                    Tool    $C004
                    <<<
~OpenPoly           MAC
                    PHS     2
_OpenPoly           MAC
                    Tool    $C104
                    <<<
_ClosePoly          MAC
                    Tool    $C204
                    <<<
~KillPoly           MAC
                    PHL     ]1
_KillPoly           MAC
                    Tool    $C304
                    <<<
~OffsetPoly         MAC
                    PHL     ]1
                    PxW     ]2;]3
_OffsetPoly         MAC
                    Tool    $C404
                    <<<
~MapPoly            MAC
                    PxL     ]1;]2;]3
_MapPoly            MAC
                    Tool    $C504
                    <<<
~SetClipHandle      MAC
                    PHL     ]1
_SetClipHandle      MAC
                    Tool    $C604
                    <<<
~GetClipHandle      MAC
                    PHS     2
_GetClipHandle      MAC
                    Tool    $C704
                    <<<
~SetVisHandle       MAC
                    PHL     ]1
_SetVisHandle       MAC
                    Tool    $C804
                    <<<
~GetVisHandle       MAC
                    PHS     2
_GetVisHandle       MAC
                    Tool    $C904
                    <<<
_InitCursor         MAC
                    Tool    $CA04
                    <<<
~SetBufDims         MAC
                    PxW     ]1;]2;]3
_SetBufDims         MAC
                    Tool    $CB04
                    <<<
~ForceBufDims       MAC
                    PxW     ]1;]2;]3
_ForceBufDims       MAC
                    Tool    $CC04
                    <<<
~SaveBufDims        MAC
                    PHL     ]1
_SaveBufDims        MAC
                    Tool    $CD04
                    <<<
~RestoreBufDims     MAC
                    PHL     ]1
_RestoreBufDims     MAC
                    Tool    $CE04
                    <<<
~GetFGSize          MAC
                    PHA
_GetFGSize          MAC
                    Tool    $CF04
                    <<<
~SetFontID          MAC
                    PHL     ]1
_SetFontID          MAC
                    Tool    $D004
                    <<<
~GetFontID          MAC
                    PHS     2
_GetFontID          MAC
                    Tool    $D104
                    <<<
~SetTextSize        MAC
                    PHW     ]1
_SetTextSize        MAC
                    Tool    $D204
                    <<<
~GetTextSize        MAC
                    PHA
_GetTextSize        MAC
                    Tool    $D304
                    <<<
~SetCharExtra       MAC
                    PHL     ]1
_SetCharExtra       MAC
                    Tool    $D404
                    <<<
~GetCharExtra       MAC
                    PHS     2
_GetCharExtra       MAC
                    Tool    $D504
                    <<<
~PPToPort           MAC
                    PxL     ]1;]2
                    PxW     ]3;]4;]5
_PPToPort           MAC
                    Tool    $D604
                    <<<
~InflateTextBuffer  MAC
                    PxW     ]1;]2
_InflateTextBuffer  MAC
                    Tool    $D704
                    <<<
~GetRomFont         MAC
                    PHL     ]1
_GetRomFont         MAC
                    Tool    $D804
                    <<<
~GetFontLore        MAC
                    PHA
                    PHLW    ]1;]2
_GetFontLore        MAC
                    Tool    $D904
                    <<<
_Get640Colors       MAC
                    Tool    $DA04
                    <<<
_Set640Color        MAC
                    Tool    $DB04
                    <<<
~qQDStartUp         MAC
                    NextDP  ]1;$300
                    PxW     ]2;]3;]4
                    Tool    $204
                    <<<

