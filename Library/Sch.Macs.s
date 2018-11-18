* Scheduler macros
*   by Dave Klimas
;
; Copyright Apple Computer, Inc. 1986, 1987
; and Roger Wagner Publishing, Inc. 1988
; All Rights Reserved
;
_SchBootInit  MAC
              Tool  $107
              <<<
_SchStartUp   MAC
              Tool  $207
              <<<
_SchShutDown  MAC
              Tool  $307
              <<<
~SchVersion   MAC
              PHA
_SchVersion   MAC
              Tool  $407
              <<<
_SchReset     MAC
              Tool  $507
              <<<
~SchStatus    MAC
              PHA
_SchStatus    MAC
              Tool  $607
              <<<
~SchAddTask   MAC
              P1SL  ]1
_SchAddTask   MAC
              Tool  $907
              <<<
_SchFlush     MAC
              Tool  $A07
              <<<

