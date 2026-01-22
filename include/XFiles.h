/*=================================================================================================================
  =                                                                                                               =
  =  XFiles.h                                                                                                     =
  =                                                                                                               =
  =  Typdefinitionen und Iteratoren fuer Gesetzestafeln                                                           =
  =                                                                                                               =
  =  27.04.98 Aus Ordnungsgenerator extrahiert. Thomas.                                                           =
  =                                                                                                               =
  =================================================================================================================*/


#ifndef XFILES_H
#define XFILES_H

#include "FehlerBehandlung.h"
#include "Sinai.h"
#include "Termpaare.h"
#include "VariablenMengen.h"
#include "XFilesFuerPMh.h"



/*=================================================================================================================*/
/*= I. Testbetrieb und lokale Speicherkonstanten                                                                  =*/
/*=================================================================================================================*/

#define XF_TEST 0                                                                            /* Alternativen: 0, 1 */

#if XF_TEST
#define pr(Argumente) printf Argumente
#else
#define pr(Argumente) 
#endif

#if XF_TEST
#define /*void*/ test_error(/*BOOLEAN*/ Fehler, /*char **/ Meldung)                                                 \
  { if (Fehler) our_error(Meldung); }
#else
#define /*void*/ test_error(/*BOOLEAN*/ Fehler, /*char **/ Meldung)                                                 \
  { ; }
#endif


#define maxAnzPraemissen  17                                  /* maximale Anzahl von Praemissen in einer Zuordnung */

/* Siehe inzwischen XFilesFuerPMh.h fuer:
   maxAnzOperatoren
   Signatur_SymbolT
   Signatur_ZeichenT
*/



/*=================================================================================================================*/
/*= II. Aufzaehlungstyp GesetznamenT                                                                              =*/
/*=================================================================================================================*/

#define __ Mantelung(
#define _ ,__
#define Mantelung(Gesetzname,Signatur,Gleichung) Gesetzname

typedef enum {
  Tafel1(),
  Unbekannt
} GesetznamenT;                                                          /* "Unbekannt" muss stets am Ende stehen! */

#undef Mantelung

#define AnzGesetze Unbekannt



/*=================================================================================================================*/
/*= III. GesetzT und Tafel1 mit Iteratoren                                                                        =*/
/*=================================================================================================================*/

typedef struct {
  GesetznamenT Name;
#if XF_TEST
  char *Klartext;
#endif
  Signatur_ZeichenT Signatur_Zeichen;
  char *Gleichung_String;
  GNTermpaarT Gleichung_Termpaar;
  Signatur_SymbolT Signatur_Symbol;
} GesetzT;


extern GesetzT Tafel1[AnzGesetze+1];


#define for_Gesetze(/*unsigned int*/ i, /*GesetzT **/ Gesetz)                                                       \
  for(i=0; i<AnzGesetze ? (Gesetz = &Tafel1[i], TRUE) : FALSE; i++)

/* In den folgenden Iterationsschleifen ist der Parameter i eine Variable vom Typ unsigned int sowie
   der Parameter Gesetz ein Ausdruck mit dem Wertebereich GesetzT* U ZuordnungsT* U PraemissenT*. */

#define Schleife_for_Operatoren(i, Gesetz, ZweigBeiTRUE)                                                            \
  for(i=0; (Gesetz)->Signatur_Zeichen[i] ? ZweigBeiTRUE : FALSE; i++)
#define Schleife_for_OperatorenOhneZ(i, Gesetz)                                                                     \
  Schleife_for_Operatoren(i,Gesetz,TRUE)
#define Schleife_for_OperatorenMitZ(i, Gesetz, ZuweisungenVorSchleifeneintritt)                                     \
  Schleife_for_Operatoren(i,Gesetz,(ZuweisungenVorSchleifeneintritt, TRUE))

#define for_Operatoren(i, Gesetz)                                                                                   \
  Schleife_for_OperatorenOhneZ(i,Gesetz)
