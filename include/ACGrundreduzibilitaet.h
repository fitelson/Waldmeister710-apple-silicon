#ifndef ACGRUNDREDUZIBILITAET_H
#define ACGRUNDREDUZIBILITAET_H

/* ACGrundreduzibilitaet.h
 * 23.10.2002
 * Christian Schmidt
 */

#include "general.h"
#include "TermOperationen.h"
#include "RUndEVerwaltung.h"

BOOLEAN AC_FaktumGrundreduzibel(RegelOderGleichungsT Objekt);
BOOLEAN AC_FaktumStrengKonsistent(RegelOderGleichungsT Objekt);
BOOLEAN AC_TermeGrundreduzibel(UTermT Term1, UTermT Term2);
BOOLEAN AC_TermeStrengKonsistent(UTermT Term1, UTermT Term2);

BOOLEAN AC_Reduzibel(UTermT t);

void AC_lohntSich(void);
#endif
