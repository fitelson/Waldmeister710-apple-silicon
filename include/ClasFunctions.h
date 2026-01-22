/**********************************************************************
* Filename : ClasFunctions
*
* Kurzbeschreibung : Funktionen zur Classifikation, LowLevel Routinen
*                    wird von GLAM und ClasHeuristics benutzt
* Autoren : Andreas Jaeger
*
* 10.03.98: Erstellung
*
*
*
**********************************************************************/

#ifndef CLASFUNCTIONS
#define CLASFUNCTIONS

#include "compiler-extra.h"
#include "RUndEVerwaltung.h"
#include "TermOperationen.h"

/* Berechnen des gewichteten Gewichts von term */
WeightT CF_Phi (TermT term, BOOLEAN isCP);

/* Berechnen des gewichteten Gewichts von term (KBO Gewicht) */
WeightT CF_Phi_KBO (TermT term);

/* gewichteter Term, Skoleminhalte auf 0 */
WeightT CF_GolemGewicht (TermT Term, BOOLEAN isCP);

int CF_AnzahlVariablen (TermT s, TermT t);

/* Kriterien */
/* Kriterien haben alle diesen Prototypen */
typedef BOOLEAN (* CriteriaProcType) (TermT, TermT, RegelOderGleichungsT, RegelOderGleichungsT);

/* ist Termpaar richtbar und kann RE reduzieren */
BOOLEAN CF_KillerPraedikatR (TermT lhs, TermT rhs,
			     RegelOderGleichungsT actualParent, RegelOderGleichungsT otherParent);

/* ist Termpaar unrichtbar und kann RE reduzieren */
BOOLEAN CF_KillerPraedikatE (TermT lhs, TermT rhs,
			     RegelOderGleichungsT actualParent, RegelOderGleichungsT otherParent);

/* kann Termpaar RE reduzieren */
BOOLEAN CF_KillerPraedikatRE (TermT lhs, TermT rhs,
			      RegelOderGleichungsT actualParent, RegelOderGleichungsT otherParent);

/* Kind von Gleichung */
BOOLEAN CF_EChildPraedikat (TermT lhs MAYBE_UNUSEDPARAM, TermT rhs MAYBE_UNUSEDPARAM, 
			    RegelOderGleichungsT actualParent, RegelOderGleichungsT otherParent);
  
void CF_PolyWeightInitialisieren(void);

BOOLEAN CF_PolyZuGross(TermT s, TermT t);

BOOLEAN CF_PolyZuGross2(TermT s, TermT t);

unsigned int CF_Generation(RegelOderGleichungsT fadder, RegelOderGleichungsT mudder);

#endif 
