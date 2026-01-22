/*=================================================================================================================
  =                                                                                                               =
  =  MatchOperationen.c                                                                                           =
  =                                                                                                               =
  =  Matchen von Termen mit Regel- oder Gleichungsbaum                                                            =
  =                                                                                                               =
  =                                                                                                               =
  =  26.03.94  Thomas.                                                                                            =
  =                                                                                                               =
  =================================================================================================================*/

#include "Ausgaben.h"
#include "DSBaumKnoten.h"
#include "general.h"
#include "Grundzusammenfuehrung.h"
#include "Hauptkomponenten.h"
#include "MatchOperationen.h"
#include "MissMarple.h"
#include "RUndEVerwaltung.h"
#include "SpeicherVerwaltung.h"
#include "SymbolOperationen.h"
#include "TermIteration.h"
#include "Ordnungen.h"

#ifndef CCE_Source
#include "Hauptkomponenten.h"
#else
#define IncVorwaertsMatchR() /* nix tun */
#define IncVorwaertsMatchE() /* nix tun */
#define IncVorwaertsMatchS() /* nix tun */
#define IncAnzRedMisserfolgeOrdnung() /* nix tun */
static unsigned long HK_AbsTime;
#endif

TI_IterationDeklarieren(MO)
TI2_IterationDeklarieren(MO)

/* Bei konkurrenter Freispeicherbenutzung sind Kommentare "$nested-feature" zu beruecksichtigen. */



/*=================================================================================================================*/
/*= I. Variablenbank fuer Substitutionen; sonstige globale Groessen; Statistik-Funktionen                         =*/
/*=================================================================================================================*/

RegelOderGleichungsT MO_AngewendetesObjekt;

static int xMDimension;           /* xO realisiert Abbildung Variablensymbol->UTerm. Das Feld wird mit den Variab- */
typedef struct {    /* lensymbolen negativ indiziert und hat die Laenge xMDimension. Fuer xM[0] wird kein Speicher */
  UTermT Substitut,    /* angelegt. Vorg ist die letzte Zelle im Anfrageterm, Nachf entsprechend am Substitutende. */
         Vorg,
         Nachf;                       /* Das Flag SchonEingebaut wird beim Anwenden der Substitution benoetigt und */
  BOOLEAN SchonEingebaut;   /* gibt an, ob die das Substitut repraesentierende Termzellenkette aus dem Anfrageterm */
} MO_EintragsT;                                        /* schon in den neuen Term eingebaut worden ist oder nicht. */
static MO_EintragsT *xM = NULL;                                                      /* vor Beginn des Substituts. */
static SymbolT xMletztes;

static TermT zuSchuetzenderTerm = NULL;
static TermT schutzTerm = NULL;

/* Beachte: Reduktionen mit Gleichungen wie f(x,y)=f(x,z) von rechts nach links, die rechts freie Variablen haben, 
   werden nur durchgefuehrt, wenn die gewaehlte Praezedenz total, die Reduktionsordnung also grundtotal ist. Fuer 
   freie Variablen wird dabei stets die kleinste Konstante eingesetzt. Der entsprechende Teil der matchenden 
   Substitution wird nicht in xM abgelegt; die Datenstruktur der Gleichung enthaelt dafuer bereits eine passende
   Teilinstanziierung der rechten Seite. */

unsigned int MO_AusdehnungVariablenbank(void)
{
  return -xMDimension;
}

void MO_VariablenfeldAnpassen(SymbolT DoppelteNeuesteBaumvariable)              /* xM wird initial als Feld mit Ausdehnung */
{                            /* MO_AnzErsterVariablen angelegt und muss gegebenfalls nach links ausgedehnt werden. */
  if (SO_VariableIstNeuer(DoppelteNeuesteBaumvariable,xMDimension)) {
    our_dealloc(xM+xMDimension);                            /* Addition von xMDimension fuer negative Adressierung */
    xMDimension = SO_VarDelta(DoppelteNeuesteBaumvariable,MO_AnzErsterVariablen);
    xM = (MO_EintragsT *) our_alloc(-xMDimension*sizeof(MO_EintragsT)) - xMDimension;
  }
}


static KantenT BaumIndex;                                         /* Zeiger fuer Durchlauf des Baumes beim Matchen */
#define aktTermrest BK_aktTIndexM(BaumIndex)
#define SymbolAusTerm TO_TopSymbol(aktTermrest)                 /* naechstes zu matchendes Zeichen aus Anfrageterm */

static TermT gefundeneRechteSeite;    /* haelt gefundene rechte Seite zwischen Matchen und Erzeugen von SigmaR vor */
static TermT gefundeneLinkeSeite;
#define TopsymbolRechteSeite TO_TopSymbol(gefundeneRechteSeite)  

static UTermT Anfrageterm;                                         /* globale Variablen; Zugriff von fast ueberall */
static UTermT letzteZelleAnfrageterm;                                                   /* dessen letzte Termzelle */
static UTermT letzteNichtVernutzteZelle;                                     /* letzte nicht rezyklierte Termzelle */

static BOOLEAN NichtGanzerTermGebunden;                           /* Information fuer Beseitigung des Anfrageterms */



/*=================================================================================================================*/
/*= II. Elementare Operationen auf oder mit Substitutionen                                                        =*/
/*=================================================================================================================*/

#define SubstAufIdentitaetSetzen()                                                                                  \
  xMletztes = SO_VorigesVarSymb(SO_ErstesVarSymb);


#define MatcherInitialisieren(/*TermT*/ Term)                                                                       \
  letzteZelleAnfrageterm = TO_TermEnde(Anfrageterm = Term);                                                         \
  NichtGanzerTermGebunden = TRUE;                                                                                   \
  SubstAufIdentitaetSetzen();


static void SubstErgaenzen(UTermT Substitut, UTermT Vorg)      /* fuegt zur Substitution x(n)<-t(n) ... x(1)<-t(1) */
{                                                                           /* die Bindung x(n+1)<-Substitut hinzu */
  SO_NaechstesVarSymbD(xMletztes);
#if 0
  IO_DruckeFlex("SSS x%d <- %t\n", -xMletztes, Substitut);
  if (xMletztes < xMDimension)
    our_error("Variable zu jung!");
#endif
  xM[xMletztes].Substitut = Substitut;
  xM[xMletztes].Nachf = TO_NaechsterTeilterm(Substitut);
  xM[xMletztes].SchonEingebaut = FALSE;
  if (!(xM[xMletztes].Vorg = Vorg))
    NichtGanzerTermGebunden = FALSE;
}


#define /*BOOLEAN*/ VarUngebunden(/*SymbolT*/ VarSymbol)                                                            \
  SO_VariableIstNeuer(VarSymbol,xMletztes)


#define /*BOOLEAN*/ VarGebunden(/*SymbolT*/ VarSymbol)                                                              \
  SO_VariableIstNichtNeuer(VarSymbol,xMletztes)


#define /*BOOLEAN*/ SymbKeineGebundeneVar(/*SymbolT*/ Symbol)                                                       \
  (SO_SymbIstFkt(Symbol) || VarUngebunden(Symbol))


#define SubstitutVon(/*SymbolT*/ VarSymbol)                                                                         \
  xM[VarSymbol].Substitut


static BOOLEAN EsFolgtSubstitutVon(SymbolT VarSymbol, UTermT TermIndex) 
/* ermittelt, ob der bezeichnete Term als Praefix Sigma(VarSymbol) hat */
{
  UTermT SubstitutIndex = SubstitutVon(VarSymbol);
  UTermT NULLSubstitut = TO_NaechsterTeilterm(SubstitutIndex);
  do {
    if (SO_SymbUngleich(TO_TopSymbol(TermIndex),TO_TopSymbol(SubstitutIndex))) {
      return FALSE;
    }
    TermIndex = TO_Schwanz(TermIndex);
    SubstitutIndex = TO_Schwanz(SubstitutIndex);
  } while (TO_PhysischUngleich(SubstitutIndex,NULLSubstitut));
  return TRUE;
}


void MO_DruckeMatch(void)
{
  SymbolT VarIndex;

  SO_forSymboleAbwaerts(VarIndex,SO_ErstesVarSymb,xMletztes) {
    printf("x");
    IO_DruckeGanzzahl(SO_VarNummer(VarIndex));
    printf("<-");
    IO_DruckeTerm(SubstitutVon(VarIndex));
    IO_LF();
  }
}



/*=================================================================================================================*/
/*= III. Operationen zum Matchen mit Regeln auf Blattebene                                                        =*/
/*=================================================================================================================*/

static SymbolT xMletztesAlt;

static BOOLEAN RegelInBlattGefunden(void)       /* Im Baum begonnenen Match-Versuch fortsetzen und falls moeglich beenden */
{
  UTermT Termrest = aktTermrest,
         Regelrest = BK_Rest(BaumIndex),
         Vorg = BK_MVorgZelle(BaumIndex);        /* letzte Termzelle vor gerade betrachtetem Rest des Anfrageterms */

  xMletztesAlt = xMletztes;                   /* letzte im Baum vorgenommene Bindung vermerken (fuer Ruecksetzung) */
  while (Regelrest) {                              /* anstatt Abfrage auf Ende des (unbeschnittenen!) Anfrageterms */
    SymbolT SymbAusRegel = TO_TopSymbol(Regelrest);
    if (SO_SymbIstVar(SymbAusRegel)) {
      if (VarUngebunden(SymbAusRegel)) {                                     /* ungebundene Variable aus Regelterm */
        SubstErgaenzen(Termrest,Vorg);                                                    /* in Anfrageterm binden */
        Termrest = TO_Schwanz(Vorg = TO_TermEnde(Termrest));
        Regelrest = TO_Schwanz(Regelrest);
      }
      else if (EsFolgtSubstitutVon(SymbAusRegel,Termrest)) {                   /* fuer gebundene Variable pruefen, */
        Termrest = TO_Schwanz(Vorg = TO_TermEnde(Termrest));                             /* ob sich der Anfragterm */
        Regelrest = TO_Schwanz(Regelrest);                                               /* entsprechend fortsetzt */
      }
      else {                                                           /* Falls nicht, den Matchversuch abbrechen. */
        xMletztes = xMletztesAlt;                                          /* Dabei Bindungstabelle zuruecksetzen. */
        return FALSE;
      }
    }
    else if (SO_SymbUngleich(SymbAusRegel,TO_TopSymbol(Termrest))) {              /* Abbruch ebenso bei ungleichen */
      xMletztes = xMletztesAlt;                                                              /* Funktionssymbolen. */
      return FALSE;
    }
    else {                                               /* Bei gleichen Funktionssymbolen naechste Symbole lesen. */
      Termrest = TO_Schwanz(Vorg = Termrest);
      Regelrest = TO_Schwanz(Regelrest);
    }
  }
  return TRUE;
}


#define /*void*/ MatcherNachBlattZuruecksetzen(/*void*/)                                                            \
  xMletztes = xMletztesAlt
  

#define /*BOOLEAN*/ FreieVariablenOK(/*GleichungsT*/ Richtung)           /* Bezug: Gleichungen a la f(x,y)=f(x,z) */\
  (ORD_OrdnungTotal || !TP_FreieVariablen(Richtung))



/*=================================================================================================================*/
/*= IV. Operationen zum Matchen mit Regel- sowie Gleichungsbaum                                                   =*/
/*=================================================================================================================*/

extern BOOLEAN drucken;
void druckeSchutzverletzung(RegelOderGleichungsT Objekt);

static BOOLEAN spezielleSchutzverletzung(TermT Term, RegelOderGleichungsT Objekt)
{
  if ((zuSchuetzenderTerm != NULL) 
    && (zuSchuetzenderTerm == Term)
  /* &&  MO_existiertMatch(re->linkeSeite,Term) 
     && !MO_existiertMatch(t, re->linkeSeite); XXX */){
    if (drucken) druckeSchutzverletzung(Objekt);
    return TRUE;
  }
  return FALSE;
}

#define /*void*/ BaumwurzelInitialisieren(/*DSBaumT*/ DSBaum, /*UTermT*/ Term)                                      \
  BK_MSymbolSei(BaumIndex = DSBaum.Wurzel,BK_NULL);           /* erstmalig besuchten Knoten als solchen markieren */\
  BK_aktTIndexMSei(BaumIndex,Term)                                        /* Anfrageterm in Wurzelknoten schreiben */


