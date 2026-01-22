/**********************************************************************
* Filename : Zielverwaltung.h
*
* Kurzbeschreibung : "Mittelschicht" Zielverwaltung,
*                    also gemeinsame Schnittstelle von NormaleZiele und
*                    MNF_Ziele nach oben
*
* Autoren : Andreas Jaeger
*
* 10.07.96: Erstellung aus der urspruenglichen Zielverwaltung heraus
**********************************************************************/

#ifndef ZIELVERWALTUNG_H
#define ZIELVERWALTUNG_H


#include "TermOperationen.h"
#include "RUndEVerwaltung.h"

typedef void (*Z_BeobachtungsDurchlaufProcT) (TermT, TermT, unsigned long);

typedef void (*Z_BeweisDurchlaufProcT) (TermT lhs, TermT rhs, 
                                        unsigned long nummer, 
                                        unsigned int notimes,
                                        AbstractTime *times);

typedef void (*Z_BeobachtungsProzT) (TermT, TermT);

typedef enum {
   Z_NichtDefiniert, Z_NormaleZiele, Z_MNF_Ziele, Z_Paramodulation, Z_Repeat
} Z_ZielbehandlungsModulT;

extern Z_ZielbehandlungsModulT Z_ZielModul;

typedef enum { /* Reihenfolge entspricht Prioritaet! */
  Z_AlleBewiesen,     /* alle Ziele zusammengefuehrt/subsummiert */
  Z_AlleWiederlegt,   /* alle wiederlegt */
  Z_EinigeWiederlegt, /* einige Ziele wiederlegt, Rest bewiesen */
  Z_EinigeBewiesen,   /* Einige bewiesen, Rest wiederlegt */
  Z_Offen,            /* nichts bekannt */
  Z_KeineZiele        /* keine Ziele vorhanden */
} Z_ZielGesamtStatusT;

extern Z_ZielGesamtStatusT Z_ZielGesamtStatus;
/* wird bei Aufruf von Z_AusgabeZieleEnde gesetzt */

/* Anzahl bewiesener Ziele */
int Z_AnzahlBewiesenerZiele (void);

/* Anzahl spezifizierter Ziele */
int Z_AnzahlZiele (void);

void Z_ZieleAusSpezifikationHolen(void);

void Z_EinZielEinfuegen (TermT left, TermT right, unsigned short int Index, 
                         Z_ZielbehandlungsModulT modul);

BOOLEAN Z_ZielmengeLeer(void);
/* Liefert TRUE, wenn es kein zu beweisendes Ziel mehr gibt
   (entweder wurden alle Ziele gezeigt, oder es gab keine) */

BOOLEAN Z_MindEinZielBewiesen (void);
/* Mindestens ein Ziel wurde bewiesen */

BOOLEAN Z_ZieleBewiesen(void);
/* Liefert TRUE, wenn alle gewuenschten Ziele (gemaess Einstellungen alle oder
 * nur das erste) bewiesen werden konnten.*/

/* Zielmengen-Durchlauf */
/* -------------------- */

void Z_ZielmengenBeobachtungsDurchlauf(Z_BeobachtungsDurchlaufProcT AP);
/* Wendet AP auf jedes Ziel an, wobei AP die Ziele nicht veraendert.*/

void Z_ZielBeobachtungsProzAnmelden(Z_BeobachtungsProzT proz);
/* Anmeldung einer Prozedur proz, die aufgerufen wird, sobald sich das
   1. Ziel aendert.  Wurde das erste Ziel bewiesen, so wird proz mit
   dem naechsten Ziel aufgerufen.  proz wird insbesondere auch dann
   aufgerufen, sobald das erste Ziel eingefuegt wird.
   
   Aenderungen werden nur innerhalb der Prozedur Z_ZielmengenDurchlauf
   mitgeteilt, die uebrigen Durchlaufmoeglichkeiten rufen proz nicht
   auf!
*/


/* Zielmengen-Ausgabe */       
/* ------------------ */       

typedef enum  {
  Z_AusgabeAlles, Z_AusgabeInitial, Z_AusgabeSpez, Z_AusgabeKurz
} Z_AusgabeArtT;

void Z_AusgabeZielmenge (Z_AusgabeArtT Z_AusgabeArt);

/* Zusammenfassung der Ziele mit Statistik, completed ist TRUE wenn
   das System vervollstaendig ist, so ist unterscheidbar, ob die nicht
   bewiesenen Ziele Wiederlegt wurden oder nicht */
void Z_AusgabeZieleEnde (BOOLEAN completed);


/*===========================================================================*/
/*=  Zielbehandlung                                                         =*/
/*===========================================================================*/

BOOLEAN Z_ZieleErledigtMitNeuemFaktum(RegelOderGleichungsT Faktum);

BOOLEAN Z_FinalerZielDurchlauf(void);

/* Rueckgabe der letzten drei Funktionen ist TRUE, wenn gemaess
   Parametrisierung eines oder alle Ziele bewiesen wurden. */

BOOLEAN  Z_InitAposteriori(void);
   
void Z_Cleanup (void);
   

void Z_UngleichungEinmanteln(UTermT *links, UTermT *rechts);

BOOLEAN Z_IstGemantelteUngleichung(TermT links, TermT rechts);

BOOLEAN Z_IstGemanteltesTermpaar(TermT links, TermT rechts);
  /* Z_IstGemantelteUngleichung || eq(x,x) = true */

void Z_UngleichungEntmanteln(UTermT *OhneMantelLinks, 
                             UTermT *OhneMantelRechts,
                             TermT MitMantelLinks, TermT MitMantelRechts);

BOOLEAN Z_UngleichungEntmantelt(UTermT *OhneMantelLinks, 
                                UTermT *OhneMantelRechts, 
                                TermT MitMantelLinks, TermT MitMantelRechts);


BOOLEAN Z_IstSkolemvarianteZuUngleichung(TermT links, TermT rechts);

BOOLEAN Z_IstGrundungleichung(TermT links, TermT rechts);

#endif /* ZIELVERWALTUNG_H */
