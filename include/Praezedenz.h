#ifndef PRAEZEDENZ_H
#define PRAEZEDENZ_H

#include "general.h"
#include "SymbolOperationen.h"

extern SymbolT PR_minimaleKonstante;

extern BOOLEAN PR_PraezedenzTotal;

VergleichsT PR_Praezedenz(SymbolT f, SymbolT g);

#define /* BOOLEAN */ PR_SymbGroesser(symbol1, symbol2) (PR_Praezedenz(symbol1,symbol2)==Groesser)

/*=================================================================================================================*/
/*= VII. Praezedenz aus Kommandozeile lesen und auswerten                                                         =*/
/*=================================================================================================================*/

BOOLEAN PR_PraezedenzKorrekt(char *ps);

BOOLEAN PR_PraezedenzErneuert(char *ps);

void PR_PraezedenzVonSpecLesen(void);

void PR_CleanUp(void);

void PR_PraezedenzAusgeben(void);

#endif