#define AbstiegUmSymbol aktTermrest
#define AbstiegUmTeilterm TO_TermEnde(aktTermrest)
#define AbstiegVerwalten(/*SymbolT*/ Symbol, /*UTermT*/ VorNeuemTerm)                                               \
{                                                                                                                   \
  KantenT Nachf = BK_Nachf(BaumIndex,Symbol);                                                                       \
  BK_MSymbolSei(BaumIndex,Symbol);                                                                                  \
  BK_aktTIndexMSei(Nachf,TO_Schwanz(VorNeuemTerm));                   /* noch zu bearbeitenden Termrest eintragen */\
  BK_MVorgZelleSei(Nachf,VorNeuemTerm);                                                                             \
  BK_MSymbolSei(Nachf,BK_NULL);                                 /* Knoten mit erster Uebergangsfunktion markieren */\
  BaumIndex = Nachf;                                /* absteigen. Achtung: aktTermrest ist Makro ueber BaumIndex! */\
  return TRUE;                                                                                                      \
}


#define AbMitFunktionssymbol()                                                                                      \
  if (SO_SymbIstFkt(SymbolAusTerm) && (!GZ_GRUNDZUSAMMENFUEHRUNG || !SO_IstExtrakonstante(SymbolAusTerm)) && BK_Nachf(BaumIndex,SymbolAusTerm)) {         /* Abstieg mit Funktionssymbol */\
    AbstiegVerwalten(SymbolAusTerm,AbstiegUmSymbol);                                                                \
  }


#define AbMitNeuerVariable()                                                                                        \
  if (BK_letzteVarBelegt(BaumIndex)) {                                                                              \
    SubstErgaenzen(aktTermrest,BK_MVorgZelle(BaumIndex));                                                           \
    AbstiegVerwalten(BK_letzteVar(BaumIndex),AbstiegUmTeilterm);                                                    \
  }


#define BindungAufheben()                                                                                           \
  SO_VorigesVarSymbD(xMletztes)


#define KeineBindungAufheben()


#define AbMitAlterVariable(/*SymbolT*/ ersteKandidatin, /*Statement*/ BindungAufhebenOderAuchNicht)                 \
{                                                                                                                   \
  SymbolT letzteKandidatin = min(SO_ErstesVarSymb,BK_maxSymb(BaumIndex)),                                           \
          VarIndex;                                                                                                 \
  BindungAufhebenOderAuchNicht;                                                                                     \
  SO_forSymboleAufwaerts(VarIndex,ersteKandidatin,letzteKandidatin)                                                 \
    if (BK_Nachf(BaumIndex,VarIndex) && EsFolgtSubstitutVon(VarIndex,aktTermrest)) {                                \
      AbstiegVerwalten(VarIndex,AbstiegUmTeilterm);                                                                 \
    }                                                                                                               \
}


static BOOLEAN EinenKnotenAbOderVieleKnotenAufgestiegen(void)
{ /* Entweder einen Knoten absteigen oder so lange hoch, bis es moeglich ist, in einen noch nicht besuchten Knoten */
  do {                                                    /* abzusteigen, dies dann tun, oder bis ueber die Wurzel */
    SymbolT MSymbol = BK_MSymbol(BaumIndex);
    if (SO_SymbGleich(MSymbol,BK_NULL)) {                                         /* Knoten wird erstmalig besucht */
      AbMitFunktionssymbol();
      AbMitNeuerVariable();
      AbMitAlterVariable(BK_minSymb(BaumIndex),KeineBindungAufheben());
    }
    else if (SO_SymbIstFkt(MSymbol)) {                            /* Wiederbesuch nach Abstieg mit Funktionssymbol */
      AbMitNeuerVariable();
      AbMitAlterVariable(BK_minSymb(BaumIndex),KeineBindungAufheben());
    }
    else if (SO_SymbGleich(MSymbol,BK_letzteVar(BaumIndex))) {  /* ... nach Abstieg mit gerade gebundener Variable */
      AbMitAlterVariable(SO_VorigesVarSymb(MSymbol),BindungAufheben());
    }
    else {                                               /* ... nach Abstieg mit schon frueher gebundener Variable */
      AbMitAlterVariable(SO_VorigesVarSymb(MSymbol),KeineBindungAufheben());
    }
  } while ((BaumIndex = BK_Vorg(BaumIndex)));
  return FALSE;
}         

#define /*void*/ GgfProtokollieren(/*UTermT*/ Term)

/* ******************************************************************************************************************* */
/* ******************************************************************************************************************* */
/* ******************************************************************************************************************* */

#define SubstErgaenzenM(/*BOOLEAN*/DO_EQNS,/*UTermT*/ SubstitutT, /*UTermT*/ VorgT)	\
{									\
  SO_NaechstesVarSymbD(lxMletztes);					\
  xM[lxMletztes].Substitut = SubstitutT;/*Opt bringt nichts */		\
  /*xM[lxMletztes].Nachf = TO_NaechsterTeilterm(SubstitutT);*/		\
  /*xM[lxMletztes].SchonEingebaut = FALSE;*/				\
  xM[lxMletztes].Vorg = VorgT;						\
  /* nicht noetig bei Regeln: */					\
  if (DO_EQNS && !(VorgT))							\
      NichtGanzerTermGebunden = FALSE;					\
}

#if 1
/* Bactracking im Baum */
#define DeclareBacktrack()			\
  KantenT BackTrIndex;  

#define InitBacktrack()				\
  BackTrIndex = NULL;

#define BacktrackPush(MSymbol)			\
{						\
  BK_backtrKnotenSei(BaumIndex,BackTrIndex);	\
  BK_backtrSubSei(BaumIndex,lxMletztes);	\
  BK_MVorgZelleSei(BaumIndex,Vorg);		\
  BK_MSymbolSei(BaumIndex,MSymbol);		\
  BackTrIndex = BaumIndex;			\
}

#define BacktrackPop()											\
{													\
  BaumIndex   = BackTrIndex;										\
  BackTrIndex = BK_backtrKnoten(BaumIndex);								\
  lxMletztes  = BK_backtrSub(BaumIndex);								\
  Symbol = BK_MSymbol(BaumIndex);									\
  Vorg = BK_MVorgZelle(BaumIndex);									\
  Termrest = (Vorg) ? TO_Schwanz(Vorg) : Term /* Problem: Backtrack to root-node -> Vorg == NULL!!*/;	\
}

#define canBacktrack()				\
  (BackTrIndex != NULL)

#else
/* Backtracking in extra Stack */
typedef struct {
  SymbolT MSymbol;
  SymbolT xMletztes;
  KantenT BaumIndex;
  UTermT  Vorg;
} BacktrackT;

static BacktrackT bt[1000];

#define DeclareBacktrack()			\
  BacktrackT *BackTrIndex;  

#define InitBacktrack()				\
  BackTrIndex = bt - 1;

#define BacktrackPush(Symbol)				\
{						\
  BackTrIndex++;				\
  BackTrIndex->MSymbol   = Symbol;		\
  BackTrIndex->xMletztes = lxMletztes;		\
  BackTrIndex->BaumIndex = BaumIndex;		\
  BackTrIndex->Vorg      = Vorg;		\
}

#define BacktrackPop()											\
{													\
  Symbol      = BackTrIndex->MSymbol;									\
  lxMletztes  = BackTrIndex->xMletztes;									\
  BaumIndex   = BackTrIndex->BaumIndex;									\
  Vorg        = BackTrIndex->Vorg;									\
  Termrest = (Vorg) ? TO_Schwanz(Vorg) : Term /* Problem: Backtrack to root-node -> Vorg == NULL!!*/;	\
  BackTrIndex--;											\
}

#define canBacktrack()				\
  (BackTrIndex >= bt)

#endif



#define AbstiegUmSymbolM Termrest
#define AbstiegUmTeiltermM TO_TermEnde(Termrest)

#define AbstiegVerwaltenM(/*UTermT*/ VorNeuemTerm)		\
{								\
  Vorg = VorNeuemTerm;						\
  Termrest = TO_Schwanz(Vorg);					\
  BaumIndex = BK_Nachf(BaumIndex,Symbol);			\
  if (BK_IstBlatt(BaumIndex)) goto leave; else goto tryFct;	\
}

#define SubstInOrdnungBringen()										   \
{													   \
    letzteZelleAnfrageterm = ((Anfrageterm = Term)->Ende);						   \
    NichtGanzerTermGebunden = TRUE;									   \
    xMletztes = lxMletztes;										   \
    for (Symbol = SO_ErstesVarSymb; Symbol >= lxMletztes; Symbol--){					   \
/*	xM[Symbol].Substitut = TO_Schwanz(xM[Symbol].Vorg); / *schlecht wg. Test fuer bereits geb. Vars */ \
	xM[Symbol].Nachf = TO_NaechsterTeilterm(xM[Symbol].Substitut);					   \
	xM[Symbol].SchonEingebaut = FALSE;								   \
    }													   \
}

#if 0

/* Hier muss Symbol++ auskommentiert werden!! */

static SymbolT calcLetzteKandidatin(KantenT BaumIndex)
{
  SymbolT lk;
  for (lk = -1; lk > BK_minSymb(BaumIndex); lk --){
    if (BK_Nachf(BaumIndex,lk)){
      return lk;
    }
  }
  return lk;
  /*  return min(SO_ErstesVarSymb,BK_maxSymb(BaumIndex));*/
}
static BOOLEAN esGibtNochAlteVarMitBindung(KantenT BaumIndex, SymbolT Symbol, UTermT Termrest)
{
  SymbolT lk;
  for (lk = -1; lk > Symbol; lk --){
    if (BK_Nachf(BaumIndex,lk) && EsFolgtSubstitutVon(lk,Termrest)){
      return TRUE;
    }
  }
  return FALSE;
}
static SymbolT naechsteAlteVarMitBindung(KantenT BaumIndex, SymbolT Symbol, UTermT Termrest)
{
  SymbolT lk;
  for (lk = Symbol+1; lk < 0; lk++){
    if (BK_Nachf(BaumIndex,lk) && EsFolgtSubstitutVon(lk,Termrest)){
      return lk;
    }
  }
  return 0;
}

#define Backtrack1()								\
{										\
    if (BK_Nachf(BaumIndex,BK_letzteVar(BaumIndex))){				\
	BacktrackPush(Symbol);							\
    }										\
    else {									\
	SymbolT x = naechsteAlteVarMitBindung(BaumIndex,Symbol,Termrest);	\
	if (x) BacktrackPush(x);						\
    }										\
}

#define Backtrack2()							\
{									\
    SymbolT x = naechsteAlteVarMitBindung(BaumIndex,Symbol,Termrest);	\
    if (x) BacktrackPush(x);						\
}

#define Backtrack3()							\
{									\
    SymbolT x = naechsteAlteVarMitBindung(BaumIndex,Symbol,Termrest);	\
    if (x) BacktrackPush(x);						\
}

#else


#define BacktrackP1() /* SO_SymbIstVar(BK_minSymb(BaumIndex))*/			\
 (BK_Nachf(BaumIndex,BK_letzteVar(BaumIndex)) ||				\
  esGibtNochAlteVarMitBindung(BaumIndex,BK_letzteVar(BaumIndex),Termrest))

#define BacktrackP2() /* BK_maxSymb(BaumIndex)*/ /*calcLetzteKandidatin(BaumIndex) > Symbol)*/	\
  (Symbol != SO_ErstesVarSymb) && 								\
    esGibtNochAlteVarMitBindung(BaumIndex,Symbol,Termrest)

#define BacktrackP3() /*letzteKandidatin > Symbol*/		\
  esGibtNochAlteVarMitBindung(BaumIndex,Symbol,Termrest)

#define Backtrack1()							\
{									\
    if (SO_SymbIstVar(BK_minSymb(BaumIndex))) BacktrackPush(Symbol);	\
}

#define Backtrack2()											\
{													\
    if ((Symbol != SO_ErstesVarSymb) && (BK_maxSymb(BaumIndex) > Symbol)) BacktrackPush(Symbol);	\
}

#define Backtrack3()						\
{								\
    if (letzteKandidatin > Symbol) BacktrackPush(Symbol);	\
}

