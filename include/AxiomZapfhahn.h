/* **************************************************************************
 * AxiomZapfhahn.h  Aufzählung aller Axiome
 * 
 * Jean-Marie Gaillourdet, 21.08.2003
 *
 * ************************************************************************** */

#ifndef AXIOM_ZAPFHAHN_H
#define AXIOM_ZAPFHAHN_H

#include "ClasHeuristics.h"
#include "Zapfhahn.h"


void AXZ_mkZapfhahn( ZapfhahnT z, BOOLEAN ziele, ClassifyProcT classify );
void AXZ_AxiomHinzufuegen( ZapfhahnT z, unsigned long axnummer, TermT linkeSeite,
			   TermT rechteSeite );

#endif /* AXIOM_ZAPFHAHN_H */
