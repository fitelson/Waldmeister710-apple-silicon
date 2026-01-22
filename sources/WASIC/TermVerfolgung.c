/*=================================================================================================================
  =                                                                                                               =
  =  TermVerfolgung.c                                                                                             =
  =                                                                                                               =
  =  Aufspueren verlorener oder ueberschriebener Termzellen                                                       =
  =                                                                                                               =
  =  04.09.95 Thomas.                                                                                             =
  =                                                                                                               =
  =================================================================================================================*/

#include <stdarg.h>
#include <stdio.h>
#include <string.h>

#include "Ausgaben.h"
#include "compiler-extra.h"
#include "DSBaumOperationen.h"
#include "FehlerBehandlung.h"
#include "Parameter.h"
#include "RUndEVerwaltung.h"
#include "SymbolOperationen.h"
#include "TermIteration.h"
#include "TermOperationen.h"
#include "Termpaare.h"
#include "TermVerfolgung.h"
#include "WaldmeisterII.h"
#include "Zielverwaltung.h"

#if TV_TEST

TI_IterationDeklarieren(TV)

/*=================================================================================================================*/
/*= I. Markieren und Entmarkieren                                                                                 =*/
/*=================================================================================================================*/

/* Gegeben Symbol s <= n; n>=0 groesstes Funktionssymbol.
   Nun s' := -s+2*n+1; dann s'>n, da
     s'>n <=> -s+2*n+1>n <=> -s+n+1>0 <=> n+1>s <=> n>=s.
   Wenn s' gegeben, dann s = -s'+2*n; d. h. '^-1=='. */

static void TermzelleMarkieren(TermzellenZeigerT Zelle)
{
  SymbolT Symbol = TO_TopSymbol(Zelle);
  Symbol = -Symbol+2*SO_GroesstesFktSymb+1;
  TO_SymbolBesetzen(Zelle,Symbol);
}

#define /*void*/ TermzelleEntmarkieren(/*TermzellenZeigerT*/ Zelle)                                                 \
  TermzelleMarkieren(Zelle)

#define /*BOOLEAN*/ TermzelleIstMarkiert(/*TermzellenZeigerT*/ Zelle)                                               \
  (TO_TopSymbol(Zelle)>SO_GroesstesFktSymb)



/*=================================================================================================================*/
/*= II. Termoperationen auf der ganzen Termmenge                                                                  =*/
/*=================================================================================================================*/

typedef void (*OperationsT)(TermT);

static OperationsT OperationMerker;

static void OperationZuTP(GNTermpaarT Termpaar)                                            /* Liften auf Termpaare */
{
  (*OperationMerker)(TP_LinkeSeite(Termpaar));
  (*OperationMerker)(TP_RechteSeite(Termpaar));
  if (!TP_TermpaarIstRegel(Termpaar) && TP_IstMonogleichung(Termpaar)) {
    (*OperationMerker)(TP_LinkeSeite(TP_Gegenrichtung(Termpaar)));
    (*OperationMerker)(TP_RechteSeite(TP_Gegenrichtung(Termpaar)));
  }
}

static void OperationZuKP(TermT lhs, TermT rhs)           /* Liften auf Kritische Paaren */
{
  (*OperationMerker)(lhs);
  (*OperationMerker)(rhs);
}

static void OperationZuZiel(TermT lhs, TermT rhs, unsigned long int Nummer MAYBE_UNUSEDPARAM)
{
   (*OperationMerker)(lhs);
   (*OperationMerker)(rhs);
}
   
static void FuerAlleTerme(OperationsT Operation)
{
  GNTermpaarT TPIndex;
  SymbolT VarIndex;
  unsigned int i;

  OperationMerker = Operation;
  RE_forRegelnRobust(TPIndex,OperationZuTP(TPIndex));                                                /* Regelmenge */
  RE_forGleichungenRobust(TPIndex,OperationZuTP(TPIndex));                                      /* Gleichungsmenge */
  SO_forSymboleAbwaerts(VarIndex,SO_ErstesVarSymb,TO_SymbolNeuesterVariablenterm())
    (*Operation)(TO_Variablenterm(VarIndex));                                            /* Feld TO_Variablenterme */
  Z_ZielmengenBeobachtungsDurchlauf(OperationZuZiel);
  for(i = TI_Stapelhoehe(); i; i--)
    (*Operation)(*(TermT *)TI_Kopierstapel[i].Teilterm);
}