static SymbolT calcLetzteKandidatin(KantenT BaumIndex)
{
#if 1
  SymbolT lk;
  for (lk = -1; lk > BK_minSymb(BaumIndex); lk --){
    if (BK_Nachf(BaumIndex,lk)){
      return lk;
    }
  }
  return lk;
#else
  return min(SO_ErstesVarSymb,BK_maxSymb(BaumIndex));
#endif
}
#endif



BOOLEAN MO_RegelGefunden(UTermT Term) 
{
  IncVorwaertsMatchR();
  if (RE_RegelmengeLeer())
    return FALSE;
  else {
    AbstractTime time = HK_AbsTime;
    SymbolT Symbol,lxMletztes,letzteKandidatin;
    KantenT BaumIndex;
    UTermT Termrest, Regelrest, Vorg;
    DeclareBacktrack();
  
    /* Initialisierung der lokalen Variablen */
    
    BaumIndex = RE_Regelbaum.Wurzel;
    Termrest = Term;
    Vorg = NULL;
    lxMletztes = SO_VorigesVarSymb(SO_ErstesVarSymb);
    InitBacktrack();

    tryFct:
        Symbol = TO_TopSymbol(Termrest);
        if (SO_SymbIstFkt(Symbol) &&
            (!GZ_GRUNDZUSAMMENFUEHRUNG || !SO_IstExtrakonstante(Symbol)) &&
            BK_Nachf(BaumIndex,Symbol)){
            Backtrack1();
            AbstiegVerwaltenM(AbstiegUmSymbolM);
        }
    tryNVar:
        Symbol = BK_letzteVar(BaumIndex);
        if (BK_Nachf(BaumIndex,Symbol)) {
            Backtrack2();
            SubstErgaenzenM(FALSE,Termrest,Vorg);
            AbstiegVerwaltenM(AbstiegUmTeiltermM);
        }
  /*tryOVar:*/
        Symbol = BK_minSymb(BaumIndex);                    /* Optimierung: Wir wissen, dass es keine neue Var gibt */
    tryOVar1:
        letzteKandidatin = calcLetzteKandidatin(BaumIndex);
        while (Symbol <= letzteKandidatin){
            if (BK_Nachf(BaumIndex,Symbol) &&
                EsFolgtSubstitutVon(Symbol,Termrest)) {
                Backtrack3();
                AbstiegVerwaltenM(AbstiegUmTeiltermM);
            }
            Symbol++;
        }
    backtrack:
        if (!canBacktrack())
            return FALSE;
        BacktrackPop();
        if (SO_SymbIstFkt(Symbol))
          goto tryNVar;
        Symbol++;
        goto tryOVar1;
    leave:
        Regelrest = BK_Rest(BaumIndex);

        while (Regelrest) {                        /* anstatt Abfrage auf Ende des (unbeschnittenen!) Anfrageterms */
            SymbolT SymbAusRegel = TO_TopSymbol(Regelrest);
            if (SO_SymbIstVar(SymbAusRegel)) {
                if (SO_VariableIstNeuer(SymbAusRegel,lxMletztes)) {          /* ungebundene Variable aus Regelterm */
                    SubstErgaenzenM(FALSE,Termrest,Vorg);                                 /* in Anfrageterm binden */
                }
                else if (!EsFolgtSubstitutVon(SymbAusRegel,Termrest)) {        /* fuer gebundene Variable pruefen, */
                    goto backtrack;                /* ob sich der Anfragterm entsprechend fortsetzt oder eben nicht*/
                }
                Termrest = TO_Schwanz(Vorg = TO_TermEnde(Termrest));                 /* einen Teilterm weitergehen */
                Regelrest = TO_Schwanz(Regelrest);
            }
            else if (SO_SymbUngleich(SymbAusRegel,TO_TopSymbol(Termrest))) {      /* Abbruch ebenso bei ungleichen */
                goto backtrack;
            }
            else {                                       /* Bei gleichen Funktionssymbolen naechste Symbole lesen. */
                Termrest = TO_Schwanz(Vorg = Termrest);                                       /* ein Symbol weiter */
                Regelrest = TO_Schwanz(Regelrest);
            }
        }/*success:*/
	/*ab hier im Blatt im Baum*/
        {
	  RegelT Index = BK_Regeln(BaumIndex);
	  do {
	    if (TP_lebtZumZeitpunkt_M(Index, time /*HK_getAbsTime_M()*/)) {
	      if (!(Index->DarfNichtReduzieren)) {
                SubstInOrdnungBringen();
		gefundeneRechteSeite = Index->rechteSeite;
                gefundeneLinkeSeite = BK_Regeln(BaumIndex)->linkeSeite;
		if (!spezielleSchutzverletzung(Term, Index)) {
		  MO_AngewendetesObjekt = Index;
		  return TRUE;
		}
                else {
                  goto backtrack;
                }
	      }
	    }
	    else {
	      if (0&&IO_level4()) {
		if (Index->todesTag < maximalAbstractTime()){
		  IO_DruckeFlex("Die Regel %t -> %t lebt zum Zeitpunkt %d nicht mehr! Gestorben %ul\n", Index->linkeSeite,\
				Index->rechteSeite, HK_getAbsTime_M(), Index->todesTag);
		}
		else {
		  IO_DruckeFlex("Die Regel %t -> %t lebt zum Zeitpunkt %d noch nicht! Geboren %ul\n", Index->linkeSeite,\
				Index->rechteSeite, HK_getAbsTime_M(), Index->geburtsTag);
		}
	      }
#if 0
	      our_error("Fehler HS002");
#endif
	    }
	    Index = Index->Nachf;
	  } while (Index != NULL);
	}
	goto backtrack;
  }
}


static TermT SigmaRGanzNeu(void);

static BOOLEAN mo_IstCC_(GleichungsT e)
{
  SymbolT f = TO_TopSymbol(gefundeneLinkeSeite);
  return MM_CGleichung(f) == e || MM_C_Gleichung(f) == e;
}

BOOLEAN MO_GleichungGefunden(UTermT Term) /* Sucht im Gleichungsbaum nach einer Generalisierung von Term. SubstAn-
  wenden liefert dann diezugehoerige substituierte rechte Seite der ersten Regel in der zum Blatt gehoerenden Re-
  gelliste. Suchstrategie: Tiefensuche. Besuchsreihenfolge fuer Unterbaeume: zu passendem Funktionssymbol, zu noch */
{    /*  nicht gebundener Variable, zu schon gebundenen Variablen, beginnend mit den davon neueren, endend bei x1. */
  IncVorwaertsMatchE();
  if (RE_GleichungsmengeLeer())
    return FALSE;
  else {
    SymbolT Symbol,lxMletztes,letzteKandidatin;
    KantenT BaumIndex;
    UTermT Termrest, Regelrest, Vorg;
    DeclareBacktrack();
  
    /* Initialisierung der lokalen Variablen */
    BaumIndex = RE_Gleichungsbaum.Wurzel;
    Termrest = Term;
    Vorg = NULL;
    lxMletztes = SO_VorigesVarSymb(SO_ErstesVarSymb);
    InitBacktrack();
  
    tryFct:
        Symbol = TO_TopSymbol(Termrest);
        if (SO_SymbIstFkt(Symbol) && 
                  (!GZ_GRUNDZUSAMMENFUEHRUNG || !SO_IstExtrakonstante(Symbol)) &&
                  BK_Nachf(BaumIndex,Symbol)){
                  Backtrack1();
                  AbstiegVerwaltenM(AbstiegUmSymbolM);
        }
    tryNVar:
        Symbol = BK_letzteVar(BaumIndex);
        if (BK_Nachf(BaumIndex,Symbol)) {
                  Backtrack2();
                  SubstErgaenzenM(TRUE,Termrest,Vorg);
                  AbstiegVerwaltenM(AbstiegUmTeiltermM);
        }
  /*tryOVar:*/
        Symbol = BK_minSymb(BaumIndex); /* Optimierung: Wir wissen, dass es keine neue Var gibt */
    tryOVar1:
        letzteKandidatin = calcLetzteKandidatin(BaumIndex);
        while (Symbol <= letzteKandidatin){
                  if (BK_Nachf(BaumIndex,Symbol) && 
                      EsFolgtSubstitutVon(Symbol,Termrest)) {
                      Backtrack3();
                      AbstiegVerwaltenM(AbstiegUmTeiltermM);
                  }
                  Symbol++;
        }
    backtrack:
        if (!canBacktrack()) 
                  return FALSE;
        BacktrackPop();
        if (SO_SymbIstFkt(Symbol))
          goto tryNVar;
        Symbol++;
        goto tryOVar1;
    leave:
        Regelrest = BK_Rest(BaumIndex);

        while (Regelrest) {                        /* anstatt Abfrage auf Ende des (unbeschnittenen!) Anfrageterms */
          SymbolT SymbAusRegel = TO_TopSymbol(Regelrest);
          if (SO_SymbIstVar(SymbAusRegel)) {
            if (SO_VariableIstNeuer(SymbAusRegel,lxMletztes)) {              /* ungebundene Variable aus Regelterm */
              SubstErgaenzenM(TRUE,Termrest,Vorg);                                        /* in Anfrageterm binden */
            }
            else if (!EsFolgtSubstitutVon(SymbAusRegel,Termrest)) {            /* fuer gebundene Variable pruefen, */
              goto backtrack;                      /* ob sich der Anfragterm entsprechend fortsetzt oder eben nicht*/
            }
            Termrest = TO_Schwanz(Vorg = TO_TermEnde(Termrest));                     /* einen Teilterm weitergehen */
            Regelrest = TO_Schwanz(Regelrest);
          }
          else if (SO_SymbUngleich(SymbAusRegel,TO_TopSymbol(Termrest))) {        /* Abbruch ebenso bei ungleichen */
            goto backtrack;
          }
          else {                                         /* Bei gleichen Funktionssymbolen naechste Symbole lesen. */
            Termrest = TO_Schwanz(Vorg = Termrest);                                           /* ein Symbol weiter */
            Regelrest = TO_Schwanz(Regelrest);
          }
        }
                                                                                                         /*success:*/
        /*ab hier im Blatt im Baum*/ 
        SubstInOrdnungBringen();
        {
          GleichungsT Index = BK_Regeln(BaumIndex);
          gefundeneLinkeSeite = BK_Regeln(BaumIndex)->linkeSeite;
          do {
            if (TP_lebtZumZeitpunkt_M(Index, HK_getAbsTime_M())) {
              if (!Index->DarfNichtReduzieren && FreieVariablenOK(Index)) {
                TermT Kopie;
                BOOLEAN Resultat;
                gefundeneRechteSeite = TP_RechteSeiteUnfrei(Index);        /* dort noch freie Variablen instanziiert */
                if (1&&mo_IstCC_(Index)){/* HACK HACK HACK */
                  Resultat = ORD_TermGroesserRed(SubstitutVon(-1),SubstitutVon(-2));
                }
                else {
                  Kopie = SigmaRGanzNeu();
                  Resultat = ORD_TermGroesserRed(Term,Kopie);           /* Umschaltung auf Ordnungszeitmessung in HK_> */
                  TO_TermLoeschen(Kopie);
                }
	        if (Resultat) {
		  if (!spezielleSchutzverletzung(Term,Index)){
		    MO_AngewendetesObjekt = Index;
	  	    return TRUE;
		  }
	        }
	        else {
	          IncAnzRedMisserfolgeOrdnung();        /* Red.versuch wg. >-Test gescheitert; nicht im else-Zweig, da */
	        }
              }
            } 
	    else {
	      if (IO_level4()) {
		if (Index->todesTag < maximalAbstractTime()){
		  IO_DruckeFlex("Die Gleichung %t = %t lebt zum Zeitpunkt %d nicht mehr! Gestorben %ul\n", Index->linkeSeite,\
				Index->rechteSeite, HK_getAbsTime_M(), Index->todesTag);
		}
		else {
		  IO_DruckeFlex("Die Gleichung %t = %t lebt zum Zeitpunkt %d noch nicht! Geboren %ul\n", Index->linkeSeite,\
				Index->rechteSeite, HK_getAbsTime_M(), Index->geburtsTag);
		}
	      }
#if 0
	      our_error("Fehler HS001");     /*die gefundene Gleichung lebt nichtmehr*/
#endif
	    }
	    Index = Index->Nachf;
          } while (Index != NULL);
        }
        goto backtrack;
  }
}

