/*=================================================================================================================
  =                                                                                                               =
  =  Unifikation2.c                                                                                               =
  =                                                                                                               =
  =  Unifizieren von simplen Higher-Order-Pattern                                                                 =
  =  (Ordnung ca. 1,1)                                                                                            =
  =                                                                                                               =
  =  04.11.97  Thomas.                                                                                            =
  =                                                                                                               =
  =================================================================================================================*/

#include "compiler-extra.h"
#include "general.h"
#include "Ausgaben.h"
#include "SpeicherVerwaltung.h"
#include "TermIteration.h"
#include "Unifikation2.h"
#include "SpeicherParameter.h"

#ifdef CCE_Source
#define WM_ErsterAnlauf() (1)
#undef  U2_AnzErsterVariablen
#define U2_AnzErsterVariablen 20
#define IncAnzUnifVers()                                                                              /* Nix Tun */
#define IncAnzUnifErf()                                                                               /* Nix Tun */

#else
#include "Statistiken.h"
#include "WaldmeisterII.h"
#endif

TI_IterationDeklarieren(U2)


/*=================================================================================================================*/
/*= I. Variablenbank fuer Substitutionen; Stack angelegter Bindungen                                              =*/
/*=================================================================================================================*/

int xUDimension2;                              /* neueste bei gegenwaertiger Ausdehnung zu bindende Variable */
static UTermT *xU;

static unsigned int StackZeiger;
typedef SymbolT StackElementT;
static StackElementT *BindungsStack;
/* StackZeiger ist stets der Index des letzten beschriebenen Elements.
   Das nullte Element wird nicht beschrieben.
   -xUDimension2 ist der Index des letzten bei gegenwaertiger Ausdehnung noch zu schreibenden Elements.
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


void U2_VariablenfelderAnpassen(SymbolT DoppelteNeuesteBaumvariable)   /* Variablenbank und Stapel angelegter Bin- */
{    /* dungen so anpassen, dass Variablen x1, x2, ... , x(DoppelteNeuesteBaumvariable) verdaut werden. Darf nicht */
  if (SO_VariableIstNeuer(DoppelteNeuesteBaumvariable,xUDimension2)) { /* waehrend des Unifizierens aufgerufen wer- */
    SymbolT Index;                                                                      /* den, sondern nur davor. */
    unsigned int n;
    negarray_dealloc(xU,-xUDimension2);
    xUDimension2 = SO_VarDelta(DoppelteNeuesteBaumvariable,U2_AnzErsterVariablen);
    n = SO_VarNummer(xUDimension2);
    xU = negarray_alloc(n,UTermT,n);
    SO_forSymboleAbwaerts(Index,SO_ErstesVarSymb,xUDimension2)
      xU[Index] = NULL;
    posarray_realloc(BindungsStack,n+SO_AnzahlFunktionsvariablen(),StackElementT,1);
    posarray_realloc(Endestapel1,n,EndeEintragsT,1);
    posarray_realloc(Endestapel2,n,EndeEintragsT,1);
    posarray_realloc(Endestapel3,n,EndeEintragsT,1);
  }
}


static SymbolT *FU;
#define Fnull SO_ErstesVarSymb

void U2_FunktionsvariablenEintragen(void)
{
  unsigned int Anzahl = SO_AnzahlFunktionsvariablen();
  if (Anzahl) {
    SymbolT Index;
    FU = posarray_alloc(Anzahl,SymbolT,SO_ErsteFVar);
    SO_forFktVarSymbole(Index)
      FU[Index] = Fnull;
    posarray_realloc(BindungsStack,U2_AnzErsterVariablen+SO_AnzahlFunktionsvariablen(),StackElementT,1);
  }
}



/*=================================================================================================================*/
/*= II. Operationen auf oder mit Substitutionen                                                                   =*/
/*=================================================================================================================*/

#define /*UTermT*/ SubstitutVon(/*SymbolT*/ VarSymbol)                                                              \
  (xU[VarSymbol])

#define /*SymbolT*/ PatternsubstitutVon(/*SymbolT*/ VarSymbol)                                                      \
  (FU[VarSymbol])

#define /*BOOLEAN*/ VarGebunden(/*SymbolT*/ VarSymbol)                                                              \
  (SubstitutVon(VarSymbol))

#define /*BOOLEAN*/ PatternGebunden(/*SymbolT*/ VarSymbol)                                                          \
  (SO_SymbUngleich(PatternsubstitutVon(VarSymbol),Fnull))

#define /*BOOLEAN*/ SymbGebundeneVar(/*SymbolT*/ Symbol)                                                            \
  (SO_SymbIstVar(Symbol) && (VarGebunden(Symbol)))

