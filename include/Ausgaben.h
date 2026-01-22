/*
Dieses Modul enth"alt die Ausgabe-Funktionen sowie die Bezeichner der Funktions-Symbole aus der eingelesenen Spezifikation.
Ferner sind hier alle noch verwendbaren Daten der Spezifikation gesammelt, um ein Freigeben des vom Parser allokierten Speichers
zu erm"oglichen. (noch nicht realisiert!)

ACHTUNG : Ausgaben sind erst moeglich, nachdem mit IO_Initialisieren die internen Strukturen besetzt sind.

28.02.94 Angelegt. Arnim.
02.03.94 DruckeKopf, DruckeStatistik eingefuehrt. Arnim.
02.03.94 DruckeTerm eingefuegt. Thomas
02.03.94 DruckeKopf erweitert und IO_LF eingebaut. Arnim.
03.03.94 Funktions-Pointer und Makros fuer die Ausgaben waehrend des Systemslauf eingebaut.
         Bisher aber noch alles auskommentiert.
21.03.94 Makros fuer Ausgabe-Level-abhaengige Ausgaben eingebaut, die Sachen vom 03.03. 'rausgeworfen.
         Getestet mit und ohne Ausgaben, laeuft so wie es soll!
00.07.94 Weitere Makros eingebaut, andere geaendert bzw. geloescht.
22.02.95 Rekursion in DruckeTerm eliminiert. Thomas
20.06.96 Statistik- und Ergebnis-Kram ausgelagert (Statistiken.h) Arnim.
*/

/* Mit Hilfe zweier Konstanten aus Statistik-Parameter kann Kode ohne Aufrufe von Ausgabe-Funktionen 
   erzeugt werden! */


#ifndef AUSGABEN_H
#define AUSGABEN_H


/* -------------------------------------------------------*/
/* ------------- Includes --------------------------------*/
/* -------------------------------------------------------*/

#include "SymbolOperationen.h"
#include <stdlib.h>
#include <stdio.h>
#include "general.h"
#include "FehlerBehandlung.h"
#include "TermOperationen.h"

/* -------------------------------------------------------*/
/* ------------- IO-Konstanten und Defines ---------------*/
/* -------------------------------------------------------*/

#define IO_AUSGABEN normal_power_comp(1,1,0)
/* Alternativen: 0, 1 */

#define MaximaleBezeichnerLaenge 25     /* Dies ist die im Parser des RS-Praktikums vorgesehene Laenge, sie erscheint dort aber nur im Code !!! */
#define maxDrucklaengeInnererPfad 200   /* maximale Laenge des in der Prozedur IO_DruckeBaum ausgegebenen Pfades zu einem Blatt. Treten noch
                                           laengere Pfade auf, so werden fuer jedes Blatt nur die ersten 200 Zeichen des zu ihm fuehrenden
                                           Pfades ausgegeben, danach unmittelbar die Informationen aus den Blaettern. */
#define StartzahlVerkettungskuerzung 3  /* Sei s beliebiges einstelliges Funktionssymbol, t Element von Term(F,V), Topsymbol von t <> s.
					   Ein Term s(s(...s(t)...)) wird genau dann als s^n(t) ausgegeben, wenn n>=StartzahlVerkettungskuerzung. */

unsigned long int IO_AusdehnungAusgabestapel(void);

/* -------------------------------------------------------*/
/* -------------------------------------------------------*/
/* ------------- Initialisierung -------------------------*/
/* -------------------------------------------------------*/
/* -------------------------------------------------------*/

void IO_InitApriori(void);
/* Vor der ersten Ausgabe aufzurufen!!!! */

void IO_InitAposterioriEarly(void);
/* wird nach Einlesen der Spezifikation aufgerufen, setzt defaults */

void IO_InitAposteriori(int level);
/* Legt die Ausgabe-relevanten Daten aus der Spezifikation als Datei-lokale Variablen ab. 
   ACHTUNG : Diese Initialisierung kann erst NACH der Initialisierung der Symbol-Operationen mit
   SO_Operationen erfolgreich durchgefuehrt werden. (siehe inits->IN_Initialisieren. *)
*/

void IO_CleanUp(void);
/* Interne Daten abraeumen. */

void IO_AusgabeLevelBesetzen(int level);
/* Stellt einen neuen Ausgabelevel ein. */

int IO_AusgabeLevel(void);
/* Liefert den aktuellen Ausgabelevel zurueck. */

void IO_AusgabenAus(void);
/* Schaltet unabhaengig vom aktuellen Ausgabelevel die Ausgaben aus.*/

void IO_AusgabenEin(void);
/* Schaltet die mit IO_AusgabenAus zuvor ausgeschalteten Ausgaben gemaess Einstellung wieder ein.*/

void IO_SpezifikationsnamenSetzen(char *Filename);
/* Setzt den Namen der bearbeiteten Spezifikation (incl. Pfad) ein.*/

char *IO_Spezifikationsname(void);
/* Gibt den Namen (und Pfad) der zu verwendeten Spez.datei zurueck.*/

char *IO_SpezifikationsBasisname(void);
/* Gibt den Basisnamen (ohne Pfad) der zu verwendeten Spez.datei zurueck.*/

