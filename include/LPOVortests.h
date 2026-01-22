/*=================================================================================================================
  =                                                                                                               =
  =  LPOVortests.h                                                                                                =
  =                                                                                                               =
  =  Funktionen zum Abpruefen von Variablenverhaeltnis, Teiltermeigenschaft, Gleichheit                           =
  =                                                                                                               =
  =  04.03.94 Thomas.                                                                                             =
  =                                                                                                               =
  =================================================================================================================*/

#ifndef LPOVORTESTS_H
#define LPOVORTESTS_H


#include "TermOperationen.h"
#include "general.h"


void LV_InitAposteriori(void);                                      /* Initialisierung der Freispeicherverwaltung. */


BOOLEAN LV_VortestLPO(UTermT *Term1, UTermT *Term2, VergleichsT *Vergleich);
/* prueft, ob einer der Terme Teilterm des anderen ist, in welchem Verhaeltnis die LPOVortests zueinander ste-
   hen, ob die Terme gleich sind, kuerzt Term1 und Term2. Es wird TRUE zurueckgegeben, wenn schon hier geklaert
   werden konnte, in welchem Verhaeltnis Term1 und Term2 stehen; dann steht dieses Verhaeltnis in *Vergleich.
   Sonst wird FALSE geliefert; *Vergleich enthaelt das Ergebnis des Variablenmengenvergleichs. Sind die Mengen un-
   vergleichbar, wird natuerlich TRUE zurueckgegeben.                                                              */


BOOLEAN LV_VortestLPOGroesser(UTermT *Term1, UTermT *Term2, BOOLEAN *Vergleich);
/* Funktionalitaet wie VortestLPO, jedoch mit *Vergleich nur vom Typ BOOLEAN, da nur zwei Werte benoetigt.
   Rueckgabe TRUE, falls schon herausgefunden werden konnte, ob Term1>Term2 oder Term1!>Term2; *Vergleich hat dann
   den entsprechenden Wert.
   Rueckgabe FALSE, wenn keine Schluesse gezogen werden konnten.                                                   */


BOOLEAN LV_VortestLPOGroesserGleich(UTermT *Term1, UTermT *Term2, VergleichsT *Vergleich);
/* Analog VortestLPOGroesser.
   Rueckgabe TRUE, falls schon herausgefunden werden konnte, dass Term1>Term2 oder Term1=Term2; *Vergleich hat
   dann den entsprechenden Wert.
   Rueckgabe FALSE, wenn keine Schluesse gezogen werden konnten.                                                   */


#endif
