/**********************************************************************
* Filename : NormaleZiele.h
*
* Kurzbeschreibung : Normale Zielverwaltung
*
* Autoren : Andreas Jaeger
*
* 10.10.96: Erstellung aus Zielverwaltung.h heraus
*
* Die Funktionen in diesem Modul werden nie direkt aufgerufen, sondern
* nur von Zielverwaltung!
*
**********************************************************************/

#ifndef NORMALEZIELE_H
#define NORMALEZIELE_H

#include "TermOperationen.h"
#include "RUndEVerwaltung.h"
#include "Zielverwaltung.h"
#include "Zielverwaltung_private.h"

void NZ_ZielEinfuegen (ZieleT * ziel, TermT links, TermT rechts);

void NZ_LoeschePrivateData (ZieleT *ziel);

void NZ_AusgabeZiel (ZieleT * ziel, Z_AusgabeArtT Z_AusgabeArt);
   
void NZ_ZielBeobachtung (ZieleT *ziel, Z_BeobachtungsDurchlaufProcT AP);

/* Testen, ob einzelnes Ziel erledigt ist. */
BOOLEAN NZ_TesteZielErledigt (ZieleT *ziel);

BOOLEAN NZ_ZielErledigtMitNeuemFaktum (ZieleT *ziel, 
                                       RegelOderGleichungsT Faktum);

BOOLEAN NZ_ZielFinalerDurchlauf (ZieleT * ziel);

void NZ_InitZielBeobachtungsProz (ZieleT  *ziel);

#endif /* NORMALEZIELE_H */