/*=================================================================================================================*/
/*= III. Termzellenpraedikate auf der ganzen Termmenge                                                            =*/
/*=================================================================================================================*/

typedef BOOLEAN (*PraedikatsT)(TermzellenZeigerT);

static PraedikatsT PraedikatMerker;

typedef enum {
  RegelLinks, RegelRechts, 
  AusgGleichungLinks, AusgGleichungRechts, 
  UnausGleichungLinks, UnausGleichungRechts,
  KritischesPaarLinks, KritischesPaarRechts,
  ZielLinks, ZielRechts, 
  Variablenterm,
  LokalerTerm
} TermsortenT;

typedef struct {
  TermzellenZeigerT Praedikatszelle;
  TermsortenT Termsorte;
  BOOLEAN Gefunden;
  unsigned long int Nummer;
} UmgebungsT;

static UmgebungsT Praedikatsumgebung,
                  AltePraedikatsumgebung;

static void PraedikatZuTerm(TermT Term, TermsortenT Termsorte, unsigned long int Nummer)       /* Liften auf Terme */
{
  do
    if (!Praedikatsumgebung.Gefunden && (*PraedikatMerker)(Term)) {
      Praedikatsumgebung.Praedikatszelle = Term;
      Praedikatsumgebung.Gefunden = TRUE;
      Praedikatsumgebung.Termsorte = Termsorte;
      Praedikatsumgebung.Nummer = Nummer;
    }
  while ((Term = TO_Schwanz(Term)));
}

static void PraedikatZuTP(GNTermpaarT Termpaar)                               /* Liften auf Regeln und Gleichungen */
{
  if (TP_TermpaarIstRegel(Termpaar)) {
    PraedikatZuTerm(TP_LinkeSeite(Termpaar),RegelLinks,TP_ElternNr(Termpaar));
    PraedikatZuTerm(TP_RechteSeite(Termpaar),RegelRechts,TP_ElternNr(Termpaar));
  }
  else if (TP_IstKeineMonogleichung(Termpaar)) {
    PraedikatZuTerm(TP_LinkeSeite(Termpaar),
      TP_RichtungAusgezeichnet(Termpaar) ? AusgGleichungLinks : UnausGleichungLinks,TP_ElternNr(Termpaar));
    PraedikatZuTerm(TP_RechteSeite(Termpaar),
      TP_RichtungAusgezeichnet(Termpaar) ? AusgGleichungRechts : UnausGleichungRechts,TP_ElternNr(Termpaar));
  }
  else {
    PraedikatZuTerm(TP_LinkeSeite(Termpaar),AusgGleichungLinks,TP_ElternNr(Termpaar));
    PraedikatZuTerm(TP_RechteSeite(Termpaar),AusgGleichungRechts,TP_ElternNr(Termpaar));
    PraedikatZuTerm(TP_LinkeSeite(TP_Gegenrichtung(Termpaar)),UnausGleichungLinks,TP_ElternNr(Termpaar));
    PraedikatZuTerm(TP_RechteSeite(TP_Gegenrichtung(Termpaar)),UnausGleichungRechts,TP_ElternNr(Termpaar));
  }
}

static void PraedikatZuKP(TermT lhs, TermT rhs)                                     /* Liften auf Kritische Paaren */
{
  PraedikatZuTerm(lhs,KritischesPaarLinks,0);
  PraedikatZuTerm(rhs,KritischesPaarRechts,0);
}

static void PraedikatZuZiel(TermT lhs, TermT rhs, unsigned long Nummer)
{
   PraedikatZuTerm(lhs,ZielLinks,Nummer);
   PraedikatZuTerm(rhs,ZielRechts,Nummer);
}

static BOOLEAN FuerAlleZellenPraed(PraedikatsT Praedikat)
/* Durchlaeuft alle Termzellen und wendet Praedikat auf sie an bis zu folgendem Zeitpunkt:
   Bei der ersten Termzelle, die das Praedikat erfuellt, wird die Variable Praedikatsumgebung besetzt;
   und auf die restlichen Termzellen wird kein Praedikat mehr angewandt. */
{
  GNTermpaarT TPIndex;
  SymbolT VarIndex;
  unsigned int i;
  PraedikatMerker = Praedikat;
  Praedikatsumgebung.Gefunden = FALSE;

  RE_forRegelnRobust(TPIndex,PraedikatZuTP(TPIndex));                                                /* Regelmenge */
  RE_forGleichungenRobust(TPIndex,PraedikatZuTP(TPIndex));                                      /* Gleichungsmenge */
  SO_forSymboleAbwaerts(VarIndex,SO_ErstesVarSymb,TO_SymbolNeuesterVariablenterm())    /* Feld TO_Variablenterme */
    PraedikatZuTerm(TO_Variablenterm(VarIndex),Variablenterm,SO_VarNummer(VarIndex));
  Z_ZielmengenBeobachtungsDurchlauf(PraedikatZuZiel);
  for(i = TI_Stapelhoehe(); i; i--)
    PraedikatZuTerm(*(TermT *)TI_Kopierstapel[i].Teilterm,LokalerTerm,i);
  if (!Praedikatsumgebung.Gefunden)                                                                     /* KPMenge */
  return FALSE;
}



