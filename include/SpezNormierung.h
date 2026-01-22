#ifndef SPEZNORMIERUNG_H
#define SPEZNORMIERUNG_H

/* -------------------------------------------------------*/
/* --------------- includes ------------------------------*/
/* -------------------------------------------------------*/


#include "general.h"
#include "parser.h"

extern int SN_AnzahlGleichungen,
           SN_AnzahlZiele;

extern int *SN_PosVon;

#define SN_WMvonDISCOUNT(symbol) ((SymbolT) SN_PosVon[symbol-1])

extern term *SN_LHS, *SN_RHS;

extern struct SN_ZieleS {
  term l, r;
} *SN_Ziele;

/* Linke und rechte Seiten der Gleichungen im Discount-Format, ggf. spezifikationsnormiert. */

void SN_SpezifikationAuswerten(BOOLEAN Normiere);
/* Besetzt obige Variablen. Aufruf darf erst nach dem Einparsen der Spezifikation erfolgen! */

#endif
