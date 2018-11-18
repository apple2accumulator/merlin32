* MIDI tool macros
*   by Dave Klimas
;
; Copyright Apple Computer, Inc. 1986, 1987
; and Roger Wagner Publishing, Inc. 1988
; All Rights Reserved
;

_MidiBootInit     MAC
                  Tool  $120
                  <<<
~MidiStartUp      MAC
                  PxW   ]1;]2
_MidiStartUp      MAC
                  Tool  $220
                  <<<
_MidiShutDown     MAC
                  Tool  $320
                  <<<
~MidiVersion      MAC
                  PHA
_MidiVersion      MAC
                  Tool  $420
                  <<<
_MidiReset        MAC
                  Tool  $520
                  <<<
~MidiStatus       MAC
                  PHA
_MidiStatus       MAC
                  Tool  $620
                  <<<
~MidiControl      MAC
                  PHWL  ]1;]2
_MidiControl      MAC
                  Tool  $920
                  <<<
~MidiDevice       MAC
                  PHWL  ]1;]2
_MidiDevice       MAC
                  Tool  $A20
                  <<<
~MidiClock        MAC
                  PHWL  ]1;]2
_MidiClock        MAC
                  Tool  $B20
                  <<<
~MidiInfo         MAC
                  P2SW  ]1
_MidiInfo         MAC
                  Tool  $C20
                  <<<
~MidiReadPacket   MAC
                  P1SL  ]1
                  PHW   ]2
_MidiReadPacket   MAC
                  Tool  $D20
                  <<<
~MidiWritePacket  MAC
                  P1SL  ]1
_MidiWritePacket  MAC
                  Tool  $E20
                  <<<
_MidiRecordSeq    MAC
                  Tool  $F20
                  <<<
_MidiStopRecord   MAC
                  Tool  $1020
                  <<<
_MidiPlaySeq      MAC
                  Tool  $1120
                  <<<
_MidiStopPlay     MAC
                  Tool  $1220
                  <<<
_MidiConvert      MAC
                  Tool  $1320
                  <<<