BOOLEAN MO_ObjektGefunden(DSBaumP Baum, UTermT Term)
 /* Sucht in Baum nach einer Generalisierung von Term. SubstAnwenden liefert dann diezugehoerige substituierte
    rechte Seite der ersten Regel in der zum Blatt gehoerenden Regelliste.
    Suchstrategie: Tiefensuche. Besuchsreihenfolge fuer Unterbaeume: zu passendem Funktionssymbol, zu noch nicht ge-
    bundener Variable, zu schon gebundenen Variablen, beginnend mit den davon neueren, endend bei x1. */
{
  if (Baum == &RE_Regelbaum) {
    return MO_RegelGefunden(Term);
  }
  else {
    return MO_GleichungGefunden(Term);
  }
}

void MO_schuetzeTerm(TermT Term, TermT Schutz)
/* Falls auf Term gematcht wird, werden nur Regeln/Gleichungen akzeptiert, die in der Dreiecksordnung kleiner
   als Schutz sind. */
{
  zuSchuetzenderTerm = Term; /* Achtung, hier wird EZ/LZ ausgenutzt! */
  schutzTerm         = Schutz;
}

void MO_keinenSchutz(void)
/* Hebt den Schutz wieder auf. */
{
  zuSchuetzenderTerm = NULL;
  schutzTerm         = NULL;
}

/*=================================================================================================================*/
/*= V. Matching mit nur einer Regel oder Gleichung                                                                =*/
/*=================================================================================================================*/

BOOLEAN MO_TermpaarPasst(TermT links, TermT rechts, UTermT Termrest)
/* Liefert TRUE, wenn das Termpaar links->rechts als Regel auf Term anwendbar ist. In diesem Falle ist die 
   entsprechende Substitution wie gehabt zum Einpassen bereit und kann mit den unten aufgefuehrten Routinen in
   den Gesamtterm eingebaut werden. Das Termpaar muss bereits normiert sein. */
{
  UTermT Vorg = NULL;

  gefundeneLinkeSeite = links;                        /* Nach erfolgreichem Schleifendurchlauf gilt links == NULL. */
  MatcherInitialisieren(Termrest);
  do {
    if (SO_SymbIstVar(TO_TopSymbol(links))) {
      if (VarUngebunden(TO_TopSymbol(links))) {                              /* ungebundene Variable aus Regelterm */
        SubstErgaenzen(Termrest,Vorg);                                                    /* in Anfrageterm binden */
        Termrest = TO_Schwanz(Vorg = TO_TermEnde(Termrest));
        links = TO_Schwanz(links);
      }
      else if (EsFolgtSubstitutVon(TO_TopSymbol(links),Termrest)) {            /* fuer gebundene Variable pruefen, */
        Termrest = TO_Schwanz(Vorg = TO_TermEnde(Termrest));                             /* ob sich der Anfragterm */
        links = TO_Schwanz(links);                                                       /* entsprechend fortsetzt */
      }
      else                                                             /* Falls nicht, den Matchversuch abbrechen. */
        return FALSE;
    }
    else if (SO_SymbUngleich(TO_TopSymbol(links),TO_TopSymbol(Termrest)))         /* Abbruch ebenso bei ungleichen */
      return FALSE;                                                                           /* Funktionssymbolen */
    else {                                               /* Bei gleichen Funktionssymbolen naechste Symbole lesen. */
      Termrest = TO_Schwanz(Vorg = Termrest);
      links = TO_Schwanz(links);
    }
  } while (links);
  gefundeneRechteSeite = rechts;
  return TRUE;
}


BOOLEAN MO_SigmaTermIstGeneralisierung(UTermT links, UTermT Termrest)                                /* Fuer Joerg */
/* Liefert TRUE, wenn sigma(links) Verallgemeinerung von Termrest ist.
   Es sind dann aber keine neuen Terme unter dieser Substitution erzeugbar.
   Der Term links muss in Fortsetzung der bisherigen Anfragen normiert sein (insbesondere keine Variablen-Luecken!). 
   Die Substitution wird stets wieder zurueckgesetzt.
   */
{
  SymbolT xMletztesAlt = xMletztes;
  UTermT Vorg = NULL,
         NULLTerm = TO_NULLTerm(links);
  do {
    if (SO_SymbIstVar(TO_TopSymbol(links))) {
      if (VarUngebunden(TO_TopSymbol(links))) {                              /* ungebundene Variable aus Regelterm */
        SubstErgaenzen(Termrest,Vorg);                                                    /* in Anfrageterm binden */
        Termrest = TO_Schwanz(Vorg = TO_TermEnde(Termrest));
        links = TO_Schwanz(links);
      }
      else if (EsFolgtSubstitutVon(TO_TopSymbol(links),Termrest)) {            /* fuer gebundene Variable pruefen, */
        Termrest = TO_Schwanz(Vorg = TO_TermEnde(Termrest));                             /* ob sich der Anfragterm */
        links = TO_Schwanz(links);                                                       /* entsprechend fortsetzt */
      }
      else {                                                           /* Falls nicht, den Matchversuch abbrechen. */
        xMletztes = xMletztesAlt;                                                    /* Substitution zuruecksetzen */
        return FALSE;
      }
    } else if (SO_SymbUngleich(TO_TopSymbol(links),TO_TopSymbol(Termrest))) {       /* Abbruch ebenso bei ungleichen */
      xMletztes = xMletztesAlt;                                                               /* Funktionssymbolen */
      return FALSE;                                                                  /* Substitution zuruecksetzen */
    }
    else {                                               /* Bei gleichen Funktionssymbolen naechste Symbole lesen. */
      Termrest = TO_Schwanz(Vorg = Termrest);
      links = TO_Schwanz(links);
    }
  } while (TO_PhysischUngleich(links,NULLTerm));
  xMletztes = xMletztesAlt;                                                          /* Substitution zuruecksetzen */
  return TRUE;
}


BOOLEAN MO_TermIstGeneralisierung(UTermT links, UTermT Termrest)                                     /* Fuer Joerg */
/* Liefert TRUE, wenn links Verallgemeinerung von Termrest ist.
   Es sind dann aber keine neuen Terme unter der Substitution erzeugbar.
   Der Term links muss bereits normiert sein. 
   Die Substitution bleibt erhalten und kann fuer MO_SigmaTermIstGeneralisierung verwendet werden.
   */
{ 
  SubstAufIdentitaetSetzen();
  return MO_SigmaTermIstGeneralisierung(links,Termrest);
}

BOOLEAN MO_existiertMatch(TermT links, TermT Term)
/* liefert TRUE, wenn ein Match von links auf Term existiert. Die Substitution wird dabei NICHT veraendert! */
/* Da mit globalen Variablen hantiert wird: Retten und zurueckschreiben. Code aus Modul zusammengeklaut. */
{
  BOOLEAN res;
  MO_EintragsT *xMRett = xM;
  SymbolT xMletztesRett = xMletztes;
  xM = (MO_EintragsT *) our_alloc(-xMDimension*sizeof(MO_EintragsT)) - xMDimension;
  res = MO_TermIstGeneralisierung(links, Term);
  our_dealloc(xM+xMDimension);                            /* Addition von xMDimension fuer negative Adressierung */
  xM = xMRett;
  xMletztes = xMletztesRett;
  return res;
}

BOOLEAN MO_TermpaarPasstMitResultat(TermT links, TermT rechts, TermT TermOld, TermT *TermNew, UTermT aktTeilterm)
/* Liefert TRUE, wenn das Termpaar links=rechts auf Stelle aktTeilterm in
   TermOld auf Toplevel anwendbar ist und
   liefert in diesem Fall das Ergebnis den geaenderten Term als TermNew mit neuen
   Termzellen zurueck. Es wird nicht geprueft, ob TermOld > TermNew ist.
   Die Funktion ist also nicht destruktiv, TermNew muss freigegeben werden.
   links, rechts muss normiert sein!
   */
{
   if (MO_TermpaarPasst(links, rechts, aktTeilterm)) {
      *TermNew = MO_AllTermErsetztNeu(TermOld, aktTeilterm);
      return TRUE;
   }
   return FALSE;
}

static BOOLEAN RegelPasst(RegelT Regel, UTermT Term)
{
  if (MO_TermpaarPasst(TP_LinkeSeite(Regel),TP_RechteSeite(Regel), Term)){
    MO_AngewendetesObjekt = Regel;
    return TRUE;
  }
  return FALSE;
}

static BOOLEAN GleichungsrichtungPasst(GleichungsT Gleichung, UTermT Term)                             /* vgl. MNF */
{
  if (MO_TermpaarPasst(TP_LinkeSeite(Gleichung), TP_RechteSeite(Gleichung),Term)) {
    UTermT Kopie;
    BOOLEAN Resultat;
    if (FreieVariablenOK(Gleichung)) {
      gefundeneRechteSeite = TP_RechteSeiteUnfrei(Gleichung);            /* dort noch freie Variablen instanziiert */
      Kopie = SigmaRGanzNeu();
      Resultat = ORD_TermGroesserRed(Term,Kopie);
      TO_TermLoeschen(Kopie);
      if (Resultat) {
        MO_AngewendetesObjekt = Gleichung;
        return TRUE;
      }
    }
  }
  return FALSE;
}

static BOOLEAN GleichungPasst(GleichungsT Gleichung, UTermT Term)
/* Testet, ob die gegebene Gleichung den Term t reduzieren kann. Dieser Test geschieht 
   in beiden Richtungen und unter Beruecksichtigung der eingestellten Ordnung.*/
{
  GleichungsT hin = Gleichung,
              her = TP_Gegenrichtung(Gleichung);
  return GleichungsrichtungPasst(hin,Term) ||
    (TP_IstKeineMonogleichung(Gleichung) && GleichungsrichtungPasst(her,Term));
}

BOOLEAN MO_ObjektPasst(RegelOderGleichungsT Objekt, UTermT t)
/* Liefert TRUE, wenn Objekt auf Term anwendbar ist. In diesem Falle ist die entsprechende Substitution wie gehabt
   zum Einpassen bereit und kann mit den unten aufgefuehrten Routinen in den Gesamt-Term eingebaut werden.*/
{
  if (TP_TermpaarIstRegel(Objekt)){
    return RegelPasst(Objekt, t);
  }
  else {
    return GleichungPasst(Objekt, t);
  }
}


/*=================================================================================================================*/
/*= VI. Anwenden von Substitutionen auf Terme                                                                     =*/
/*=================================================================================================================*/

#define /*BOOLEAN*/ SchonEingebaut(/*SymbolT*/ VarSymbol)                                                           \
  xM[VarSymbol].SchonEingebaut


#define /*BOOLEAN*/ NochNichtEingebaut(/*SymbolT*/ VarSymbol)                                                       \
  !xM[VarSymbol].SchonEingebaut


#define /*BOOLEAN*/ SubstitutNichtAmAnfragetermEnde(/*SymbolT*/ VarSymbol)                                          \
  TO_PhysischUngleich(TO_TermEnde(SubstitutVon(VarSymbol)),letzteZelleAnfrageterm)


#define /*BOOLEAN*/ NichtGanzerTermRezykliert(/*void*/)                                                             \
  (NichtGanzerTermGebunden || NochNichtEingebaut(SO_ErstesVarSymb))