#define for_OperatorenC(i, Gesetz, /*char*/ Zeichen)                                                                \
  Schleife_for_OperatorenMitZ(i,Gesetz,(Zeichen = (Gesetz)->Signatur_Zeichen[i]))
#define for_OperatorenS(i, Gesetz, /*SymbolT*/ Symbol)                                                              \
  Schleife_for_OperatorenMitZ(i,Gesetz,(Symbol = (Gesetz)->Signatur_Symbol[i]))
#define for_OperatorenCS(i, Gesetz, /*char*/ Zeichen, /*SymbolT*/ Symbol)                                           \
  Schleife_for_OperatorenMitZ(i,Gesetz,                                                                             \
    (Zeichen = (Gesetz)->Signatur_Zeichen[i], Symbol = (Gesetz)->Signatur_Symbol[i]))



/*=================================================================================================================*/
/*= IV. StrukturnamenT und Tafel2 mit Iteratoren                                                                  =*/
/*=================================================================================================================*/

#define Mantelung(Strukturname, Strategien) Strukturname

typedef enum {
  Tafel2(),
  Orkus
} StrukturnamenT;

#undef Mantelung
#undef __
#undef _

#define AnzStrukturen Orkus

typedef struct {
  StrukturnamenT Name;
  char *Klartext;
} StrukturT;

extern StrukturT Tafel2[AnzStrukturen+1];


#define Schleife_for_Strukturen(/*StrukturnamenT*/ i, /*StrukturnamenT*/ LetzteStruktur, /*BOOLEAN*/ ZweigBeiTRUE)  \
  for(i=(StrukturnamenT)0; i<=LetzteStruktur ? ZweigBeiTRUE : FALSE; i=(StrukturnamenT)((int)i+1))

#define for_Strukturen(/*StrukturnamenT*/ i)                                                                        \
  Schleife_for_Strukturen(i,Orkus-1,TRUE)                             /* Auch hier Ann.: Datenfelder niemals leer! */

#define for_StrukturenS(/*StrukturnamenT*/ i, /*StrukturT**/ Struktur)                                              \
  Schleife_for_Strukturen(i,Orkus-1,(Struktur = &Tafel2[i],TRUE))                                          /* dito */

#define for_StrukturenMitOrkus(/*StrukturnamenT*/ i)                                                                \
  Schleife_for_Strukturen(i,Orkus,TRUE)

#define for_StrukturenMitOrkusS(/*StrukturnamenT*/ i, /*StrukturT**/ Struktur)                                      \
  Schleife_for_Strukturen(i,Orkus,(Struktur = &Tafel2[i],TRUE))



/*=================================================================================================================*/
/*= V. PraemissenT, ZuordnungsT und Tafel3 mit Iteratoren                                                         =*/
/*=================================================================================================================*/

typedef struct {
  GesetznamenT Name;
  Signatur_ZeichenT Signatur_Zeichen;
} PraemissenT;


typedef struct {
  StrukturnamenT Name;
  Signatur_ZeichenT Signatur_Zeichen;
  PraemissenT Praemissen[maxAnzPraemissen+1];
  Signatur_SymbolT Signatur_Symbol;
} ZuordnungsT;

extern const unsigned int AnzZuordnungen;


extern ZuordnungsT Tafel3[];


#define for_Zuordnungen(/*unsigned int*/ i, /*ZuordnungsT**/ Zuordnung)                                             \
  for(i=0; i<AnzZuordnungen ? (Zuordnung = &Tafel3[i], TRUE) : FALSE ; i++)

#define for_Praemissen(/*ZuordnungsT**/ Zuordnung, /*unsigned int*/ i, /*PraemissenT**/ Praemisse)                  \
  for(i=0; (Praemisse = &Zuordnung->Praemissen[i])->Name!=Unbekannt; i++)

#endif
