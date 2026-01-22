/*=================================================================================================================
  =                                                                                                               =
  =  Subsumption.c                                                                                                =
  =                                                                                                               =
  =  Subsumption mit der Gleichungsmenge bzw. einzelnen Gleichungen                                               =
  =                                                                                                               =
  =  27.03.96  Arnim. Aus der Interreduktion ausgegliedert.                                                       =
  =================================================================================================================*/

#include "MatchOperationen.h"
#include "Subsumption.h"
#include "TermOperationen.h"

static BOOLEAN NachfolgendeTeiltermeGleich(UTermT Ende, UTermT Teilterm1, UTermT Teilterm2)
/* Prueft, ob die auf Teilterm1 folgenden Teilterme von Term1 mit den korrespondierenden Teiltermen, die Teilterm2
   nachfolgen, uebereinstimmen. Der hier nicht anzugebende term2, dessen Teilterm Teilterm2 ist, hat das gleiche
   Topsymbol wie Term1, und Teilterm1 wie Teilterm2 sind der jeweils i-te Teilterm von Term1 bzw. term2.*/
{
  Teilterm1 = TO_NaechsterTeilterm(Teilterm1);                                /* auf nachfolgenden Teilterm setzen */
  Teilterm2 = TO_NaechsterTeilterm(Teilterm2);
  while (TO_PhysischUngleich(Teilterm1,Ende)) {
    if (TO_TopSymboleGleich(Teilterm1,Teilterm2)) {
      Teilterm1 = TO_Schwanz(Teilterm1);                                               /* Symbole gleich -> weiter */
      Teilterm2 = TO_Schwanz(Teilterm2);
    }
    else
      return FALSE;                                                  /* unterschiedliches Symbol gefunden -> FALSE */
  }
  return TRUE;                                                                  /* erfolgreich durchgelaufen -> ok.*/
}

/* leider gibt es keine gescheiten Closures in C: */
#define SubsumptionBody(subsumptionstest,links,rechts)                          \
{                                                                               \
   TermT Nullterm;                                                              \
                                                                                \
   while (!(subsumptionstest)){                                                 \
     if (!TO_TopSymboleGleich(links, rechts))                                   \
       return FALSE;   /* unterschiedliche Topsymbole */                        \
                                                                                \
     Nullterm  = TO_NULLTerm(links);                                            \
     links  = TO_ErsterTeilterm(links);                                         \
     rechts = TO_ErsterTeilterm(rechts);                                        \
     while (TO_TermeGleich(links, rechts)){                                     \
       /* Ersten unterschiedlichen Teilterm suchen,                             \
          bricht auf jeden Fall ab, da die Terme ja unterschiedlich sind */     \
       links  = TO_NaechsterTeilterm(links);                                    \
       rechts = TO_NaechsterTeilterm(rechts);                                   \
     }                                                                          \
     if (!NachfolgendeTeiltermeGleich(Nullterm, links, rechts))                 \
       return FALSE;   /* mehr als ein unterschiedlicher Teilterm */            \
                                                                                \
   }                                                                            \
   return TRUE;  /* Subsumption gelungen */                                     \
}

BOOLEAN SS_TermpaarSubsummiertVonGM (TermT links, TermT rechts)
/* TRUE, wenn die angegebene Gleichung links = rechts bzw. rechts = links von einer bereits vorhandenen 
   Gleichung an irgend einer Stelle subsummiert wird.
   Vorbedingung: - links = rechts ist noch nicht in GM aufgenommen => kein Ausschluss zu beachten.
                 - links ist nicht identisch zu rechts
*/
{
   SubsumptionBody(MO_SubsummierendeGleichungGefunden(links, rechts), links, rechts);
}

BOOLEAN SS_TermpaarSubsummiertTermpaar (TermT Glinks, TermT Grechts, TermT links, TermT rechts)
/* Liefert TRUE, wenn die gleichung das Termpaar links, rechts subsummiert.
   Dabei werden alle Subgleichungen des Termpaar berechnet und getestet, und zwar in beiden Richtungen.*/
{
  SubsumptionBody(MO_TermpaarSubsummiertZweites(Glinks, Grechts, links, rechts) ||
                  MO_TermpaarSubsummiertZweites(Grechts, Glinks, links, rechts)    , links, rechts);
}

