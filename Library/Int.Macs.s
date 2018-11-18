* Integer Math macros
*   by Dave Klimas
;
; Copyright Apple Computer, Inc. 1986, 1987
; and Roger Wagner Publishing, Inc. 1988
; All Rights Reserved
;
_IMBootInit   MAC
              Tool  $10B
              <<<
_IMStartUp    MAC
_IntStartUp   MAC
              Tool  $20B
              <<<
_IMShutDown   MAC
_IntShutDown  MAC
              Tool  $30B
              <<<
~IMVersion    MAC
              PHA
_IMVersion    MAC
              Tool  $40B
              <<<
_IMReset      MAC
              Tool  $50B
              <<<
~IMStatus     MAC
              PHA
_IMStatus     MAC
              Tool  $60B
              <<<
~Multiply     MAC
              PHS   2
              PxW   ]1;]2
_Multiply     MAC
              Tool  $90B
              <<<
~SDivide      MAC
              PHS   2
              PxW   ]1;]2
_SDivide      MAC
              Tool  $A0B
              <<<
~UDivide      MAC
              PHS   2
              PxW   ]1;]2
_UDivide      MAC
              Tool  $B0B
              <<<
~LongMul      MAC
              PHS   4
              PxL   ]1;]2
_LongMul      MAC
              Tool  $C0B
              <<<
~LongDivide   MAC
              PHS   4
              PxL   ]1;]2
_LongDivide   MAC
              Tool  $D0B
              <<<
~FixRatio     MAC
              PHS   2
              PxW   ]1;]2
_FixRatio     MAC
              Tool  $E0B
              <<<
~FixMul       MAC
              PHS   2
              PxL   ]1;]2
_FixMul       MAC
              Tool  $F0B
              <<<
~FracMul      MAC
              PHS   2
              PxL   ]1;]2
_FracMul      MAC
              Tool  $100B
              <<<
~FixDiv       MAC
              PHS   2
              PxL   ]1;]2
_FixDiv       MAC
              Tool  $110B
              <<<
~FracDiv      MAC
              PHS   2
              PxL   ]1;]2
_FracDiv      MAC
              Tool  $120B
              <<<
~FixRound     MAC
              P1SL  ]1
_FixRound     MAC
              Tool  $130B
              <<<
~FracSqrt     MAC
              P2SL  ]1
_FracSqrt     MAC
              Tool  $140B
              <<<
~FracCos      MAC
              P2SL  ]1
_FracCos      MAC
              Tool  $150B
              <<<
~FracSin      MAC
              P2SL  ]1
_FracSin      MAC
              Tool  $160B
              <<<
~FixATan2     MAC
              PHS   2
              PxL   ]1;]2
_FixATan2     MAC
              Tool  $170B
              <<<
~HiWord       MAC
              P1SL  ]1
_HiWord       MAC
              Tool  $180B
              <<<
~LoWord       MAC
              P1SL  ]1
_LoWord       MAC
              Tool  $190B
              <<<
~Long2Fix     MAC
              P2SL  ]1
_Long2Fix     MAC
              Tool  $1A0B
              <<<
~Fix2Long     MAC
              P2SL  ]1
_Fix2Long     MAC
              Tool  $1B0B
              <<<
~Fix2Frac     MAC
              P2SL  ]1
_Fix2Frac     MAC
              Tool  $1C0B
              <<<
~Frac2Fix     MAC
              P2SL  ]1
_Frac2Fix     MAC
              Tool  $1D0B
              <<<
~Fix2X        MAC
              PxL   ]1;]2
_Fix2X        MAC
              Tool  $1E0B
              <<<
~Frac2X       MAC
              PxL   ]1;]2
_Frac2X       MAC
              Tool  $1F0B
              <<<
~X2Fix        MAC
              P2SL  ]1
_X2Fix        MAC
              Tool  $200B
              <<<
~X2Frac       MAC
              P2SL  ]1
_X2Frac       MAC
              Tool  $210B
              <<<
~Int2Hex      MAC
              PHW   ]1
              PHLW  ]2;]3
_Int2Hex      MAC
              Tool  $220B
              <<<
~Long2Hex     MAC
              PxL   ]1;]2
              PHW   ]3
_Long2Hex     MAC
              Tool  $230B
              <<<
~Hex2Int      MAC
              PHA
              PHLW  ]1;]2
_Hex2Int      MAC
              Tool  $240B
              <<<
~Hex2Long     MAC
              PHS   2
              PHLW  ]1;]2
_Hex2Long     MAC
              Tool  $250B
              <<<
~Int2Dec      MAC
              PHWL  ]1;]2
              PxW   ]3;]4
_Int2Dec      MAC
              Tool  $260B
              <<<
~Long2Dec     MAC
              PxL   ]1;]2
              PxW   ]3;]4
_Long2Dec     MAC
              Tool  $270B
              <<<
~Dec2Int      MAC
              P1SL  ]1
              PxW   ]2;]3
_Dec2Int      MAC
              Tool  $280B
              <<<
~Dec2Long     MAC
              P2SL  ]1
              PxW   ]2;]3
_Dec2Long     MAC
              Tool  $290B
              <<<
~HexIt        MAC
              P2SW  ]1
_HexIt        MAC
              Tool  $2A0B
              <<<