/* -------------------------------------------------------*/
/* -------------- Kopf und Spezifikation -----------------*/
/* -------------------------------------------------------*/

void IO_DruckeKopf(void);
/* Druckt die Start-Meldung des Programmes. */

void IO_DruckeBeispiel(void);
/* Gibt die Parameter-Einstellungen aus */

/* -------------------------------------------------------*/
/* -------------------------------------------------------*/
/* ------------- Elementare Druck-Funktionen -------------*/
/* -------------------------------------------------------*/
/* -------------------------------------------------------*/
#define IO_Ueberschriftenbreite 80

void IO_DruckeZeile(char zeichen);
/* Druckt eine ganze Zeile mit dem Zeichen zeichen. */


#define /*void*/ IO_LF(/*void*/) putc('\n',stdout)
/* Druckt einen Zeilenvorschub. */

void IO_DruckeFlex(const char *fmt, ...);
/* Aehnlich 'printf', aber es werden nur die Platzhalter %s fuer Strings, %d fuer int, %f fuer double, %l fuer
   long und %u fuer unsigned long verarbeitet. Zusaetzlich kann man aber mit 
   %t einen Term vom Typ TermT, %p eine Stelle (L.1.2.3), %q eine Seite (L/R)
   ausgeben.
*/

void IO_DruckeFlexStrom(FILE *Strom, const char *fmt, ...);
/* IO_DruckeFlex auf bel. Stroeme; Terme werden ungekuerzt ausgegeben.
 */

void IO_DruckeGanzzahl(int n);
/* Ganze Zahl ausgeben. */

void IO_DruckeUeberschrift(const char *text);
/* Ueberschrift drucken */

void IO_DruckeKurzUeberschrift(const char *text,  char zeichen);

/* -------------------------------------------------------*/
/* -------------------------------------------------------*/
/* ------------- Druck-Funktionen fuer System-Strukturen +*/
/* -------------------------------------------------------*/
/* -------------------------------------------------------*/

void IO_DruckeSymbol(SymbolT symbol);
/* Gibt die externe Repraesentation eines Symbols aus. */

char *IO_FktSymbName(SymbolT FktSymb);
/* Zeiger auf Zeichenketten-Darstellung des Druckenamens des Funktionssymbols */

void IO_DruckeTerm(UTermT Term);
/* Gibt einen Term auf den eingestellten Ausgabestrom aus. */

void IO_DruckeSymbolStrom(FILE *Strom, SymbolT symbol);
/* Ausgabe eines Symbols auf dem angegebenen Ausgabestrom.*/

void IO_DruckeTermPaar(UTermT term1, const char *trenner,UTermT term2);
/* Gibt ein Term-Paar mit dem String trenner als einzufuegendes Zeichen aus.
   Ist z.B. fuer Gleichungen ein " = ", fuer Regeln ein " -> ",
   fuer kritische Paare ein " !=! " etc. o.ae.*/

/* -------------------------------------------------------*/
/* -------------------------------------------------------*/
/* -------------- Ausgabelevel-Abhaengige Funktionen -----*/
/* -------------------------------------------------------*/
/* -------------------------------------------------------*/


/* -------------- Ausgabe-Levels -------------------------*/
/* -------------------------------------------------------*/

/* Level       Bedeutung
---------------------------------------------------
     0           Stumm
     1           Punkte
     2           Discount
     3           Alle Meldungen, aber ohne Terme
     4           Alle Meldungen, mit Termen
     5           Incl. Rewriteschritte
---------------------------------------------------*/

extern unsigned int IO_level;
#if IO_AUSGABEN
#define IO_level0() (IO_level == 0)
#define IO_level1() (IO_level == 1)
#define IO_level2() (IO_level == 2)
#define IO_level3() (IO_level == 3)
#define IO_level4() (IO_level == 4)
#define IO_level5() (IO_level == 5)
#else
#define IO_level0() (TRUE)
#define IO_level1() (FALSE)
#define IO_level2() (FALSE)
#define IO_level3() (FALSE)
#define IO_level4() (FALSE)
#define IO_level5() (FALSE)
#endif




/* -------------------------------------------------------------------------------------------------------------*/
/* ------------ GENERATION -------------------------------------------------------------------------------------*/
/* -------------------------------------------------------------------------------------------------------------*/

extern BOOLEAN IO_GUeberschrift;

void IO_DruckeFaktum(TermT l, BOOLEAN orientierbar, TermT r);

/*+++++++++++++++++++++++++++++++++++++++++++++++++++++++*/
/*+++++++++++++++++++++ Sonstiges +++++++++++++++++++++++*/
/*+++++++++++++++++++++++++++++++++++++++++++++++++++++++*/

unsigned int IO_SymbAusgabelaenge(SymbolT Symbol);     /*ermittelt die Laenge der zugehorigen Ausgabezeichenkette*/


#ifdef CCE_Protocol
typedef enum {CCE_Add, CCE_Delete, CCE_Test, CCE_Undef} CCE_loggingT;

void IO_CCEinit(char *filename);
void IO_CCElog(CCE_loggingT tp, TermT lhs, TermT rhs);
#endif

#endif
