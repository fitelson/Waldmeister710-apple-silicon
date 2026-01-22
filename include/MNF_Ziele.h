/**********************************************************************
* Filename : MNF_Ziele.h
*
* Kurzbeschreibung : Zielbehandlung mit MultipleNormalFormen
*
* Autoren : Andreas Jaeger
*
* 09.10.96: Erstellung
*
**********************************************************************/

#ifndef MNF_ZIELE_H
#define MNF_ZIELE_H

#include "RUndEVerwaltung.h"
#include "TermOperationen.h"
#include "Zielverwaltung.h"
#include "Zielverwaltung_private.h"

void MNFZ_InitAposteriori(void);

void MNFZ_ZielEinfuegen (ZieleT * ziel);

void MNFZ_LoeschePrivateData (ZieleT *ziel);

void MNFZ_AusgabeZiel (ZieleT * ziel, Z_AusgabeArtT Z_AusgabeArt);
   
void MNFZ_ZielBeobachtung (ZieleT *ziel, Z_BeobachtungsDurchlaufProcT AP);

BOOLEAN MNFZ_TesteZielErledigt (ZieleT *ziel);

BOOLEAN MNFZ_ZielErledigtMitNeuemFaktum (ZieleT *ziel, 
                                         RegelOderGleichungsT Faktum);

BOOLEAN MNFZ_ZielFinalerDurchlauf (ZieleT * ziel);

void MNFZ_InitZielBeobachtungsProz (ZieleT  *ziel);

#endif /* MNF_ZIELE_H */
