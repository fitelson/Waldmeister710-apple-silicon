#ifndef STRINGTERMS_H
#define STRINGTERMS_H

#include "TermOperationen.h"

typedef byte *PTermpaarT;                                /* Datentyp fuer eingepacktes Termpaar */

unsigned long int STT_TermpaarPlatzbedarf(TermT l, TermT r);
/* Termpaar durchlaufen und dabei tatsaechlichen Platzbedarf ermitteln:
   jede Variablen-Nummer < -127 wird mit drei Byte abgelegt!*/

void STT_TermpaarEinpackenFragil(TermT lhs, TermT rhs, PTermpaarT tp);
/* Die Gleichung lhs=rhs in das uebergebene PTermpaar einschreiben. 
   Kein Test auf ausreichende Groesse von tp! */

PTermpaarT STT_TermpaarEinpacken(TermT lhs, TermT rhs, int *len);
/* Die Gleichung lhs=rhs in ein neu allokiertes PTermpaar einschreiben. 
   Rueckgabe: neues PTermpaar und dessen Laenge in len */

void STT_TermpaarAuspacken(PTermpaarT tp, TermT *lhsPtr, TermT *rhsPtr);

void STT_DruckePTermpaar(PTermpaarT tp, const char *trenner);

#endif
