#ifndef INTERREDUKTION_H
#define INTERREDUKTION_H

/* ------------------------------------------------------------------------- */
/*                                                                           */
/* Interreduktion.h                                                          */
/*                                                                           */
/* Interreduktion der Regel- und Gleichungsmenge                             */
/*                                                                           */
/* --------------------------------------------------------------------------*/

#include "RUndEVerwaltung.h"
#include "TermOperationen.h"

/* --------------------------------------------------------------------------*/
/* --------------- Vereinfachte Schnittstelle -------------------------------*/
/* --------------------------------------------------------------------------*/

void IR_NeueInterreduktion(RegelOderGleichungsT neuesObjekt);

/* --------------------------------------------------------------------------*/
/* --------------- Test auf Killer-Eigenschaft ------------------------------*/
/* --------------------------------------------------------------------------*/

/* Die beiden folgenden Prozeduren ueberpruefen, ob das gegebene Termpaar als 
   Regel (IR_TPRReduziertRE), dh. von links nach rechts, bzw. als Gleichung
   (IR_TPGReduziertRE) gesehen, die linke Seite einer Regel oder Gleichung
   aus RE reduziert. */

BOOLEAN IR_TPRReduziertRE(TermT Links, TermT Rechts);

BOOLEAN IR_TPEReduziertRE(TermT Links, TermT Rechts);

#endif