static void SubstitutAlsEingebautVermerken(SymbolT VarSymbol) /* setzt entsprechendes Flag und erhaelt die Verket- */
{                                               /* tung der nicht benoetigten Termzellen des Anfrageterms aufrecht */
/* Konzept:
   Komponenten Vorg und Nachf geben bei noch nicht eingebauten Variablen die jeweils erste Zelle im Anfrageterm
   vor bzw. nach dem Substitut an. Nachf zeigt schlimmstenfalls auf TO_Schwanz(letzteZelleAnfrageterm),
   Vorg auf Anfrageterm, da Aufruf nur bei mehrstelligen Anfragetermen erfolgt. */

  UTermT SubstitutVorg = xM[VarSymbol].Vorg;

  if (SubstitutVorg) {
    UTermT Substitut = SubstitutVon(VarSymbol),
           SubstitutNachf = xM[VarSymbol].Nachf;

    /* Aktualisierung der Zeiger auf erste vorhergehende nicht eingebaute Termzelle bei den noch nicht
       eingebauten Variablen. Betrifft max. eine Variable, deren Substitut das von VarIndex als Vorg hat. */
    SymbolT VarIndex;
    SO_forSymboleAbwaerts(VarIndex,SO_NaechstesVarSymb(VarSymbol),xMletztes)
      if (NochNichtEingebaut(VarIndex)) {
        if (TO_PhysischGleich(TO_TermEnde(Substitut),xM[VarIndex].Vorg))
          xM[VarIndex].Vorg = SubstitutVorg;
        break;
      }

    /* analog fuer Nachf-Zeiger */
    SO_forSymboleAufwaerts(VarIndex,SO_VorigesVarSymb(VarSymbol),SO_ErstesVarSymb)
      if (NochNichtEingebaut(VarIndex)) {
        if (TO_PhysischGleich(Substitut,xM[VarIndex].Nachf))
          xM[VarIndex].Nachf = SubstitutNachf;
        break;
      }

    /* Nun Verkettung der noch nicht rezyklierten Termzellen aktualisieren. In vorhergehender noch nicht
       eingebauter Zelle, die immer existiert (Topzelle!), den Nachfolger auf Kandidat setzen und ggf.
       letzteNichtVernutzteZelle aktualisieren. */
    if (TO_PhysischGleich(TO_TermEnde(Substitut),letzteNichtVernutzteZelle))
      letzteNichtVernutzteZelle = SubstitutVorg;
    TO_NachfBesetzen(SubstitutVorg,SubstitutNachf);
  }
  xM[VarSymbol].SchonEingebaut = TRUE;
}


#define /*UTermT*/ EVSubstitutVon(/*SymbolT*/ Symbol)                                                               \
  (SubstitutAlsEingebautVermerken(Symbol),SubstitutVon(Symbol))


#define /*UTermT*/ AufAngehaengteZelle(/*UTermT*/ Index, /*SymbolT*/ Symbol)                                        \
{                                                                                                                   \
  UTermT NeueZelle;                                                                                                 \
  TO_TermzelleHolen(NeueZelle);                                                                                     \
  TO_NachfBesetzen(Index,NeueZelle);                                                                                \
  TO_SymbolBesetzen(Index = NeueZelle,Symbol);                                                                      \
  TO_ClearSubstFlag(Index);\
}


#define /*UTermT*/ AufAngehaengtesNullstelliges(/*UTermT*/ Index, /*SymbolT*/ Symbol)                               \
{                                                                                                                   \
  AufAngehaengteZelle(Index,Symbol);                                                                                \
  TO_EndeBesetzen(Index,Index);                                                                                     \
}


#define /*UTermT*/ AufAngehaengtesSubstitut(/*UTermT*/ Index, /*UTermT*/ Substitut)	\
{											\
  UTermT NeuerTeilterm = Substitut;							\
  TO_NachfBesetzen(Index,NeuerTeilterm);						\
  Index = TO_TermEnde(NeuerTeilterm);							\
  TO_SetSubstFlag(NeuerTeilterm);								\
}



/*=================================================================================================================*/
/*= VII. Erzeugung von SigmaR beginnend mit der ersten Zelle des Anfrageterms                                     =*/
/*=================================================================================================================*/

void MO_SigmaRInEZ(void)       /* SigmaR beliebiger Stelligkeit beginnend mit erster Zelle des Anfrageterms erzeu- */
{                                                         /* gen. Fallunterscheidung nach Laenge des Anfrageterms. */
  if (TO_Nullstellig(Anfrageterm)) { /* Ist der Anfrageterm nullstellig, muessen keine Termzellen geloscht werden. */
    if (TO_Nullstellig(gefundeneRechteSeite)) { /* Ist die rechte Seite eine Konstante oder eine ungebundene Vari- */
      if (SymbKeineGebundeneVar(TopsymbolRechteSeite)){                                    /* able, so muss nur das */
        TO_SymbolBesetzen(Anfrageterm,TopsymbolRechteSeite);
	TO_SetSubstFlag(Anfrageterm);
      }
      else { /* kommt eigentlich nicht vor */
	  TO_ClearSubstFlag(Anfrageterm);
      }
   /* entsprechende Symbol uebertragen werden. Ist die
           rechte Seite eine gebundene Variable, also x1, so geht die Bindung in den Anfrageterm. Dieser ist null-
           stellig; also geht die Bindung an den ganzen Term. Beim Substituieren wuerde der Anfrageterm durch sich
                selbst ersetzt werden; es muss also nichts unternommen werden. Ein exotischer Fall ("Regel" x->x). */
    }  /* Ansonsten geht mit einer ebenfalls exotischen Regel eine Variable oder eine Konstante an einen Grundterm
        oder einen Term, in dem eine gebundene Variable, also x1, wieder vorkommt, oder eine ungebundene Variable.
       Wie oben erfolgt  Bindung hoechstens an nullstelligen Term. Unterscheiden, ob eine Variable gebunden wurde. */
    else if (VarGebunden(SO_ErstesVarSymb))        /* Dann Substitut merken, da es beim Kopieren der rechten Seite */
      TO_TermkopieEZx1Subst(gefundeneRechteSeite,Anfrageterm);
    else                                                                            /* Ansonsten sorglos kopieren. */
      TO_TermkopieEZ(gefundeneRechteSeite,Anfrageterm);
  } else                                                                 /* Falls der Anfrageterm mehrstellig ist: */
    if (TO_Nullstellig(gefundeneRechteSeite)) {      /* Vorgehen fuer nullstellige rechte Seite unterscheiden nach */
      if (SymbKeineGebundeneVar(TopsymbolRechteSeite)) {                                  /* Konstanten und freien */
        TO_SymbolBesetzen(Anfrageterm,TopsymbolRechteSeite);            /* Variablen (dann ein Symbol uebertragen, */
	TO_ClearSubstFlag(Anfrageterm);
        TO_TermzellenlisteLoeschen(TO_Schwanz(Anfrageterm),TO_TermEnde(Anfrageterm));
        TO_NachfBesetzen(Anfrageterm,NULL);                                      /* Rest des Anfrageterms loeschen */
        TO_EndeBesetzen(Anfrageterm,Anfrageterm);                             /* und Zeiger in Termzelle anpassen) */
      }
      else { /* sowie gebundenen Variablen; ausfuehrliche Erlaeuterung siehe oben. Erste Termzelle vom Anfrageterm
          mit TO_Schwanz(Substitut) versehen, dabei restliche Zellen des Anfrageterms verkettet lassen. Exotischer 
                Fall, wenn eine Variable an ganzen Term gebunden ist, also Regel x->gebundene Variable, also x->x. */
        if (NichtGanzerTermGebunden) {                                                 /* Dann nichts unternehmen. */
          UTermT Substitut = SubstitutVon(TopsymbolRechteSeite);
          TO_SymbolBesetzen(Anfrageterm,TO_TopSymbol(Substitut));     /* Topsymbol in erste Zelle des Anfrageterms */
	  TO_SetSubstFlag(Anfrageterm);
          if (TO_Nullstellig(Substitut)) {  /* Falls Substitut nullstellig ist, den gesamten Rest des Anfrageterms */
            TO_TermzellenlisteLoeschen(TO_Schwanz(Anfrageterm), TO_TermEnde(Anfrageterm));                  /* weg */
            TO_NachfBesetzen(Anfrageterm,NULL);                                         /* und Termzeiger anpassen */
            TO_EndeBesetzen(Anfrageterm,Anfrageterm);
          }
          else {   /* Ansonsten TO_Schwanz(Substitut) bis TO_TermEnde(Substitut) aus der Verkettung der Termzellen */
            UTermT SchwanzAnfrageterm = TO_Schwanz(Anfrageterm);                    /* im Anfrageterm herausnehmen */
            TO_NachfBesetzen(Anfrageterm,TO_Schwanz(Substitut));
            /* Fallunterscheidung, ob Substitut am Ende des Anfrageterms steht */
            if (SubstitutNichtAmAnfragetermEnde(TopsymbolRechteSeite)) {                            /* Falls nein: */
              TO_NachfBesetzen(Substitut,TO_NaechsterTeilterm(Substitut));
              TO_TermzellenlisteLoeschen(SchwanzAnfrageterm,letzteZelleAnfrageterm);
              TO_EndeBesetzen(Anfrageterm,TO_TermEnde(Substitut));                                /*$nested-feature*/
              TO_NachfBesetzen(TO_TermEnde(Anfrageterm),NULL);       /* Da Substitut mitten im Term stand, gibt es */
            }                                                            /* eine nicht mehr gueltige Nachf-Angabe. */
            else
              TO_TermzellenlisteLoeschen(SchwanzAnfrageterm,Substitut);
          }
        }
      }
    }
  else {
    TermzellenT DummyTopzelle;
    UTermT IndexNeu = &DummyTopzelle,
           NULLAnfrageterm = TO_Schwanz(letzteZelleAnfrageterm);
    letzteNichtVernutzteZelle = letzteZelleAnfrageterm;
    TO_SymbolBesetzen(Anfrageterm,TopsymbolRechteSeite);
    TO_ClearSubstFlag(Anfrageterm);
    TI_PushMitAnz(IndexNeu,TO_AnzahlTeilterme(gefundeneRechteSeite));
    do {
      SymbolT TopSymbol = TO_TopSymbol(gefundeneRechteSeite = TO_Schwanz(gefundeneRechteSeite));
      if (TO_Nullstellig(gefundeneRechteSeite)) {
        if (SymbKeineGebundeneVar(TopSymbol))
          AufAngehaengtesNullstelliges(IndexNeu,TopSymbol)
        else
          AufAngehaengtesSubstitut(IndexNeu,SchonEingebaut(TopSymbol) ? 
            TO_Termkopie(SubstitutVon(TopSymbol)) : EVSubstitutVon(TopSymbol));
        while (!(--TI_TopZaehler)) {
          TO_EndeBesetzen(TI_TopTeilterm,IndexNeu);
          if (TI_PopUndLeer()) {
            TO_EndeBesetzen(Anfrageterm,IndexNeu);
            TO_NachfBesetzen(IndexNeu,NULL);
            if (NichtGanzerTermRezykliert() && TO_PhysischUngleich(TO_Schwanz(Anfrageterm),NULLAnfrageterm))
              TO_TermzellenlisteLoeschen(TO_Schwanz(Anfrageterm),letzteNichtVernutzteZelle);
            TO_NachfBesetzen(Anfrageterm,DummyTopzelle.Nachf);
            return;
          }
        }
      }
      else {
        AufAngehaengteZelle(IndexNeu,TopSymbol);
        TI_Push(IndexNeu);
      }
    } while (TRUE);
  }
}



/*=================================================================================================================*/
/*= VIII. Erzeugung von SigmaR endend mit der letzten Zelle des Anfrageterms                                      =*/
/*=================================================================================================================*/

static UTermT VonEndeNachMitteEVSubstitutVon(SymbolT VarSymbol)
{
  UTermT NeuesEnde,
         Substitut = SubstitutVon(VarSymbol);
  TO_TermzelleHolen(NeuesEnde);
  SubstitutAlsEingebautVermerken(VarSymbol);                /* ausreichend, da am Ende des Anfrageterms befindlich */
  TO_NachfBesetzen(NeuesEnde,NULL);
  TO_EndeBesetzen(NeuesEnde,NeuesEnde);
  if (TO_Nullstellig(Substitut)) {
    TO_SymbolBesetzen(NeuesEnde,TO_TopSymbol(Substitut));
    TO_SetSubstFlag(NeuesEnde);
    return NeuesEnde;
  }
  else {
    UTermT AltesEnde = TO_TermEnde(Substitut),
           SubstitutVorg;
    TO_EndeBesetzen(Substitut,NeuesEnde);
    while (TO_PhysischUngleich(Substitut = TO_Schwanz(SubstitutVorg = Substitut),AltesEnde))
      if (TO_PhysischGleich(TO_TermEnde(Substitut),AltesEnde))
        TO_EndeBesetzen(Substitut,NeuesEnde);
    TO_NachfBesetzen(SubstitutVorg,NeuesEnde);
    TO_SymbolBesetzen(NeuesEnde,TO_TopSymbol(AltesEnde));
    TO_SetSubstFlag(SubstitutVon(VarSymbol));
    return SubstitutVon(VarSymbol);
  }
}


