/* **************************************************************************
 * ZapfAnlage.h  Dispatcher für die verschiedenen Zapfen
 * 
 * Die ZapfAnlage verwaltet die verschiedenen Aktiven Zapfen in zwei
 * verschiedenen Queues: high und low
 * Solange in einer der Zapfen der high-Queue noch Fakten verfügbar sind, 
 * werden diese selektiert. Wenn dies nicht der Fall ist, wird ein Zapfen 
 * aus der low-Queue ausgewählt, der das nächste Faktum liefert.
 * 
 * Jean-Marie Gaillourdet, 18.08.2003
 *
 * ************************************************************************** */

#ifndef ZAPFANLAGE_H
#define ZAPFANLAGE_H

#include "Id.h"
#include "RUndEVerwaltung.h"
#include "Zapfhahn.h"

void ZPA_Initialisieren(void);
void ZPA_ZapfhahnAnmelden( ZapfhahnT z, BOOLEAN bevorzugt, int Gewicht, int welcherRing );
void ZPA_ZapfhahnAbmelden( ZapfhahnT z );
void ZPA_Aufrauemen(void);

BOOLEAN ZPA_Selektiere( SelectRecT* selRec );
BOOLEAN ZPA_BereitsSelektiert( IdMitTermenT tid );
BOOLEAN ZPA_BereitsSelektiertWId( WId* id );

void ZPA_Weiterdrehen(void);

#endif /* ZAPFANLAGE_H */
