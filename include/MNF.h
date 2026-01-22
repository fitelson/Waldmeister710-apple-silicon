/**********************************************************************
* Filename : MNF.h
*
* Kurzbeschreibung : MultipleNormalFormen-Bildung: Externe Funktionen
*                    (Verwalten von Nachfolgermengen)
*
* Autoren : Andreas Jaeger
*
* 03.09.96: Erstellung
*
**********************************************************************/

#ifndef MNF_H
#define MNF_H

#include "general.h"
#include "RUndEVerwaltung.h"
#include "TermOperationen.h"

void MNF_InitAposteriori (void);
void MNF_Cleanup (void);

/* Zielverwaltung */
/* -------------- */

/* Die Struktur ist folgendermassen:
   - Das Modul muss mit MNF_InitNMengeAposteriori initialisiert werden.
   - Fuer jede Zielgleichung muss eine Menge angelegt werden, dies
     geschieht mit MNF_InitNMenge
   - Jede neue Regel/Gleichung muss mit MNF_AddRegelNM/MNF_AddGleichungNM,
     nachdem (!) sie aufgenommen wurde, der Menge gemeldet werden, die dann
     diese Regel/Gleichung entsprechend erstmalig bearbeitet.
   - Nach Aufrufen von MNF_InitNMenge, MNF_AddRegelNM und MNF_AddGleichungNM
     kann MNF_ExpandNMenge aufgerufen werden um die Nachfolgermenge zu 
     expandieren. Der Aufruf von MNF_ExpandNMenge muss nicht nach jedem Aufruf 
     von MNF_InitNMenge und MNF_Add*NM kommen, sondern kann auch nur in 
     regelmaessigen Abstaenden kommen.
   - Nach Aufrufen von MNF_InitNMenge, MNF_ExpandNMenge, MNF_AddRegelNM,
     MNF_AddGleichungNM sollte MNF_JoinedNMenge aufgerufen werden um 
     festzustellen, ob die beiden Terme zusammengefuehrt werden konnten.
   - Um den Speicher freizugeben muss zum Schluss MNF_DeleteNMenge aufgerufen
     werden.
     */

struct MNF_MengeTS;

struct MNF_MengeTS *MNF_InitNMenge (TermT s, TermT t);

void MNF_ExpandNMenge (struct MNF_MengeTS *menge);

void MNF_DeleteNMenge (struct MNF_MengeTS *menge);

void MNF_AddRegelNM (struct MNF_MengeTS *menge, RegelT Regel);

void MNF_AddGleichungNM (struct MNF_MengeTS *menge, GleichungsT Gleichung);


/* Abfragefunktionen */
/* ----------------- */
/* Der zurueckgegebene Term darf von der aufgerufenen Funktion
   nicht freigegeben werden. Evtl. muss eine Termkopie gemacht werden */
TermT MNF_GetJoinedTerm (struct MNF_MengeTS *menge);

/* GetJoinedTermpair liefert das zusammengefuehrte bzw. subsummierte Termpaar
   zurueck */
/* Die zurueckgegebenen Terme duerfen von der aufgerufenen Funktion
   nicht freigegeben werden. Evtl. muss eine Termkopie gemacht werden */
void MNF_GetJoinedTermpair (struct MNF_MengeTS *menge, 
                            TermT *left, TermT *right);
   
/* aktuelle Anzahl von Elementen in Nachfolgermenge */
unsigned int MNF_GetSize (struct MNF_MengeTS *menge);

/* Wurde Nachfolgermenge zusammengefuehrt */
BOOLEAN MNF_JoinedNMenge (struct MNF_MengeTS *menge);

#endif /* MNF_H */
