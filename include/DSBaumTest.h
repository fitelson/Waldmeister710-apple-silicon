/*=================================================================================================================
  =                                                                                                               =
  =  DSBaumTest.h                                                                                                 =
  =                                                                                                               =
  =  Datenstrukur auf Konsistenz ueberpruefen                                                                     =
  =                                                                                                               =
  =                                                                                                               =
  =  16.11.95  Thomas.                                                                                            =
  =================================================================================================================*/

#ifndef DSBAUMTEST_H
#define DSBAUMTEST_H

#include "DSBaumKnoten.h"

/* Man beachte:
   Im Kopilat sind die Testfunktionen genau dann vorhanden,
   wenn RE_TEST in RUndEVerwaltung.h von 0 verschieden ist. */



/*=================================================================================================================*/
/*= IV. Testaufruf                                                                                                =*/
/*=================================================================================================================*/

typedef BOOLEAN (*BT_TPPraedikatsT)(GNTermpaarT);

BOOLEAN BT_IndexTesten(DSBaumT Baum, BT_TPPraedikatsT TPPraedikat, BOOLEAN MitSprungverweisen);


/*=================================================================================================================*/
/*= V. Initialisierung                                                                                            =*/
/*=================================================================================================================*/

void BT_InitApriori(void);

#endif
