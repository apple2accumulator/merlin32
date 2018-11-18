* Scrap manager macros
*   by Dave Klimas
;
; Copyright Apple Computer, Inc. 1986, 1987
; and Roger Wagner Publishing, Inc. 1988
; All Rights Reserved
;
_ScrapBootInit   MAC
                 Tool  $116
                 <<<
_ScrapStartUp    MAC
                 Tool  $216
                 <<<
_ScrapShutDown   MAC
                 Tool  $316
                 <<<
~ScrapVersion    MAC
                 PHA
_ScrapVersion    MAC
                 Tool  $416
                 <<<
_ScrapReset      MAC
                 Tool  $516
                 <<<
~ScrapStatus     MAC
                 PHA
_ScrapStatus     MAC
                 Tool  $616
                 <<<
_UnloadScrap     MAC
                 Tool  $916
                 <<<
_LoadScrap       MAC
                 Tool  $A16
                 <<<
_ZeroScrap       MAC
                 Tool  $B16
                 <<<
~PutScrap        MAC
                 PHL   ]1
                 PHWL  ]2;]3
_PutScrap        MAC
                 Tool  $C16
                 <<<
~GetScrap        MAC
                 PHLW  ]1;]2
_GetScrap        MAC
                 Tool  $D16
                 <<<
~GetScrapHandle  MAC
                 P2SW  ]1
_GetScrapHandle  MAC
                 Tool  $E16
                 <<<
~GetScrapSize    MAC
                 P2SW  ]1
_GetScrapSize    MAC
                 Tool  $F16
                 <<<
~GetScrapPath    MAC
                 PHS   2
_GetScrapPath    MAC
                 Tool  $1016
                 <<<
~SetScrapPath    MAC
                 PHL   ]1
_SetScrapPath    MAC
                 Tool  $1116
                 <<<
~GetScrapCount   MAC
                 PHA
_GetScrapCount   MAC
                 Tool  $1216
                 <<<
~GetScrapState   MAC
                 PHA
_GetScrapState   MAC
                 Tool  $1316
                 <<<
_GetIndScrap     MAC
                 Tool  $1416
                 <<<
_ShowClipboard   MAC
                 Tool  $1516
                 <<<

