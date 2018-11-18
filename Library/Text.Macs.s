* Text tool macros
*   by Dave Klimas
;
; Copyright Apple Computer, Inc. 1986, 1987
; and Roger Wagner Publishing, Inc. 1988
; All Rights Reserved
;
_TextBootInit     MAC
                  Tool  $10C
                  <<<
_TextStartUp      MAC
                  Tool  $20C
                  <<<
_TextShutDown     MAC
                  Tool  $30C
                  <<<
~TextVersion      MAC
                  PHA
_TextVersion      MAC
                  Tool  $40C
                  <<<
_TextReset        MAC
                  Tool  $50C
                  <<<
~TextStatus       MAC
                  PHA
_TextStatus       MAC
                  Tool  $60C
                  <<<
~SetInGlobals     MAC
                  PxW   ]1;]2
_SetInGlobals     MAC
                  Tool  $90C
                  <<<
~SetOutGlobals    MAC
                  PxW   ]1;]2
_SetOutGlobals    MAC
                  Tool  $A0C
                  <<<
~SetErrGlobals    MAC
                  PxW   ]1;]2
_SetErrGlobals    MAC
                  Tool  $B0C
                  <<<
~GetInGlobals     MAC
                  PHS   2
_GetInGlobals     MAC
                  Tool  $C0C
                  <<<
~GetOutGlobals    MAC
                  PHS   2
_GetOutGlobals    MAC
                  Tool  $D0C
                  <<<
~GetErrGlobals    MAC
                  PHS   2
_GetErrGlobals    MAC
                  Tool  $E0C
                  <<<
~SetInputDevice   MAC
                  PHWL  ]1;]2
_SetInputDevice   MAC
                  Tool  $F0C
                  <<<
~SetOutputDevice  MAC
                  PHWL  ]1;]2
_SetOutputDevice  MAC
                  Tool  $100C
                  <<<
~SetErrorDevice   MAC
                  PHWL  ]1;]2
_SetErrorDevice   MAC
                  Tool  $110C
                  <<<
~GetInputDevice   MAC
                  PHS   3
_GetInputDevice   MAC
                  Tool  $120C
                  <<<
~GetOutputDevice  MAC
                  PHS   3
_GetOutputDevice  MAC
                  Tool  $130C
                  <<<
~GetErrorDevice   MAC
                  PHS   3
_GetErrorDevice   MAC
                  Tool  $140C
                  <<<
~InitTextDev      MAC
                  PHW   ]1
_InitTextDev      MAC
                  Tool  $150C
                  <<<
~CtlTextDev       MAC
                  PxW   ]1;]2
_CtlTextDev       MAC
                  Tool  $160C
                  <<<
~StatusTextDev    MAC
                  PxW   ]1;]2
_StatusTextDev    MAC
                  Tool  $170C
                  <<<
~WriteChar        MAC
                  PHW   ]1
_WriteChar        MAC
                  Tool  $180C
                  <<<
~ErrWriteChar     MAC
                  PHW   ]1
_ErrWriteChar     MAC
                  Tool  $190C
                  <<<
~WriteLine        MAC
                  PHL   ]1
_WriteLine        MAC
                  Tool  $1A0C
                  <<<
~ErrWriteLine     MAC
                  PHL   ]1
_ErrWriteLine     MAC
                  Tool  $1B0C
                  <<<
~WriteString      MAC
                  PHL   ]1
_WriteString      MAC
                  Tool  $1C0C
                  <<<
~ErrWriteString   MAC
                  PHL   ]1
_ErrWriteString   MAC
                  Tool  $1D0C
                  <<<
~TextWriteBlock   MAC
                  PHL   ]1
                  PxW   ]2;]3
_TextWriteBlock   MAC
                  Tool  $1E0C
                  <<<
~ErrWriteBlock    MAC
                  PHL   ]1
                  PxW   ]2;]3
_ErrWriteBlock    MAC
                  Tool  $1F0C
                  <<<
~WriteCString     MAC
                  PHL   ]1
_WriteCString     MAC
                  Tool  $200C
                  <<<
~ErrWriteCString  MAC
                  PHL   ]1
_ErrWriteCString  MAC
                  Tool  $210C
                  <<<
~ReadChar         MAC
                  P1SW  ]1
_ReadChar         MAC
                  Tool  $220C
                  <<<
~TextReadBlock    MAC
                  PHL   ]1
                  PxW   ]2;]3;]4
_TextReadBlock    MAC
                  Tool  $230C
                  <<<
~ReadLine         MAC
                  P1SL  ]1
                  PxW   ]2;]3;]4
_ReadLine         MAC
                  Tool  $240C
                  <<<

PrintLn           MAC               ; print a line of text
                  pea   ^text
                  pea   text
                  ldx   #$1A0C
                  jsl   $E10000
                  bra   cont
text              str   ]1
cont
                  <<<

