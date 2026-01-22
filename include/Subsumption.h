/*=================================================================================================================
  =                                                                                                               =
  =  Subsumption.h                                                                                                =
  =                                                                                                               =
  =  Subsumption mit der Gleichungsmenge bzw. einzelnen Gleichungen                                               =
  =                                                                                                               =
  =  27.03.96  Arnim. Aus der Interreduktion ausgegliedert.                                                       =
  =================================================================================================================*/
#ifndef SUBSUMPTION_H
#define SUBSUMPTION_H

#include "TermOperationen.h"

BOOLEAN SS_TermpaarSubsummiertVonGM (TermT links, TermT rechts);
/* TRUE, wenn die angegebene Gleichung links = rechts bzw. rechts = links von einer bereits vorhandenen 
   Gleichung subsummiert wird. 
   Vorbedingung: - links = rechts ist noch nicht in GM aufgenommen => kein Ausschluss zu beachten.
                 - links ist nicht identisch zu rechts
*/

BOOLEAN SS_TermpaarSubsummiertTermpaar (TermT Glinks, TermT Grechts, TermT links, TermT rechts);
/* Liefert TRUE, wenn die Gleichung Glinks = Grechts das Termpaar links, rechts subsummiert.
   Dabei werden alle Subgleichungen des Termpaar berechnet und getestet, und zwar in beiden Richtungen.*/

#endif
