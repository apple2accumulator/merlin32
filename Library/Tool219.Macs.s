* SoundSmith Tool
* FTA, 1991

~STStartUp          mac
                    PHW   ]1
_STStartUp          mac
                    Tool  $02DB
                    <<<
_STShutDown         mac
                    Tool  $03DB
                    <<<
~STVersion          mac
                    phd               ; WordResult
_STVersion          mac
                    Tool  $04DB
                    <<<
_STReset            mac
                    Tool  $05DB
                    <<<
~STStatus           mac
                    phd               ; WordResult
STStatus            mac
                    Tool  $06DB
                    <<<
~STLoadOneMusic     mac
                    PHL   ]1
_STLoadOneMusic     mac
                    Tool  $09DB
                    <<<
~STPlayMusic        mac
                    PHW   ]1
_STPlayMusic        mac
                    Tool  $0ADB
                    <<<
_STStopMusic        mac
                    Tool  $0BDB
                    <<<
~STGetEOfMusic      mac
                    phd               ; WordResult
_STGetEOfMusic      mac
                    Tool  $0CDB
                    <<<
~STAddToBatch       mac
                    PHLW  ]1;]2
_STAddToBatch       mac
                    Toll  $0DDB
                    <<<
~STSelectBatch      mac
                    PHW   ]1
_STSelectBatch      mac
                    Tool  $0EDB
                    <<<
~STKillBatch        mac
                    PHW   ]1
_STKillBatch        mac
                    Tool  $0FDB
                    <<<
~STGetPlayingMusic  mac
                    phd               ; WordResult
_STGetPlayingMusic  mac
                    Tool  $10DB
                    <<<
~STPlayBatch        mac
                    PHL   ]1
_STPlayBatch        mac
                    Tool  $11DB
                    <<<
~STGetTrackVu       mac
                    phd               ;Long
                    phd               ;    Result
_STGetTrackVu       mac
                    Tool  $12DB
                    <<<
_STPauseMusic       mac
                    Tool  $13DB
                    <<<
_STContinueMusic    mac
                    Tool  $14DB
                    <<<
_STinternal15       mac
                    Tool  $15DB
                    <<<
_STinternal16       mac
                    Tool  $16DB
                    <<<

