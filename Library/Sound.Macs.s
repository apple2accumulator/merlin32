* Sound manager macros
*   by Dave Klimas
;
; Copyright Apple Computer, Inc. 1986, 1987
; and Roger Wagner Publishing, Inc. 1988
; All Rights Reserved
;
_SoundBootInit      MAC
                    Tool    $108
                    <<<
~SoundStartUp       MAC
                    PHW     ]1
_SoundStartUp       MAC
                    Tool    $208
                    <<<
_SoundShutDown      MAC
                    Tool    $308
                    <<<
~SoundVersion       MAC
                    PHA
_SoundVersion       MAC
                    Tool    $408
                    <<<
_SoundReset         MAC
                    Tool    $508
                    <<<
~SoundToolStatus    MAC
                    PHA
_SoundToolStatus    MAC
                    Tool    $608
                    <<<
~WriteRamBlock      MAC
                    PHL     ]1
                    PxW     ]2;]3
_WriteRamBlock      MAC
                    Tool    $908
                    <<<
~ReadRamBlock       MAC
                    PHL     ]1
                    PxW     ]2;]3
_ReadRamBlock       MAC
                    Tool    $A08
                    <<<
~GetTableAddress    MAC
                    PHS     2
_GetTableAddress    MAC
                    Tool    $B08
                    <<<
~GetSoundVolume     MAC
                    P1SW    ]1
_GetSoundVolume     MAC
                    Tool    $C08
                    <<<
~SetSoundVolume     MAC
                    PxW     ]1;]2
_SetSoundVolume     MAC
                    Tool    $D08
                    <<<
~FFStartSound       MAC
                    PHWL    ]1;]2
_FFStartSound       MAC
                    Tool    $E08
                    <<<
~FFStopSound        MAC
                    PHW     ]1
_FFStopSound        MAC
                    Tool    $F08
                    <<<
~FFSoundStatus      MAC
                    PHA
_FFSoundStatus      MAC
                    Tool    $1008
                    <<<
~FFGeneratorStatus  MAC
                    P1SW    ]1
_FFGeneratorStatus  MAC
                    Tool    $1108
                    <<<
~SetSoundMIRQV      MAC
                    PHL     ]1
_SetSoundMIRQV      MAC
                    Tool    $1208
                    <<<
~SetUserSoundIRQV   MAC
                    P2SL    ]1
_SetUserSoundIRQV   MAC
                    Tool    $1308
                    <<<
~FFSoundDoneStatus  MAC
                    P1SW    ]1
_FFSoundDoneStatus  MAC
                    Tool    $1408
                    <<<
_FFSetUpSound       MAC
                    Tool    $1508
                    <<<
_FFStartPlaying     MAC
                    Tool    $1608
                    <<<
_SetDocReg          MAC
                    Tool    $1708
                    <<<
_ReadDocReg         MAC
                    Tool    $1808
                    <<<

~qSoundStartUp      MAC
                    NextDP  ]1;$100
                    Tool    $208
                    <<<

