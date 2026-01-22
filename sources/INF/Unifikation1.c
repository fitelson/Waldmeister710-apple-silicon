/*=================================================================================================================
  =                                                                                                               =
  =  Unifikation1.c                                                                                               =
  =                                                                                                               =
  =                                                                                                               =
  =                                                                                                               =
  =  07.10.94  Thomas.                                                                                            =
  =  07.02.95  Thomas. Rekursionen eliminiert.                                                                    =
  =                                                                                                               =
  =================================================================================================================*/

#ifndef CCE_Source
#include "Parameter.h"
#include "PhilMarlow.h"
#include "TankAnlage.h"
#include "WaldmeisterII.h"
#include "SpeicherParameter.h"

#else
#include "parse-util.h"
#define PA_SubconP() (0)
#define PA_SubconR() (0)
#define PA_SubconE() (0)
#define KPV_insertCP( lhs,  rhs,  i,  j, xp)
#define PA_acgRechts() (1)
#define PA_skRechts() (1)
#define PA_doSK() (1)
#define WM_ErsterAnlauf() (1)
#define PA_EinsSternAn() (0)
#define PA_NusfuAn() (0)
#define PA_KernAn() (0)
#define PA_BackAn() (0)
#define TO_KombinatorapparatAktivieren() 

#define U1_AnzErsterVariablen 10

#endif

#include "compiler-extra.h"
#include "general.h"
#include "Ordnungen.h"
#include "ACGrundreduzibilitaet.h"
#include "Ausgaben.h"
#include "Constraints.h"
#include "Dispatcher.h"
#include "DSBaumOperationen.h"
#include "FehlerBehandlung.h"
#include "Grundzusammenfuehrung.h"
#include "Hauptkomponenten.h"
#include "Konsulat.h"
#include "KPVerwaltung.h"
#include "MatchOperationen.h"
#include "MissMarple.h"
#include "NFBildung.h"
#include "Position.h"
#include "Statistiken.h"
#include <string.h>
#include "TermIteration.h"
#include "TermOperationen.h"
#include "Termpaare.h"
#include "Unifikation1.h"
#include "Unifikation2.h"
#include "Zielverwaltung.h"


TI_IterationDeklarieren(U1)
TI2_IterationDeklarieren(U1)

static void GebildetesKPBehandelnMitVater(RegelOderGleichungsT father, TermT lhs, TermT rhs, UTermT actualPosition,
                                          RegelOderGleichungsT actualParent, TermT lhsReplica);

static void GebildetesKPBehandelnMitMutter(RegelOderGleichungsT mother, TermT lhs, TermT rhs, UTermT actualPosition,
                                           RegelOderGleichungsT actualParent, TermT lhsReplica);







/*=================================================================================================================*/
/*= I. Variablenbank fuer Substitutionen; Stack angelegter Bindungen; sonstige globale Groessen                   =*/
/*=================================================================================================================*/

int xUDimension;                              /* neueste bei gegenwaertiger Ausdehnung zu bindende Variable */
typedef struct {
  UTermT Substitut;
} U1_EintragsT;
static U1_EintragsT *xU;


static unsigned int StackZeiger;
typedef SymbolT StackElementT;
static StackElementT *BindungsStack;
/* StackZeiger ist stets der Index des letzten beschriebenen Elements.
   Das nullte Element wird nicht beschrieben.
   -xUDimension ist der Index des letzten bei gegenwaertiger Ausdehnung noch zu schreibenden Elements.
*/

#define /*void*/ PushVariable(/*SymbolT*/ Variable)                                                                 \
  BindungsStack[++StackZeiger] = Variable;

#define /*unsigned int*/ PopVariable(/*void*/)                                                                      \
  (BindungsStack[StackZeiger--])

#define /*BOOLEAN*/ SchonVariablenGebunden(/*void*/)                                                                \
  (StackZeiger)


static unsigned int Endestapel1Zeiger,
                    Endestapel2Zeiger,
                    Endestapel3Zeiger;
typedef struct {
  UTermT Weiter,
         Ende;
} EndeEintragsT;
static EndeEintragsT *Endestapel1,
                     *Endestapel2,
                     *Endestapel3;                                     /* Endestapel laufen ebenfalls ab Element 1 */


void U1_VariablenfelderAnpassen(SymbolT DoppelteNeuesteBaumvariable)   /* Variablenbank und Stapel angelegter Bin- */
{    /* dungen so anpassen, dass Variablen x1, x2, ... , x(DoppelteNeuesteBaumvariable) verdaut werden. Darf nicht */
  if (SO_VariableIstNeuer(DoppelteNeuesteBaumvariable,xUDimension)) { /* waehrend des Unifizierens aufgerufen wer- */
    SymbolT Index;                                                                      /* den, sondern nur davor. */
    unsigned int n;
    negarray_dealloc(xU,-xUDimension);
    xUDimension = SO_VarDelta(DoppelteNeuesteBaumvariable,U1_AnzErsterVariablen);
    n = SO_VarNummer(xUDimension);
    xU = negarray_alloc(n,U1_EintragsT,n);
    SO_forSymboleAbwaerts(Index,SO_ErstesVarSymb,xUDimension)
      xU[Index].Substitut = NULL;
    posarray_realloc(BindungsStack,n,StackElementT,1);
    posarray_realloc(Endestapel1,n,EndeEintragsT,1);
    posarray_realloc(Endestapel2,n,EndeEintragsT,1);
    posarray_realloc(Endestapel3,n,EndeEintragsT,1);
  }
}


static KantenT BaumIndex;                                     /* Zeiger fuer Durchlauf des Baumes beim Unifizieren */
#define aktTermrest BK_aktTIndexU(BaumIndex)
#define SymbolAusTerm TO_TopSymbol(aktTermrest)                 /* naechstes zu matchendes Zeichen aus Anfrageterm */

static unsigned int AnzImKnotenAngelegterBindungen;                                     /* wie der Name schon sagt */
static BOOLEAN AbstiegMitZielErfolgt;
static UTermT TermNachZiel;

/* Zur Verwaltung der Groesse AnzImKnotenAngelegterBindungen

   Initialwert wird in U1_InitApriori gesetzt und ist 0.
   Dieser Wert wird nach dem Verlassen von U1_KPsBilden wieder angenommen.

   Unifikation erfolgt in den drei Kategorien
   -MitEigenenTeiltermen,
   -MitDSBaumTeiltermen,
   -MitDSBaum.

   Jede dieser drei Kategorien muss nach dem Verlassen die Variable wieder
   auf 0 setzen. Fuer die ersten beiden Kategorien ist dies trivial und
   geschieht durch Aufruf von KnotenbindungenAufheben() nach jedem Aufruf
   von Unifiziert, also auch nach dem letzten.

   Unifizieren mit dem Baum: Wird ein Knoten betrachtet, so gibt es mehre-
   re Moeglichkeiten, aus diesem Knoten weiterzukommen. Diese sind unter-
   schieden nach der Beschaffenheit des aktuellen Symbols aus dem Anfrage-
   term. Der Knoten kann nach unten oder oben verlassen werden. In diesem
   Fall wird AnzIKAB auf 0 gesetzt; ggf. werden Bindungen aufgehoben.
   Damit diese Entscheidung zustande kommen kann, werden eine Reihe von
   Abstiegsvarianten ueberprueft. Nur fuer die erfolglose Unifikation sind
   dabei Bindungen aufzuheben und AnzIKAB rueckzusetzen. Die Funktion
   Unifiziert eine Variante MGUMitBlattGefunden. Deren Veraenderungen von
   MGU und AnzIKAB werden aber durch AusBlattAufsteigen() explizit rueck-
   gaengig gemacht.

   Da bei jedem Aufstieg zu einem Knoten AnzIKAB den Wert 0 hat, gilt dies
   auch beim letzten Aufstieg zur Wurzel. Die dort unternommenen Versuche,
   nochmals abzusteigen, schlagen fehl und belassen AnzIKAB den Wert 0.
*/



/*=================================================================================================================*/
/*= II. Operationen auf oder mit Substitutionen                                                                   =*/
/*=================================================================================================================*/

#define /*UTermT*/ SubstitutVon(/*SymbolT*/ VarSymbol)                                                              \
  (xU[VarSymbol].Substitut)


#define /*BOOLEAN*/ VarGebunden(/*SymbolT*/ VarSymbol)                                                              \
  (SubstitutVon(VarSymbol))


#define /*BOOLEAN*/ SymbGebundeneVar(/*SymbolT*/ Symbol)                                                            \
  (SO_SymbIstVar(Symbol) && (VarGebunden(Symbol)))


static void BindungenAufheben(unsigned int Anzahl)
{
  while (Anzahl--)
    xU[PopVariable()].Substitut = NULL;
}


#define /*void*/ Entbinden(/*SymbolT*/ Topsymbol, /*UTermT*/ Term)                                                  \
  while (SymbGebundeneVar(Topsymbol))                                                                               \
    Topsymbol = TO_TopSymbol(Term = SubstitutVon(Topsymbol))


#define /*void*/ FertigEntbindenMitEnde(/*SymbolT*/ Topsymbol, /*UTermT*/ TermAlt, /*UTermT*/ TermAltEnde)          \
{                                                                                                                   \
  do                                                                                                                \
    Topsymbol = TO_TopSymbol(TermAlt = SubstitutVon(Topsymbol));                                                    \
  while (SymbGebundeneVar(Topsymbol));                                                                              \
  TermAltEnde = TO_TermEnde(TermAlt);                                                                               \
}


#define /*void*/ KnotenbindungenAufheben(/*void*/)                                                                  \
 BindungenAufheben(AnzImKnotenAngelegterBindungen);                                                                 \
 AnzImKnotenAngelegterBindungen = 0;



/*=================================================================================================================*/
/*= III. Operationen zum flachen Unifizieren                                                                      =*/
/*=================================================================================================================*/

/* Verwaltung der Endestapel */

#define /*void*/ Endestapel12Initialisieren(/*void*/)                                                               \
  Endestapel1Zeiger = Endestapel2Zeiger = 0;


#define /*void*/ Endestapel1Initialisieren(/*void*/)                                                                \
  Endestapel1Zeiger = 0;


#define /*void*/ Endestapel3Initialisieren(/*void*/)                                                                \
  Endestapel3Zeiger = 0;


#define /*void*/ PushEndestapel(/*{1;2;3}*/ Wohin, /*UTermT*/ Rest, /*UTermT*/ Enderest)                            \
{                                                                                                                   \
  Endestapel ## Wohin [++Endestapel ## Wohin ## Zeiger].Weiter = Rest;                                              \
  Endestapel ## Wohin [Endestapel ## Wohin ## Zeiger].Ende = Enderest;                                              \
}


#define /*void*/ PopEndestapelIn(/*{1;2;3}*/ Woher, /*UTermT*/ Rest, /*UTermT*/ Enderest)                           \
{                                                                                                                   \
  Rest = Endestapel ## Woher [Endestapel ## Woher ## Zeiger].Weiter;                                                \
  Enderest = Endestapel ## Woher [Endestapel ## Woher ## Zeiger--].Ende;                                            \
}