/*=================================================================================================================*/
/*= IV. Operationen und Praedikate                                                                                =*/
/*=================================================================================================================*/

static unsigned long int AnzahlZellen;

static void LaengenAddieren(TermT Term)
{
  AnzahlZellen += TO_Termlaenge(Term);
}

static BOOLEAN SchonMarkiert(TermzellenZeigerT Zelle)
{
  AnzahlZellen++;
  if (TermzelleIstMarkiert(Zelle))
    return TRUE;
  TermzelleMarkieren(Zelle);
  return FALSE;
}

static BOOLEAN ErstesAuftreten(TermzellenZeigerT Zelle)
{
  return TO_PhysischGleich(Zelle,AltePraedikatsumgebung.Praedikatszelle);
}

static BOOLEAN InhaltSinnvoll(TermzellenZeigerT Zelle)
{
  SymbolT Symbol = TO_TopSymbol(Zelle);
  AnzahlZellen++;
  return (2*BO_NeuesteBaumvariable()>Symbol && TO_SymbolNeuesterVariablenterm()>Symbol) 
    || Symbol>SO_GroesstesFktSymb;
}

static void TermEntmarkieren(TermT Term)
{
  do
    TermzelleEntmarkieren(Term);
  while ((Term = TO_Schwanz(Term)));
}



/*=================================================================================================================*/
/*= V. Fehlerausgaenge                                                                                            =*/
/*=================================================================================================================*/

static void AnzahlZellenTesten(unsigned int Grad)
{
  unsigned long int delta = SV_AnzNochBelegterElemente(TO_FreispeicherManager)-AnzahlZellen;
  if (delta) {
    char Meldung[100];
    sprintf(Meldung,"TV_TermzellenVerfolger(%u): Es fehlen %lu Termzellen.",Grad,delta);
    our_error(Meldung);
  }
}

/* Achtung: Die folgende Funktion kann evtl. den String Meldung ueberschreiben. */
static void UmgebungAnhaengen(char *Meldung, UmgebungsT Umgebung)
{
  TermsortenT Sorte = Umgebung.Termsorte;
  unsigned long int Nummer = Umgebung.Nummer;
  Meldung += strlen(Meldung);

  if (Sorte<=RegelRechts)
    sprintf(Meldung,"Regel Nr. %lu, %s Seite",Nummer,Sorte==RegelLinks ? "linke" : "rechte");
  else if (Sorte<=UnausGleichungRechts)
    sprintf(Meldung,"Gleichung Nr. %lu, %sausgezeichnete Richtung, %s Seite",Nummer,
      Sorte<UnausGleichungLinks ? "" : "un",
      (Sorte==AusgGleichungLinks || Sorte==UnausGleichungLinks) ? "linke" : "rechte");
  else if (Sorte<=KritischesPaarRechts)
    sprintf(Meldung,"KritischesPaar Nr. %lu, %s Seite",Nummer,Sorte==KritischesPaarLinks ? "linke" : "rechte");
  else if (Sorte<=ZielRechts)
    sprintf(Meldung,"Ziel Nr. %lu, %s Seite",Nummer,Sorte==ZielLinks ? "linke" : "rechte");
  else if (Sorte<=Variablenterm)
    sprintf(Meldung,"Variablenterm");
  else if (Sorte<=LokalerTerm)
    sprintf(Meldung,"LokalerTerm");
  else
    our_error("TV_UmgebungAnhaengen: unbekannte Termsorte");
}

static void InhaltTesten(void)
{
  if (Praedikatsumgebung.Gefunden) {
    char Meldung[100];
    sprintf(Meldung,"TV_TermzellenVerfolger(3): unsinniger Zelleninhalt\n");
    UmgebungAnhaengen(Meldung,Praedikatsumgebung);
    our_error(Meldung);
  }
}

