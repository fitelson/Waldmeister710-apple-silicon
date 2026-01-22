#ifndef SYMBOLGEWICHTE_H
#define SYMBOLGEWICHTE_H

#include "InfoStrings.h"
#include "general.h"
#include "SymbolOperationen.h"

typedef long int *SG_GewichtstafelT;

extern SG_GewichtstafelT Sg_CG_Tafel;
extern SG_GewichtstafelT Sg_CP_Tafel;
extern SG_GewichtstafelT Sg_KBO_Tafel;

extern long int Sg_KBO_Mu_Gewicht;
#define SG_KBO_Mu_Gewicht() (Sg_KBO_Mu_Gewicht)

#define SG_SymbolGewicht(/* SG_GewichtstafelT */tafel, /*SymbolT*/ symbol)\
  ((SO_SymbIstVar(symbol)||SO_IstExtrakonstante(symbol)) ? (tafel)[SO_ErstesVarSymb] : (tafel)[(symbol)])

#define SG_SymbolGewichtCP(symbol)  SG_SymbolGewicht(Sg_CP_Tafel,(symbol))
#define SG_SymbolGewichtCG(symbol)  SG_SymbolGewicht(Sg_CG_Tafel,(symbol))
#define SG_SymbolGewichtKBO(symbol) SG_SymbolGewicht(Sg_KBO_Tafel,(symbol))

void SG_SymbGewichteEintragen(InfoStringT DEF, InfoStringT USR, int welcheTafel);
/* Aufdroeseln des Aufruf-Parameters nach Default-, Variablen- und den einzelnen Funktionsgewichten.
   welcheTafel = 0: Sg_CG_Tafel
   welcheTafel = 1: Sg_CP_Tafel
   welcheTafel = 2: Sg_KBO_Tafel
*/

void SG_KBOGewichteVonSpecLesen(void);

void SG_CleanUp(void);

#endif