#define /*BOOLEAN*/ SymbGebundenesPattern(/*SymbolT*/ Symbol)                                                       \
  (SO_SymbIstFVar(Symbol) && (PatternGebunden(Symbol)))

#define /*void*/ Entbinden(/*SymbolT*/ Topsymbol, /*UTermT*/ Term)                                                  \
  while (SymbGebundeneVar(Topsymbol))                                                                               \
    Topsymbol = TO_TopSymbol(Term = SubstitutVon(Topsymbol))

#define /*void*/ PatternEntbinden(/*SymbolT*/ Symbol)                                                               \
  while (SymbGebundenesPattern(Symbol))                                                                             \
    Symbol = PatternsubstitutVon(Symbol)

#define /*void*/ FertigEntbindenMitEnde(/*SymbolT*/ Topsymbol, /*UTermT*/ TermAlt, /*UTermT*/ TermAltEnde)          \
{                                                                                                                   \
  do                                                                                                                \
    Topsymbol = TO_TopSymbol(TermAlt = SubstitutVon(Topsymbol));                                                    \
  while (SymbGebundeneVar(Topsymbol));                                                                              \
  TermAltEnde = TO_TermEnde(TermAlt);                                                                               \
}


unsigned int U2_AnzBindungen;

void U2_BindungenAufhebenBis(unsigned int AnzBindungen)
{
  while (U2_AnzBindungen>AnzBindungen) {
    SymbolT Variable = PopVariable();
    if (SO_SymbIstFVar(Variable))
      FU[Variable] = Fnull;
    else
      xU[Variable] = NULL;
    U2_AnzBindungen--;
  }
}

void U2_SubstAufIdentitaetSetzen(void)
{
  U2_BindungenAufhebenBis(0);
}

void U2_DruckeMGU(void)
{
  SymbolT VarIndex;
  SO_forSymboleAbwaerts(VarIndex,SO_ErstesVarSymb,xUDimension2)
    if (VarGebunden(VarIndex)) {
      TermT Substitut = U2_MGU(SubstitutVon(VarIndex));
      IO_DruckeFlex("x%d <- %t\n",SO_VarNummer(VarIndex),Substitut);
      TO_TermLoeschen(Substitut);
    }
  SO_forFktVarSymbole(VarIndex)
    if (PatternGebunden(VarIndex))
      IO_DruckeFlex("%s <- %s\n",IO_FktSymbName(VarIndex),IO_FktSymbName(PatternsubstitutVon(VarIndex)));
}

static void DruckeBindungen(void) MAYBE_UNUSEDFUNC;
static void DruckeBindungen(void)
{
  SymbolT VarIndex;
  SO_forSymboleAbwaerts(VarIndex,SO_ErstesVarSymb,xUDimension2)
    if (VarGebunden(VarIndex))
      IO_DruckeFlex("x%d <- %t\n",SO_VarNummer(VarIndex),SubstitutVon(VarIndex));
  SO_forFktVarSymbole(VarIndex)
    if (PatternGebunden(VarIndex))
      IO_DruckeFlex("%s <- %s\n",IO_FktSymbName(VarIndex),IO_FktSymbName(PatternsubstitutVon(VarIndex)));
}

BOOLEAN U2_VarGebunden(SymbolT x)
{
  return VarGebunden(x) != NULL;
}

/*=================================================================================================================*/
/*= III. Unifizieren                                                                                              =*/
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
  xU[Variable] = Term;
  PushVariable(Variable);
  U2_AnzBindungen++;
  return TRUE;
}


static BOOLEAN PatternsubstErgaenzt(SymbolT Variable, SymbolT Symbol)
{
  PatternEntbinden(Symbol);
  if (SO_SymbGleich(Variable,Symbol))
    return TRUE;
  else if (SO_StelligkeitenGleich(Variable,Symbol)) {
    FU[Variable] = Symbol;
    PushVariable(Variable);
    U2_AnzBindungen++;
    return TRUE;
  }
  else
    return FALSE;
}


#define /*void*/ AufTeiltermende(/*{1;2}*/ Wo)                                                                      \
  Term ## Wo = TO_TermEnde(Term ## Wo)