static void UeberschreibenTesten(unsigned int Grad)
{
  if (Praedikatsumgebung.Gefunden) {                                    /* dann doppelt benutzte Zelle aufgetreten */
    char Meldung[100];
    AltePraedikatsumgebung = Praedikatsumgebung;
    sprintf(Meldung,"TV_TermzellenVerfolger(%u): doppelt benutzteZelle\n",Grad);
    UmgebungAnhaengen(Meldung,Praedikatsumgebung);
    sprintf(Meldung+strlen(Meldung),"\n");
    UmgebungAnhaengen(Meldung,AltePraedikatsumgebung);
    FuerAlleZellenPraed(ErstesAuftreten);
    our_error(Meldung);
  }
}



/*=================================================================================================================*/
/*= VI. Anzeigen lokaler Terme                                                                                    =*/
/*=================================================================================================================*/

void TV_LokaleTermeAnVerfolgerMelden(unsigned int Anzahl, ...)
{                  /* Die lokalen Variablen werden ueber ihre Adresse uebergeben, duerfen also den Inhalt aendern. */
  va_list ArgumentListe;

  va_start(ArgumentListe,Anzahl);
  while (Anzahl--) {
    TermT *Term = va_arg(ArgumentListe,TermT*);
    TI_Push((TermT)Term);
  }
  va_end(ArgumentListe);
}


void TV_LokaleTermeAbmelden(unsigned int Anzahl)               /* Komplementaer zu TV_LokaleTermeAnVerfolgerMelden */
{
  while (Anzahl--)
    TI_Pop();
}



/*=================================================================================================================*/
/*= VII. Testfunktion                                                                                             =*/
/*=================================================================================================================*/

void TV_TermzellenVerfolger(unsigned int Grad)
{/*
   Grad von 0 bis 3.
   0 - nichts tun.
   1 - Durchlauf der Regelmenge, der Gleichungsmenge, der KP-Menge, der TO_Variablenterme 
       dabei Anzahl der verwendeten Termzellen bestimmen
       schlussendlich Vergleich mit Wert im Freispeichermanager der Termoperationen
       gegebenenfalls Fehlermeldung
   2 - Zusaetzlich ueberpruefen, dass keine Zelle mehrfach verwendet wird.
       Durchlauf der Regelmenge, der Gleichungsmenge, der KP-Menge, der TO_Variablenterme
       dabei Anzahl der verwendeten Termzellen bestimmen und jede erreichte Termzelle markieren
       Es wird vorausgesetzt, dass die Zelleninhalte sinnvoll sind.
       Fehler erkannt, falls bereits markierte Termzelle wieder erreicht wird
         dann nochmaliger Durchlauf, Feststellen des ersten Auftretens und Fehlermeldung
       ansonsten erkannte Anzahl mit Wert im Freispeichermanager der Termoperationen vergleichen
       gegebenenfalls Fehlermeldung
       andernfalls Markierungen aufheben
   3 - Zusaetzlich ueberpruefen, dass die Zelleninhalte sinnvoll sind.
*/
  printf("Termzellen verfolgen ... \n");
#if !DM_STATISTIK
  printf("                     ... nicht moeglich ohne Speicherstatistik.\n");
  return;
#endif
  AnzahlZellen = 0;

  switch (Grad) {
  case 1:
    FuerAlleTerme(LaengenAddieren);
    AnzahlZellenTesten(1);
    break;

  case 3:
    FuerAlleZellenPraed(InhaltSinnvoll);
    InhaltTesten();
    AnzahlZellenTesten(3);
    AnzahlZellen = 0;
  case 2:
    FuerAlleZellenPraed(SchonMarkiert);
    UeberschreibenTesten(2);
    AnzahlZellenTesten(2);
    FuerAlleTerme(TermEntmarkieren);
    break;

  default:
    break;
  }

  printf("                     ... ok\n");
}


/*=================================================================================================================*/
/*= IX. Leere Funktionen                                                                                          =*/
/*=================================================================================================================*/

#else

void TV_LokaleTermeAnVerfolgerMelden(unsigned int Anzahl MAYBE_UNUSEDPARAM, ...) {;}

void TV_LokaleTermeAbmelden(unsigned int Anzahl MAYBE_UNUSEDPARAM) {;}              

void TV_TermzellenVerfolger(unsigned int Grad MAYBE_UNUSEDPARAM) {;}

unsigned int TV_AusdehnungKopierstapel(void) {return 0;}

#endif
