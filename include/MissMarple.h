#ifndef MISSMARPLE_H
#define MISSMARPLE_H

#include "RUndEVerwaltung.h"
#include "SymbolOperationen.h"

void MM_untersuchen(RegelOderGleichungsT re);

BOOLEAN MM_AC_aktiviert(SymbolT f);
BOOLEAN MM_ACC_aktiviert(SymbolT f);
BOOLEAN MM_ACCII_aktiviert(SymbolT f);

RegelT      MM_ARegel(SymbolT f);
GleichungsT MM_CGleichung(SymbolT f);
GleichungsT MM_C_Gleichung(SymbolT f);
RegelT      MM_IRegel(SymbolT f);
RegelT      MM_I_Regel(SymbolT f);

void MM_SoftCleanup(void);
void MM_Cleanup(void);

#endif
