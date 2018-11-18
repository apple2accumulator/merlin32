*-----------------------------
* LoGo's useful tools...
*-----------------------------

_En8        mac
            sec
            xce
            sep       #$30
            <<<

_En16       mac
            clc
            xce
            rep       #$30
            <<<

_ClrScr     mac
            ldx       #$7ffe
            lda       #$0000
]lp         stal      $e12000,x
            stal      $012000,x
            dex
            dex
            bpl       ]lp
            <<<

_fadeIN     mac                   ; Fait un fondu de l'image
            lda       ]1          ;   A= banc/adrh de l'image
            ldy       ]2          ;   Y= $0000, fondu sur l'image
            jsr       fadeIN      ;      $ffff, que sur les palettes
            <<<

_fadeOUT    mac                   ; Efface l'ecran doucement
            jsr       fadeOUT
            <<<

_File       mac                   ; Charge un fichier
            lda       ]1
            ldx       ]2
            jsr       loadFILE
            <<<

_Key        mac                   ; Attend une touche au clavier
]lp         ldal      $e0bfff
            bpl       ]lp
            stal      $e0c010
            <<<

_Unpack     mac                   ; Decompacte un fichier
            lda       ]1          ;    A= banc/adrh du fichier source
            jsr       unPACK
            <<<

_wait       mac                   ; Routine d'attente
            lda       ]1          ;    A= duree d'attente (env. 1 seconde)
            jsr       nowWAIT
            eom

_Write8     mac                   ; Affiche un message
            lda       ]1          ;    A= adresse de la chaine
            ldx       ]2          ;    X= coordonnee sur l'ecran
            ldy       ]3          ;    Y= banc/adrh ou afficher
            jsr       Print8
            <<<

_Write16    mac                   ; Affiche un message
            lda       ]1          ;    A= adresse de la chaine
            ldx       ]2          ;    X= coordonnee sur l'ecran
            ldy       ]3          ;    Y= banc/adrh ou afficher
            jsr       Print16
            <<<

_Reset      mac
            lda       #$51
            sta       $0
            PushWord  #2
            PushWord  #0
            PushWord  #0
            PushWord  #8
            Tool      $0909
            sec
            xce
            lda       #0
            stal      $0003f4
            jmp       ($fffc)
            <<<