static UTermT VonMitteNachEndeEVSubstitutVon(SymbolT VarSymbol)
{
  UTermT Substitut = SubstitutVon(VarSymbol);

/* Verkettung der nicht benoetigten Termzellen des Anfrageterms aufrechterhalten. Die letzte Zelle des Substituts 
   bleibt in der Verkettung stehen, daher bei folgenden Substituten nichts zu unternehmen, sondern nur bei voran-
   gehenden. LetzteNichtVernutzteZelle muss nicht aktualisiert werden. */
  xM[VarSymbol].SchonEingebaut = TRUE;
  if (TO_Nullstellig(Substitut)) {
    TO_SymbolBesetzen(letzteZelleAnfrageterm,TO_TopSymbol(Substitut));
    TO_SetSubstFlag(letzteZelleAnfrageterm);
    return letzteZelleAnfrageterm;
  }
  else {
    SymbolT VarIndex;  
    UTermT AltesEnde = TO_TermEnde(Substitut),
           SubstitutVorg;
    SO_forSymboleAufwaerts(VarIndex,SO_VorigesVarSymb(VarSymbol),SO_ErstesVarSymb)
      if (NochNichtEingebaut(VarIndex)) {
        if (TO_PhysischGleich(Substitut,xM[VarIndex].Nachf))
          xM[VarIndex].Nachf = AltesEnde;
        break;
      }
    TO_NachfBesetzen(xM[VarSymbol].Vorg/*==SubstitutVorg*/,AltesEnde);    
    TO_EndeBesetzen(Substitut,letzteZelleAnfrageterm);
    while (TO_PhysischUngleich(Substitut = TO_Schwanz(SubstitutVorg = Substitut),AltesEnde))
      if (TO_PhysischGleich(TO_TermEnde(Substitut),AltesEnde))
        TO_EndeBesetzen(Substitut,letzteZelleAnfrageterm);
    TO_NachfBesetzen(SubstitutVorg,letzteZelleAnfrageterm);
    TO_SymbolBesetzen(letzteZelleAnfrageterm,TO_TopSymbol(AltesEnde));
    TO_SetSubstFlag(SubstitutVon(VarSymbol));
    return SubstitutVon(VarSymbol);
  }
}


#define /*UTermT*/ SubstAnwendenLZ(/*UTermT*/ InitialwertLetzteNichtVernutzteZelle,                                 \
                                   /*UTermT*/ RezykliertesSubstitut,                                                \
                                   /*Block*/ AktionMitLetzterZelle,                                                 \
                                   /*Block*/ KorrekturLetzteNichtVernutzteZelle)                                    \
{                                                                                                                   \
  TermT TermNeu;                                                                                                    \
  UTermT IndexNeu;                                                                                                  \
  TO_TermzelleHolen(TermNeu);                                                                                       \
  IndexNeu = TermNeu;                                                                                               \
  letzteNichtVernutzteZelle = InitialwertLetzteNichtVernutzteZelle;                                                 \
  TO_SymbolBesetzen(TermNeu,TopsymbolRechteSeite);                                                                  \
  TO_ClearSubstFlag(TermNeu);\
  TI_Push(TermNeu);                                                                                                 \
  while (TO_PhysischUngleich(gefundeneRechteSeite = TO_Schwanz(gefundeneRechteSeite),EndeRechteSeite)) {            \
    SymbolT TopSymbol = TO_TopSymbol(gefundeneRechteSeite);                                                         \
    if (TO_Nullstellig(gefundeneRechteSeite)) {                                                                     \
      if (SymbKeineGebundeneVar(TopSymbol))                                                                         \
        AufAngehaengtesNullstelliges(IndexNeu,TopSymbol)                                                            \
      else                                                                                                          \
        AufAngehaengtesSubstitut(IndexNeu,                                                                          \
          SchonEingebaut(TopSymbol) ? TO_Termkopie(SubstitutVon(TopSymbol)) : (RezykliertesSubstitut));            \
      while (!(--TI_TopZaehler)) {                                                                                  \
        TO_EndeBesetzen(TI_TopTeilterm,IndexNeu);                                                                   \
        TI_Pop();                                                                                                   \
      }                                                                                                             \
    }                                                                                                               \
    else {                                                                                                          \
      AufAngehaengteZelle(IndexNeu,TopSymbol);                                                                      \
      TI_Push(IndexNeu);                                                                                            \
    }                                                                                                               \
  }                                                                                                                 \
  AktionMitLetzterZelle;                                                                                            \
  do                                                                                                                \
    TO_EndeBesetzen(TI_TopTeilterm,letzteZelleAnfrageterm);                                                         \
  while (TI_PopUndNichtLeer());                                                                                     \
  if (NichtGanzerTermRezykliert()) {                                                                                \
    KorrekturLetzteNichtVernutzteZelle;                                                                             \
    TO_TermzellenlisteLoeschen(Anfrageterm,letzteNichtVernutzteZelle);                                              \
  }                                                                                                                 \
  return TermNeu;                                                                                                   \
}


UTermT MO_SigmaRInLZ(void)
{
  if (TO_Nullstellig(Anfrageterm)) {                                  /* dann Anfrageterm == letzteZelleAnfrageterm */
    if (TO_Nullstellig(gefundeneRechteSeite)) {
	if (SymbKeineGebundeneVar(TopsymbolRechteSeite)){
	    TO_SymbolBesetzen(Anfrageterm,TopsymbolRechteSeite);
	    TO_ClearSubstFlag(Anfrageterm);
	}
	else {
	    TO_SetSubstFlag(Anfrageterm);
	}
      return Anfrageterm;
    }
    else if (VarGebunden(SO_ErstesVarSymb))
      return TO_TermkopieLZx1Subst(gefundeneRechteSeite,Anfrageterm);
    else                                                                            /* Ansonsten sorglos kopieren. */
      return TO_TermkopieLZm(gefundeneRechteSeite,Anfrageterm);
  }
  else {
    if (TO_Nullstellig(gefundeneRechteSeite)) {
      if (SymbKeineGebundeneVar(TopsymbolRechteSeite)) {                                  /* Konstanten und freien */
        TO_SymbolBesetzen(letzteZelleAnfrageterm,TopsymbolRechteSeite); /* Variablen (dann ein Symbol uebertragen, */
	TO_ClearSubstFlag(letzteZelleAnfrageterm);
        TO_TermzellenlisteLoeschen(Anfrageterm,TO_VorigeTermzelle(letzteZelleAnfrageterm,Anfrageterm));
        return letzteZelleAnfrageterm;
      }
      else {                                                                        /* nun Regel f(...,x,...) -> x */
        UTermT Substitut = SubstitutVon(TopsymbolRechteSeite);
        if (SubstitutNichtAmAnfragetermEnde(TopsymbolRechteSeite)) {
          if (TO_Nullstellig(Substitut)) {        /* nullstelliges Substitut in letzteZelleAnfrageterm uebertragen */
            TO_SymbolBesetzen(letzteZelleAnfrageterm,TO_TopSymbol(Substitut));
	    TO_SetSubstFlag(letzteZelleAnfrageterm);
            TO_TermzellenlisteLoeschen(Anfrageterm,TO_VorigeTermzelle(letzteZelleAnfrageterm,Anfrageterm));
            return letzteZelleAnfrageterm;
          }
          else {                                /* mehrstelliges Substitut mit letzteZelleAnfrageterm enden lassen */
            UTermT SubstitutVorg,
                   SubstitutEnde = TO_TermEnde(Substitut);
            TO_SymbolBesetzen(letzteZelleAnfrageterm,TO_TopSymbol(SubstitutEnde));
            TO_EndeBesetzen(Substitut,letzteZelleAnfrageterm);
            while (TO_PhysischUngleich(Substitut = TO_Schwanz(SubstitutVorg = Substitut),SubstitutEnde))
              if (TO_PhysischGleich(TO_TermEnde(Substitut),SubstitutEnde))
                TO_EndeBesetzen(Substitut,letzteZelleAnfrageterm);
            TO_NachfBesetzen(xM[TopsymbolRechteSeite].Vorg,SubstitutEnde);
            TO_TermzellenlisteLoeschen(Anfrageterm,TO_VorigeTermzelle(letzteZelleAnfrageterm,SubstitutEnde));
            TO_NachfBesetzen(SubstitutVorg,letzteZelleAnfrageterm);
	    TO_SetSubstFlag(SubstitutVon(TopsymbolRechteSeite));
            return SubstitutVon(TopsymbolRechteSeite);
          }
	}
        else {                                                   /* falls Substitut am Ende des Anfrageterms steht */
          if (NichtGanzerTermGebunden)
            TO_TermzellenlisteLoeschen(Anfrageterm,xM[TopsymbolRechteSeite].Vorg);
          return Substitut;
        }
      }
    }
    else {                                      /* Kasus Knackus: Anfrageterm und gefundeneRechteSeite mehrstellig */
      UTermT EndeRechteSeite = TO_TermEnde(gefundeneRechteSeite);
      SymbolT letztesSymbRechteSeite = TO_TopSymbol(EndeRechteSeite),
              letztesSymbLinkeSeite = TO_TopSymbol(TO_TermEnde(gefundeneLinkeSeite));

      #define /*Block*/ AnderenSchlussterm(/*void*/)                                                                \
        if (SymbKeineGebundeneVar(letztesSymbRechteSeite)) {                                                        \
          TO_SymbolBesetzen(letzteZelleAnfrageterm,letztesSymbRechteSeite);                                         \
	  TO_ClearSubstFlag(letzteZelleAnfrageterm);\
          TO_NachfBesetzen(IndexNeu,letzteZelleAnfrageterm);                                                        \
        }                               /* 1 Am Ende der gefundenen rechten Seite steht keine gebundene Variable. */\
        else {                          /* 2 Am Ende der gefundenen rechten Seite steht eine gebundene Variable.  */\
          TO_NachfBesetzen(IndexNeu,                                                                                \
            SchonEingebaut(letztesSymbRechteSeite) ?                                                                \
              TO_TermkopieLZ(SubstitutVon(letztesSymbRechteSeite),letzteZelleAnfrageterm)                           \
            :                                                                                                       \
              VonMitteNachEndeEVSubstitutVon(letztesSymbRechteSeite));                                              \
        }
      
      #define /*void*/ DenselbenSchlussterm(/*void*/)                                                               \
       TO_NachfBesetzen(IndexNeu,EVSubstitutVon(letztesSymbRechteSeite))

      #define /*void*/ KeineKorrekturLNVZ(/*void*/)


      /* 1 Der Anfrageterm wird durch kein Substitut beendet. */

      if (SymbKeineGebundeneVar(letztesSymbLinkeSeite) || SubstitutNichtAmAnfragetermEnde(letztesSymbLinkeSeite))
        SubstAnwendenLZ(TO_VorletzteZelle(Anfrageterm), 
          EVSubstitutVon(TopSymbol), AnderenSchlussterm(), KeineKorrekturLNVZ())

      /* 2 Der Anfrageterm wird durch ein Substitut beendet. */
      else 

        /* 2.1 Der Anfrageterm wird durch dasselbe Substitut beendet wie der neue Term. */
        if (SO_SymbGleich(letztesSymbLinkeSeite,letztesSymbRechteSeite))
          SubstAnwendenLZ(letzteZelleAnfrageterm,
            SO_SymbGleich(TopSymbol,letztesSymbRechteSeite) ?
              TO_Termkopie(SubstitutVon(TopSymbol)) : EVSubstitutVon(TopSymbol),
            DenselbenSchlussterm(), KeineKorrekturLNVZ())

        /* 2.2 Der Anfrageterm wird nicht durch dasselbe Substitut beendet wie der neue Term. */
        else
          SubstAnwendenLZ(xM[letztesSymbLinkeSeite].Vorg, 
            SO_SymbGleich(TopSymbol,letztesSymbLinkeSeite) ?
              VonEndeNachMitteEVSubstitutVon(TopSymbol) : EVSubstitutVon(TopSymbol),
            AnderenSchlussterm(),
            if (!SchonEingebaut(letztesSymbLinkeSeite) && TO_NichtNullstellig(SubstitutVon(letztesSymbLinkeSeite)))
              letzteNichtVernutzteZelle = TO_VorletzteZelle(SubstitutVon(letztesSymbLinkeSeite)));
    }
  }
}



