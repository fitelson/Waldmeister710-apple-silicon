#ifndef ZWANGSGRUND_H
#define ZWANGSGRUND_H

#include "general.h"
#include "TermOperationen.h"

/* Ordnungen, die die Terme als implizit skolemisiert
 * ansehen und somit total sind.
 *
 * Praezedenz wie eingestellt, erweitert um
 *   x_{i+1}  <  x_i  <  f,  f Fkt-Symbol
 * 
 * Die Terme duerfen keine Extrakonstanten enthalten!
 *
 * Bernd Loechner, 20.06.03
 */

BOOLEAN ZGO_LPOGroesser(UTermT Term1, UTermT Term2);
  
VergleichsT ZGO_LPO(UTermT Term1, UTermT Term2);

BOOLEAN ZGO_KBOGroesser(UTermT term1, UTermT term2);

VergleichsT ZGO_KBOVergleich(UTermT term1, UTermT term2);

#endif