#define /*BOOLEAN*/ EndestapelLeer(/*{1;2;3}*/ Welcher)                                                             \
  (!Endestapel ## Welcher ## Zeiger)


static BOOLEAN DoppeltWeitergeschaltet12(UTermT *Term1, UTermT *Term1Ende, UTermT *Term2, UTermT *Term2Ende)
{
  while (TO_PhysischGleich(*Term1,*Term1Ende))
    if (EndestapelLeer(1))
      return FALSE;
    else
      PopEndestapelIn(1,*Term1,*Term1Ende);
  while (TO_PhysischGleich(*Term2,*Term2Ende))
    PopEndestapelIn(2,*Term2,*Term2Ende);
  *Term1 = TO_Schwanz(*Term1);
  *Term2 = TO_Schwanz(*Term2);
  return TRUE;
}


static BOOLEAN Weitergeschaltet3(UTermT *Term, UTermT *EndeTerm)
{
  while (TO_PhysischGleich(*Term,*EndeTerm))
    if (EndestapelLeer(3))
      return FALSE;
    else
      PopEndestapelIn(3,*Term,*EndeTerm);
  *Term = TO_Schwanz(*Term);
  return TRUE;
}


#define /*Statements*/ RumpfWeiterschalten(/*{1;2;3}*/ StapelNr, /*UTermT*/ Term, /*UTermT*/ Schluss)               \
  while (TO_PhysischGleich(Term,Schluss) && !EndestapelLeer(StapelNr))                                              \
    PopEndestapelIn(StapelNr,Term,Schluss);                                                                         \
  Term = TO_Schwanz(Term);                                                                                          \


#define /*void*/ Weiterschalten3(/*UTermT*/ Term, /*UTermT*/ Schluss)                                               \
RumpfWeiterschalten(3,Term,Schluss)


#define /*void*/ Weiterschalten1(/*UTermT*/ Term, /*UTermT*/ Schluss)                                               \
RumpfWeiterschalten(1,Term,Schluss)


static BOOLEAN SubstErgaenzt(SymbolT Variable, UTermT Term)
{                 /* Vorbedingung: Variable darf noch nicht gebunden sein. Macht dann occur check und passt xU an. */
  if (SchonVariablenGebunden()) {
    SymbolT Topsymbol = TO_TopSymbol(Term);                                        /* Bindung x(i)<-x(i) abfangen; */
    Entbinden(Topsymbol,Term);                                        /* klappt naemlich immer und bewirkt nichts. */
    if (SO_SymbGleich(Topsymbol,Variable))
      return TRUE;
    /* Ausserdem wurde, falls Term=x(i), x(i)<-x(j), x(j)<-x(k), ... gleich das am weitesten aussubstituierte Sub-
       stitut gewaehlt, das ohne Anlegen neuer Termzellen gebildet werden kann.
       Term ist nun sicher keine gebundene Variable mehr.  */
    if (SO_SymbIstFkt(Topsymbol)) {                         /* wenn Topsymbol Variable, dann frei => gleich binden */
      UTermT TermIndex = Term,                            /* sonst Term durchlaufen, aussubstituieren, occur check */
             TermIndexEnde = TO_TermEnde(Term);
      SymbolT Symbol;
      Endestapel3Initialisieren();
      while (Weitergeschaltet3(&TermIndex,&TermIndexEnde)) {
      Start:
        Symbol = TO_TopSymbol(TermIndex);
        if (SO_SymbIstVar(Symbol)) {
          if (SO_SymbGleich(Symbol,Variable))
            return FALSE;
          else if (VarGebunden(Symbol)) {
            PushEndestapel(3,TermIndex,TermIndexEnde);
            TermIndexEnde = TO_TermEnde(TermIndex = SubstitutVon(Symbol));
            goto Start;
          }
	}
      }
    }
  } /* Da die miteinander zu unifizierenden Terme variablendisjunkt sind,
       kann man sich bei der ersten Bindung den Occur check sparen. */

  xU[Variable].Substitut = Term;
  PushVariable(Variable);
  AnzImKnotenAngelegterBindungen++;
  return TRUE;
}


#define /*void*/ AufTeiltermende(/*{1;2}*/ Wo)                                                                      \
  Term ## Wo = TO_TermEnde(Term ## Wo)


#define /*void*/ AufSubstitut(/*{1;2}*/ Wo)                                                                         \
  PushEndestapel(Wo,Term ## Wo,Term ## Wo ## Ende);                                                                 \
  Term ## Wo ## Ende = TO_TermEnde(Term ## Wo = SubstitutVon(Symbol ## Wo));                                        \
  Symbol ## Wo = TO_TopSymbol(Term ## Wo);


#define /*BOOLEAN*/ RumpfUnifiziertBis(/*UTermT*/ Term1, /*UTermT*/ Term1Ende,/*UTermT*/ Term2,/*UTermT*/ Term2Ende)\
{                                                                                                                   \
  Endestapel12Initialisieren();                                                                                     \
  do {                                                                                                              \
    SymbolT Symbol1 = TO_TopSymbol(Term1),                                                                          \
            Symbol2 = TO_TopSymbol(Term2);                                                                          \
  Start:                                                                                                            \
    if (SO_SymbUngleich(Symbol1,Symbol2)) {                                                                         \
      if (SO_SymbIstVar(Symbol1)) {                                                                                 \
        if (VarGebunden(Symbol1)) {                                                                                 \
          AufSubstitut(1);                                                                                          \
          goto Start;                                                                                               \
        }                                                                                                           \
        else if (SubstErgaenzt(Symbol1,Term2))                                                                      \
          AufTeiltermende(2);                                                                                       \
        else                                                                                                        \
          return FALSE;                                                                                             \
      }                                                                                                             \
      else if (SO_SymbIstVar(Symbol2)) {                                                                            \
        if (VarGebunden(Symbol2)) {                                                                                 \
          AufSubstitut(2);                                                                                          \
          goto Start;                                                                                               \
        }                                                                                                           \
        else if (SubstErgaenzt(Symbol2,Term1))                                                                      \
          AufTeiltermende(1);                                                                                       \
        else                                                                                                        \
          return FALSE;                                                                                             \
      }                                                                                                             \
      else                                                                                                          \
        return FALSE;                                                                                               \
    }                                                                                                               \
  } while (DoppeltWeitergeschaltet12(&Term1,&Term1Ende,&Term2,&Term2Ende));                                         \
  return TRUE;                                                                                                      \
}


static BOOLEAN UnifiziertBis(UTermT Term1, UTermT Term1Ende, UTermT Term2, UTermT Term2Ende)
  RumpfUnifiziertBis(Term1,Term1Ende,Term2,Term2Ende)


static BOOLEAN Unifiziert(UTermT Term1, UTermT Term2)
{
  UTermT Term1Ende = TO_TermEnde(Term1),
         Term2Ende = TO_TermEnde(Term2);
  RumpfUnifiziertBis(Term1,Term1Ende,Term2,Term2Ende)
}

BOOLEAN U1_Unifiziert(UTermT Term1, UTermT Term2)  /* Dies ist eine kleine Routine zum Unifizieren zweier beliebi- */
{  /* ger Terme. Der verwendete Unifikationsapparat geht normalerweise davon aus, dass die Terme variablendisjunkt */
  BOOLEAN Unifizierbar;     /* sind; daher ist beim Anlegen der ersten Bindung kein Occur check erforderlich. Dies */
  PushVariable(xUDimension);   /* wird hier mit einem Trick umschifft. Fuer zeitkritischen Einsatz ist diese Funk- */
  AnzImKnotenAngelegterBindungen++;                                                      /* tion weniger geeignet. */
  Unifizierbar = Unifiziert(Term1,Term2);
  KnotenbindungenAufheben();
  return Unifizierbar;
}



/*=================================================================================================================*/
/*= IV. Erzeugung von Termkopien unter Substitutionen und Termersetzungen                                         =*/
/*=================================================================================================================*/

/* ------- 0. Praeliminarien ------------------------------------------------------------------------------------- */

#define /*void*/ AufAngehaengteZelle(/*UTermT*/ Index, /*SymbolT*/ Symbol)                                          \
{                                                                                                                   \
  TO_AufNaechsteNeueZelle(Index);                                                                                   \
  TO_SymbolBesetzen(Index,Symbol);                                                                                  \
}


#define /*void*/ AufNaechstesLesesymbol(/*UTermT*/ TermAlt, /*{1;2;3}*/ Wohin)                                      \
{                                                                                                                   \
  Topsymbol = TO_TopSymbol(TermAlt);                                                                                \
  if (SymbGebundeneVar(Topsymbol)) {                       /* sichern, dass TermAlt keine gebundene Variable ist, */\
    PushEndestapel(Wohin,TermAlt,TermAlt ## Ende);                        /* sondern freie Variable oder Funktion */\
    FertigEntbindenMitEnde(Topsymbol,TermAlt,TermAlt ## Ende);                                                      \
  }                                                                                                                 \
}                                                           /* vorausgesetzte Bezeichner: 'Topsymbol, TermAlt'Ende */


#define /*Block*/ EinSymbolWeiter(/*UTermT*/ TermAlt, /*{1;2;3}*/ EndestapelNr,                                     \
                                                                  /*{TI;TI2}*/ KopierstapelNr, /*Block*/ NullAction)\
{                                                                                                                   \
  AufNaechstesLesesymbol(TermAlt,EndestapelNr);                                                                     \
  AufAngehaengteZelle(IndexNeu,Topsymbol);     /* neue Termzelle bereitstellen und an bisherige Zellen anhaengen, */\
                                                                                  /* ausserdem Symbol uebertragen */\
  if (TO_Nullstellig(TermAlt)) {                       /* mehrsymbolige Terme wegen offenen Ende-Zeigers stapeln, */\
    TO_EndeBesetzen(IndexNeu,IndexNeu);                /* einsymboliger gleich mit richtigem Ende-Zeiger versehen */\
    while (!(--KopierstapelNr ## _TopZaehler)) {           /* und offene Ende-Zeiger aus Stapel einmuenden lassen */\
      TO_EndeBesetzen(KopierstapelNr ## _TopTeilterm,IndexNeu);                                                     \
      if (KopierstapelNr ## _PopUndLeer()) {                                          /* fertig, wenn Stapel leer */\
        NullAction;                                                                                                 \
      }                                                                                                             \
    }                                                                                                               \
    Weiterschalten ## EndestapelNr(TermAlt,TermAlt ## Ende);           /* weiter auf naechstes Symbol aus TermAlt */\
  }                                              /* dabei ggf. aus gerade zu Ende gelesenem Substitut herausgehen */\
  else {                                                                                                            \
    KopierstapelNr ## _Push(IndexNeu);                                                                              \
    TermAlt = TO_Schwanz(TermAlt);                                                                                  \
  }                                                                                                                 \
}                                                /* vorausgesetzte Bezeichner: 'Topsymbol, TermAlt'Ende, 'IndexNeu */


static TermT MGU(UTermT TermAlt);

void U1_DruckeMGU(void)
{
  SymbolT VarIndex;

  SO_forSymboleAbwaerts(VarIndex,SO_ErstesVarSymb,xUDimension)
    if (VarGebunden(VarIndex)) {
      TermT Substitut = MGU(SubstitutVon(VarIndex));
      IO_DruckeFlex("x%d <- %t\n",SO_VarNummer(VarIndex),Substitut);
      TO_TermLoeschen(Substitut);
    }
}


/* ------- 1. Funktionen fuer gewoehnliche Termdarstellung ------------------------------------------------------- */

static TermT MGU(UTermT TermAlt)                                                /* liefert Term mit EndeNachf NULL */
{
  SymbolT Topsymbol =  TO_TopSymbol(TermAlt);
  Entbinden(Topsymbol,TermAlt);

  if (TO_Nullstellig(TermAlt))    /* wenn nullstellig (Konstante oder freie Variable), dann eine Termzelle zurueck */
    return TO_NullstelligerTerm(Topsymbol);
  else {                                                                         /* ansonsten TermAlt durchwandern */
    UTermT TermNeu = TO_HoleErsteNeueZelle(),
           IndexNeu = TermNeu,
           TermAltEnde = TO_TermEnde(TermAlt);
    TO_SymbolBesetzen(TermNeu,Topsymbol);                       /* Termzelle fuer Topsymbol besetzen mit Topsymbol */
    TI_Push(TermNeu);
    TermAlt = TO_Schwanz(TermAlt);                                      /* weiter auf naechstes Symbol aus TermAlt */
    Endestapel3Initialisieren();
    do
      EinSymbolWeiter(TermAlt,3,TI, {                                                  /* fertig, wenn Stapel leer */
        TO_LetzteNeueZelleMelden(IndexNeu);
        TO_NachfBesetzen(IndexNeu,NULL);                                                      /* Ende-Nachf setzen */
        return TermNeu;                                                                                 /* und weg */
      })
    while (TRUE);
  }
}


static TermT MGUMitSt(TermT TermAlt, UTermT Stelle, TermT *SubstStelle)
{
  if (TO_PhysischGleich(TermAlt,Stelle))           /* Wenn Stelle = TermAlt, dann sofort MGU(Stelle) zurueckgeben. */
    return (*SubstStelle = MGU(Stelle));          /* Im anderen Fall den Term durchwandern. Dieser ist dann sicher */
  else {                                /* nicht nullstellig, da nullstellige Terme nur eine einzige Stelle haben. */
    UTermT TermNeu = TO_HoleErsteNeueZelle(),
           IndexNeu = TermNeu,
           TermAltEnde = TO_TermEnde(TermAlt);
    SymbolT Topsymbol = TO_TopSymbol(TermAlt);

    /*** 1. Schritt: Erzeugung von sigma(TermAlt) bis vor Stelle ***/
    TO_SymbolBesetzen(TermNeu,Topsymbol);                               /* Erste Termzelle besetzen mit Topsymbol. */
    TI_Push(TermNeu);         /* Nun TermAlt durchwandern, bis Stelle erreicht. Stelle liegt im Inneren des Terms. */
    TermAlt = TO_Schwanz(TermAlt);
    Endestapel3Initialisieren();
    while (TO_PhysischUngleich(TermAlt,Stelle))
      EinSymbolWeiter(TermAlt,3,TI,{;});              /* keine Aktion bei leerem Stapel, da Stapel nicht leer wird */

    /*** 2. Schritt: Erzeugung von sigma(Stelle) ***/
    EinSymbolWeiter(TermAlt,3,TI, {                                                    /* fertig, wenn Stapel leer */
      TO_LetzteNeueZelleMelden(IndexNeu);                                /* ggf. dann diese innere Stelle zuweisen */
      TO_NachfBesetzen(*SubstStelle = IndexNeu,NULL);                                         /* Ende-Nachf setzen */
      return TermNeu;                                                                                   /* und weg */
    });
    *SubstStelle = IndexNeu;       /* andernfalls folgen noch weitere Teilterme; jetzt aber innere Stelle zuweisen */

    /*** 3. Schritt: Erzeugung des Endes von sigma(TermAlt) ***/
    do
      EinSymbolWeiter(TermAlt,3,TI, {                                                  /* fertig, wenn Stapel leer */
        TO_LetzteNeueZelleMelden(IndexNeu);
        TO_NachfBesetzen(IndexNeu,NULL);                                                      /* Ende-Nachf setzen */
        return TermNeu;                                                                                 /* und weg */
      })
    while (TRUE);

  }
}


static TermT MGUNachTE(TermT TermAlt, UTermT Stelle, TermT Einbau)
#define RumpfMGUNachTE(/*LAusdruck=*/ Ergaenzung1, /*Zuweisung*/ Ergaenzung2)                                       \
{                                                                                                                   \
  if (TO_PhysischGleich(TermAlt,Stelle))          /* Wenn Stelle = TermAlt, dann sofort MGU(Einbau) zurueckgeben. */\
    return (Ergaenzung1 MGU(Einbau));                           /* Bei MGUNachTEMitSt hier Eingesetztes zuweisen. */\
  else {                      /* Im anderen Fall den Term durchwandern. Dieser ist dann sicher nicht nullstellig, */\
    UTermT TermNeu = TO_HoleErsteNeueZelle(),             /* da nullstellige Terme nur eine einzige Stelle haben. */\
           IndexNeu = TermNeu,                                                                                      \
           TermAltEnde = TO_TermEnde(TermAlt);                                                                      \
    SymbolT Topsymbol = TO_TopSymbol(TermAlt);                                                                      \
                                                                                                                    \
    /*** 1. Schritt: Erzeugung von sigma(TermAlt) bis vor der Einbaustelle ***/                                     \
    TO_SymbolBesetzen(TermNeu,Topsymbol);                              /* Erste Termzelle besetzen mit Topsymbol. */\
    TI2_Push(TermNeu);       /* Nun TermAlt durchwandern, bis Stelle erreicht. Stelle liegt im Inneren des Terms. */\
    TermAlt = TO_Schwanz(TermAlt);                                                                                  \
    Endestapel3Initialisieren();                                                                                    \
    while (TO_PhysischUngleich(TermAlt,Stelle))                                                                     \
      EinSymbolWeiter(TermAlt,3,TI2,{;});            /* keine Aktion bei leerem Stapel, da Stapel nicht leer wird */\
                                                                                                                    \
    /*** 2. Schritt: Erzeugung von sigma(Einbau) ***/                                                               \
    {                                                                                                               \
      Topsymbol =  TO_TopSymbol(Einbau);                             /* IndexNeu ist die bisher letzte neue Zelle */\
      Entbinden(Topsymbol,Einbau);                           /* sichern, dass Einbau keine gebundene Variable ist */\
      AufAngehaengteZelle(IndexNeu,Topsymbol);                 /* Termzelle fuer Topsymbol besetzen mit Topsymbol */\
      Ergaenzung2;                                               /* bei MGUNachTEMitSt hier Eingesetztes zuweisen */\
                                                                                                                    \
      if (TO_Nullstellig(Einbau))   /* wenn nullstellig (Konst. oder freie Variable), dann eine Termzelle zurueck */\
        TO_EndeBesetzen(IndexNeu,IndexNeu);                                                                         \
      else {                                                                     /* ansonsten Einbau durchwandern */\
        UTermT EinbauEnde = TO_TermEnde(Einbau);                                                                    \
        TI_Push(IndexNeu);                                                                                          \
        Einbau = TO_Schwanz(Einbau);                                                                                \
        Endestapel1Initialisieren();                                                                                \
        do                                                                                                          \
          EinSymbolWeiter(Einbau,1,TI,{goto Fertig;})                                                               \
        while (TRUE);                                                                                               \
        Fertig: ;                                                                                                   \
      }                                                                                                             \
    }                                                                                                               \
    while (!(--TI2_TopZaehler)) {                              /* offene Ende-Zeiger aus Stapel einmuenden lassen */\
      TO_EndeBesetzen(TI2_TopTeilterm,IndexNeu);                                                                    \
      if (TI2_PopUndLeer()) {                                                         /* fertig, wenn Stapel leer */\
        TO_LetzteNeueZelleMelden(IndexNeu);                                                                         \
        TO_NachfBesetzen(IndexNeu,NULL);                                                     /* Ende-Nachf setzen */\
        return TermNeu;                                                                               /* und weg. */\
      }                                                                                                             \
    }                                                                                                               \
                                                                                                                    \
    /*** 3. Schritt: Erzeugung des Endes von sigma(TermAlt) ***/                                                    \
    TermAlt = TO_NaechsterTeilterm(TermAlt);                                        /* Einbaustelle ueberspringen */\
    do                                                                                                              \
      EinSymbolWeiter(TermAlt,3,TI2, {                                                /* fertig, wenn Stapel leer */\
        TO_LetzteNeueZelleMelden(IndexNeu);                                                                         \
        TO_NachfBesetzen(IndexNeu,NULL);                                                     /* Ende-Nachf setzen */\
        return TermNeu;                                                                                /* und weg */\
      })                                                                                                            \
    while (TRUE);                                                                                                   \
  }                                                                                                                 \
}
  RumpfMGUNachTE(,)

static TermT MGUNachTEMitSt(TermT TermAlt, UTermT Stelle, TermT Einbau, TermT *Eingesetztes)
  RumpfMGUNachTE(*Eingesetztes =,*Eingesetztes = IndexNeu)



/*=================================================================================================================*/
/*= V. Operationen zum Unifizieren auf Blattebene                                                                 =*/
/*=================================================================================================================*/

#define /*UTermT*/ RegelrestBeimBlatterreichen(/*void*/)                                                            \
  (AbstiegMitZielErfolgt ? TermNachZiel : BK_Rest(BaumIndex))


#define /*BOOLEAN*/ MGUMitBlattGefunden(/*DSBaumT*/ Baum)                                                           \
  (!RegelrestBeimBlatterreichen() ||                                                                                \
   UnifiziertBis(RegelrestBeimBlatterreichen(),TO_TermEnde(BK_linkeSeite(BaumIndex)),                               \
                 aktTermrest,TO_TermEnde(BK_aktTIndexU(Baum.Wurzel))))


#define /*BOOLEAN*/ MGUMitBlattGefundenAusschluss(/*DSBaumT*/ Baum, /*GNTermpaarT*/ Ausschluss)                     \
  (BK_Regeln(BaumIndex)!=Ausschluss && MGUMitBlattGefunden(Baum))
/* Prueft, ob das Ausschlussobjekt als erstes im Blatt steht. */



/*=================================================================================================================*/
/*= VI. Im Baum absteigen fuer SymbolAusTerm Funktionssymbol                                                      =*/
/*=================================================================================================================*/

static BOOLEAN Delta0(void);
static BOOLEAN DeltaFktSymbol1(void);

#define /*void*/ AbstiegVerwalten(/*KantenT*/ Nachfolger,                                                           \
                                  /*{Ziel,Symbol}*/ SorteDatum,                                                     \
                                  /*SprungeintragsT U SymbolT*/ naechstesDatum,                                     \
                                  /*UDeltaT*/ naechstesDelta,                                                       \
                                  /*UTermT*/ neuerTermrest,                                                         \
                                  /*unsigned short int*/ AnzahlBindungen)                                           \
{                                                                                                                   \
  KantenT Nachf = Nachfolger;                                                                                       \
  BK_UVorgSei(Nachf,BaumIndex);                                                                                     \
  BK_UAnzBindungenSei(BaumIndex,AnzahlBindungen);                                                                   \
  BK_UDatumSei(BaumIndex,SorteDatum,naechstesDatum);                                                                \
  BK_UDeltaSei(BaumIndex,naechstesDelta);                                                                           \
  BK_aktTIndexUSei(Nachf,neuerTermrest);                                                                            \
  BK_UDeltaSei(Nachf,Delta0);                                                                                       \
  BaumIndex = Nachf;                                                                                                \
  AnzImKnotenAngelegterBindungen = 0;                                                                               \
  return TRUE;                                                                                                      \
}


#define MitSelbemSymbolAb(/*SymbolT*/ naechstesSymbol, /*UDeltaT*/ naechstesDelta)                                  \
{                                                                                                                   \
  if (BK_Nachf(BaumIndex,SymbolAusTerm))                                                                            \
    AbstiegVerwalten(BK_Nachf(BaumIndex,SymbolAusTerm),                                                             \
      Symbol,naechstesSymbol,naechstesDelta,TO_Schwanz(aktTermrest),0);                                             \
}


#define TermAnBVarGebundenUndAb(/*SymbolT*/ BaumVar, /*UDeltaT*/ naechstesDelta)                                    \
/* versucht Bindung Baumvariable<-aktueller Teilterm des Anfrageterms vorzunehmen. FALSE, falls Fehler beim Occur   \
   check; ansonsten wird Baum absteigen. */                                                                         \
/* Aufruf nur in TermAnEineBVarGebundenOderUnifiziertUndAb(erste,letzte) */                                         \
{                                                                                                                   \
  if (SubstErgaenzt(BaumVar,aktTermrest))                                                                           \
    AbstiegVerwalten(BK_Nachf(BaumIndex,BaumVar),Symbol,                                                            \
      SO_VorigesVarSymb(BaumVar),naechstesDelta,TO_NaechsterTeilterm(aktTermrest),AnzImKnotenAngelegterBindungen);  \
}


#define TermMitBVarUnifiziertUndAb(/*SymbolT*/ BaumVar, /*UDeltaT*/ naechstesDelta)                                 \
/* versucht aktuellen Teilterm des Anfrageterms mit gebundener Variable aus dem Baum zu unifizieren. */             \
/* Aufruf nur in TermAnEineBVarGebundenOderUnifiziertUndAb(erste,letzte) */                                         \
{                                                                                                                   \
  if (Unifiziert(SubstitutVon(BaumVar),aktTermrest))                                                                \
    AbstiegVerwalten(BK_Nachf(BaumIndex,BaumVar),Symbol,SO_VorigesVarSymb(BaumVar),naechstesDelta,                  \
      TO_NaechsterTeilterm(aktTermrest),AnzImKnotenAngelegterBindungen);                                            \
  KnotenbindungenAufheben();                                                                                        \
}


#define TermAnEineBVarGebundenOderUnifiziertUndAb(/*SymbolT*/ ersteKandidatin,/*UDeltaT*/ naechstesDelta)           \
/* wird auch fuer SymbolAusTerm gebundene Variable verwendet */                                                     \
{                                                                                                                   \
  SymbolT letzteKandidatin = min(SO_ErstesVarSymb,BK_maxSymb(BaumIndex)),                                           \
          BaumVar;                                                                                                  \
  SO_forSymboleAufwaerts(BaumVar,ersteKandidatin,letzteKandidatin)                                                  \
    if (BK_Nachf(BaumIndex,BaumVar)) {                                                                              \
      if (VarGebunden(BaumVar))                                                                                     \
        TermMitBVarUnifiziertUndAb(BaumVar,naechstesDelta)                                                          \
      else                                                                                                          \
        TermAnBVarGebundenUndAb(BaumVar,naechstesDelta);                                                            \
    }                                                                                                               \
}


static BOOLEAN DeltaFktSymbol0(void)
{
  MitSelbemSymbolAb(BK_minSymb(BaumIndex),DeltaFktSymbol1);
  TermAnEineBVarGebundenOderUnifiziertUndAb(BK_minSymb(BaumIndex),DeltaFktSymbol1);
  return FALSE;
}

static BOOLEAN DeltaFktSymbol1(void)
{
  TermAnEineBVarGebundenOderUnifiziertUndAb(BK_USymbol(BaumIndex),DeltaFktSymbol1);
  return FALSE;
}



/*=================================================================================================================*/
/*= VII. Im Baum absteigen fuer SymbolAusTerm gebundene Variable                                                  =*/
/*=================================================================================================================*/

static BOOLEAN DeltaGebundeneVar1(void);
static BOOLEAN DeltaGebundeneVar2(void);

#define EinZielMitTVarUnifiziertUndAb(/*SprungeintragsT*/ erstesZiel, /*UDeltaT*/ naechstesDelta)                   \
{                                                                                                                   \
  SprungeintragsT ZielIndex;                                                                                        \
  BK_forSprungziele(ZielIndex,erstesZiel) {                                                                         \
    if (Unifiziert(ZielIndex->Teilterm,aktTermrest)) {                                                              \
      TermNachZiel = TO_NaechsterTeilterm(ZielIndex->Teilterm);                                                     \
      AbstiegMitZielErfolgt = TRUE;                                                                                 \
      AbstiegVerwalten(ZielIndex->Zielknoten,Ziel,ZielIndex->NaechsterZieleintrag,naechstesDelta,                   \
        TO_NaechsterTeilterm(aktTermrest),AnzImKnotenAngelegterBindungen);                                          \
    }                                                                                                               \
    KnotenbindungenAufheben();                                                                                      \
  }                                                                                                                 \
}

static BOOLEAN DeltaGebundeneVar0(void)
{
  TermAnEineBVarGebundenOderUnifiziertUndAb(BK_minSymb(BaumIndex),DeltaGebundeneVar1);
  EinZielMitTVarUnifiziertUndAb(BK_Sprungausgaenge(BaumIndex),DeltaGebundeneVar2);
  return FALSE;
}

static BOOLEAN DeltaGebundeneVar1(void)
{
  TermAnEineBVarGebundenOderUnifiziertUndAb(BK_USymbol(BaumIndex),DeltaGebundeneVar1);
  EinZielMitTVarUnifiziertUndAb(BK_Sprungausgaenge(BaumIndex),DeltaGebundeneVar2);
  return FALSE;
}

static BOOLEAN DeltaGebundeneVar2(void)
{
  EinZielMitTVarUnifiziertUndAb(BK_UZiel(BaumIndex),DeltaGebundeneVar2);
  return FALSE;
}



/*=================================================================================================================*/
/*= VIII. Im Baum absteigen fuer SymbolAusTerm freie Variable                                                     =*/
/*=================================================================================================================*/

static BOOLEAN DeltaFreieVar1(void);
static BOOLEAN DeltaFreieVar2(void);

#define EineBVarAnTVarGebundenUndAb(/*SymbolT*/ ersteKandidatin, /*UDeltaT*/ naechstesDelta)                        \
{                                                                                                                   \
  SymbolT letzteKandidatin = min(SO_ErstesVarSymb,BK_maxSymb(BaumIndex)),                                           \
          BaumVar;                                                                                                  \
  SO_forSymboleAufwaerts(BaumVar,ersteKandidatin,letzteKandidatin) {                                                \
    if (BK_Nachf(BaumIndex,BaumVar) && SubstErgaenzt(SymbolAusTerm,TO_Variablenterm(BaumVar)))                      \
      AbstiegVerwalten(BK_Nachf(BaumIndex,BaumVar),Symbol,                                                          \
        SO_VorigesVarSymb(BaumVar),naechstesDelta,TO_Schwanz(aktTermrest),AnzImKnotenAngelegterBindungen);          \
  }                                                                                                                 \
}


#define EinZielAnTVarGebundenUndAb(/*SprungeintragsT*/ erstesZiel, /*UDeltaT*/ naechstesDelta)                      \
{                                                                                                                   \
  SprungeintragsT ZielIndex;                                                                                        \
  BK_forSprungziele(ZielIndex,erstesZiel)                                                                           \
    if (SubstErgaenzt(SymbolAusTerm,ZielIndex->Teilterm)) {                                                         \
      TermNachZiel = TO_NaechsterTeilterm(ZielIndex->Teilterm);                                                     \
      AbstiegMitZielErfolgt = TRUE;                                                                                 \
      AbstiegVerwalten(ZielIndex->Zielknoten,Ziel,                                                                  \
        ZielIndex->NaechsterZieleintrag,naechstesDelta,TO_Schwanz(aktTermrest),AnzImKnotenAngelegterBindungen);     \
    }                                                                                                               \
}


static BOOLEAN DeltaFreieVar0(void)
{
  EineBVarAnTVarGebundenUndAb(BK_minSymb(BaumIndex),DeltaFreieVar1);
  EinZielAnTVarGebundenUndAb(BK_Sprungausgaenge(BaumIndex),DeltaFreieVar2);
  return FALSE;
}

static BOOLEAN DeltaFreieVar1(void)
{
  EineBVarAnTVarGebundenUndAb(BK_USymbol(BaumIndex),DeltaFreieVar1);
  EinZielAnTVarGebundenUndAb(BK_Sprungausgaenge(BaumIndex),DeltaFreieVar2);
  return FALSE;
}

static BOOLEAN DeltaFreieVar2(void)
{
  EinZielAnTVarGebundenUndAb(BK_UZiel(BaumIndex),DeltaFreieVar2);
  return FALSE;
}



/*=================================================================================================================*/
/*= IX. Unifizieren eines Terms mit eigenen Teiltermen, mit DSBaum auf Toplevel, mit DSBaum-Teiltermen            =*/
/*=================================================================================================================*/

static BOOLEAN Delta0(void)                                                                           /* Verteiler */
{
  return
    SO_SymbIstFkt(SymbolAusTerm) ?
      DeltaFktSymbol0()
    :
      VarGebunden(SymbolAusTerm) ?
        DeltaGebundeneVar0()
      :
        DeltaFreieVar0();
}


#define /*void*/ EinenKnotenAufsteigen(/*void*/)                                                                    \
  if ((BaumIndex = BK_UVorg(BaumIndex))) {                                                                          \
    BindungenAufheben(BK_UAnzBindungen(BaumIndex));                                                                 \
    AnzImKnotenAngelegterBindungen = 0;                                                                             \
  }


#define /*void*/ AusBlattAufsteigen(/*void*/)                                                                       \
  BindungenAufheben(AnzImKnotenAngelegterBindungen);                                                                \
  EinenKnotenAufsteigen()


#define /*void*/ EinenKnotenAbOderVieleKnotenAufsteigen(/*void*/)                                                   \
{                                                                                                                   \
  AbstiegMitZielErfolgt = FALSE;                                                                                    \
  do                                                                                                                \
    if (BK_UDelta(BaumIndex))                                                                                       \
      break;                                                                                                        \
    else                                                                                                            \
      EinenKnotenAufsteigen()                                                                                       \
  while (BaumIndex);                                                                                                \
}


#define /*Block*/ RumpfTermMitDSBaumUnifizieren(/*UTermT*/ Term, /*DSBaumT*/ Baum, /*Block*/ Action,                \
                                                                                            /*BOOLEAN*/ MGUMitBlatt)\
{                                                                                                                   \
  if (Baum.Wurzel) {                                                                                                \
    BK_UDeltaSei(BaumIndex = Baum.Wurzel,Delta0);                                                                   \
    BK_aktTIndexUSei(BaumIndex,Term);                                                                               \
                                                                                                                    \
    EinenKnotenAbOderVieleKnotenAufsteigen();                                                                       \
    while (BaumIndex) {                                                                                             \
      if (BK_IstBlatt(BaumIndex)) {                                                                                 \
        if (MGUMitBlatt) {                                                                                          \
          Action;                                                                                                   \
        }                                                                                                           \
        AusBlattAufsteigen();                                                                                       \
      }                                                                                                             \
      EinenKnotenAbOderVieleKnotenAufsteigen();                                                                     \
    }                                                                                                               \
  }                                                                                                                 \
}


#define /*void*/ TermMitDSBaumUnifizieren(/*UTermT*/ Term, /*DSBaumT*/ Baum, /*Block*/ Action)                      \
  RumpfTermMitDSBaumUnifizieren(Term,Baum,Action,MGUMitBlattGefunden(Baum))


#define /*void*/ TermMitDSBaumUnifizierenAusschluss(/*UTermT*/ Term, /*DSBaumT*/ Baum, /*GNTermpaarT*/ Ausschluss,  \
                                                                                                   /*Block*/ Action)\
  RumpfTermMitDSBaumUnifizieren(Term,Baum,Action,MGUMitBlattGefundenAusschluss(Baum,Ausschluss))


#define /*void*/ TermMitDSBaumTeiltermenUnifizieren(/*TermT*/ Term, /*DSBaumT*/ Baum, /*Block*/ Action)             \
{                                                                                                /* furchtbar ... */\
  GNTermpaarT RegelIndex;                                                                                           \
  BK_forRegeln(RegelIndex,Baum, {                   /* Baum leer => Regelliste leer; daher for-Schleife angebracht*/\
    UTermT LinkeSeiteIndex = TO_Schwanz(RegelIndex->linkeSeite);                                                    \
    if(TP_lebtZumZeitpunkt_M(RegelIndex, HK_getAbsTime_M()) && !RegelIndex->DarfNichtUeberlappen){\
    while (LinkeSeiteIndex) {                                                  /* alle linken Seiten durchwandern */\
      if (SO_SymbIstFkt(TO_TopSymbol(LinkeSeiteIndex))) {                                                           \
        if (Unifiziert(Term,LinkeSeiteIndex)) {                              /* Wenn Unifikation erfolgreich war: */\
          Action;                                                                                                   \
        }                                                                                                           \
        KnotenbindungenAufheben();                                                                                  \
      }                                                                                                             \
      LinkeSeiteIndex = TO_Schwanz(LinkeSeiteIndex);                                                                \
    }                                                                                                               \
    }\
  })                                                                                                                \
}


#define /*void*/ Term1MitTerm2Unifizieren(/*TermT*/ Term1, /*TermT*/ Term2, /*Block*/ Action)                       \
{                                                                                                                   \
  if (Unifiziert(Term1,Term2))                                                                                      \
    Action;                                                                                                         \
  KnotenbindungenAufheben();                                                                                        \
}



/*=================================================================================================================*/
/*= X. Apparat fuer KP-Filter                                                                                     =*/
/*=================================================================================================================*/

static TermT KPLinks,
             KPRechts,
             KPRechtsInnen,
             Ueberlappung,
             UeberlappungInnen;


static void Nullen (void)
{
  KPLinks = NULL;
  KPRechts = KPRechtsInnen = NULL;
  Ueberlappung = UeberlappungInnen = NULL;
}
static void KPLinksProduzieren(RegelOderGleichungsT Vater MAYBE_UNUSEDPARAM,
			       UTermT VaterStelle MAYBE_UNUSEDPARAM, RegelOderGleichungsT Mutter MAYBE_UNUSEDPARAM)
{
  if (KPLinks == NULL){
    KPLinks = MGU(TP_RechteSeite(Vater));
  }
}
static void KPRechtsProduzieren(RegelOderGleichungsT Vater MAYBE_UNUSEDPARAM,
				UTermT VaterStelle MAYBE_UNUSEDPARAM, RegelOderGleichungsT Mutter MAYBE_UNUSEDPARAM)
{
  if (KPRechts == NULL){
    KPRechts = MGUNachTEMitSt(TP_LinkeSeite(Vater),VaterStelle,TP_RechteSeite(Mutter),&KPRechtsInnen);
  }
}

#define KPTermeProduzieren() (KPLinksProduzieren(Vater,VaterStelle,Mutter),KPRechtsProduzieren(Vater,VaterStelle,Mutter),0)

static void UeberlappungProduzieren(RegelOderGleichungsT Vater MAYBE_UNUSEDPARAM,
				    UTermT VaterStelle MAYBE_UNUSEDPARAM, RegelOderGleichungsT Mutter MAYBE_UNUSEDPARAM)
{
  if (Ueberlappung == NULL){
    Ueberlappung = MGUMitSt(TP_LinkeSeite(Vater),VaterStelle,&UeberlappungInnen);
  }
}

static void KPLinksVerwerfen(void)
{
  if (KPLinks != NULL){ 
    TO_TermLoeschen(KPLinks);
    IncAnzUnifUeberflKritTerme(); 
  }
}
static void KPRechtsVerwerfen(void)
{
  if (KPRechts != NULL){
    TO_TermLoeschen(KPRechts);
    IncAnzUnifUeberflKritTerme(); 
  }
}
static void UeberlappungVerwerfen(void)
{
  if (Ueberlappung != NULL){ 
    TO_TermLoeschen(Ueberlappung); 
  }
}

static void Verwerfen(void)
{
  KPLinksVerwerfen();
  KPRechtsVerwerfen();
  UeberlappungVerwerfen();
}


typedef BOOLEAN (*FilterT)(RegelOderGleichungsT Vater, UTermT si, RegelOderGleichungsT Mutter);

static FilterT *KPFilter;
static unsigned int AnzahlFilter;

static void KPFilterInitialisieren(void)
{
  AnzahlFilter = 0;
  KPFilter = array_alloc(AnzahlFilter+1,FilterT);
  KPFilter[AnzahlFilter] = NULL;
}

static void KPFilterErgaenzen(FilterT Filter)
{
  KPFilter[AnzahlFilter++] = Filter;
  array_realloc(KPFilter,AnzahlFilter+1,FilterT);
  KPFilter[AnzahlFilter] = NULL;
}

static BOOLEAN Weggefiltert(RegelOderGleichungsT Vater, UTermT VaterStelle, RegelOderGleichungsT Mutter)
{
  {
    FilterT *Filter = KPFilter;
    while (*Filter)
      if (!(*(Filter++))(Vater,VaterStelle,Mutter))
        return TRUE;
    return FALSE;
  }
}

/*=================================================================================================================*/
/*= XI. KP-Filter Trivialitaeten: darfUeberlappen & lebtNoch                                                      =*/
/*=================================================================================================================*/

static unsigned long num;
extern BOOLEAN HK_modulo;
extern int     HK_noOfSlaves;
extern int     HK_myId;

static BOOLEAN DarfUeberlappenUndLebtNochFilter(RegelOderGleichungsT Vater MAYBE_UNUSEDPARAM,
  UTermT VaterStelle MAYBE_UNUSEDPARAM, RegelOderGleichungsT Mutter MAYBE_UNUSEDPARAM)
{
#ifndef CCE_Source
  if (Vater->DarfNichtUeberlappen || Mutter->DarfNichtUeberlappen)
    return FALSE;
  /*sollten Vater oder Mutter schon nichtmehr aktiv sein, dh ->todesTag < aktuelle Zeit, auch wegfiltern*/
  if (!TP_lebtZumZeitpunkt_M(Vater, HK_getAbsTime_M()) || !TP_lebtZumZeitpunkt_M(Mutter, HK_getAbsTime_M())){
    if (IO_level4()) {    
      IO_DruckeFlex("Vater(ID %d) lebt jetzt nicht - gest.%ul  ||  Mutter(ID %d) lebt jetzt nicht - gest.%ul - Zeitpunkt %d\n", 
		    Vater->Nummer, Vater->todesTag, Mutter->Nummer, Mutter->todesTag, HK_getAbsTime_M());
    }
#if 0
      our_error("Fehler HS006");
#endif
    return FALSE;
  }
  if (!(Vater->lebtNoch && Mutter->lebtNoch)){
    if (IO_level4()) {    
      IO_DruckeFlex("Jemand inzwichen verstorben: Vater(ID %d) gest.%ul  ||  Mutter(ID %d) - gest.%ul - Zeitpunkt %d\n", 
		    Vater->Nummer, Vater->todesTag, Mutter->Nummer, Mutter->todesTag, HK_getAbsTime_M());
    }
    return FALSE;
  }

  if ((TP_IstACGrundreduzibel(Vater) || TP_IstACGrundreduzibel(Mutter)) ||
      (PA_doSK() && (Vater->StrengKonsistent || Mutter->StrengKonsistent))){
    if(0)printf("Wieder einer... %ld %ld\n", TP_ElternNr(Vater), TP_ElternNr(Mutter));
    return FALSE;
  }

  if (COMP_DIST){
    if (PA_Slave()){
      num++;
      if (HK_modulo && (num % (unsigned)HK_noOfSlaves != (unsigned)HK_myId)){
	if(0)printf("Slave %d (of %d) FALSE\n", HK_myId, HK_noOfSlaves);
	return FALSE;
      }
      if(0)printf("Slave %d (of %d) not FALSE\n", HK_myId, HK_noOfSlaves);
    }
    else if (MASTER_RECHNET_MIT && (HK_noOfSlaves > 0)){
      num++;
      if (HK_modulo && (num % (unsigned)HK_noOfSlaves != (unsigned)HK_noOfSlaves-1)){
	if(0)printf("Master %d (of %d) FALSE\n", HK_noOfSlaves-1, HK_noOfSlaves);
	return FALSE;
      }
    }
  }
#endif
  return TRUE;
}

/*=================================================================================================================*/
/*= XI. KP-Filter Ordnungstests bei Gleichungen                                                                   =*/
/*=================================================================================================================*/

/* NOCH einzuarbeiten: */
#if CO_DO_OVERLAP_CONSTRAINTS

#define /*BOOLEAN*/ TermGroesserUnif(/*UTermT*/ s, /*UTermT*/ t, /*UTermT*/ u, /*UTermT*/ v, /*{"RG","GG"}*/ Wo) \
  (CO_TermGroesserUnif(s,t,u,v) ? TRUE : ((void)IncAnzUnifMisserfolgeOrdnung##Wo(), FALSE))
#endif

static BOOLEAN VaterIstGleichung(RegelOderGleichungsT Vater MAYBE_UNUSEDPARAM,
  UTermT VaterStelle MAYBE_UNUSEDPARAM, RegelOderGleichungsT Mutter MAYBE_UNUSEDPARAM)
{
  if (RE_IstGleichung(Vater)){
    KPLinksProduzieren(Vater,VaterStelle,Mutter);
    UeberlappungProduzieren(Vater,VaterStelle,Mutter);
    if (ORD_TermGroesserUnif(KPLinks,Ueberlappung)){
      return FALSE;
    }
  }
  return TRUE;
}

static BOOLEAN MutterIstGleichung(RegelOderGleichungsT Vater MAYBE_UNUSEDPARAM,
  UTermT VaterStelle MAYBE_UNUSEDPARAM, RegelOderGleichungsT Mutter MAYBE_UNUSEDPARAM)
{
  if (RE_IstGleichung(Mutter)){
    KPRechtsProduzieren(Vater,VaterStelle,Mutter);
    UeberlappungProduzieren(Vater,VaterStelle,Mutter);
    if (ORD_TermGroesserUnif(KPRechtsInnen,UeberlappungInnen)){
      return FALSE;
    }
  }
  return TRUE;
}

/* Fuer Constraints ist ein spezieller VaterUndMutterGleichung-Filter moeglich, der
   nach der Existenz einer echten Grundspitze forscht.*/

/*=================================================================================================================*/
/*= XI. KP-Filter Verbunden-unterhalb                                                                             =*/
/*=================================================================================================================*/

static BOOLEAN Primueberlappung(RegelOderGleichungsT Vater MAYBE_UNUSEDPARAM,
  UTermT VaterStelle, RegelOderGleichungsT Mutter MAYBE_UNUSEDPARAM)
{
  UeberlappungProduzieren(Vater,VaterStelle,Mutter);
  if (NF_TermInnenReduzibel(PA_SubconR(), PA_SubconE(), UeberlappungInnen)) {
    IncAnzKapurOpfer();
    return FALSE;
  }
  else {
    return TRUE;
  }
}

static BOOLEAN ACreduzibel(RegelOderGleichungsT Vater MAYBE_UNUSEDPARAM,
  UTermT VaterStelle, RegelOderGleichungsT Mutter MAYBE_UNUSEDPARAM)
{
  /* NOTBEHELF fuer Vater,Mutter \notin ACC': */
  if ((Vater->laengeLinks > 5) && (Mutter->laengeLinks > 5)){ 
    UeberlappungProduzieren(Vater,VaterStelle,Mutter);
    if (AC_Reduzibel(Ueberlappung)) {
      IO_DruckeFlex("ACreduzibel: %t\n", Ueberlappung);
      IncAnzKapurOpfer();
      return FALSE;
    }
  }
  return TRUE;
}

static BOOLEAN MGUreduzibel(RegelOderGleichungsT Vater MAYBE_UNUSEDPARAM,
  UTermT VaterStelle MAYBE_UNUSEDPARAM, RegelOderGleichungsT Mutter MAYBE_UNUSEDPARAM) /* evtl. muss x mit sigma(x) reduzibel in CP vorkommen!! XXX */
{
  SymbolT VarIndex;
  BOOLEAN res;

  SO_forSymboleAbwaerts(VarIndex,SO_ErstesVarSymb,xUDimension)
    if (VarGebunden(VarIndex)) {
      TermT Substitut = MGU(SubstitutVon(VarIndex));
      res = NF_TermReduzibel(PA_SubconR(),PA_SubconE(),Substitut);
      if (0&&res){
	IO_DruckeFlex("x%d <- %t reduzibel\n",SO_VarNummer(VarIndex),Substitut);
      }
      TO_TermLoeschen(Substitut);
      if (res) return FALSE;
    }
  return TRUE;
}


/*=================================================================================================================*/
/*= XII. KP-Filter 1*                                                                                             =*/
/*=================================================================================================================*/

static BOOLEAN AnEinsSternIn(UTermT si, UTermT s)
{
  while (TO_PhysischUngleich(s,si))
    if (TO_Nullstellig(s))
      return FALSE;
    else
      s = TO_ErsterTeilterm(s);
  return TRUE;
}


static BOOLEAN EinsSternUeberlappung(RegelOderGleichungsT Vater, UTermT si, RegelOderGleichungsT Mutter)
{
#ifndef CCE_Source
  TermT s = TP_LinkeSeite(Vater);
  BOOLEAN Resultat =
    PM_Existenzziele() && TO_TopSymbol(s)==SO_EQ ?
      TO_TopSymbol(TP_LinkeSeite(Mutter))==SO_EQ ?
        TO_TopSymbol(TP_RechteSeite(Vater))==SO_TRUE || TO_TopSymbol(TP_RechteSeite(Mutter))==SO_TRUE
      :
        AnEinsSternIn(si,s=TO_ErsterTeilterm(s)) || AnEinsSternIn(si,s=TO_NaechsterTeilterm(s))
    :
      AnEinsSternIn(si,s); 
  if (!Resultat) {
    HK_SetUnfair();
    IncAnzNonEinsSternKPs();
  }
  return Resultat;
#else
  return TRUE;
#endif
}



/*=================================================================================================================*/
/*= XIIa. KP-Filter Nusfu                                                                                          =*/
/*=================================================================================================================*/

static BOOLEAN Nusfu(UTermT sj, UTermT s)
{
#ifndef CCE_Source
  UTermT NULLs = TO_NULLTerm(s);
  do {
    SymbolT f = TO_TopSymbol(s);
    if (SO_IstSkolemSymbol(f) && (!PM_Existenzziele || f<SO_EQ)) {
      UTermT NULLsi = TO_NULLTerm(s);
      while (TO_PhysischUngleich(s = TO_Schwanz(s),NULLsi))
        if (TO_PhysischGleich(s,sj))
          return FALSE;
    }
    else
      s = TO_Schwanz(s);
  }
  while (TO_PhysischUngleich(s,NULLs));
#endif
  return TRUE;
}


static BOOLEAN NusfUeberlappung(RegelOderGleichungsT Vater, UTermT si, RegelOderGleichungsT Mutter MAYBE_UNUSEDPARAM)
{
  TermT s = TP_LinkeSeite(Vater);
  BOOLEAN Resultat = Nusfu(si,s);
  /*  IO_DruckeFlex("Nusfu(%t,%t)=%s\n",si,s,Resultat?"TRUE":"FALSE");*/
  if (!Resultat)
    IncAnzNonNusfuKPs();
  return Resultat;
}



/*=================================================================================================================*/
/*= XIII. KP-Filter Kern                                                                                          =*/
/*=================================================================================================================*/

/* ------- 0. Rueckwaerts beweisen ------------------------------------------------------------------------------- */

static BOOLEAN IstVerdrehterKombinator(RegelOderGleichungsT Faktum)
{
  TermT s = TP_RechteSeite(Faktum),
        t = TP_LinkeSeite(Faktum);
  return TO_IstKombinatorTupel(s,t);
}


/* ------- 1. Stufe 1 -------------------------------------------------------------------------------------------- */

static BOOLEAN IstRechtsInZiel(UTermT ti, RegelOderGleichungsT Faktum)
{                                    /* keine Seitenrelaxierung erforderlich, da Bezug auf intern kreiertes Faktum */
#ifndef CCE_Source
  UTermT s, t;
  return RE_UngleichungEntmantelt(&s,&t, Faktum) && TO_PhysischTeilterm(ti,t);
#else
  return FALSE;
#endif
}


static BOOLEAN IstKombinatorSymbol(SymbolT f)
{                                                  /* Aufwendig, aber schick. Macht nichts, da nicht zeitkritisch. */
#ifndef CCE_Source
  if (SO_SymbIstKonst(f)) {
    ListenT Fakten;
    for (Fakten = PM_Gleichungen; Fakten; Fakten = cdr(Fakten)) {
      RegelOderGleichungsT Faktum = car(Fakten);
      TermT u = TP_LinkeSeite(Faktum),
            v = TP_RechteSeite(Faktum);
      UTermT s = TO_IstKombinatorTupel(u,v) ? u : TO_IstKombinatorTupel(v,u) ? v : NULL;
      if (s && TO_SymbolEnthalten(f,s))
        return TRUE;
    }
  }
#endif
  return FALSE;
}

#define for_KombinatorSymbole(/*SymbolT*/ Index)                                                                    \
  SO_forKonstSymbole(Index)                                                                                         \
    if (IstKombinatorSymbol(Index))


static BOOLEAN KernUebergehen(TermT K)
{     /* Grundkerne mit kleinen Quadraten am Ende uebergehen, da zu speziell (wenn ich Wos richtig verstanden habe */
  if (TO_TermIstNichtGrund(K) || SO_SymbUngleich(TO_TopSymbol(K),TO_ApplyOperator))
    return FALSE;
  else {
    TermT t = TO_NaechsterTeilterm(TO_ErsterTeilterm(K));
    unsigned int Kombinatorlaenge = 1+TO_Symbollaenge(TO_ApplyOperator,t);
    if (Kombinatorlaenge>4)
      return FALSE;
    else {
      UTermT s = TO_ErsterTeilterm(K);
      while (SO_SymbGleich(TO_TopSymbol(s),TO_ApplyOperator))
        if (TO_TermeGleich(s,t))
          return TRUE;
        else
          s = TO_NaechsterTeilterm(TO_ErsterTeilterm(s));
      return FALSE;
    }
  }
}


static void Erfolg1Testen(TermT l, TermT r)
{
#ifndef CCE_Source
  UTermT s, t;
  if (!KO_Kern && Z_UngleichungEntmantelt(&s,&t, l,r)) {
    if (U2_NeuUnifiziert(s,t)) {
      if (KO_Mehrstufig) {
        TermT K = U2_MGU(s);
        SymbolT C;
        for_KombinatorSymbole(C)
          if (!TO_SymbolEnthalten(C,K))
            return;
        if (!KernUebergehen(K)) {
          KO_KonsultationErforderlich = TRUE; 
          KO_Kern = K; 
        }
      }
      else
        KO_Stufe = 12;
    }
  }
#endif
}


/* ------- 2. Stufe 2 -------------------------------------------------------------------------------------------- */

static BOOLEAN IstLinksVariable(TermT l, TermT r)
{                                    /* keine Seitenrelaxierung erforderlich, da Bezug auf intern kreiertes Faktum */
#ifndef CCE_Source
  UTermT s, t;
  return Z_UngleichungEntmantelt(&s,&t, l,r) && TO_TermIstVar(s);
#else
  return FALSE;
#endif
}

static BOOLEAN IstFixedPositiv_(TermT s, TermT t)
{
  return SO_SymbGleich(TO_TopSymbol(s),SO_EQ) &&
    SO_SymbGleich(TO_TopSymbol(t),SO_TRUE) &&
    SO_SymbIstFkt(TO_TopSymbol(TO_ErsterTeilterm(s)));
}

static BOOLEAN IstFixedPositiv(RegelOderGleichungsT Faktum)
{
  return IstFixedPositiv_(TP_LinkeSeite(Faktum),TP_RechteSeite(Faktum)) ||
    IstFixedPositiv_(TP_RechteSeite(Faktum),TP_LinkeSeite(Faktum));
}

static void Erfolg2Testen(RegelOderGleichungsT Vater, RegelOderGleichungsT Mutter)
{
#ifndef CCE_Source
  BOOLEAN Resultat1 = RE_IstUngleichung(Vater) && IstFixedPositiv(Mutter),
          Resultat2 = !Resultat1 && RE_IstUngleichung(Mutter) && IstFixedPositiv(Vater),
          Resultat = Resultat1 || Resultat2;
  if (Resultat) {
    RegelOderGleichungsT FixedPos = Resultat1 ? Mutter : Vater;
    TermT l = TP_LinkeSeite(FixedPos), r = TP_LinkeSeite(FixedPos);
    UTermT t = SO_SymbGleich(TO_TopSymbol(l),SO_EQ) ? l : r;
    while (TO_TermIstNichtVar(t = TO_Schwanz(t)));
    KO_Fixpunktkombinator = MGU(t);
    /*IO_DruckeFlex("SFP %t\n",KO_Fixpunktkombinator);*/
    KO_KonsultationErforderlich = TRUE; 
  }
#endif
}


/* ------- 3. Stufe 3 -------------------------------------------------------------------------------------------- */

static BOOLEAN IstUnifizierbaresZiel(TermT l, TermT r)
{
#ifndef CCE_Source
  UTermT s, t;
  return Z_UngleichungEntmantelt(&s,&t, l,r) && U2_NeuUnifiziert(s,t);
#else
  return FALSE;
#endif
}


static BOOLEAN IstTrueGleichFalse(TermT l, TermT r)
{
#ifndef CCE_Source
  if (PM_Existenzziele()) {
    SymbolT f = TO_TopSymbol(l),
            g = TO_TopSymbol(r);
    return (f==SO_TRUE && g==SO_FALSE) || (f==SO_FALSE && g==SO_TRUE);
  }
  else
#endif
    return FALSE;
}

#define Erfolg4 IstTrueGleichFalse


/* ------- 4. Filter --------------------------------------------------------------------------------------------- */

static BOOLEAN KernUeberlappung(RegelOderGleichungsT Vater, UTermT VaterStelle, RegelOderGleichungsT Mutter)
{
#ifndef CCE_Source
  BOOLEAN Resultat = 

    KO_Stufe==1  ? ( IstVerdrehterKombinator(Mutter) ) && 
                   ( IstRechtsInZiel(VaterStelle,Vater) ) && 
                   ( KPTermeProduzieren(), Erfolg1Testen(KPLinks,KPRechts), TRUE ) 
    : 
    KO_Stufe==2  ? ( IstVerdrehterKombinator(Mutter) && 
                     IstRechtsInZiel(VaterStelle,Vater) && 
                     (KPTermeProduzieren(), IstLinksVariable(KPLinks,KPRechts)) ) ||
                   ( Erfolg2Testen(Vater,Mutter), FALSE )
    : 
    KO_Stufe==3  ? ( FALSE )
    : 
    KO_Stufe==4  ? ( KPTermeProduzieren(), IstUnifizierbaresZiel(KPLinks,KPRechts) ) || 
                   ( Erfolg4(KPLinks,KPRechts) )
    : 
    KO_Stufe==12 ? ( KPTermeProduzieren(), IstTrueGleichFalse(KPLinks,KPRechts) )
    :                
                   ( TRUE );

  HK_SetUnfair();

  return Resultat;
#else
  return FALSE;
#endif
}


#if 0
  {
    KPTermeProduzieren();
    IO_DruckeFlex("Mutter: %t = %t %s\n", TP_LinkeSeite(Mutter),TP_RechteSeite(Mutter), 
      IstVerdrehterKombinator(Mutter) ? "verdrehter Kombinator!" : "");
    IO_DruckeFlex("Vater:  %t = %t\n", TP_LinkeSeite(Vater),TP_RechteSeite(Vater));
    IO_DruckeFlex("VaterStelle: %t %s\n",VaterStelle,
      IstRechtsInZiel(VaterStelle,Vater) ? "rechts in Ziel!" : "");
    IO_DruckeFlex("Faktum: %t = %t --- %sunifizierbar, %sErfolg\n", KPLinks, KPRechts,
      IstUnifizierbar(KPLinks,KPRechts) ? "" : "nicht ", IstErfolg(KPLinks,KPRechts) ? "" : "kein ");
    IO_DruckeFlex("Relevantes Faktum? %s\n", Resultat ? "TRUE" : "FALSE");
  }
#endif



/*=================================================================================================================*/
/*= XIV. Filter fuer Rueckwaertsgang                                                                              =*/
/*=================================================================================================================*/

static BOOLEAN RichtigeZielseiteLinks;

static void RichtigeZielseiteSuchen(void)
{
#ifndef CCE_Source
  if (PM_Ziele) {
    ListenT Ziel = car(PM_Ziele);
    UTermT t = car(Ziel);
    while (SO_SymbGleich(TO_TopSymbol(t),TO_ApplyOperator))
      t = TO_Schwanz(t);
    RichtigeZielseiteLinks = SO_IstSkolemSymbol(TO_TopSymbol(t));
  }
  else
#endif
    RichtigeZielseiteLinks = FALSE;
}

static BOOLEAN IstInRichtigerZielseite(UTermT ti, RegelOderGleichungsT Faktum)
{
#ifndef CCE_Source
  UTermT s, t;
  return RE_UngleichungEntmantelt(&s,&t, Faktum) && TO_PhysischTeilterm(ti,RichtigeZielseiteLinks ? s : t);
#else
  return FALSE;
#endif
}


static BOOLEAN RueckwartigeUeberlappung(RegelOderGleichungsT Vater, UTermT VaterStelle, RegelOderGleichungsT Mutter)
{
#ifndef CCE_Source
  BOOLEAN Resultat = ( IstVerdrehterKombinator(Mutter) && IstInRichtigerZielseite(VaterStelle,Vater) ) || 
                     ( KPTermeProduzieren(), IstUnifizierbaresZiel(KPLinks,KPRechts) ) || 
                     ( IstTrueGleichFalse(KPLinks,KPRechts) );
  if (!Resultat){
    HK_SetUnfair();
  }
  return Resultat;
#else
  return FALSE;
#endif
}



/*=================================================================================================================*/
/*= XV. KP-Actions                                                                                                =*/
/*=================================================================================================================*/

/*   Die Vaterregel oder -gleichung habe die Gestalt l->r. Die Mutterregel oder -gleichung l'->r' gehe durch Va-
     riablenumbenennung aus l->r hervor, so dass die l->r und l'->r' variablendisjunkt sind. l an der Stelle
     p<>lambda und l' an der Stelle lambda seien mit MGU sigma unifizierbar. Dann ergibt sich die Divergenz
                                                  sigma(l)
                                                 /       \
                                          sigma(r)        sigma(l|p<-r')

     und damit das kritische Paar [sigma(r), sigma(l|p<-r')].
     Ist der Vater l->r eine Gleichung, so ist zusaetzlich sigma(l)!<sigma(r) zu ueberpruefen.
     Ist die Mutter l'->r' eine Gleichung, so ist sigma(l)!<sigma(l|p<-r') zu ueberpruefen.
     Der Ordnungstest ist in den Filterapparat verschoben worden. BL.
*/

/* ------- 1. Einmalige Benachrichtigungen ----------------------------------------------------------------------- */

static struct GNTermpaarTS Elter1KopieS;
static const RegelOderGleichungsT Elter1Kopie = &Elter1KopieS;
static RegelOderGleichungsT Elter1;


#define /*TermT*/ l                                                                                                 \
  TP_LinkeSeite(Elter1Kopie)

static void Elter1Anmelden(RegelOderGleichungsT elter1)
{
  Elter1 = elter1;
  TP_LinkeSeite(Elter1Kopie) = TO_TermkopieVarUmbenannt(TP_LinkeSeite(elter1),BO_NeuesteBaumvariable());
  TP_RechteSeite(Elter1Kopie) = TO_TermkopieVarUmbenannt(TP_RechteSeite(elter1),BO_NeuesteBaumvariable());
  Elter1Kopie->Nummer = elter1->Nummer;
  Elter1Kopie->lebtNoch = elter1->lebtNoch;
  Elter1Kopie->IstUngleichung = elter1->IstUngleichung; /* AAAAAARRRRGGGGHHH! */
  TP_setGeburtsTag_M(Elter1Kopie,TP_getGeburtsTag_M(elter1));
  TP_setTodesTag_M(Elter1Kopie,TP_getTodesTag_M(elter1));
/*   KPV_NewParent(elter1); */
}


static void Elter1Andersherum(void)
{
  TermT Merker = TP_LinkeSeite(Elter1Kopie);
  TP_LinkeSeite(Elter1Kopie) = TP_RechteSeite(Elter1Kopie);
  TP_RechteSeite(Elter1Kopie) = Merker;
  if (TP_Gegenrichtung(Elter1) != NULL){
    Elter1 = TP_Gegenrichtung(Elter1);
  }
}


static void Elter1Abmelden(RegelOderGleichungsT elter1)
{
  TO_TermeLoeschen(TP_LinkeSeite(Elter1Kopie),TP_RechteSeite(Elter1Kopie));
/*   KPV_FinishParent(elter1); */
}



/* ------- 2. KPActions ------------------------------------------------------------------------------------------ */

static BOOLEAN KPActionXX(RegelT Vater, UTermT VaterStelle, RegelT Mutter)
{
  Nullen();
  if (Weggefiltert(Vater,VaterStelle,Mutter)){
    Verwerfen();
    return FALSE; 
  }
  else { 
    KPLinksProduzieren(Vater,VaterStelle,Mutter); 
    KPRechtsProduzieren(Vater,VaterStelle,Mutter); 
    UeberlappungVerwerfen(); 
    return TRUE;
  }
}


/* ------- 3. Benachrichtigungen ueber Ueberlappungen ------------------------------------------------------------ */

#define SUE_CPGeneratedWithVater  GebildetesKPBehandelnMitMutter
#define SUE_CPGeneratedWithMutter GebildetesKPBehandelnMitVater               /* Bezeichnung in SUE kreuzweise */


#define MacheElterMit(/*{Vater; Mutter}*/ Elternart, /*Bezeichner*/ KPAction,                                       \
  /*RegelOderGleichungsT*/ Elter2, /*UTermT*/ VaterStelle)               \
{                                                                                                                   \
  if (KPAction) {                                                                                                   \
    SUE_CPGeneratedWith ## Elternart(Elter2,KPLinks,KPRechts,VaterStelle,Elter1,l);                                          \
  }                                                                                                                 \
}

#define MacheVaterMit(Elter2, VaterStelle)                             \
  MacheElterMit(Vater,KPActionXX(Elter1Kopie,VaterStelle,Elter2),Elter2,VaterStelle)

#define MacheMutterMit(Elter2, VaterStelle)                             \
  MacheElterMit(Mutter,KPActionXX(Elter2,VaterStelle,Elter1Kopie),Elter2,VaterStelle)


/* #define RVaterMitR(x,y,z)  MacheVaterMit (x,y) */
/* #define RVaterMitG(x,y,z)  MacheVaterMit (x,y) */
/* #define GVaterMitR(x,y,z)  MacheVaterMit (x,y) */
/* #define GVaterMitG(x,y,z)  MacheVaterMit (x,y) */
/* #define RMutterMitR(x,y,z) MacheMutterMit(x,y) */
/* #define RMutterMitG(x,y,z) MacheMutterMit(x,y) */
/* #define GMutterMitR(x,y,z) MacheMutterMit(x,y) */
/* #define GMutterMitG(x,y,z) MacheMutterMit(x,y) */

/* #define XVaterMitX(x,y)  MacheVaterMit (x,y) */
/* #define XMutterMitX(x,y) MacheMutterMit(x,y) */


/* ------- 4. Sonstiges ------------------------------------------------------------------------------------------ */

#define /*Schleife*/ MitAllen(/*Block*/ Action)                                                                     \
{ GleichungsT GleichungsIndex = BK_Regeln(BaumIndex);                                                               \
  do {                                                                                                              \
    Action;                                                                                                         \
  } while ((GleichungsIndex = GleichungsIndex->Nachf));                                                             \
}                                                                                                              
                                                                                                               
#define /*Schleife*/ MitAllenAusser(/*GleichungsT*/ Ausschluss, /*Block*/ Action)                                   \
{ GleichungsT GleichungsIndex = BK_Regeln(BaumIndex);                                                               \
  do                                                                                                                \
    if (GleichungsIndex!=Ausschluss && GleichungsIndex!=TP_Gegenrichtung(Ausschluss)) {                             \
      Action;                                                                                                       \
    }                                                                                                               \
  while ((GleichungsIndex = GleichungsIndex->Nachf));                                                               \
}



/*=================================================================================================================*/
/*= XVI. KP-Bildung zu neuer Regel                                                                                =*/
/*=================================================================================================================*/

static void U1_KPsBildenZuRegel(RegelT Regel)
{
/* Die neue Regel sei l->r; R sei die Menge der linken Seiten der alten Regeln.
   Ueberlappungen zwischen Termen und Termmengen sind so zu interpretieren,
   dass der jeweilige Term mit allen Mengenelementen zu unifizieren ist.
   Ist die Menge aber leer, so ist nichts zu tun.
   Ueberlappungen von Mengen A =? B sind ueber das Kreuzprodukt zu bilden,
   also alle Aufgaben a =? b zu betrachten mit a E A, b E B.
   Es sei TT(t):={ t/p : p E O(t)} die Menge aller Teilterme von t (=>t E TT(t)
   und eTT(t):=TT(t)\{t}; ferner seien TT und eTT auf Mengen von Termen fortgesetzt.
   E sei die Menge aller linken und rechten Seiten aller Gleichungen.

   Folgende Ueberlappungen sind zu bilden:
   (1) l     =? eTT(l)
   (2) TT(l) =? R
   (3) l     =? eTT(R)
   (4) TT(l) =? E
   (5) l     =? eTT(E) (jeweils mit >-Test)

   Sei R':=R U {l}. Die Unifikation wurde hier so realisiert, dass zum Aufrufzeitpunkt
   die neue Regel schon im Baum steht. Dies stellt die volle Reduktionskraft zur Verfuegung
   und erspart das Puffern der kritischen Paare. Der zu zahlende Preis besteht in einer
   Anzahl ueberfluessiger erfolgreicher Unifikationen, ist jedoch niedriger als der Gewinn.

   Daher werden die Ueberlappungen wie folgt gebildet:
   (1) TT(l) =? R
   (2) l     =? eTT(R')
   (3) TT(l) =? E
   (4) l     =? eTT(E) (jeweils mit >-Test)
   Dabei muss R aus R' rekonstruiert werden. Dazu erhaelt TermMitDSBaumUnifizieren ein
   Ausschlussobjekt als zusaetzlichen Parameter. */


  /* 1. Regel kopieren, Variablendisjunktheit herstellen */

  UTermT IndexLinks;
  Elter1Anmelden(Regel);
  if (!GZ_ZSFB_BEHALTEN || !Regel->Grundzusammenfuehrbar) {

  /* 2. linke Seite und alle Teilterme ausser Variablen mit Regeln und Gleichungen auf Top-Level unifizieren */

  IndexLinks = l;
  do
    if (TO_TermIstNichtVar(IndexLinks)) {
      IncAnzUnifVersRegelnToplevel();
      TermMitDSBaumUnifizierenAusschluss(IndexLinks,RE_Regelbaum,Regel,
        MacheVaterMit(BK_Regeln(BaumIndex),IndexLinks));
      IncAnzUnifVersGleichgnToplevel();
      TermMitDSBaumUnifizieren(IndexLinks,RE_Gleichungsbaum,
        MitAllen(MacheVaterMit(GleichungsIndex,IndexLinks)));
    }
  while ((IndexLinks = TO_Schwanz(IndexLinks)));


  /* 3. linke Seite mit allen echten Teiltermen aus Regel- und Gleichungsbaum ausser Variablen unifizieren */

  IncAnzUnifVersRegelnTeilterme();
  TermMitDSBaumTeiltermenUnifizieren(l,RE_Regelbaum,
    MacheMutterMit(RegelIndex,LinkeSeiteIndex));
  IncAnzUnifVersGleichgnTeilterme();
  TermMitDSBaumTeiltermenUnifizieren(l,RE_Gleichungsbaum,
    MacheMutterMit(RegelIndex,LinkeSeiteIndex));


  /* 4. Nicht mehr benoetigte Termkopien freigeben */
  }
  Elter1Abmelden(Regel);
}



/*=================================================================================================================*/
/*= XVII. KP-Bildung zu neuer Gleichung                                                                           =*/
/*=================================================================================================================*/

static void U1_KPsBildenZuGleichung(GleichungsT Gleichung)
/* Die neue Gleichung sei gegeben durch eine ausgezeichnete Richtung l->r sowie die Gegenrichtung r'->l'.
   Alle Bezeichnungen wie vor. Sei E':=E U {l->r} U {r'->l'}.
   Die Unifikation wurde hier realisiert unter der Voraussetzung, dass zum Aufrufzeitpunkt
   die neue Gleichung schon mit Hin- und Herrichtung im Baum steht.

       zu bildende  |Entf||       realisiert in       |       Abfolge im Programm     |   Abfolge im Programm
     Ueberlappungen |Mono||                           |       bei Stereogleichung     |    bei Monogleichung
   -----------------+----++---------------------------+-------------------------------+-----------------------------
1   l     =? eTT(l) |    || l     =? eTT(E') U eTT(R) |  A  TT(l) =? E U R            | TT(l) =? E U R
2   r     =? eTT(r) | ** || r     =? eTT(E') U eTT(R) |  B  l     =? eTT(E') U eTT(R) | l     =? eTT(E') U eTT(R)
3   l     =? eTT(r) |    || l     =? eTT(E') U eTT(R) |  C  l     =? r                |
4   r     =? eTT(l) | ** || r     =? eTT(E') U eTT(R) |  D  TT(r) =? E U R            |
5   l     =? r      | ** || l     =? r                |  E  r     =? eTT(E') U eTT(R) |
                    |    ||                           |                               |
6   TT(l) =? E      |    || TT(l) =? E U R            |                               |
7   TT(r) =? E      | ** || TT(r) =? E U R            |                               |
8   l     =? eTT(E) |    || l     =? eTT(E') U eTT(R) |                               |
9   r     =? eTT(E) | ** || r     =? eTT(E') U eTT(R) |                               |
                    |    ||                           |                               |
10  TT(l) =? R      |    || TT(l) =? E U R            |                               |
11  TT(r) =? R      | ** || TT(r) =? E U R            |                               |
12  l     =? eTT(R) |    || l     =? eTT(E') U eTT(R) |                               |
13  r     =? eTT(R) | ** || r     =? eTT(E') U eTT(R) |                               |

*/

{
  /* 0. Antigleichung herausfischen und Variablendisjunktheit herstellen */

  UTermT Index;
  Elter1Anmelden(Gleichung);
  if (!GZ_ZSFB_BEHALTEN || !Gleichung->Grundzusammenfuehrbar) {

  /* A.  TT(l) =? E U R                      (6, 10) */

  Index = l;
  do
    if (TO_TermIstNichtVar(Index)) {
      IncAnzUnifVersRegelnToplevel();
      TermMitDSBaumUnifizieren(Index,RE_Regelbaum,
        MacheVaterMit(BK_Regeln(BaumIndex),Index));
      IncAnzUnifVersGleichgnToplevel();
      TermMitDSBaumUnifizieren(Index,RE_Gleichungsbaum,
        MitAllenAusser(Gleichung,MacheVaterMit(GleichungsIndex,Index)))
    }
  while ((Index = TO_Schwanz(Index)));


  /* B.  l     =? eTT(E') U eTT(R)     (1, 3, 8, 12) */

  IncAnzUnifVersRegelnTeilterme();
  TermMitDSBaumTeiltermenUnifizieren(l,RE_Regelbaum,
    MacheMutterMit(RegelIndex,LinkeSeiteIndex));
  IncAnzUnifVersGleichgnTeilterme();
  TermMitDSBaumTeiltermenUnifizieren(l,RE_Gleichungsbaum,
    MacheMutterMit(RegelIndex,LinkeSeiteIndex));


  /* F.  l     =? l                                  */

  IncAnzUnifVersGleichgnEigeneTterme();
  Term1MitTerm2Unifizieren(l,TP_LinkeSeite(Gleichung),
    MacheVaterMit(Gleichung,l));


  /* C.  l     =? r                              (5)      Ab hier nur noch fuer Stereogleichung */

  Elter1Andersherum();                                         /* vertauscht in Elter1Kopie linke und rechte Seite */
  if (TP_IstKeineMonogleichung(Gleichung)) {
    IncAnzUnifVersGleichgnEigeneTterme();
    Term1MitTerm2Unifizieren(l,TP_LinkeSeite(Gleichung),
      MacheVaterMit(Gleichung,l));


  /* D.  TT(r) =? E U R                      (7, 11) */

    Index = l;
    do
      if (TO_TermIstNichtVar(Index)) {
        IncAnzUnifVersGleichgnToplevel();
        TermMitDSBaumUnifizieren(Index,RE_Gleichungsbaum,
          MitAllenAusser(Gleichung,MacheVaterMit(GleichungsIndex,Index)));
        IncAnzUnifVersRegelnToplevel();
        TermMitDSBaumUnifizieren(Index,RE_Regelbaum,
          MacheVaterMit(BK_Regeln(BaumIndex),Index));
      }
    while ((Index = TO_Schwanz(Index)));


  /* E.  r     =? eTT(E') U eTT(R)     (2, 4, 9, 13) */

    IncAnzUnifVersGleichgnTeilterme();
    TermMitDSBaumTeiltermenUnifizieren(l,RE_Gleichungsbaum,
      MacheMutterMit(RegelIndex,LinkeSeiteIndex));
    IncAnzUnifVersRegelnTeilterme();
    TermMitDSBaumTeiltermenUnifizieren(l,RE_Regelbaum,
      MacheMutterMit(RegelIndex,LinkeSeiteIndex));


  /* G.  r     =? r                                  */

    IncAnzUnifVersGleichgnEigeneTterme();
    Term1MitTerm2Unifizieren(l,TP_LinkeSeite(TP_Gegenrichtung(Gleichung)),
      MacheVaterMit(TP_Gegenrichtung(Gleichung),l));
  }


  /* 6. Kopie der Antigleichung wieder freigeben */

  }
  Elter1Abmelden(Gleichung);
}

#if 0
static void u1_KPsBildenZuFaktum(RegelOderGleichungsT Faktum)
{
  Elter1Anmelden(Faktum);
  AlleTTmitToplevelA(Faktum);
  ToplevelMitAllenETTA(Faktum);
  if(RE_IstGleichung(Faktum)){
    TermMitTerm();
    if (TP_IstKeineMonogleichung(Faktum)){
      Elter1Andersherum();
      TermMitTerm();
      AlleTTmitToplevelA(Faktum);
      ToplevelMitAllenETTA(Faktum);
      TermMitTerm();
    }
  }      
  Elter1Abmelden(Faktum);
}
#endif

static UTermT rekonstruierePos(UTermT li, UTermT lip, UTermT li_)
{
  UTermT res = li_;

  while (lip != li) {
    li = TO_Schwanz(li);
    res = TO_Schwanz(res);
  }

  return res;
}

/* Bildet alle AC-kritischen Paare zu diesem Faktum,
 * auf der linken Seite (rechts = FALSE),
 * oder auf der rechten Seite (rechts = TRUE). */
static void u1_ACKPsBildenZuFaktum_intern(BOOLEAN rechts,
                                          RegelOderGleichungsT Faktum)
{
  UTermT tt1, tt2, t, tp, t_, tp_, t0;
  UTermT faktumLinkeSeite, faktumRechteSeite;
  unsigned pos;
  BOOLEAN rechtsbit = rechts || TP_RichtungUnausgezeichnet(Faktum);

  faktumLinkeSeite = Faktum->linkeSeite;
  faktumRechteSeite = Faktum->rechteSeite;
  t = (rechts) ? faktumRechteSeite : faktumLinkeSeite;
  t0 = TO_NULLTerm(t);
  pos = 0;
  for (tp = t ; tp != t0 ; tp = TO_Schwanz(tp), pos++) {
    if (MM_ACC_aktiviert(TO_TopSymbol(tp))) {
      tt1 = TO_ErsterTeilterm(tp);
      tt2 = TO_ZweiterTeilterm(tp);
      if (TO_TopSymbol(tt2) == TO_TopSymbol(tp)) { /* C_ gefunden */
        UTermT ersterTeiltermVontt2;
        ersterTeiltermVontt2 = TO_ErsterTeilterm(tt2);
        if (!ORD_TermGroesserUnif(ersterTeiltermVontt2, tt1)) {
          t_ = TO_Termkopie(t);
          tp_ = rekonstruierePos(t, tp, t_);
          /* tp_ == +(tt1,+(ersterTeiltermVontt2, zweiterTeiltermVontt2)), 
           * deshalb ist der Vorgaenger von tt1 tp_ 
           * und der Vorgaenger von ersterTeiltermVontt2 ist
           * TO_ZweiterTeilterm(tp_) == +(ersterTeiltermVontt2, 
           * zweiterTeiltermVontt2) */
          TO_TeiltermeTauschen(tp_, TO_ZweiterTeilterm(tp_));
          if(0){
            IO_DruckeFlex("\n### C_-Ueberlappung: %t = %t %s\n"
                          "### aus              %t = %t\n"
                          "### an Stelle %t (p = %p)\n\n",
                          rechts ? faktumLinkeSeite : t_, 
                          rechts ? t_ : faktumRechteSeite,
                          rechts ? "rechts" : "links",
                          faktumLinkeSeite, faktumRechteSeite, 
                          tp, rechts ? rechteSeite : linkeSeite, t, tp);
            IO_DruckeFlex("### lhs = %t, rhs = %t, i = %d, j = %d, xp = %z\n",
                          rechts ? t_ : TO_Termkopie(faktumRechteSeite) /*lhs*/,
                       rechts ? TO_Termkopie(faktumLinkeSeite) : t_  /*rhs*/, 
                       TP_getGeburtsTag_M(Faktum), /* i (juengeres Faktum) */ 
                       /* j aelteres Faktum, eigentlich C_ 
                        * aber neuerdings immer 0 (steht fuer AC-Ueberlapp) */
                       0,
                       /* Position, into_i, i_right, j_right */
                       POS_mk_pos(pos, 1, rechtsbit, 0));
          }
          /* Das KP muss vertauscht eingetragen werden.
           * Bsp: Es wurde links in l = r ueberlappt zu l' = Kopie(r).
           * Eingetragen wird lhs: Kopie(r), rhs: l' */
          KPV_insertCP(rechts ? t_ : TO_Termkopie(faktumRechteSeite) /*lhs*/,
                       rechts ? TO_Termkopie(faktumLinkeSeite) : t_  /*rhs*/, 
                       TP_getGeburtsTag_M(Faktum), /* i (juengeres Faktum) */ 
                       /* j aelteres Faktum, eigentlich C_ 
                        * aber neuerdings immer 0 (steht fuer AC-Ueberlapp) */
                       0,
                       /* Position, into_i, i_right, j_right */
                       POS_mk_pos(pos, 1, rechtsbit, 0));
        }
      }
      else {
        /* top(tp) element AC _und_ top(tt2) =/= top(tp) (also normales C) */
        if (!ORD_TermGroesserUnif(tt2, tt1)) {
          t_ = TO_Termkopie(t);
          tp_ = rekonstruierePos(t, tp, t_);      
          /* tp_ == +(tt1,tt2), deshalb ist der 
           * Vorgaenger von tt1 tp_ und der Vorgaenger von tt2
           * die letzte Zelle von tt1 */
          TO_TeiltermeTauschen(tp_, TO_TermEnde(TO_ErsterTeilterm(tp_)));
          if(0){
            IO_DruckeFlex("\n### C-Ueberlappung: %t = %t %s\n"
                          "### aus             %t = %t\n"
                          "### an Stelle %t (p = %p)\n\n",
                          rechts ? faktumLinkeSeite : t_,
                          rechts ? t_ : faktumRechteSeite,
                          rechts ? "rechts" : "links",
                          faktumLinkeSeite, faktumRechteSeite, 
                          tp, rechts ? rechteSeite : linkeSeite, t, tp);
            IO_DruckeFlex("### lhs = %t, rhs = %t, i = %d, j = %d, xp = %z\n",
                          rechts ? t_ : TO_Termkopie(faktumRechteSeite) /*lhs*/,
                       rechts ? TO_Termkopie(faktumLinkeSeite) : t_  /*rhs*/, 
                       TP_getGeburtsTag_M(Faktum), /* i (juengeres Faktum) */
                       /* j aelteres Faktum, eigentlich C_ 
                        * aber neuerdings immer 0 (steht fuer AC-Ueberlapp) */
                       0,
                       POS_mk_pos(pos, 1, rechtsbit, 0));
          }
          /* Das KP muss vertauscht eingetragen werden.
           * Bsp: Es wurde links in l = r ueberlappt zu l' = Kopie(r)
           * Eingetragen wird lhs: Kopie(r), rhs: l' */
          KPV_insertCP(rechts ? t_ : TO_Termkopie(faktumRechteSeite) /*lhs*/,
                       rechts ? TO_Termkopie(faktumLinkeSeite) : t_  /*rhs*/, 
                       TP_getGeburtsTag_M(Faktum), /* i (juengeres Faktum) */
                       /* j aelteres Faktum, eigentlich C_ 
                        * aber neuerdings immer 0 (steht fuer AC-Ueberlapp) */
                       0,
                       POS_mk_pos(pos, 1, rechtsbit, 0));
        }
      }
    }
  }
}

static void u1_ACKPsBildenZuFaktum_links(RegelOderGleichungsT Faktum)
{
  u1_ACKPsBildenZuFaktum_intern(FALSE /*links*/, Faktum);
}

static void u1_ACKPsBildenZuFaktum_rechts(RegelOderGleichungsT Faktum)
{
  if (TP_IstACGrundreduzibel(Faktum) && PA_acgRechts()){
    u1_ACKPsBildenZuFaktum_intern(TRUE /*rechts*/, Faktum);
  }
  else if (Faktum->StrengKonsistent && PA_skRechts()){
    u1_ACKPsBildenZuFaktum_intern(TRUE /*rechts*/, Faktum);
  }
}

static void u1_ACKPsBildenZuFaktum(RegelOderGleichungsT Faktum)
{
  u1_ACKPsBildenZuFaktum_links(Faktum);

  if (RE_IstRegel(Faktum)){
    u1_ACKPsBildenZuFaktum_rechts(Faktum);
  } 
  else if (TP_IstKeineMonogleichung(Faktum)){
    u1_ACKPsBildenZuFaktum_links(TP_Gegenrichtung(Faktum));
  }
}


void U1_KPsBildenZuFaktum(RegelOderGleichungsT Faktum)
{
#if IF_TERMZELLEN_JAGD
  IO_DruckeFlexStrom(stderr, "Start U1_KPsBildenZuFaktum: %d %t %t (%d / %d) \n", 
                                        Faktum->Nummer, Faktum->linkeSeite, Faktum->rechteSeite, TO_FreispeicherManager.Anforderungen,SV_AnzNochBelegterElemente(TO_FreispeicherManager));
#endif
  if (TP_IstACGrundreduzibel(Faktum)){
    HK_SetUnfair(); /* we are not yet absolutely sure about the implementation ... */
    u1_ACKPsBildenZuFaktum(Faktum);
  }
  else if (Faktum->Grundzusammenfuehrbar){
    HK_SetUnfair(); /* we are not yet absolutely sure about the implementation ... */
    return;
  }
  else if (PA_doSK()&&(Faktum->StrengKonsistent)){
    HK_SetUnfair(); /* we are not yet absolutely sure about the implementation ... */
    if(!COMP_COMP && !COMP_POWER)printf("Achtung streng! %ld\n", Faktum->Nummer);
    u1_ACKPsBildenZuFaktum(Faktum);
  }
  else if (RE_IstRegel(Faktum)){
    U1_KPsBildenZuRegel(Faktum);
  }
  else {
    U1_KPsBildenZuGleichung(Faktum);
  }
#if IF_TERMZELLEN_JAGD
  IO_DruckeFlexStrom(stderr, "Stop U1_KPsBildenZuFaktum: %d %t %t (%d / %d) \n", 
                                        Faktum->Nummer, Faktum->linkeSeite, Faktum->rechteSeite, TO_FreispeicherManager.Anforderungen,SV_AnzNochBelegterElemente(TO_FreispeicherManager));
#endif
}

/*=================================================================================================================*/
/*=================================================================================================================*/
/* Fuer Expansion von p-Entries */
/*=================================================================================================================*/
/*=================================================================================================================*/

static void u1_inLinkeSeite(RegelOderGleichungsT Faktum2)
{
  UTermT links = TP_LinkeSeite(Elter1Kopie);
#if 1
  /* toplevel einsparen: -- Sinnvoll ?? */
  if ((Elter1 == Faktum2) && RE_IstRegel(Faktum2)){
    links = TO_Schwanz(links);
  }
#endif
  while (links != NULL){
    if (TO_TermIstNichtVar(links)){
      Term1MitTerm2Unifizieren(links, TP_LinkeSeite(Faktum2), MacheVaterMit(Faktum2,links));
    }
    links = TO_Schwanz(links);
  } 
}

static void u1_mitLinkerSeite(RegelOderGleichungsT Faktum2)
{
  if ((Elter1== Faktum2) /*&& RE_IstRegel(Faktum2)*/){
    return;
  }
  else {
  UTermT links = TO_Schwanz(TP_LinkeSeite(Faktum2)); /* keine toplevel doublette */
  while (links != NULL){
    if (TO_TermIstNichtVar(links)){
      Term1MitTerm2Unifizieren(links, TP_LinkeSeite(Elter1Kopie), MacheMutterMit(Faktum2,links));
    }
    links = TO_Schwanz(links);
  }
  }
}

static void u1_bildeKPsMitLinkerSeite(RegelOderGleichungsT Faktum2)
{
  u1_inLinkeSeite(Faktum2);
  u1_mitLinkerSeite(Faktum2);
  if (RE_IstGleichung(Faktum2) && TP_IstKeineMonogleichung(Faktum2)){
    u1_inLinkeSeite(TP_Gegenrichtung(Faktum2));
    u1_mitLinkerSeite(TP_Gegenrichtung(Faktum2));
  }
}

void U1_KPsBildenZuZweiFakten(RegelOderGleichungsT Faktum1, RegelOderGleichungsT Faktum2)
{
  if (Faktum1->Grundzusammenfuehrbar || Faktum2->Grundzusammenfuehrbar){
    return;
  }
  /* ACG dito abfangen und ggf. entsprechend verzweigen */
  Elter1Anmelden(Faktum1);
  u1_bildeKPsMitLinkerSeite(Faktum2);
  if (RE_IstGleichung(Faktum1) && TP_IstKeineMonogleichung(Faktum1)){
    Elter1Andersherum();
    u1_bildeKPsMitLinkerSeite(Faktum2);
  }
  Elter1Abmelden(Faktum1);
}

/*=================================================================================================================*/
/*= XVIII. Rekonstruktion von kritischen Paaren                                                                   =*/
/*=================================================================================================================*/

static void SubstErgaenzenOhneOC(SymbolT Variable, UTermT Term)
{                                                            /* fuer sowieso unifizierbare Terme wie SubstErgaenzt */
  
  SymbolT Topsymbol = TO_TopSymbol(Term);                                 /* triviale Bindung x(i)<-x(i) abfangen; */
  Entbinden(Topsymbol,Term);
  if (SO_SymbUngleich(Topsymbol,Variable)) {
    xU[Variable].Substitut = Term;
    PushVariable(Variable);
    AnzImKnotenAngelegterBindungen++;
  }
}


static void Reunifizieren(UTermT Term1, UTermT Term2)
{
  UTermT Term1Ende = TO_TermEnde(Term1),
         Term2Ende = TO_TermEnde(Term2);
  Endestapel12Initialisieren();
  do {
    SymbolT Symbol1 = TO_TopSymbol(Term1),
            Symbol2 = TO_TopSymbol(Term2);
  Start:
    if (SO_SymbUngleich(Symbol1,Symbol2)) {
      if (SO_SymbIstVar(Symbol1)) {
        if (VarGebunden(Symbol1)) {
          AufSubstitut(1);
          goto Start;
        }
        else {
          SubstErgaenzenOhneOC(Symbol1,Term2);
          AufTeiltermende(2);
        }
      }
      else                                                                           /* Symbol2 muss Variable sein */
        if (VarGebunden(Symbol2)) {
          AufSubstitut(2);
          goto Start;
        }
        else {
          SubstErgaenzenOhneOC(Symbol2,Term1);
          AufTeiltermende(1);
        }
    }
  } while (DoppeltWeitergeschaltet12(&Term1,&Term1Ende,&Term2,&Term2Ende));
}


void U1_KPRekonstruieren(RegelOderGleichungsT Vater, unsigned int Stellennummer, RegelOderGleichungsT Mutter,
/* Die Stellennummer wird ab Null gezaehlt. */                                       TermT *KPLinks, TermT *KPRechts)
{
  UTermT Stelle = TO_TermAnStelle(TP_LinkeSeite(Vater),Stellennummer);
#if IF_TERMZELLEN_JAGD
  IO_DruckeFlexStrom(stderr, "Start U1_KPRekonstruieren: %d %d  (%d / %d) \n", 
                                        Vater->Nummer, Mutter->Nummer, TO_FreispeicherManager.Anforderungen,SV_AnzNochBelegterElemente(TO_FreispeicherManager));
#endif

  if (Vater==Mutter) {                                          /* dann kann Mutter nicht variablenumbenannt werden */
    if (TP_Gegenrichtung(Vater)) {
      GleichungsT Antigleichung = TP_Gegenrichtung(Vater);
      RE_FaktumExternalisieren(Antigleichung);
      Reunifizieren(Stelle,TP_RechteSeite(Antigleichung));
      *KPRechts = MGUNachTE(TP_LinkeSeite(Vater),Stelle,TP_LinkeSeite(Antigleichung));
      *KPLinks = MGU(TP_RechteSeite(Vater));
      RE_FaktumReinternalisieren(Antigleichung);
    }
    else {
      TermT MutterLinks = TO_TermkopieVarUmbenannt(TP_LinkeSeite(Mutter),BO_NeuesteBaumvariable()),
            MutterRechts = TO_TermkopieVarUmbenannt(TP_RechteSeite(Mutter),BO_NeuesteBaumvariable());
      Reunifizieren(Stelle,MutterLinks);
      *KPRechts = MGUNachTE(TP_LinkeSeite(Vater),Stelle,MutterRechts);
      *KPLinks = MGU(TP_RechteSeite(Vater));
      TO_TermeLoeschen(MutterLinks,MutterRechts);
    }
  }
  else {
    RE_FaktumExternalisieren(Mutter);
    Reunifizieren(Stelle,TP_LinkeSeite(Mutter));
    *KPRechts = MGUNachTE(TP_LinkeSeite(Vater),Stelle,TP_RechteSeite(Mutter));
    *KPLinks = MGU(TP_RechteSeite(Vater));
    RE_FaktumReinternalisieren(Mutter);
  }
  KnotenbindungenAufheben();
#if IF_TERMZELLEN_JAGD
  IO_DruckeFlexStrom(stderr, "StopU1_KPRekonstruieren: %d %d  (%d / %d) -> %t = %t\n", 
                     Vater->Nummer, Mutter->Nummer, TO_FreispeicherManager.Anforderungen,
                     SV_AnzNochBelegterElemente(TO_FreispeicherManager),
                                        *KPLinks, *KPRechts);
#endif
}

void U1_KPAusTermenRekonstruieren(TermT VaterLinks, TermT VaterRechts, TermT MutterLinks, TermT MutterRechts, 
                                  unsigned int Stellennummer, TermT *KPLinks, TermT *KPRechts)
/* spezielle Fassung von U1_KPRekonstruieren ohne Termpaare fuer Bernd. */
{
  UTermT Stelle = TO_TermAnStelle(VaterLinks,Stellennummer);
  
  if ((VaterLinks == MutterLinks) || (VaterLinks == MutterRechts)){/* CP mit sich selber ? */
    MutterLinks  = TO_TermkopieVarUmbenannt(MutterLinks,  BO_NeuesteBaumvariable());
    MutterRechts = TO_TermkopieVarUmbenannt(MutterRechts, BO_NeuesteBaumvariable());
    Reunifizieren(Stelle, MutterLinks);
    *KPRechts = MGUNachTE(VaterLinks,Stelle,MutterRechts);
    *KPLinks  = MGU(VaterRechts);
    TO_TermeLoeschen(MutterLinks,MutterRechts);
  }
  else{
    signed int Delta = SO_VarNummer(BO_NeuesteBaumvariable());
    TO_VariablenUmbenennen(MutterLinks,Delta);
    TO_VariablenUmbenennen(MutterRechts,Delta);
    Reunifizieren(Stelle, MutterLinks);
    *KPRechts = MGUNachTE(VaterLinks,Stelle,MutterRechts);
    *KPLinks  = MGU(VaterRechts);
    TO_VariablenUmbenennen(MutterLinks,-Delta);
    TO_VariablenUmbenennen(MutterRechts,-Delta);
  }
  KnotenbindungenAufheben();
}

void U1_CC_KPRekonstruieren(RegelOderGleichungsT vater, Position pos, BOOLEAN rechts,
                            TermT *KPLinks, TermT *KPRechts, RegelOderGleichungsT* ACGleichung)
/* Rekonstruiert das CC_KP mit vater.
   rechts gibt an, ob rechts (TRUE) oder links (FALSE) ueberlappt werden soll.
   rechts ist nur TRUE, fals vater eine Regel ist.
   Achtung: wg. PCL/Emulation als CPs werden die Seiten gedreht, falls rechts != TRUE.
*/
{
  TermT opferAtPos;
  unsigned i;

  if (rechts){
    *KPLinks = TO_Termkopie(vater->linkeSeite);
    *KPRechts = TO_Termkopie(vater->rechteSeite);
    opferAtPos = *KPRechts;
  }
  else {
    *KPLinks = TO_Termkopie(vater->rechteSeite);
    *KPRechts = TO_Termkopie(vater->linkeSeite);
    opferAtPos = *KPRechts;
  }
  
  for (i = 0; i < POS_pos(pos); i++){
    opferAtPos = TO_Schwanz(opferAtPos);
  }

  if (MM_ACC_aktiviert(TO_TopSymbol(opferAtPos))){
    TermT tt1, tt2;
    tt1 = TO_ErsterTeilterm(opferAtPos);
    tt2 = TO_ZweiterTeilterm(opferAtPos);
    
    if (TO_TopSymbol(tt2) == TO_TopSymbol(opferAtPos)) { /* C_ gefunden */
      /* opferAtPos == +(tt1,+(ersterTeiltermVontt2, zweiterTeiltermVontt2)), 
       * deshalb ist der Vorgaenger von tt1 opferAtPos
       * und der Vorgaenger von ersterTeiltermVontt2 ist
       * TO_ZweiterTeilterm(opferAtPos) == +(ersterTeiltermVontt2, 
       * zweiterTeiltermVontt2) */
      TO_TeiltermeTauschen(opferAtPos, TO_ZweiterTeilterm(opferAtPos));
      *ACGleichung = MM_C_Gleichung(TO_TopSymbol(opferAtPos));
    }
    else {
      /* top(opferAtPos) element AC 
         _und_ top(tt2) =/= top(opferAtPos) (also normales C) */
      TO_TeiltermeTauschen(opferAtPos,
                           TO_TermEnde(TO_ErsterTeilterm(opferAtPos)));
      *ACGleichung = MM_CGleichung(TO_TopSymbol(opferAtPos));
    }
    if(0)IO_DruckeFlex("links = %t, rechts = %t, pos = %z, Stelle = %t rechts = %b...\n",
                  *KPLinks, *KPRechts, pos, opferAtPos, rechts); 
  }
  else {
    IO_DruckeFlex("links = %t, rechts = %t, pos = %z, Stelle = %t rechts = %b...\n",
                  *KPLinks, *KPRechts, pos, opferAtPos, rechts); 
    our_error("U1_CC_KPRekonstruieren: die Position war kein AC-Symbol!\n");
  }
}

/*=================================================================================================================*/
/*= XIX. Mass fuer relative Unifizierbarkeit                                                                      =*/
/*=================================================================================================================*/

static void Durchsubstituieren(SymbolT VarSymbol, UTermT SubstitutOriginal,
                               TermT *Start, UTermT *Vorg, UTermT *Stelle)
/* In der Termzellenliste *Start bis NULL wird jedes Vorkommen der 
   Variablen VarSymbol ersetzt durch eine Kopie von SubstitutOriginal.
   Falls es Ersetzungen gibt an den Stellen *Start, *Vorg, *Stelle,
   so werden die entsprechend bezeichneten Variablen ebenfalls veraendert.
   Die Termzellenliste darf auch schon leer sein.
   Die Ende-Zeiger der vorhergehenden Zellen ab TermAnfang werden korrekt angepasst. */
{
  UTermT TermIndexVorg = NULL,
         TermIndex = *Start;
  while (TermIndex) {
    if (SO_SymbGleich(TO_TopSymbol(TermIndex),VarSymbol)) {
      UTermT Substitut = TO_Termkopie(SubstitutOriginal),
             SubstitutEnde = TO_TermEnde(Substitut),
             TermIndex2 = *Start;
      while (TO_PhysischUngleich(TermIndex2,TermIndex)) {                    /* vorhergehende Ende-Zeiger anpassen */
        if (TO_PhysischGleich(TO_TermEnde(TermIndex2),TermIndex))
          TO_EndeBesetzen(TermIndex2,SubstitutEnde);
        TermIndex2 = TO_Schwanz(TermIndex2);
      }
      TO_NachfBesetzen(SubstitutEnde,TO_Schwanz(TermIndex));                        /* Nachf-Verzeigerung anpassen */
      if (TermIndexVorg)
        TO_NachfBesetzen(TermIndexVorg,Substitut);
      else
        *Start = Substitut;                                                           /* ggf. *Start aktualisieren */
      if (TO_PhysischGleich(TermIndex,*Vorg))
        *Vorg = SubstitutEnde;
      if (TO_PhysischGleich(TermIndex,*Stelle))
        *Stelle = Substitut;
      TO_TermzelleLoeschen(TermIndex);
      TermIndex = SubstitutEnde;
    }
    TermIndex = TO_Schwanz(TermIndexVorg = TermIndex);
  }
}


static void ZelleSchneiden(TermT *Start, UTermT *Vorg, UTermT *Term)
{
  TermT Nachf = TO_NaechsterTeilterm(*Term);
  if (*Vorg)
    TO_NachfBesetzen(*Vorg,TO_Schwanz(*Term));
  else
    *Start = TO_Schwanz(*Term);
  if (TO_NichtNullstellig(*Term))
    *Vorg = TO_TermEnde(*Term);
  TO_TermzelleLoeschen(*Term);
  *Term = Nachf;
}


static void TermSchneiden(TermT *Start, UTermT Vorg, UTermT *Term)
{
  TermT Nachf = TO_NaechsterTeilterm(*Term);
  if (Vorg)
    TO_NachfBesetzen(Vorg,Nachf);
  else
    *Start = Nachf;
  TO_TermLoeschen(*Term);
  *Term = Nachf;
}


static void VariableBinden(TermT *VarTermStart, UTermT *VarTermVorg, UTermT *VarTerm, 
                           TermT *SubstTermStart, UTermT *SubstTermVorg, UTermT *SubstTerm,
                           long unsigned int d, long unsigned int *Mass)
{
  UTermT TermIndex = TO_Schwanz(*SubstTerm),
         NULLTerm = TO_NULLTerm(*SubstTerm);
  BOOLEAN ok = TRUE;
  while (TO_PhysischUngleich(TermIndex,NULLTerm)) {                                                 /* occur check */
    if (TO_TopSymboleGleich(*VarTerm,TermIndex)) {
      *Mass += 2*d;
      ok = FALSE;
    }
    TermIndex = TO_Schwanz(TermIndex);
  }
  if (ok) {
    SymbolT VarSymbol = TO_TopSymbol(*VarTerm);
    TermT Substitut = *SubstTerm;
    TermSchneiden(VarTermStart,*VarTermVorg,VarTerm);
    Durchsubstituieren(VarSymbol,Substitut,VarTermStart,VarTermVorg,VarTerm);
    Durchsubstituieren(VarSymbol,Substitut,SubstTermStart,SubstTermVorg,SubstTerm);
  }
  else
    TermSchneiden(VarTermStart,*VarTermVorg,VarTerm);
  TermSchneiden(SubstTermStart,*SubstTermVorg,SubstTerm);
}


unsigned long int U1_Unifikationsmass(TermT links, TermT rechts)
{                         /* links und rechts muessen vom Typ TermT sein. Rueckgabe 0 <=> Terme sind unifizierbar. */
  unsigned long int TiefeLinks = TO_Termtiefe(links),
                    TiefeRechts = TO_Termtiefe(rechts),
                    d = max(TiefeLinks,TiefeRechts),
                    Mass = 0;
  TermT linksStart = TO_Termkopie(links),
        rechtsStart = TO_Termkopie(rechts);
  do {
    UTermT linksVorg = NULL,
           rechtsVorg = NULL;
    links = linksStart;
    rechts = rechtsStart;
    do {
      if (TO_TopSymboleGleich(links,rechts)) {
        ZelleSchneiden(&linksStart,&linksVorg,&links);
        ZelleSchneiden(&rechtsStart,&rechtsVorg,&rechts);
      }
      else if (TO_TermIstVar(links))
        VariableBinden(&linksStart,&linksVorg,&links,
                       &rechtsStart,&rechtsVorg,&rechts,
                       d,&Mass);
      else if (TO_TermIstVar(rechts))
        VariableBinden(&rechtsStart,&rechtsVorg,&rechts,
                       &linksStart,&linksVorg,&links,
                       d,&Mass);
      else {
        Mass += d;
        TermSchneiden(&linksStart,linksVorg,&links);
        TermSchneiden(&rechtsStart,rechtsVorg,&rechts);
      }
    } while (links);
   if (!(--d))
     d = 1;
  } while (linksStart);
  return Mass;
}


/*=================================================================================================================*/
/*= XXI. Initialisierung                                                                                          =*/
/*=================================================================================================================*/


void U1_InitApriori(void)
{
  if (WM_ErsterAnlauf()) {
    SymbolT VarIndex;
    xU = negarray_alloc(U1_AnzErsterVariablen,U1_EintragsT,U1_AnzErsterVariablen);
    xUDimension = -U1_AnzErsterVariablen;
    SO_forSymboleAbwaerts(VarIndex,SO_ErstesVarSymb,xUDimension)
      xU[VarIndex].Substitut = NULL;
    AnzImKnotenAngelegterBindungen = 0;
    StackZeiger = 0;
    BindungsStack = posarray_alloc(U1_AnzErsterVariablen,StackElementT,1);
    Endestapel1 = posarray_alloc(U1_AnzErsterVariablen,EndeEintragsT,1);
    Endestapel2 = posarray_alloc(U1_AnzErsterVariablen,EndeEintragsT,1);
    Endestapel3 = posarray_alloc(U1_AnzErsterVariablen,EndeEintragsT,1);
  }
  KPFilterInitialisieren();
}


void U1_InitAposteriori(void)
{
  KPFilterErgaenzen(DarfUeberlappenUndLebtNochFilter);
  KPFilterErgaenzen(VaterIstGleichung);
  KPFilterErgaenzen(MutterIstGleichung);

  if (PA_SubconR() || PA_SubconE()) {
    if (PA_SubconSubst())
      KPFilterErgaenzen(MGUreduzibel);
    else
      KPFilterErgaenzen(Primueberlappung);
  }
  else if (PA_SubconP()){
    KPFilterErgaenzen(ACreduzibel);
  }
  if (PA_EinsSternAn())
    KPFilterErgaenzen(EinsSternUeberlappung);
  if (PA_NusfuAn())
    KPFilterErgaenzen(NusfUeberlappung);
  if (PA_KernAn())
    KPFilterErgaenzen(KernUeberlappung);
  if (PA_BackAn()) {
    TO_KombinatorapparatAktivieren();
    RichtigeZielseiteSuchen();
    KPFilterErgaenzen(RueckwartigeUeberlappung);
  }
}



/* Berechnet (falls moeglich) das CP von Vater und Mutter an Stelle Pos.
   Pos gibt eine Stelle in der linken Seite von Mutter an. Also muss ggf.
   die aufrufende Funktion bei Gleichungen die passende Gegenrichtung
   uebergeben. Pos == 0 bedeutet toplevel. Vater und Mutter sind in Aktiva, 
   d.h. sie duerfen nicht veraendert werden und es sind genuegend Variablen
   vorhanden. Die Variablenumbenennung erfolgt in der Routine.
   Ob Pos eine Variablenposition ist, wird in der Routine ueberprueft.
   Weiterhin kann es sein, dass die Unifikation fehlschlaegt, der
   Occur-Check muss durchgefuehrt werden. Ausserdem sind die U1-Filter zu 
   durchlaufen. Exisitiert dann ein CP, wird dessen linke Seite in resLhs, 
   die rechte in resRhs geschrieben und TRUE zurueckgegeben.
   Andernfalls wird FALSE zurueckgegeben.
 */

BOOLEAN U1_KPKonstruiert(RegelOderGleichungsT Vater, unsigned int Stellennummer, 
			 RegelOderGleichungsT Mutter, TermT *Links, TermT *Rechts)
{
  UTermT Stelle = TO_TermAnStelle(TP_LinkeSeite(Vater),Stellennummer);
  BOOLEAN Erfolg = TO_TermIstNichtVar(Stelle);
#if IF_TERMZELLEN_JAGD
  IO_DruckeFlexStrom(stderr, "Start U1_KPKonstruiert: %d %d  (%d / %d) \n", 
                     Vater->Nummer, Mutter->Nummer, TO_FreispeicherManager.Anforderungen,SV_AnzNochBelegterElemente(TO_FreispeicherManager));
#endif
  if (Erfolg) {
    BOOLEAN Kopiert = Vater==Mutter;
    struct GNTermpaarTS MutterS;
    if (Kopiert) {
      MutterS = *Mutter; Mutter = &MutterS;      /* ggf. ausduennen */
      TP_LinkeSeite(Mutter) = TO_Termkopie(TP_LinkeSeite(Mutter));
      TP_RechteSeite(Mutter) = TO_Termkopie(TP_RechteSeite(Mutter));
    }
    RE_FaktumExternalisieren(Mutter);
    if ((Erfolg = Unifiziert(Stelle,TP_LinkeSeite(Mutter)) && KPActionXX(Vater,Stelle,Mutter))){
/*       IO_DruckeFlex("Stelle = %t, lhs = %t, res: %t = %t\n", */
/* 		    Stelle, TP_LinkeSeite(Mutter), KPLinks, KPRechts); */
      *Links = KPLinks; *Rechts = KPRechts;
      KPLinks = NULL;
      KPRechts = NULL;
   }
    KnotenbindungenAufheben();
    if (Kopiert)
      TP_LinksRechtsLoeschen(Mutter);
    else
      RE_FaktumReinternalisieren(Mutter);
  }
#if IF_TERMZELLEN_JAGD
  IO_DruckeFlexStrom(stderr, "Stop U1_KPKonstruiert: %d %d  (%d / %d) -> %t = %t\n", 
                     Vater->Nummer, Mutter->Nummer, TO_FreispeicherManager.Anforderungen,
                     SV_AnzNochBelegterElemente(TO_FreispeicherManager),
                                        Erfolg ? *Links : NULL, Erfolg ? *Rechts : NULL);
#endif
  return Erfolg;
}

static Position calcPosition (RegelOderGleichungsT ActualParent, RegelOderGleichungsT OtherParent, 
			      UTermT actualPosition, BOOLEAN isCPWithFather, TermT lhsReplica)
{
  Position xp = 0;
  if (OtherParent == NULL){ 
    return xp;
  }

  if (ActualParent->geburtsTag <OtherParent->geburtsTag) {
    our_error( "############## ActualParent ist nicht A[i]\n");
  }

  POS_set_into_i(xp, !isCPWithFather);
  POS_set_i_right(xp, !(TP_RichtungAusgezeichnet(ActualParent)));
  POS_set_j_right(xp, !(TP_RichtungAusgezeichnet(OtherParent)));

  if (!POS_into_i(xp)) {
    POS_set_pos(xp, TO_StelleInTerm(actualPosition, TP_LinkeSeite(OtherParent)));
  } else {
    POS_set_pos(xp, TO_StelleInTerm(actualPosition, lhsReplica));
  }
  return xp;
}

static void GebildetesKPBehandelnMitVater(RegelOderGleichungsT father, TermT lhs, TermT rhs, UTermT actualPosition,
				       RegelOderGleichungsT actualParent, TermT lhsReplica)
{
  AbstractTime i = TP_getGeburtsTag_M(actualParent);
  AbstractTime j = TP_getGeburtsTag_M(father);
  Position xp = calcPosition(actualParent,father,actualPosition,TRUE,lhsReplica);
  if ((actualParent != Elter1) || (lhsReplica != Elter1Kopie->linkeSeite)) our_error("ups");
  KPV_insertCP(lhs,rhs,i,j,xp);
}

static void GebildetesKPBehandelnMitMutter(RegelOderGleichungsT mother, TermT lhs, TermT rhs, UTermT actualPosition,
					  RegelOderGleichungsT actualParent, TermT lhsReplica)
{
  AbstractTime i = TP_getGeburtsTag_M(actualParent);
  AbstractTime j = TP_getGeburtsTag_M(mother);
  Position xp = calcPosition(actualParent,mother,actualPosition,FALSE,lhsReplica);
  if ((actualParent != Elter1) || (lhsReplica != Elter1Kopie->linkeSeite)) our_error("ups");
  KPV_insertCP(lhs,rhs,i,j,xp);
}