/*=================================================================================================================*/
/*= IX. Erzeugung von SigmaR nur mit neuen Termzellen                                                              */
/*=================================================================================================================*/

#define /*UTermT*/ AufAngehaengtesSubstitutGNVon(/*UTermT*/ Index, /*SymbolT*/ VarSymbol)                           \
{                                                                                                                   \
  UTermT NeuerTeilterm = TO_Termkopie(SubstitutVon(VarSymbol));                                                    \
  TO_NachfBesetzen(Index,NeuerTeilterm);                                                                            \
  Index = TO_TermEnde(NeuerTeilterm);                                                                               \
}


static TermT SigmaRGanzNeu(void)
{
  if (TO_Nullstellig(gefundeneRechteSeite)) {         /* Vorgehen fuer nullstellige rechte Seite unterscheiden nach */
    if (SymbKeineGebundeneVar(TopsymbolRechteSeite)) {                                    /* Konstanten und freien */
      TermT TermNeu;
      TO_TermzelleHolen(TermNeu);
      TO_SymbolBesetzen(TermNeu,TopsymbolRechteSeite);                  /* Variablen (dann ein Symbol uebertragen, */
      TO_NachfBesetzen(TermNeu,NULL);                                            /* Rest des Anfrageterms loeschen */
      TO_EndeBesetzen(TermNeu,TermNeu);                                       /* und Zeiger in Termzelle anpassen) */
      return TermNeu;
    }
    else
      return TO_Termkopie(SubstitutVon(TopsymbolRechteSeite));
  }
  else {
    TermT TermNeu,
          TermAlt = gefundeneRechteSeite;
    UTermT IndexNeu;
    TO_TermzelleHolen(TermNeu);
    IndexNeu = TermNeu;
    TO_SymbolBesetzen(TermNeu,TopsymbolRechteSeite);
    TI_Push(TermNeu);
    do {
      SymbolT TopSymbol = TO_TopSymbol(TermAlt = TO_Schwanz(TermAlt));
      if (TO_Nullstellig(TermAlt)) {
        if (SymbKeineGebundeneVar(TopSymbol))
          AufAngehaengtesNullstelliges(IndexNeu,TopSymbol)
        else
          AufAngehaengtesSubstitutGNVon(IndexNeu,TopSymbol);
        while (!(--TI_TopZaehler)) {
          TO_EndeBesetzen(TI_TopTeilterm,IndexNeu);
          if (TI_PopUndLeer()) {
            TO_NachfBesetzen(IndexNeu,NULL);
            return TermNeu;
          }
        }
      }
      else {
        AufAngehaengteZelle(IndexNeu,TopSymbol);
        TI_Push(IndexNeu);
      }
    } while (TRUE);
  }
}



/*=================================================================================================================*/
/*= X. Subsumptionstests                                                                                          =*/
/*=================================================================================================================*/

static void SubstErgaenzenSubsumption(UTermT Substitut)        /* fuegt zur Substitution x(n)<-t(n) ... x(1)<-t(1) */
{                                 /* die Bindung x(n+1)<-Substitut hinzu. Dabei wird aber kein Einbau vorbereitet! */
  SO_NaechstesVarSymbD(xMletztes);
  xM[xMletztes].Substitut = Substitut;
}


BOOLEAN MO_TermpaarSubsummiertZweites(TermT links1, TermT rechts1, UTermT links2, UTermT rechts2)
/* Liefert TRUE, wenn das Termpaar links1==rechts1 allgemeiner ist als das Termpaar links2==rechts2, dh. wenn
   eine Substitution @ existiert, so dass links1 == @(links2) und rechts1 == @(rechts2).
   Getestet wird dabei nur in dieser einen Richtung.
*/
{
  SubstAufIdentitaetSetzen();
  do {
    SymbolT Symbol1 = TO_TopSymbol(links1);
    if (SO_SymbIstVar(Symbol1)) {
      if (VarUngebunden(Symbol1)) {                                          /* ungebundene Variable aus Regelterm */
        SubstErgaenzenSubsumption(links2);                                                /* in Anfrageterm binden */
        links2 = TO_NaechsterTeilterm(links2);
        links1 = TO_Schwanz(links1);
      }
      else if (EsFolgtSubstitutVon(Symbol1,links2)) {                          /* fuer gebundene Variable pruefen, */
        links2 = TO_NaechsterTeilterm(links2);                                           /* ob sich der Anfragterm */
        links1 = TO_Schwanz(links1);                                                     /* entsprechend fortsetzt */
      }
      else                                                             /* Falls nicht, den Matchversuch abbrechen. */
        return FALSE;
    }
    else if (SO_SymbUngleich(Symbol1,TO_TopSymbol(links2)))                       /* Abbruch ebenso bei ungleichen */
      return FALSE;                                                                           /* Funktionssymbolen */
    else {                                               /* Bei gleichen Funktionssymbolen naechste Symbole lesen. */
      links2 = TO_Schwanz(links2);
      links1 = TO_Schwanz(links1);
    }
  } while (links1);
  do {
    SymbolT Symbol1 = TO_TopSymbol(rechts1);
    if (SO_SymbIstVar(Symbol1)) {
      if (VarUngebunden(Symbol1)) {                                          /* ungebundene Variable aus Regelterm */
        SubstErgaenzenSubsumption(rechts2);                                               /* in Anfrageterm binden */
        rechts2 = TO_NaechsterTeilterm(rechts2);
        rechts1 = TO_Schwanz(rechts1);
      }
      else if (EsFolgtSubstitutVon(Symbol1,rechts2)) {                         /* fuer gebundene Variable pruefen, */
        rechts2 = TO_NaechsterTeilterm(rechts2);                                         /* ob sich der Anfragterm */
        rechts1 = TO_Schwanz(rechts1);                                                   /* entsprechend fortsetzt */
      }
      else                                                             /* Falls nicht, den Matchversuch abbrechen. */
        return FALSE;
    }
    else if (SO_SymbUngleich(Symbol1,TO_TopSymbol(rechts2)))                      /* Abbruch ebenso bei ungleichen */
      return FALSE;                                                                           /* Funktionssymbolen */
    else {                                               /* Bei gleichen Funktionssymbolen naechste Symbole lesen. */
      rechts2 = TO_Schwanz(rechts2);
      rechts1 = TO_Schwanz(rechts1);
    }
  } while (rechts1);

  return TRUE;
}


static BOOLEAN SubsummierendeGleichungInBlattGefunden(UTermT rechteSeite)
{
  SymbolT xMletztesAlt = xMletztes;
  UTermT Termrest = aktTermrest,
         Gleichungsrest = BK_Rest(BaumIndex);
  GNTermpaarT GleichungsIndex;

  /* 1. Ueberpruefen, ob im Blatt befindlicher Rest der linken Seite passt */

  while (Gleichungsrest) {
    SymbolT SymbAusRegel = TO_TopSymbol(Gleichungsrest);
    if (SO_SymbIstVar(SymbAusRegel)) {
      if (VarUngebunden(SymbAusRegel)) {
        SubstErgaenzenSubsumption(Termrest);
        Termrest = TO_NaechsterTeilterm(Termrest);
        Gleichungsrest = TO_Schwanz(Gleichungsrest);
      }
      else if (EsFolgtSubstitutVon(SymbAusRegel,Termrest)) {
        Termrest = TO_NaechsterTeilterm(Termrest);
        Gleichungsrest = TO_Schwanz(Gleichungsrest);
      }
      else {
        xMletztes = xMletztesAlt;
        return FALSE;
      }
    }
    else if (SO_SymbUngleich(SymbAusRegel,TO_TopSymbol(Termrest))) {
      xMletztes = xMletztesAlt;
      return FALSE;
    }
    else {
      Termrest = TO_Schwanz(Termrest);
      Gleichungsrest = TO_Schwanz(Gleichungsrest);
    }
  }


  /* 2. Nun alle im Blatt vorhandenen rechten Seiten durchnudeln */

  GleichungsIndex = BK_Regeln(BaumIndex);

  do {
    if (TP_lebtZumZeitpunkt_M(GleichungsIndex, HK_getAbsTime_M())) {/*Gleichung darf nur subsummieren, falls noch am leben*/
      UTermT rechts1 = TP_RechteSeite(GleichungsIndex),                         /* von potentiell allgemeinerer Gleichung */
	rechts2 = rechteSeite;                                                          /* von Anfragegleichung */
      SymbolT xMletztesAlt2 = xMletztes;
      do {
	SymbolT Symbol1 = TO_TopSymbol(rechts1);
	if (SO_SymbIstVar(Symbol1)) {
	  if (VarUngebunden(Symbol1)) {                                        /* ungebundene Variable aus Regelterm */
	    SubstErgaenzenSubsumption(rechts2);                                             /* in Anfrageterm binden */
	    rechts2 = TO_NaechsterTeilterm(rechts2);
	    rechts1 = TO_Schwanz(rechts1);
	  }
	  else if (EsFolgtSubstitutVon(Symbol1,rechts2)) {                       /* fuer gebundene Variable pruefen, */
	    rechts2 = TO_NaechsterTeilterm(rechts2);                                       /* ob sich der Anfragterm */
	    rechts1 = TO_Schwanz(rechts1);                                                 /* entsprechend fortsetzt */
	  }
	  else {                                                         /* Falls nicht, den Matchversuch abbrechen. */
	    xMletztes = xMletztesAlt2;
	    break;
	  }
	}
	else if (SO_SymbUngleich(Symbol1,TO_TopSymbol(rechts2))) {                  /* Abbruch ebenso bei ungleichen */
	  xMletztes = xMletztesAlt2;
	  break;
	}                                                                                       /* Funktionssymbolen */
	else {                                             /* Bei gleichen Funktionssymbolen naechste Symbole lesen. */
	  rechts2 = TO_Schwanz(rechts2);
	  rechts1 = TO_Schwanz(rechts1);
	}
      } while (rechts1);
      
      if (!rechts1)  {                                                               /* Falls Subsumption geglueckt: */
        MO_AngewendetesObjekt = GleichungsIndex;
	return TRUE;
      }
    }
    else {
      if (IO_level4()) {
	IO_DruckeFlex("Die Gleichung %t = %t lebt zum Zeitpunkt %d nicht mehr! Gestorben %d\n",                         \
		      GleichungsIndex->linkeSeite, GleichungsIndex->rechteSeite, HK_getAbsTime_M(),                   \
		      GleichungsIndex->todesTag);
      }
#if 0
      our_error("Fehler HS003");
#endif
    }
  } while ((GleichungsIndex = GleichungsIndex->Nachf));

  xMletztes = xMletztesAlt;
  return FALSE;
}


BOOLEAN MO_SubsummierendeGleichungGefunden(UTermT links, UTermT rechts)
/* Sucht im Gleichungsbaum nach einer Generalisierungen von links==rechts.
   Suchstrategie: Tiefensuche. Besuchsreihenfolge fuer Unterbaeume: zu passendem Funktionssymbol, zu noch nicht    */
{           /* gebundener Variable, zu schon gebundenen Variablen, beginnend mit den davon neueren, endend bei x1. */
  IncVorwaertsMatchS();
  if (RE_GleichungsmengeLeer())
    return FALSE;
  else {
    SubstAufIdentitaetSetzen();
    BaumwurzelInitialisieren(RE_Gleichungsbaum,links);
    while (EinenKnotenAbOderVieleKnotenAufgestiegen()) {
      if (BK_IstBlatt(BaumIndex)) {
        if (SubsummierendeGleichungInBlattGefunden(rechts))
          return TRUE;
        else
          BaumIndex = BK_Vorg(BaumIndex);
      }
    }
    return FALSE;
  }
}



/*=================================================================================================================*/
/*= XI. Initialisierung                                                                                           =*/
/*=================================================================================================================*/

void MO_InitApriori(void)
{
  if (xM == NULL) {
    xM = (MO_EintragsT *) our_alloc(MO_AnzErsterVariablen*sizeof(MO_EintragsT)) + MO_AnzErsterVariablen;
    xMDimension = -MO_AnzErsterVariablen;
  }
}



