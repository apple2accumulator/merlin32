* Note sequencer macros
*   by Dave Klimas
;
; Copyright Apple Computer, Inc. 1986, 1987
; and Roger Wagner Publishing, Inc. 1988
; All Rights Reserved
;
_SeqBootInit     MAC
                 Tool  $011A
                 <<<
~SeqStartUp      MAC
                 PxW   ]1;]2;]3;]4
_SeqStartUp      MAC
                 Tool  $021A
                 <<<
_SeqShutDown     MAC
                 Tool  $031A
                 <<<
~SeqVersion      MAC
                 PHA
_SeqVersion      MAC
                 Tool  $041A
                 <<<
_SeqReset        MAC
                 Tool  $051A
                 <<<
~SeqStatus       MAC
                 PHA
_SeqStatus       MAC
                 Tool  $061A
                 <<<
~SetIncr         MAC
                 PHW   ]1
_SetIncr         MAC
                 Tool  $091A
                 <<<
~ClearIncr       MAC
                 PHA
_ClearIncr       MAC
                 Tool  $0A1A
                 <<<
~GetTimer        MAC
                 PHA
_GetTimer        MAC
                 Tool  $0B1A
                 <<<
~GetLoc          MAC
                 PHS   3
_GetLoc          MAC
                 Tool  $0C1A
                 <<<
_SeqAllNotesOff  MAC
                 Tool  $0D1A
                 <<<
~SetTrkInfo      MAC
                 PxW   ]1;]2;]3
_SetTrkInfo      MAC
                 Tool  $0E1A
                 <<<
~StartSeq        MAC
                 PxL   ]1;]2;]3
_StartSeq        MAC
                 Tool  $0F1A
                 <<<
_StepSeq         MAC
                 Tool  $101A
                 <<<
~StopSeq         MAC
                 PHW   ]1
_StopSeq         MAC
                 Tool  $111A
                 <<<
~SetInstTable    MAC
                 PHL   ]1
_SetInstTable    MAC
                 Tool  $121A
                 <<<
_StartInts       MAC
                 Tool  $131A
                 <<<
_StopInts        MAC
                 Tool  $141A
                 <<<
~StartSeqRel     MAC
                 PxL   ]1;]2;]3
_StartSeqRel     MAC
                 Tool  $151A
                 <<<

