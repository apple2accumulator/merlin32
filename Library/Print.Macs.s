* Print Mgr. macros
*   by Dave Klimas
;
; Copyright Apple Computer, Inc. 1986, 1987
; and Roger Wagner Publishing, Inc. 1988
; All Rights Reserved
;
_PMBootInit            MAC
                       Tool  $0113
                       <<<
~PMStartUp             MAC
                       PxW   ]1;]2
_PMStartUp             MAC
                       Tool  $0213
                       <<<
_PMShutDown            MAC
                       Tool  $0313
                       <<<
~PMVersion             MAC
                       PHA
_PMVersion             MAC
                       Tool  $0413
                       <<<
_PMReset               MAC
                       Tool  $0513
                       <<<
~PMStatus              MAC
                       PHA
_PMStatus              MAC
                       Tool  $0613
                       <<<
~PrDefault             MAC
                       PHL   ]1
_PrDefault             MAC
                       Tool  $0913
                       <<<
~PrValidate            MAC
                       P1SL  ]1
_PrValidate            MAC
                       Tool  $0A13
                       <<<
~PrStlDialog           MAC
                       P1SL  ]1
_PrStlDialog           MAC
                       Tool  $0B13
                       <<<
~PrJobDialog           MAC
                       P1SL  ]1
_PrJobDialog           MAC
                       Tool  $0C13
                       <<<
~PrPixelMap            MAC
                       PxL   ]1;]2
                       PHW   ]3
_PrPixelMap            MAC
                       Tool  $0D13
                       <<<
~PrOpenDoc             MAC
                       P2SL  ]1
                       PHL   ]2
_PrOpenDoc             MAC
                       Tool  $0E13
                       <<<
~PrCloseDoc            MAC
                       PHL   ]1
_PrCloseDoc            MAC
                       Tool  $0F13
                       <<<
~PrOpenPage            MAC
                       PxL   ]1;]2
_PrOpenPage            MAC
                       Tool  $1013
                       <<<
~PrClosePage           MAC
                       PHL   ]1
_PrClosePage           MAC
                       Tool  $1113
                       <<<
~PrPicFile             MAC
                       PxL   ]1;]2;]3
_PrPicFile             MAC
                       Tool  $1213
                       <<<
_PrControl             MAC
                       Tool  $1313
                       <<<
~PrError               MAC
                       PHA
_PrError               MAC
                       Tool  $1413
                       <<<
~PrSetError            MAC
                       PHW   ]1
_PrSetError            MAC
                       Tool  $1513
                       <<<
~PrChoosePrinter       MAC
                       PHA
_PrChoosePrinter       MAC
                       Tool  $1613
                       <<<
_GetDeviceName         MAC
                       Tool  $1703
                       <<<
~PrGetPrinterSpecs     MAC
                       PHS   2
_PrGetPrinterSpecs     MAC
                       Tool  $1813
                       <<<
_PrDevPrChanged        MAC
                       Tool  $1913
                       <<<
_PrDevStartup          MAC
                       Tool  $1A13
                       <<<
_PrDevShutDown         MAC
                       Tool  $1B13
                       <<<
_PrDevOpen             MAC
                       Tool  $1C13
                       <<<
_PrDevRead             MAC
                       Tool  $1D13
                       <<<
_PrDevWrite            MAC
                       Tool  $1E13
                       <<<
_PrDevClose            MAC
                       Tool  $1F13
                       <<<
_PrDevStatus           MAC
                       Tool  $2013
                       <<<
_PrDevAsyncRead        MAC
                       Tool  $2113
                       <<<
_PrDevWriteBackground  MAC
                       Tool  $2213
                       <<<
~PrDriverVer           MAC
                       PHA
_PrDriverVer           MAC
                       Tool  $2313
                       <<<
~PrPortVer             MAC
                       PHA
_PrPortVer             MAC
                       Tool  $2413
                       <<<
~PrGetZoneName         MAC
                       PHS   2
_PrGetZoneName         MAC
                       Tool  $2513
                       <<<
~PrGetPrinterDvrName   MAC
                       PHS   2
_PrGetPrinterDvrName   MAC
                       Tool  $2813
                       <<<
~PrGetPortDvrName      MAC
                       PHS   2
_PrGetPortDvrName      MAC
                       Tool  $2913
                       <<<
~PrGetUserName         MAC
                       PHS   2
_PrGetUserName         MAC
                       Tool  $2A13
                       <<<
~PrGetNetworkName      MAC
                       PHS   2
_PrGetNetworkName      MAC
                       Tool  $2B13
                       <<<
_PrDevIsItSafe         MAC
                       Tool  $3013
                       <<<
_GetZoneList           MAC
                       Tool  $3113
                       <<<
_GetMyZone             MAC
                       Tool  $3213
                       <<<
_GetPrinterList        MAC
                       Tool  $3313
                       <<<
~PMUnloadDriver        MAC
                       PHW   ]1
_PMUnloadDriver        MAC
                       Tool  $3413
                       <<<
~PMLoadDriver          MAC
                       PHW   ]1
_PMLoadDriver          MAC
                       Tool  $3513
                       <<<
~PrGetDocName          MAC
                       PHS   2
_PrGetDocName          MAC
                       Tool  $3613
                       <<<
~PrSetDocName          MAC
                       PHL   ]1
_PrSetDocName          MAC
                       Tool  $3713
                       <<<
~PrGetPgOrientation    MAC
                       P1SL  ]1
_PrGetPgOrientation    MAC
                       Tool  $3813
                       <<<
~qPMStartUp            MAC
                       PHW   ]1
                       CLC
                       LDA   ]2
                       ADC   ]NextDP
                       PHA
                       Tool  $0213
                       <<<