#define /*void*/ AufSubstitut(/*{1;2}*/ Wo)                                                                         \
  PushEndestapel(Wo,Term ## Wo,Term ## Wo ## Ende);                                                                 \
  Term ## Wo ## Ende = TO_TermEnde(Term ## Wo = SubstitutVon(Symbol ## Wo));                                        \
  Symbol ## Wo = TO_TopSymbol(Term ## Wo);

#define /*void*/ AufPatternsubstitut(/*{1;2}*/ Wo)                                                                  \
  Symbol ## Wo = PatternsubstitutVon(Symbol ## Wo)


static BOOLEAN Unifiziert(UTermT Term1, UTermT Term2)
{
  UTermT Term1Ende = TO_TermEnde(Term1),
         Term2Ende = TO_TermEnde(Term2);
  IncAnzUnifVers();
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
        else if (SubstErgaenzt(Symbol1,Term2))
          AufTeiltermende(2);
        else
          return FALSE;
      }
      else if (SO_SymbIstVar(Symbol2)) {
        if (VarGebunden(Symbol2)) {
          AufSubstitut(2);
          goto Start;
        }
        else if (SubstErgaenzt(Symbol2,Term1))
          AufTeiltermende(1);
        else
          return FALSE;
      }
      else if (SO_SymbIstFVar(Symbol1)) {
        if (PatternGebunden(Symbol1)) {
          AufPatternsubstitut(1);
          goto Start;
        }
        else if (PatternsubstErgaenzt(Symbol1,Symbol2))
          ;
        else
          return FALSE;
      }
      else if (SO_SymbIstFVar(Symbol2)) {
        if (PatternGebunden(Symbol2)) {
          AufPatternsubstitut(2);
          goto Start;
        }
        else if (PatternsubstErgaenzt(Symbol2,Symbol1))
          ;
        else
          return FALSE;
      }
      else
        return FALSE;
    }
  } while (DoppeltWeitergeschaltet12(&Term1,&Term1Ende,&Term2,&Term2Ende));
  IncAnzUnifErf();
  return TRUE;
}
        

BOOLEAN U2_WeiterUnifiziert(UTermT Term1, UTermT Term2)
{
  unsigned int AnzVorigerBindungen = U2_AnzBindungen();
  BOOLEAN Ergebnis = Unifiziert(Term1,Term2);
  if (!Ergebnis)
    U2_BindungenAufhebenBis(AnzVorigerBindungen);
  return Ergebnis;
}
        
BOOLEAN U2_DoppeltWeiterUnifiziert(UTermT Term1, UTermT Term2,  UTermT Term3, UTermT Term4)
{
  unsigned int AnzVorigerBindungen = (/*IO_DruckeFlex("vor Unifikation von\n\t%t=%t @= %t=%t:\n", 
                                        Term1,Term3,Term2,Term4), DruckeBindungen(),*/
                                      U2_AnzBindungen());
  BOOLEAN Ergebnis = Unifiziert(Term1,Term2) && Unifiziert(Term3,Term4);
  if (!Ergebnis)
    U2_BindungenAufhebenBis(AnzVorigerBindungen);
  /*IO_DruckeFlex(" ==> %s\n", Ergebnis ? "TRUE" : "FALSE"); DruckeBindungen();*/
  return Ergebnis;
}



/*=================================================================================================================*/
/*= IV. Erzeugung von Termkopien unter Substitution                                                               =*/
/*=================================================================================================================*/

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
  PatternEntbinden(Topsymbol);                                                                                      \
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


TermT U2_MGU(UTermT TermAlt)                                                    /* liefert Term mit EndeNachf NULL */
{
  SymbolT Topsymbol = TO_TopSymbol(TermAlt);
  Entbinden(Topsymbol,TermAlt);
  PatternEntbinden(Topsymbol);

  if (!SO_Stelligkeit(Topsymbol)) /* wenn nullstellig (Konstante oder freie Variable), dann eine Termzelle zurueck */
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



/*=================================================================================================================*/
/*= V. Initialisierung                                                                                            =*/
/*=================================================================================================================*/

void U2_InitApriori(void)
{
  if (WM_ErsterAnlauf()) {
    SymbolT VarIndex;
    xU = negarray_alloc(U2_AnzErsterVariablen,UTermT,U2_AnzErsterVariablen);
    xUDimension2 = -U2_AnzErsterVariablen;
    BindungsStack = posarray_alloc(U2_AnzErsterVariablen,StackElementT,1);
    Endestapel1 = posarray_alloc(U2_AnzErsterVariablen,EndeEintragsT,1);
    Endestapel2 = posarray_alloc(U2_AnzErsterVariablen,EndeEintragsT,1);
    Endestapel3 = posarray_alloc(U2_AnzErsterVariablen,EndeEintragsT,1);
    U2_AnzBindungen = 0;
    SO_forSymboleAbwaerts(VarIndex,SO_ErstesVarSymb,xUDimension2)
      xU[VarIndex] = NULL;
    StackZeiger = 0;
  }
}