/*=================================================================================================================*/
/*= XIII. Alle Matches                                                                                            =*/
/*=================================================================================================================*/

static BOOLEAN AllMatchFirst;
/* Initialisierung fuer MO_All*MatchNext */

/* MO_AllRegelMatchFirst/Next sind geklaut von MO_RegelGefunden */
void MO_AllRegelMatchInit (UTermT Term) 
{
  MatcherInitialisieren(Term);  
  if (!RE_RegelmengeLeer()){
    BaumwurzelInitialisieren(RE_Regelbaum,Term);
  }
  AllMatchFirst = TRUE;
}

BOOLEAN MO_AllRegelMatchNext (RegelT *regelp) 
{
  if (RE_RegelmengeLeer())
    return FALSE;
  else {
    if (AllMatchFirst) {
      AllMatchFirst = FALSE;
    }
    else {
      MatcherNachBlattZuruecksetzen();
      BaumIndex = BK_Vorg(BaumIndex);
    }
    while (EinenKnotenAbOderVieleKnotenAufgestiegen())
      if (BK_IstBlatt(BaumIndex)) {
        if (RegelInBlattGefunden()){
	  RegelT RegelIndex = BK_Regeln(BaumIndex);
	  do { 
	    if (TP_lebtZumZeitpunkt_M(RegelIndex, HK_getAbsTime_M())) {
	      if (!RegelIndex->DarfNichtReduzieren) {
		gefundeneLinkeSeite = RegelIndex->linkeSeite;
		gefundeneRechteSeite = RegelIndex->rechteSeite;
		*regelp = RegelIndex;
		return TRUE;
	      }
	    }
	    else {
	      if (IO_level4()) {
		IO_DruckeFlex("Die Regel %t -> %t lebt zum Zeitpunkt %d nicht mehr! Gestorben %d\n",                   \
			      RegelIndex->linkeSeite, RegelIndex->rechteSeite, HK_getAbsTime_M(),                      \
			      RegelIndex->todesTag);
	      }
#if 0
	      our_error("Fehler HS005");
#endif
	    } RegelIndex = RegelIndex->Nachf;
	  } while (RegelIndex != NULL);
	  /*backtracking, da keine Regel gefunden*/
	  MatcherNachBlattZuruecksetzen(); /* Argl: Ueber 2 Stunden Debugging! 26.5.01, Bernd */
	  BaumIndex = BK_Vorg(BaumIndex);
	}
        else {
          BaumIndex = BK_Vorg(BaumIndex);
	}
      }
    return FALSE;
  }
}


/* MO_AllGleichungMatchInit/Next sind geklaut aus MO_GleichungGefunden */
void MO_AllGleichungMatchInit (UTermT Term)
{
   MatcherInitialisieren(Term);  
   if (!RE_GleichungsmengeLeer()){
     BaumwurzelInitialisieren(RE_Gleichungsbaum,Term);
   }
   AllMatchFirst = TRUE;
}


/* Ich will alle Gleichungen haben, die Matchen. Es ist egal, ob die Ordnungsrelation stimmt oder nicht
   - das wird in der Ebene hoeher bestimmt. */
static GleichungsT AndreasIndex;

static BOOLEAN ErfolgImBlattGehabt (BOOLEAN Ordnungstest, GleichungsT *gleichungp)
{
  while (AndreasIndex){
    if (TP_lebtZumZeitpunkt_M(AndreasIndex, HK_getAbsTime_M())) {
      if (!(AndreasIndex->DarfNichtReduzieren) && FreieVariablenOK(AndreasIndex)) {
	gefundeneLinkeSeite = BK_Regeln(BaumIndex)->linkeSeite;
	gefundeneRechteSeite = TP_RechteSeiteUnfrei(AndreasIndex);         /* dort noch freie Variablen instanziiert */
	if (!Ordnungstest){
	  *gleichungp = AndreasIndex;
	  return TRUE;
	}
	else {
	  TermT Kopie;
	  BOOLEAN Resultat;
	  Kopie = SigmaRGanzNeu();
	  Resultat = ORD_TermGroesserRed(Anfrageterm,Kopie);           /* Umschaltung auf Ordnungszeitmessung in HK_> */
	  TO_TermLoeschen(Kopie);
	  if (Resultat) {
	    *gleichungp = AndreasIndex;
	    return TRUE;
	  }
	  IncAnzRedMisserfolgeOrdnung();        /* Red.versuch wg. >-Test gescheitert; nicht im else-Zweig, da */
	}
      }
    }
    else {
      if (IO_level4()) {
	IO_DruckeFlex("Die Gleichung %t = %t lebt zum Zeitpunkt %d nicht mehr! Gestorben %d\n", AndreasIndex->linkeSeite,\
		      AndreasIndex->rechteSeite, HK_getAbsTime_M(), AndreasIndex->todesTag);
      }
#if 0
      our_error("Fehler HS004");
#endif
    }
    AndreasIndex = AndreasIndex->Nachf;
  }
  return FALSE;
}

BOOLEAN MO_AllGleichungMatchNext (BOOLEAN Ordnungstest, GleichungsT *gleichungp)
{
  if (RE_GleichungsmengeLeer())
    return FALSE;
  else {
    if (AllMatchFirst)
      AllMatchFirst = FALSE;
    else {
      AndreasIndex = AndreasIndex->Nachf;
      if (ErfolgImBlattGehabt(Ordnungstest, gleichungp))
        return TRUE;
      MatcherNachBlattZuruecksetzen();
      BaumIndex = BK_Vorg(BaumIndex);
    }
    while (EinenKnotenAbOderVieleKnotenAufgestiegen()) {
      if (BK_IstBlatt(BaumIndex)) {
        if (RegelInBlattGefunden()) {
          AndreasIndex = BK_Regeln(BaumIndex);
          if (ErfolgImBlattGehabt(Ordnungstest, gleichungp))
            return TRUE;
        }
	MatcherNachBlattZuruecksetzen();
        BaumIndex = BK_Vorg(BaumIndex);
      }
    }
    return FALSE;
  }
}


#define /*Block*/ EinSymbolWeiter(/*UTermT*/ TermAlt, /*UTermT*/ TermNeu, /*Block*/ NullAction)                     \
{                                                                                                                   \
  TO_TermzelleHolen(TO_Schwanz(TermNeu));       /* neue Termzelle bereitstellen und an bisherige Zellen anhaengen */\
  TO_SymbolBesetzen(TermNeu = TO_Schwanz(TermNeu),TO_TopSymbol(TermAlt));         /* ausserdem Symbol uebertragen */\
  if (TO_Nullstellig(TermAlt)) {                       /* mehrsymbolige Terme wegen offenen Ende-Zeigers stapeln, */\
    TO_EndeBesetzen(TermNeu,TermNeu);                  /* einsymboliger gleich mit richtigem Ende-Zeiger versehen */\
    while (!(--TI2_TopZaehler)) {                          /* und offene Ende-Zeiger aus Stapel einmuenden lassen */\
      TO_EndeBesetzen(TI2_TopTeilterm,TermNeu);                                                                     \
      if (TI2_PopUndLeer())                                                           /* fertig, wenn Stapel leer */\
        NullAction;                                                                                                 \
    }                                                                                                               \
  }                                                                                                                 \
  else                                                                                                              \
    TI2_Push(TermNeu);                                                                                              \
}


TermT MO_AllTermErsetztNeu(UTermT TermAlt, UTermT Stelle)
{
  if (TO_PhysischGleich(TermAlt,Stelle))
    return SigmaRGanzNeu();
  else {
    TermT TermNeu,
          StartTermNeu;
    TO_TermzelleHolen(TermNeu);
    StartTermNeu = TermNeu;
    TO_SymbolBesetzen(TermNeu,TO_TopSymbol(TermAlt));
    TI2_Push(TermNeu);
    while (TO_PhysischUngleich(TermAlt = TO_Schwanz(TermAlt),Stelle))
      EinSymbolWeiter(TermAlt,TermNeu,{;});
    TO_NachfBesetzen(TermNeu,SigmaRGanzNeu());
    TermNeu = TO_TermEnde(TO_Schwanz(TermNeu));
    while (!(--TI2_TopZaehler)) {
      TO_EndeBesetzen(TI2_TopTeilterm,TermNeu);
      if (TI2_PopUndLeer())
        return StartTermNeu;
    }
    TermAlt = TO_NaechsterTeilterm(TermAlt);
    do
      EinSymbolWeiter(TermAlt,TermNeu, {
        TO_NachfBesetzen(TermNeu,NULL);
        return StartTermNeu;
      })
    while ((TermAlt = TO_Schwanz(TermAlt), TRUE));
  }
  /* hier kommen wir nie hin, aber um einige Compiler gluecklich zu machen :-) */
  our_error ("MO_AllTermErsetztNeu reached unreachable code?!");
  return NULL;
}


BOOLEAN MO_GleichungsrichtungPasstMitFreienVarsOhneOrdnung(GleichungsT Gleichung, UTermT Anfrage, 
                                                           UTermT *Ergebnis, UTermT aktTeilterm)
{
  TermT l = TP_LinkeSeite(Gleichung),
        r = TP_RechteSeiteUnfrei(Gleichung);
  return FreieVariablenOK(Gleichung) && MO_TermpaarPasstMitResultat(l,r,Anfrage,Ergebnis,aktTeilterm);
}



/*=================================================================================================================*/
/*= XIV. Matching modulo Skolemfunktionen                                                                         =*/
/*=================================================================================================================*/

BOOLEAN MO_SkolemGleich(TermT s, TermT t)
{
  UTermT NULLs = TO_NULLTerm(s);
  do
    if (TO_TopSymboleUngleich(s,t))
      return FALSE;
    else if (TO_IstSkolemTerm(s))
      s = TO_NaechsterTeilterm(s), t = TO_NaechsterTeilterm(t);
    else
      s = TO_Schwanz(s), t = TO_Schwanz(t);
  while (TO_PhysischUngleich(s,NULLs));
  return TRUE;
}

static UTermT SkolemSubstitut1(SymbolT x, TermT si, TermT s, TermT t)
{
  while (TO_PhysischUngleich(s,si))
    if (SO_SymbGleich(x,TO_TopSymbol(s)))
      return t;
    else if (TO_TermIstVar(s) || TO_IstSkolemTerm(s))
      s = TO_NaechsterTeilterm(s), t = TO_NaechsterTeilterm(t);
    else
      s = TO_Schwanz(s), t = TO_Schwanz(t);
  return NULL;
}

static UTermT SkolemSubstitut(TermT si, TermT s, TermT t, TermT s0, TermT t0)
{
  SymbolT x = TO_TopSymbol(si);
  UTermT Resultat = s0 ? SkolemSubstitut1(x, TO_NULLTerm(s0), s0,t0) : NULL;
  return Resultat ? Resultat : SkolemSubstitut1(x, si, s,t);
}

BOOLEAN MO_SkolemAllgemeiner(TermT s, TermT t,  TermT s0, TermT t0)
{
  TermT si = s,
        ti = t;
  UTermT NULLs = TO_NULLTerm(s),
         Si;
  do
    if (TO_TermIstVar(si))
      if ((Si = SkolemSubstitut(si, s,t,s0,t0)) && !MO_SkolemGleich(Si,ti))
        return FALSE;
      else
        si = TO_Schwanz(si), ti = TO_NaechsterTeilterm(ti);
    else if (TO_TopSymboleUngleich(si,ti))
      return FALSE;
    else if (TO_IstSkolemTerm(si))
      si = TO_NaechsterTeilterm(si), ti = TO_NaechsterTeilterm(ti);
    else
      si = TO_Schwanz(si), ti = TO_Schwanz(ti);
  while (TO_PhysischUngleich(si,NULLs));
  return TRUE;
}


BOOLEAN MO_TupelSkolemAllgemeiner(TermT s, TermT t,  TermT u, TermT v)
{
  return (MO_SkolemAllgemeiner(s,u,NULL,NULL) && MO_SkolemAllgemeiner(t,v,s,u)) || 
         (MO_SkolemAllgemeiner(s,v,NULL,NULL) && MO_SkolemAllgemeiner(t,u,s,v));
}
