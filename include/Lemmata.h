#ifndef LEMMATA_H
#define LEMMATA_H

#include "general.h"
#include "RUndEVerwaltung.h"
#include "SymbolOperationen.h"

typedef enum {
  FullPermLemmata, PermLemmata, IdempotenzLemmata, C_Lemma
} LemmaT;

void LE_LemmataAktivieren(LemmaT typ, SymbolT f);
/* aktiviert fuer f die entsprechenden Lemmata. 
 * Es koenenn mehrere Lemmatafamilien aktiviert werden, 
 * diese werden chronologisch abgearbeitet.
 */

BOOLEAN LE_getLemma(SelectRecT *selRecP);
/* Falls noch ein Lemma einer aktivierten Familie ansteht, 
 * liefert das naechste in selRec zurueck, Rueckgabewert TRUE.
 * Ansonsten FALSE.
 */

void LE_SoftCleanup(void);
/* Raeumt die internen Strukturen ab, fuer Strategiewechsel */

void LE_Cleanup(void);
/* Raeumt die internen Strukturen ab und gibt sie wieder frei, 
 * fuer Programmende */

#endif
