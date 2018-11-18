* File:  MGSOS
*
* Copyright 1989 by Dave Klimas
*
* IMPORTANT!  To use these macros, you must also include the
* label equates as defined in the file E16.GSOS from the TOOL.EQUATES
* directory.
*-----------------------------------------------
*
*    xGSOS <callname>;<recpointer>;class 0/1 flag
*
*    iGSOS _Create;addr;0      ;this is a class 0 Create call
*
*-----------------------------------------------
* Inline GS/OS call macro

iGSOS       MAC
            JSL   $E100A8
            DO    ]3
            DA    ]1.$2000
            ELSE
            DA    ]1
            FIN
            ADRL  ]2
            <<<

*-----------------------------------------------
* Stack GS/OS call macro

sGSOS       MAC
            PHL   ]2
            DO    ]3
            PEA   ]1.$2000
            ELSE
            PEA   ]1
            FIN
            JSL   $E100B0
            <<<

