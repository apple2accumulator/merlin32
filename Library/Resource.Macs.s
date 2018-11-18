* Resource macros
*   by Dave Klimas
;
; Copyright Apple Computer, Inc. 1986, 1987
; and Roger Wagner Publishing, Inc. 1988
; All Rights Reserved
;
_ResourceBootInit      MAC
                       Tool  $11E
                       <<<
~ResourceStartUp       MAC
                       PHW   ]1
_ResourceStartUp       MAC
                       Tool  $21E
                       <<<
_ResourceShutDown      MAC
                       Tool  $31E
                       <<<
~ResourceVersion       MAC
                       PHA
_ResourceVersion       MAC
                       Tool  $41E
                       <<<
_ResourceReset         MAC
                       Tool  $51E
                       <<<
~ResourceStatus        MAC
                       PHA
_ResourceStatus        MAC
                       Tool  $61E
                       <<<
~CreateResourceFile    MAC
                       PHLW  ]1;]2
                       PHWL  ]3;]4
_CreateResourceFile    MAC
                       Tool  $91E
                       <<<
~OpenResourceFile      MAC
                       P1SW  ]1
                       PxL   ]2;]3
_OpenResourceFile      MAC
                       Tool  $A1E
                       <<<
~CloseResourceFile     MAC
                       PHW   ]1
_CloseResourceFile     MAC
                       Tool  $B1E
                       <<<
~AddResource           MAC
                       PHLW  ]1;]2
                       PHWL  ]3;]4
_AddResource           MAC
                       Tool  $C1E
                       <<<
~UpdateResourceFile    MAC
                       PHW   ]1
_UpdateResourceFile    MAC
                       Tool  $D1E
                       <<<
~LoadResource          MAC
                       P2SW  ]1
                       PHL   ]2
_LoadResource          MAC
                       Tool  $E1E
                       <<<
~RemoveResource        MAC
                       PHWL  ]1;]2
_RemoveResource        MAC
                       Tool  $F1E
                       <<<
~MarkResourceChange    MAC
                       PxW   ]1;]2
                       PHL   ]3
_MarkResourceChange    MAC
                       Tool  $101E
                       <<<
~SetCurResourceFile    MAC
                       PHW   ]1
_SetCurResourceFile    MAC
                       Tool  $111E
                       <<<
~GetCurResourceFile    MAC
                       PHA
_GetCurResourceFile    MAC
                       Tool  $121E
                       <<<
~SetCurResourceApp     MAC
                       PHW   ]1
_SetCurResourceApp     MAC
                       Tool  $131E
                       <<<
~GetCurResourceApp     MAC
                       PHA
_GetCurResourceApp     MAC
                       Tool  $141E
                       <<<
~HomeResourceFile      MAC
                       P1SW  ]1
                       PHL   ]2
_HomeResourceFile      MAC
                       Tool  $151E
                       <<<
~WriteResource         MAC
                       PHWL  ]1;]2
_WriteResource         MAC
                       Tool  $161E
                       <<<
~ReleaseResource       MAC
                       PxW   ]1;]2
                       PHL   ]3
_ReleaseResource       MAC
                       Tool  $171E
                       <<<
~DetachResource        MAC
                       PHWL  ]1;]2
_DetachResource        MAC
                       Tool  $181E
                       <<<
~UniqueResourceID      MAC
                       P2SW  ]1
                       PHW   ]2
_UniqueResourceID      MAC
                       Tool  $191E
                       <<<
~SetResourceID         MAC
                       PHLW  ]1;]2
                       PHL   ]3
_SetResourceID         MAC
                       Tool  $1A1E
                       <<<
~GetResourceAttr       MAC
                       P1SW  ]1
                       PHL   ]2
_GetResourceAttr       MAC
                       Tool  $1B1E
                       <<<
~SetResourceAttr       MAC
                       PxW   ]1;]2
                       PHL   ]3
_SetResourceAttr       MAC
                       Tool  $1C1E
                       <<<
~GetResourceSize       MAC
                       P2SW  ]1
                       PHL   ]2
_GetResourceSize       MAC
                       Tool  $1D1E
                       <<<
~MatchResourceHandle   MAC
                       PxL   ]1;]2
_MatchResourceHandle   MAC
                       Tool  $1E1E
                       <<<
~GetOpenFileRefNum     MAC
                       P1SW  ]1
_GetOpenFileRefNum     MAC
                       Tool  $1F1E
                       <<<
~CountTypes            MAC
                       PHA
_CountTypes            MAC
                       Tool  $201E
                       <<<
~GetIndType            MAC
                       P1SW  ]1
_GetIndType            MAC
                       Tool  $211E
                       <<<
~CountResources        MAC
                       P2SW  ]1
_CountResources        MAC
                       Tool  $221E
                       <<<
~GetIndResource        MAC
                       P2SW  ]1
                       PHL   ]2
_GetIndResource        MAC
                       Tool  $231E
                       <<<
~SetResourceLoad       MAC
                       P1SW  ]1
_SetResourceLoad       MAC
                       Tool  $241E
                       <<<
~SetResourceFileDepth  MAC
                       P1SW  ]1
_SetResourceFileDepth  MAC
                       Tool  $251E
                       <<<
~GetMapHandle          MAC
                       P2SW  ]1
_GetMapHandle          MAC
                       Tool  $261E
                       <<<
~LoadAbsResource       MAC
                       P2SL  ]1
                       PHL   ]2
                       PHWL  ]3;]4
_LoadAbsResource       MAC
                       Tool  $271E
                       <<<
~ResourceConverter     MAC
                       PHL   ]1
                       PxW   ]2;]3
_ResourceConverter     MAC
                       Tool  $281E
                       <<<
_LoadResource2         MAC
                       Tool  $291E
                       <<<
_RMFindeNamedResource  MAC
                       Tool  $2A1E
                       <<<
_RMGetResourceName     MAC
                       Tool  $2B1E
                       <<<
_RMLoadNamedResource   MAC
                       Tool  $2C1E
                       <<<
_RMSetResourceName     MAC
                       Tool  $2D1E
                       <<<
_OpenResourceFileByID  MAC
                       Tool  $2E1E
                       <<<
_CompactResourceFile   MAC
                       Tool  $2F1E
                       <<<

