/*=================================================================================================================
  =                                                                                                               =
  =  DSBaumOperationen.c                                                                                          =
  =                                                                                                               =
  =  Einfuegen und Loeschen von gerichtet normierten Termpaaren in Baeume vom Typ DSBaumT                         =
  =                                                                                                               =
  =                                                                                                               =
  =  22.03.94  Thomas.                                                                                            =
  =                                                                                                               =
  =================================================================================================================*/

#include "Ausgaben.h"
#include "general.h"
#include "DSBaumKnoten.h"
#include "DSBaumOperationen.h"
#include "MatchOperationen.h"
#include "SpeicherVerwaltung.h"
#include "SymbolOperationen.h"
#include "TermIteration.h"
#include "TermOperationen.h"

#ifndef CCE_Source
#include "Unifikation1.h"
#include "Unifikation2.h"
#include "Statistiken.h"

#else

#define IncAnzSchrumpfPfadExp() /* nix tun */
#define IncAnzEingefRegeln() /* nix tun */
#define IncAnzEntfernterRegeln() /* nix tun */
#define IncAnzPfadschrumpfungen() /* nix tun */

#endif

TI_IterationDeklarieren(BO)


/* ------------------------------------------------------------------------*/
/* ------------- globale Variablen (siehe DSBaumOperationen.h) ------------*/
/* ------------------------------------------------------------------------*/

SymbolT BO_NeuesteBaumvariable;
unsigned int BO_MaximaleTiefe;

/*=================================================================================================================*/
/*= I. Operationen fuer die Variablenumbenennung                                                                  =*/
/*=================================================================================================================*/

static int xODimension;           /* xO realisiert Abbildung alterIndex->neuerIndex. Das Feld wird mit den Variab- */
static SymbolT *xO = NULL; /* lensymbolen negativ indiziert und hat die Laenge xODimension. Fuer xO[0] wird kein Speicher */
static SymbolT letzteAlteVar, letzteNeueVar;                                                          /* angelegt. */

static void VariableUmbenennen(UTermT Variable)                         /* xO wird initial als Feld mit Ausdehnung */
{                            /* BO_AnzErsterVariablen angelegt und muss gegebenfalls nach links ausgedehnt werden. */
  SymbolT VarSymbol = TO_TopSymbol(Variable);
  if (SO_VariableIstNeuer(VarSymbol,letzteAlteVar)) {                        /* Falls bisherige Ausdehnung im Feld */
    SymbolT VarIndex;                                                                 /* nach links ueberschritten */
    if (SO_VariableIstNeuer(VarSymbol,xODimension)) {
      int xODimensionNeu = SO_VarDelta(VarSymbol,BO_AnzErsterVariablen);
      SymbolT *xONeu = (SymbolT *) our_alloc(-xODimensionNeu*sizeof(SymbolT)) - xODimensionNeu;    /* Addition von */
      SO_forSymboleAbwaerts(VarIndex,SO_ErstesVarSymb,letzteAlteVar)     /* xODimension fuer negative Adressierung */
        xONeu[VarIndex] = xO[VarIndex];                                                /* alte Eintrage umkopieren */
      our_dealloc(xO+xODimension);
      xODimension = xODimensionNeu;
      xO = xONeu;
    }
    SO_forSymboleAbwaerts(VarIndex,SO_NaechstesVarSymb(letzteAlteVar),SO_VorigesVarSymb(VarSymbol))
      xO[VarIndex] = SO_KleinstesFktSymb;                                  /* "Luecken" als unbenutzt kennzeichnen */
    letzteAlteVar = VarSymbol;
    TO_SymbolBesetzen(Variable,xO[VarSymbol] = SO_NaechstesVarSymbD(letzteNeueVar));
  }
  else if (SO_SymbGleich(xO[VarSymbol],SO_KleinstesFktSymb))
    TO_SymbolBesetzen(Variable,xO[VarSymbol] = SO_NaechstesVarSymbD(letzteNeueVar));
  else
    TO_SymbolBesetzen(Variable,xO[VarSymbol]);
}

unsigned int BO_AusdehnungVariablenbank(void)
{
  return -xODimension;
}

static void GgfSprungstapelAusdehnen(unsigned int LaengeEinerLinkenSeite);                     /* Definition in II */

void BO_VariablenbaenkeAnpassen(SymbolT neuesteVariable)                  /* Das gehoert garantiert nicht hierher. */
{
  if (SO_VariableIstNeuer(neuesteVariable,BO_NeuesteBaumvariable)) {     /* diverse Felder in anderen Modulen nach */
    BO_NeuesteBaumvariable = neuesteVariable;                              /* aktuellem Variablenreichtum anpassen */
#ifndef CCE_Source
    U1_VariablenfelderAnpassen(2*BO_NeuesteBaumvariable);
    U2_VariablenfelderAnpassen(2*BO_NeuesteBaumvariable);
#endif
    MO_VariablenfeldAnpassen(2*BO_NeuesteBaumvariable);
    SO_VariablenfeldAnpassen(2*BO_NeuesteBaumvariable);
    TO_FeldVariablentermeAnpassen(2*BO_NeuesteBaumvariable);
  }
}

void BO_TermpaarNormieren(TermT Term1, TermT Term2, unsigned int *TiefeTerm1)     /* Variablenumwandlung auf Term1 */
{/* durchfuehren und auf Term2 fortsetzen. Das Termende muss hier durch Vergleich mit NULL erkannt werden koennen. */
  *TiefeTerm1 = 1;
  letzteAlteVar = letzteNeueVar = SO_VorErstemVarSymb;

  if (TO_Nullstellig(Term1)) {
    if (TO_TermIstVar(Term1))
      VariableUmbenennen(Term1);
  }
  else {
    unsigned int LaengeTerm1 = 1;
    TI_PushOT(Term1);       /* TI_Stapelhoehe()+1 ist aktuelle Termtiefe fuer den Fall, dass Term gerade auf einen */
    do {                            /* nullstelligen Teilterm zeigt. Da nur dort Maxima angenommen werden koennen, */
      Term1 = TO_Schwanz(Term1);
      LaengeTerm1++;
      if (TO_Nullstellig(Term1)) {                             /* muss maximaleTiefe nur dort aktualisiert werden. */
        if (TO_TermIstVar(Term1))
          VariableUmbenennen(Term1);
        if (TI_Stapelhoehe()>*TiefeTerm1)
          *TiefeTerm1 = TI_Stapelhoehe();
        while (!(--TI_TopZaehler))
          if (TI_PopUndLeer())
            goto fertig;
      }
      else
        TI_PushOT(Term1);
    } while (TRUE);
  fertig:
    (*TiefeTerm1)++;                                   /* da nullstellige Terme nicht auf den Stapel gelegt werden */
    GgfSprungstapelAusdehnen(LaengeTerm1);                                          /* Anpassungen in diesem Modul */
    if (*TiefeTerm1>BO_MaximaleTiefe)
      BO_MaximaleTiefe = *TiefeTerm1;
  }

  do
    if (TO_TermIstVar(Term2))
      VariableUmbenennen(Term2);
  while ((Term2 = TO_Schwanz(Term2)));

  BO_VariablenbaenkeAnpassen(letzteNeueVar);
}


void BO_TermpaarNormierenAlsKP(TermT Term1, TermT Term2)                          /* Variablenumwandlung auf Term1 */
{/* durchfuehren und auf Term2 fortsetzen. Das Termende muss hier durch Vergleich mit NULL erkannt werden koennen. */
  letzteAlteVar = letzteNeueVar = SO_VorErstemVarSymb;
  do
    if (TO_TermIstVar(Term1))
      VariableUmbenennen(Term1);
  while ((Term1 = TO_Schwanz(Term1)));
  do
    if (TO_TermIstVar(Term2))
      VariableUmbenennen(Term2);
  while ((Term2 = TO_Schwanz(Term2)));
}



/*=================================================================================================================*/
/*= II. Verwaltung des Positionszaehlers und des Sprungstapels                                                    =*/
/*=================================================================================================================*/

static unsigned int PositionImTerm;

#define /*SymbolT*/ AktuellesTopsymbol(/*UTermT*/ Term)                     /* Liefert das neue Topsymbol zurueck */\
  (PositionImTerm++, TO_TopSymbol(Term))             /* und setzt den Zaehler fuer Position in linker Seite herauf */

typedef struct {
  KantenT Startknoten;
  UTermT Teilterm, Teilterm2;
  int Zaehler, Zaehler2;
} StapeleintragsT;

static StapeleintragsT *Sprungstapel;
static unsigned int StapelIndex, StapelIndex2;
static unsigned int StapelDimension;

/* Stapelelemente sind die Startknoten der moeglichen Spruenge usw. Wird als Feld realisiert; Indizierung mit den
   Variablen StapelIndex1, StapelIndex2. Die Indizierung des Feldes soll mit dem 0. Element beginnen. Stapelindex
   zeigt auf das zuletzt aufgelegte Element. Auf Position 0 wird ein Dummy-Eintrag gelegt fuer den Fall, dass eine
   nullstellige linke Seite eingefuegt wird. Die Abfrage !(--TopZaehler) liefert dann je dasselbe Resultat. 
   StapelDimension ist der hoechste Index, zu dem gerade noch Speicherplatz zur Verfuegung steht. */   

static void GgfSprungstapelAusdehnen(unsigned int LaengeEinerLinkenSeite)
{
  if (LaengeEinerLinkenSeite>StapelDimension)
    array_realloc(Sprungstapel,
     (StapelDimension = minKXgrY(BO_ErsteSprungstapelhoehe,LaengeEinerLinkenSeite))+1,StapeleintragsT);
}


#define /*void*/ StapelInitialisieren(/*void*/)                                                                     \
  Sprungstapel[StapelIndex = 0].Zaehler = 0                    /* verkuerzt Abfrage bei nullstelliger linker Seite */

#define /*void*/ StapelParallelInitialisieren(/*void*/)                                                             \
  Sprungstapel[StapelIndex = 0].Zaehler2 = 0

#define /*BOOLEAN*/ Stapel2Gueltig(/*void*/)                                                                        \
  (StapelIndex2)

#define /*BOOLEAN*/ StapelUeber(/*unsigned int*/Untergrenze)                                                        \
  (StapelIndex>Untergrenze)

#define EintragEins 1
#define EintragUngueltig 0

#define /*void*/ Push(/*KantenT*/ Start, /*UTermT*/ Term)                                                           \
  Sprungstapel[++StapelIndex].Startknoten = Start;                                                                  \
  Sprungstapel[StapelIndex].Teilterm = Term;                                                                        \
  Sprungstapel[StapelIndex].Zaehler = TO_AnzahlTeilterme(Term)

#define /*void*/ Push2(/*KantenT*/ Start, /*UTermT*/ Term)                                                          \
  Sprungstapel[++StapelIndex2].Startknoten = Start;                                                                 \
  Sprungstapel[StapelIndex2].Teilterm2 = Term;                                                                      \
  Sprungstapel[StapelIndex2].Zaehler2 = TO_AnzahlTeilterme(Term)

#define /*void*/ PushDoppelt(/*KantenT*/ Start, /*UTermT*/ Term1, /*UTermT*/ Term2)                                 \
  Sprungstapel[++StapelIndex].Startknoten = Start;                                                                  \
  Sprungstapel[StapelIndex].Teilterm = Term1;                                                                       \
  Sprungstapel[StapelIndex].Teilterm2 = Term2;                                                                      \
  Sprungstapel[StapelIndex].Zaehler = TO_AnzahlTeilterme(Term1)

#define /*void*/ Pop(/*void*/)                                                                                      \
  (StapelIndex--)

#define /*void*/ Pop2(/*void*/)                                                                                     \
  (StapelIndex2--)

#define TopZaehler      (Sprungstapel[StapelIndex].Zaehler)
#define TopZaehler2     (Sprungstapel[StapelIndex2].Zaehler2)

#define TopTeilterm     (Sprungstapel[StapelIndex].Teilterm)
#define TopTeilterm2    (Sprungstapel[StapelIndex2].Teilterm2)

#define TopStartknoten  (Sprungstapel[StapelIndex].Startknoten)
#define TopStartknoten2 (Sprungstapel[StapelIndex2].Startknoten)

#define /*void*/ Stapel2Sichern(/*void*/)                                                                           \
{                                                                                                                   \
  unsigned int i = StapelIndex2 = StapelIndex;                                                                      \
                                                                                                                    \
  while (i) {                                                                                                       \
    Sprungstapel[i].Zaehler2 = Sprungstapel[i].Zaehler;                                                             \
    i--;                                                                                                            \
  }                                                                                                                 \
}

unsigned int BO_AusdehnungSprungstapel(void)
{
  return StapelDimension;
}



/*=================================================================================================================*/
/*= III. Sprungeintraege verfertigen                                                                              =*/
/*=================================================================================================================*/

#define RumpfSprungeintragSetzen(Start,Ziel,Term,Position)                                                          \
  SprungeintragsT NeuerEintrag = BK_SprungeintragAnlegen();                                                         \
                                                                                                                    \
  NeuerEintrag->Startknoten = Start;                                                                                \
  NeuerEintrag->Zielknoten = Ziel;                                                                                  \
  NeuerEintrag->Teilterm = Term;                                                                                    \
  NeuerEintrag->PositionNachTeilterm = Position;                                                                    \
  PraefixeintragZuNULLOderNicht();                                                                                  \
                                                                                                                    \
  /* Ausgangsseitige Verkettung */                                                                                  \
  if ((NeuerEintrag->NaechsterZieleintrag = BK_Sprungausgaenge(Start)))                                             \
    (BK_Sprungausgaenge(Start))->VorigerZieleintrag = NeuerEintrag;                                                 \
  (BK_SprungausgaengeSei(Start,NeuerEintrag))->VorigerZieleintrag = NULL;                                           \
                                                                                                                    \
  /* Eingangsseitige Verkettung */                                                                                  \
  NeuerEintrag->NaechsterStarteintrag = BK_Sprungeingaenge(Ziel);                                                   \
  BK_SprungeingaengeSei(Ziel,NeuerEintrag)

#define PraefixeintragZuNULLOderNicht()
static void SprungeintragSetzenOhnePraefixverweisNULL
  (KantenT Sprungstart, KantenT Sprungziel, UTermT Teilterm, unsigned int Position)
{ RumpfSprungeintragSetzen(Sprungstart,Sprungziel,Teilterm,Position);
}
static void SprungeintragSetzenVonPopOhnePraefixverweisNULL(KantenT Sprungziel, unsigned int Position)
{ RumpfSprungeintragSetzen(TopStartknoten,Sprungziel,TopTeilterm,Position);
  Pop();
}

#undef PraefixeintragZuNULLOderNicht
#define PraefixeintragZuNULLOderNicht() NeuerEintrag->Praefixeintrag = NULL

static void SprungeintragSetzen(KantenT Sprungstart, KantenT Sprungziel, UTermT Teilterm, unsigned int Position)
{ RumpfSprungeintragSetzen(Sprungstart,Sprungziel,Teilterm,Position);
}
static void SprungeintragSetzenVonPop(KantenT Sprungziel, unsigned int Position)
{ RumpfSprungeintragSetzen(TopStartknoten,Sprungziel,TopTeilterm,Position);
  Pop();
}
static void SprungeintragSetzenVonPop2(KantenT Sprungziel, unsigned int Position)
{ RumpfSprungeintragSetzen(TopStartknoten2,Sprungziel,TopTeilterm2,Position);
  Pop2();
}



/*=================================================================================================================*/
/*= IV. Aufnehmen von neuen Regeln in die Regellisten der Blaetter                                                =*/
/*=================================================================================================================*/

/* Zu jeder im selben Blatt abgelegten Regel gehoert eine physisch eigene linke Seite. Damit fuer den Eintrag lin-
   keSeite im Blatt nicht extra ein Term erzeugt werden muss, wird dieser Eintrag auf die linke Seite der letzten
   in der Liste befindlichen Regel gerichtet. - Die Regelliste ist doppelt verkettet.                              */

#define /*void*/ NeueRegellisteEinhaengen(/*KantenT*/ DasBlatt, /*GNTermpaarT*/ Regel)  /* linke und rechte Seite */\
{                                   /* zu einer Regel machen, neues Blatt mit einelementiger Regelliste versehen, */\
  BK_RegelnSei(Regel->Blatt = DasBlatt,Regel);                                                                      \
  Regel->Nachf = Regel->Vorg = NULL;                                                                                \
}


#define /*void*/ RegelHinzufuegen(/*KantenT*/ DasBlatt, /*GNTermpaarT*/ Regel) /* Linke und rechte Seite zu einer */\
{                                                /* Regel machen, neue Regel vor die bestehende Regelliste setzen */\
  ((Regel->Nachf = BK_Regeln(DasBlatt))->Vorg = Regel)->Vorg = NULL;                                                \
  BK_RegelnSei(Regel->Blatt = DasBlatt,Regel);                                                                      \
}



/*=================================================================================================================*/
/*= V. Einhaengen eines neuen Blattes in einen bestehenden Knoten und Blaetterverzeigerung                        =*/
/*=================================================================================================================*/

/* Die Blatter sind als doppelt verkettete Liste miteinander verzeigert.
   Die Liste ist steigend sortiert nach der Tiefe der zu den Blaettern gehoerenden linken Seiten.
   In der Struktur DSBaumT existiert ein Feld TiefeBlaetter, das eine Abbildung Nat -> Blaetter U NULL realisiert.
   Die Ausdehnung des um -1 verschobenen Feldes ist in TiefeBlaetterDimension vermerkt.
   TiefeBlaetter[n] verweist fuer 1<=n<=TiefeBlaetterDimension auf das erste Blatt mit der Tiefe n, falls ein
   solches existiert, andernfalls auf NULL.
   Die maximal auftretende Tiefe wird in maxTiefeLinks vorraetig gehalten.
   Die Komponente ErstesBlatt verweist jeweils auf das erste Blatt mit der niedrigsten Termtiefe.
*/

static void BlattEinzeigern(KantenT Blatt, DSBaumT *Baum, unsigned int TiefeLinkerSeite)
#define /*Rumpf*/ ObjektEinzeigern(/*Objekt*/ Blatt, /*Objekttyp*/ KantenT,                                         \
  /*Vorgaenger lesen*/ BK_VorgBlatt, /*Nachfolger lesen*/ BK_NachfBlatt,                                            \
  /*Vorgaenger ueberschreiben*/ BK_VorgBlattSei, /*Nachfolger ueberschreiben*/ BK_NachfBlattSei,                    \
  /*Echter Kantentyp im Baum*/ EchterKantenT)                                                                       \
{                                                                                                                   \
  if (TiefeLinkerSeite>Baum->TiefeBlaetterDimension) {                    /* Verweisfeld gegebenenfalls ausdehnen */\
    unsigned int TiefenIndex = Baum->TiefeBlaetterDimension+1,                                                      \
                 TiefeBlaetterDimensionNeu =                                                                        \
                   minKXgrY(BK_AnzahlErsterTieferBlaetter,TiefeLinkerSeite);                                        \
    Baum->TiefeBlaetter = ((EchterKantenT *)our_realloc(Baum->TiefeBlaetter+1,                                      \
      TiefeBlaetterDimensionNeu*sizeof(KantenT),"Ausdehnung eines Blaettertiefenfeldes"))-1;                        \
    do                                                                                                              \
      Baum->TiefeBlaetter[TiefenIndex] = NULL;                                                                      \
    while (TiefenIndex++<TiefeBlaetterDimensionNeu);                                                                \
    Baum->TiefeBlaetterDimension = TiefeBlaetterDimensionNeu;                                                       \
  }                                                                                                                 \
  if (Baum->TiefeBlaetter[TiefeLinkerSeite]) {           /* Falls schon ein Blatt mit dieser Termtiefe existiert, */\
    KantenT Vorg = (KantenT) Baum->TiefeBlaetter[TiefeLinkerSeite],             /* das neue hinter dieses setzen. */\
            Nachf = BK_NachfBlatt(Vorg);                                /* Vorg existiert, Nachf nicht unbedingt. */\
    BK_VorgBlattSei(Blatt,Vorg);                                                                                    \
    BK_NachfBlattSei(Blatt,Nachf);                                                                                  \
    BK_NachfBlattSei(Vorg,Blatt);                                                                                   \
    if (Nachf)                                                                                                      \
      BK_VorgBlattSei(Nachf,Blatt);                                                                                 \
  }                                                                                                                 \
  else {                                                                                            /* Ansonsten: */\
    unsigned int TiefenIndexKleiner = TiefeLinkerSeite-1,                                                           \
                 TiefenIndexGroesser = TiefeLinkerSeite+1;                                                          \
    while (TiefenIndexKleiner && !Baum->TiefeBlaetter[TiefenIndexKleiner])                                          \
      TiefenIndexKleiner--;                                                                                         \
    while (TiefenIndexGroesser<=Baum->maxTiefeLinks && !Baum->TiefeBlaetter[TiefenIndexGroesser])                   \
      TiefenIndexGroesser++;                                                                                        \
    if (TiefenIndexKleiner) {                                        /* 1. Es gibt Blaetter mit geringerer Tiefe. */\
      KantenT Vorg = (KantenT) Baum->TiefeBlaetter[TiefenIndexKleiner];                                             \
      unsigned int TiefenIndexGroesser = TiefeLinkerSeite+1;                                                        \
      while (TiefenIndexGroesser<=Baum->TiefeBlaetterDimension && !Baum->TiefeBlaetter[TiefenIndexGroesser])        \
        TiefenIndexGroesser++;                                                                                      \
      if (TiefenIndexGroesser<=Baum->maxTiefeLinks) {          /* 1.1 Es gibt auch Blaetter mit groesserer Tiefe. */\
        KantenT Nachf = (KantenT) Baum->TiefeBlaetter[TiefenIndexGroesser],   /* Zur Tiefe TiefenIndexKleiner ist */\
                VorgNeu =  BK_NachfBlatt(Vorg);        /* bisher nur das erste Blatt bekannt. Das neue Blatt soll */\
        while ((VorgNeu!=Nachf))                                   /* aber hinter das letzte dieser Tiefe kommen. */\
          VorgNeu = BK_NachfBlatt(Vorg = VorgNeu);                                                                  \
        BK_VorgBlattSei(Blatt,Vorg);                                                                                \
        BK_NachfBlattSei(Blatt,Nachf);                                                                              \
        BK_NachfBlattSei(Vorg,Blatt);                                                                               \
        BK_VorgBlattSei(Nachf,Blatt);                                                                               \
      }                                                                                                             \
      else {                                                  /* 1.2 Es gibt keine Blaetter mit groesserer Tiefe. */\
        KantenT VorgNeu = BK_NachfBlatt(Vorg);                                                                      \
        while ((VorgNeu))                                                                                           \
          VorgNeu = BK_NachfBlatt(Vorg = VorgNeu);                                                                  \
        BK_VorgBlattSei(Blatt,Vorg);                                                                                \
        BK_NachfBlattSei(Vorg,Blatt);                                                                               \
        BK_NachfBlattSei(Blatt,NULL);                                                                               \
        Baum->maxTiefeLinks = TiefeLinkerSeite;             /* maximale Tiefe der linken Baumseiten aktualisieren */\
      }                                                                                                             \
    }                                                                                                               \
    else {                                                     /* 2. Es gibt keine Blaetter mit geringerer Tiefe. */\
      Baum->ErstesBlatt = (EchterKantenT) Blatt;          /* Verweis auf Blatt mit geringster Tiefe aktualisieren */\
      BK_VorgBlattSei(Blatt,NULL);                                                                                  \
      if (TiefenIndexGroesser<=Baum->maxTiefeLinks) {               /* 2.1 Es gibt Blaetter mit groesserer Tiefe. */\
        KantenT Nachf = (KantenT) Baum->TiefeBlaetter[TiefenIndexGroesser];                                         \
        BK_NachfBlattSei(Blatt,Nachf);                                                                              \
        BK_VorgBlattSei(Nachf,Blatt);                                                                               \
      }                                                                                                             \
      else {                                                  /* 2.2 Es gibt keine Blaetter mit groesserer Tiefe. */\
        BK_NachfBlattSei(Blatt,NULL);                                                                               \
        Baum->maxTiefeLinks = TiefeLinkerSeite;             /* maximale Tiefe der linken Baumseiten aktualisieren */\
      }                                                                                                             \
    }                                                                                                               \
    Baum->TiefeBlaetter[TiefeLinkerSeite] = (EchterKantenT) Blatt;                          /* Feldverweis setzen */\
  }                                                                                /* fuer neu erzielte Termtiefe */\
}
ObjektEinzeigern(/*Objekt*/ Blatt, /*Objekttyp*/ KantenT,
  /*Vorgaenger lesen*/ BK_VorgBlatt, /*Nachfolger lesen*/ BK_NachfBlatt,
  /*Vorgaenger ueberschreiben*/ BK_VorgBlattSei, /*Nachfolger ueberschreiben*/ BK_NachfBlattSei,
  /*Echter Kantentyp im Baum*/ KantenT)

static KantenT NeuesBlattEinhaengen(KantenT Knoten, UTermT TermrestMitIntro, GNTermpaarT Regel, 
  PraefixeintragsT NeuePraefixliste, unsigned int Stapeluntergrenze, DSBaumT *Baum, unsigned int TiefeLinkerSeite)
{              /* In einen bestehenden Knoten ein neues Blatt einhaengen. Topsymbol ist normiert, Rest noch nicht. */
  KantenT Blattzeiger = BK_BlattAnlegen();
  UTermT TermIndex = TO_Schwanz(TermrestMitIntro);
  SymbolT Topsymbol = TO_TopSymbol(TermrestMitIntro);

  /* Regularien */
  BK_RestSei(Blattzeiger,TermIndex);
  BK_linkeSeiteSei(Blattzeiger,Regel->linkeSeite);
  BK_SuffixPositionSei(Blattzeiger,PositionImTerm);
  BK_NachfolgenLassen(Knoten,Topsymbol,Blattzeiger);
  BK_PraefixlisteSei(Blattzeiger,NeuePraefixliste);
  NeueRegellisteEinhaengen(Blattzeiger,Regel);
  BlattEinzeigern(Blattzeiger,Baum,TiefeLinkerSeite);


  /* zunaechst Sprungeingaenge zum einleitenden Symbol ueberpruefen oder Sprungausgaenge setzen */
  if (TO_NichtNullstellig(TermrestMitIntro)) {
    Push(Knoten,TermrestMitIntro);
  }
  else {    /* Besonderheit fuer Konstantensymbol: kann noch Sprungausgang sein. Variablensymbol natuerlich nicht. */
    if (SO_SymbIstFkt(Topsymbol) && TO_PhysischUngleich(TermrestMitIntro,Regel->linkeSeite))/* <=> nicht an Wurzel */
      SprungeintragSetzen(Knoten,Blattzeiger,TermrestMitIntro,PositionImTerm);
    while (!(--TopZaehler) && StapelUeber(Stapeluntergrenze))
      SprungeintragSetzenVonPop(Blattzeiger,PositionImTerm);
    /* Wenn Teilterme mit dem einleitenden Symbol endeten, gehen deren Sprung-
       zeiger ins neue Blatt. Kein Praefixverweis noetig, aber Sprungverweis. */
  }

  /* nun weitere Sprungeingaenge in Suffix betrachten */
  while (TermIndex) {
    Topsymbol = AktuellesTopsymbol(TermIndex);
    TopZaehler += SO_Stelligkeit(Topsymbol);       /* Ausdruecke nicht verschachtelt wg. Makro SymbStelligkeit */
    while (!(--TopZaehler) && StapelUeber(Stapeluntergrenze))
      SprungeintragSetzenVonPop(Blattzeiger,PositionImTerm);
    TermIndex = TO_Schwanz(TermIndex);
  }

  return Blattzeiger;
}



/*=================================================================================================================*/
/*= VI. Aufteilung eines bestehenden Blattes                                                                      =*/
/*=================================================================================================================*/

static void AltesBlattPolieren(KantenT AltesBlatt, KantenT NeuesBlatt)
{
  unsigned int NeueSuffixposition = BK_SuffixPosition(NeuesBlatt);
  SprungeintragsT SprungIndex = BK_Sprungeingaenge(AltesBlatt), SprungIndexVorg = NULL;

  BK_SuffixPositionSei(AltesBlatt,NeueSuffixposition);
  while (SprungIndex)
   if (SprungIndex->PositionNachTeilterm>=NeueSuffixposition) {  /* Wenn Sprung nach wie vor im Blatt landet, also */
      /* der zugehoerige Teilterm in den Suffix hineinreicht, dann eine paralle Verbindung ins neue Blatt aufbauen */
     unsigned int i = 1;
     UTermT TermIndexAlt = BK_linkeSeite(AltesBlatt), TermIndexNeu = BK_linkeSeite(NeuesBlatt);     
     SprungeintragsT NeuerSprung = BK_SprungeintragAnlegen();

     NeuerSprung->Zielknoten = NeuesBlatt;
     NeuerSprung->Startknoten = SprungIndex->Startknoten;
     NeuerSprung->Praefixeintrag = NULL;

     while (TO_PhysischUngleich(TermIndexAlt,SprungIndex->Teilterm)) {         /* Praefixe der Regeln sind gleich, */
       TermIndexAlt = TO_Schwanz(TermIndexAlt);                             /* daher blosses Abzaehlen hinreichend */
       TermIndexNeu = TO_Schwanz(TermIndexNeu);
       i++;
     }
     NeuerSprung->Teilterm = TermIndexNeu;
     
     /* PositionNachTeilterm fuer den Teilterm im neuen Blatt bestimmen */
     while (TO_PhysischUngleich(TermIndexNeu,TO_TermEnde(NeuerSprung->Teilterm))) {        /* i weiter hochzaehlen */
       TermIndexNeu = TO_Schwanz(TermIndexNeu);                         /* bis zur letzten Termzelle des Teilterms */
       i++;
     }
     NeuerSprung->PositionNachTeilterm = i+1;
     
     /* ausgangsseitig verketten: hinter den Eintrag setzen, zu dem eine Parallele aufgebaut wird */
     if ((NeuerSprung->NaechsterZieleintrag = SprungIndex->NaechsterZieleintrag))
       SprungIndex->NaechsterZieleintrag->VorigerZieleintrag = NeuerSprung;
     (SprungIndex->NaechsterZieleintrag = NeuerSprung)->VorigerZieleintrag = SprungIndex;

     /* eingangsseitig verketten: an den Anfang der Sprungeingangsliste setzen */
     NeuerSprung->NaechsterStarteintrag = BK_Sprungeingaenge(NeuesBlatt);
     BK_SprungeingaengeSei(NeuesBlatt,NeuerSprung);

     SprungIndex = (SprungIndexVorg = SprungIndex)->NaechsterStarteintrag;   /* weiter mit naechstem Starteintrag */
   }
   else {
     unsigned int i = NeueSuffixposition-SprungIndex->PositionNachTeilterm;
     KantenT GleichPfadIndex = BK_Vorg(NeuesBlatt);
     PraefixeintragsT NeuerListenkopf = BK_PraefixeintragAnlegen();
     SprungeintragsT AlterSprungIndexNachf = SprungIndex->NaechsterStarteintrag;

     /* Ansonsten den Sprungverweis umsetzen auf einen der neuen inneren Knoten. */
     /* Naemlich auf den Knoten Nr. (NeueSuffixposition-SprungIndex->PositionNachTeilterm) des GleichPfades, */
     /* gezaehlt von unten und beginnend mit 1 fuer den letzten inneren Knoten */
     while (--i)
       GleichPfadIndex = BK_Vorg(GleichPfadIndex);

     /* Herausnehmen aus der Verzeigerung der Sprungeingaenge ins alte Blatt */
     if (SprungIndexVorg)
       SprungIndexVorg->NaechsterStarteintrag = AlterSprungIndexNachf;
     else
       BK_SprungeingaengeSei(AltesBlatt,AlterSprungIndexNachf);

     /* Aufnehmen in die Verzeigerung der Sprungeingaenge in errechneten neuen Knoten */
     SprungIndex->NaechsterStarteintrag = BK_Sprungeingaenge(SprungIndex->Zielknoten = GleichPfadIndex);
     BK_SprungeingaengeSei(GleichPfadIndex,SprungIndex);      /* Sprungeintrag Komponente Zielknoten aktualisieren */

     /* Verweis in Praefixliste des alten Blattes aufnehmen */
     NeuerListenkopf->NaechsterPraefixeintrag = BK_Praefixliste(AltesBlatt);
     BK_PraefixlisteSei(AltesBlatt,NeuerListenkopf);
     (NeuerListenkopf->Sprungeintrag = SprungIndex)->Praefixeintrag = NeuerListenkopf;
     SprungIndex = AlterSprungIndexNachf;           /* weiter mit urspruenglichem Start-Nachfolger von SprungIndex */
   }
}


static void NeueSpruengeInsAlteBlatt(KantenT Blatt, UTermT VorNeuemRegelrest)        /* offene Spruenge in Stapel2 */
{                                                                      /* vom Gleichpfad in das alte Blatt fuehren */
  unsigned int PositionImTerm2 = BK_SuffixPosition(Blatt);

  /* zunaechst Sprungeingaenge zum einleitenden Symbol ueberpruefen oder Sprungausgaenge setzen */
  if (TO_NichtNullstellig(VorNeuemRegelrest)) {
    Push2(BK_Vorg(Blatt),VorNeuemRegelrest);
  }
  else {    /* Besonderheit fuer Konstantensymbol: kann noch Sprungeingang sein. Variablensymbol natuerlich nicht. */
    if (SO_SymbIstFkt(TO_TopSymbol(VorNeuemRegelrest)))
      SprungeintragSetzen(BK_Vorg(Blatt),Blatt,VorNeuemRegelrest,PositionImTerm2);
    while (!(--TopZaehler2))            /* Wenn Teilterme mit dem einleitenden Symbol endeten, gehen deren Sprung- */
      SprungeintragSetzenVonPop2(Blatt,PositionImTerm2);           /* zeiger ins neue Blatt. Sprungverweis noetig. */
  }

  /* nun weitere Sprungeingaenge in Suffix betrachten */
  while ((PositionImTerm2++, VorNeuemRegelrest = TO_Schwanz(VorNeuemRegelrest))) {
    TopZaehler2 += SO_Stelligkeit(TO_TopSymbol(VorNeuemRegelrest));
    while (!(--TopZaehler2) && Stapel2Gueltig())
      SprungeintragSetzenVonPop2(Blatt,PositionImTerm2);
  }
}


static KantenT BlattAufgeteilt(KantenT AltesBlatt, UTermT TermIndexNeu, 
                                                     GNTermpaarT Regel, DSBaumT *Baum, unsigned int TiefeLinkerSeite)
/* Existierendes Blatt wurde mit Anfrageterm erreicht. Hat die einzufuegende Regel dieselbe linke Seite, so wird
   sie in die Regelliste eingefuegt. Ansonsten muss ein neues Blatt erzeugt werden; und ein neuer bis zur Diffe-
   renzstelle linearer Unterbaum wird aufgebaut. Dessen letzter innerer Knoten hat das alte und das neue Blatt als
   einzige Nachfolger. Im alten Blatt muss der Termrest entsprechend geaendert werden.                             */

#define /*void*/ PListeVerlaengern(/*SprungeintragsT*/ Sprung)                                                      \
{                                                                                                                   \
  PraefixeintragsT NeuerListenkopf = BK_PraefixeintragAnlegen();                                                    \
  NeuerListenkopf->Sprungeintrag = Sprung;                                                                          \
  NeuerListenkopf->NaechsterPraefixeintrag = NeuePraefixliste;                                                      \
  Sprung->Praefixeintrag = NeuePraefixliste = NeuerListenkopf;                                                      \
}

#define /*void*/ AnzNoetigerVarAktualisierenMit(/*SymbolT*/ NeuesSymbol)  /* Es tritt je hoechstens eine Variable */\
  if (SO_VarNummer(NeuesSymbol) == AnzNoetigerVariablen)       /* mehr auf als im Vorgaengerknoten, und zwar gdw. */\
    AnzNoetigerVariablen++                         /* von dort mit dessen letzter Variable abgestiegen worden ist. */

{
  UTermT TermIndexAlt = BK_Rest(AltesBlatt), TermIndexNeu2 = TermIndexNeu;

  while (TermIndexAlt && (SO_SymbGleich(TO_TopSymbol(TermIndexAlt),AktuellesTopsymbol(TermIndexNeu)))) {
    TermIndexAlt = TO_Schwanz(TermIndexAlt);
    TermIndexNeu = TO_Schwanz(TermIndexNeu);
  }
  if (TermIndexAlt) {             /* Falls sich die linken Seiten noch unterscheiden, ist Aufteilung erforderlich. */
    UTermT TermIndexAlt2 = BK_Rest(AltesBlatt);              /* fuer Termzeiger bei neuen Spruengen ins alte Blatt */
    KantenT GleichPfad,GleichPfadEnde, NeuesBlatt;
    unsigned int AnzNoetigerVariablen = BK_AnzVariablen(BK_Vorg(AltesBlatt));
    unsigned int PositionImTerm2 = BK_SuffixPosition(AltesBlatt)+1;
    PraefixeintragsT NeuePraefixliste = NULL;

    IncAnzSchrumpfPfadExp();
    StapelParallelInitialisieren();
    AnzNoetigerVarAktualisierenMit(BK_VorgSymb(AltesBlatt));

    GleichPfad = GleichPfadEnde = BK_KnotenAnlegen(AnzNoetigerVariablen);  /* Jetzt linearen Baum aufbauen bis zur */

    while (TermIndexNeu2!=TermIndexNeu) {/* Differenzstelle der Regelreste, dabei Verwaltungsinformationen setzen. */
      SymbolT Symbol = TO_TopSymbol(TermIndexNeu2);
      KantenT NeuerKnoten;
      AnzNoetigerVarAktualisierenMit(Symbol);
      BK_NachfolgenLassen(GleichPfadEnde,Symbol,NeuerKnoten = BK_KnotenAnlegen(AnzNoetigerVariablen));
      BK_MinMaxSymbSei1(GleichPfadEnde,Symbol);     /* GleichPfadEnde zeigt auf das Ende des erzeugten Abschnitts. */

      /* Sprungausgaenge vom vorletzten und -eingaenge vom letzten Knoten bearbeiten */
      if (TO_NichtNullstellig(TermIndexNeu2)) {
        PushDoppelt(GleichPfadEnde,TermIndexNeu2,TermIndexAlt2);                          /* Sprungausgang stapeln */
      }
      else {
        if (SO_SymbIstFkt(Symbol)) {                                                              /* kurzer Sprung */
          SprungeintragSetzenOhnePraefixverweisNULL(GleichPfadEnde,NeuerKnoten,TermIndexNeu2,PositionImTerm2);
          PListeVerlaengern(BK_Sprungeingaenge(NeuerKnoten));
        }
        while (!(--TopZaehler)) {                    /* geht auch bei leerem Stapel nicht schief wegen Dummy bei 0 */
          SprungeintragSetzenVonPopOhnePraefixverweisNULL(NeuerKnoten,PositionImTerm2);
          PListeVerlaengern(BK_Sprungeingaenge(NeuerKnoten));
        }
      }

      GleichPfadEnde = NeuerKnoten;
      TermIndexNeu2 = TO_Schwanz(TermIndexNeu2);
      TermIndexAlt2 = TO_Schwanz(TermIndexAlt2);
      PositionImTerm2++;
    }

    /* Neues Blatt machen lassen. Praefixverweise sollen alle ins neue Blatt gehen. */
    Stapel2Sichern();  /* noetig, da noch einmal Stapel abzuarbeiten: fuer Spruenge neue innere Knoten-altes Blatt */
    NeuesBlatt = NeuesBlattEinhaengen(GleichPfadEnde,TermIndexNeu,Regel,NeuePraefixliste,EintragUngueltig,Baum,TiefeLinkerSeite);

    /* verschiedene Regularien */
    BK_NachfolgenLassen(GleichPfadEnde,TO_TopSymbol(TermIndexAlt),AltesBlatt);
    BK_RestSei(AltesBlatt,TO_Schwanz(TermIndexAlt));
    BK_MinMaxSymbSei2(GleichPfadEnde,TO_TopSymbol(TermIndexNeu),TO_TopSymbol(TermIndexAlt));

    AltesBlattPolieren(AltesBlatt,NeuesBlatt);                                 /* Ueberarbeitung des alten Blattes */
    NeueSpruengeInsAlteBlatt(AltesBlatt,TermIndexAlt2);                   /* Spruenge vom GleichPfad (aus Stapel2) */

    return GleichPfad;
  }

  else {                  /* Falls die Regeln dieselben linken Seiten haben, ist am Baum selbst nichts zu aendern. */
    RegelHinzufuegen(AltesBlatt,Regel);
    return AltesBlatt;
  }

#undef PListeVerlaengern
#undef AnzNoetigerVarAktualisierenMit
}



/*=================================================================================================================*/
/*= VII. Einfuegen von Termen in einen Baum                                                                       =*/
/*=================================================================================================================*/

void BO_ObjektEinfuegen(DSBaumT *Baum, GNTermpaarT Regel, unsigned int TiefeLinkerSeite)
{
  IncAnzEingefRegeln();                                                                   /* Statistik hochzaehlen */
  StapelInitialisieren();
  if (Baum->Wurzel) {                                                            /* Falls der Baum nicht leer ist: */
    KantenT BIndex = Baum->Wurzel;
    UTermT TermIndex = TP_LinkeSeite(Regel);

    PositionImTerm = 1;  /* Zaehler fuer Position in linker Seite initialisieren. Zaehlt eigentlich die Anzahl der
     bis dato umgewandelten Symbole aus der linken Seite, das ganze erhoeht um 1. Die Zaehlung der Symbole beginnt */

    do {                         /* mit 1+1 fuer das Topsymbol. Nun im Baum dem Anfrageterm entsprechend absteigen */
      SymbolT Topsymbol = AktuellesTopsymbol(TermIndex);
      KantenT BIndexNachf = BK_Nachf(BIndex,Topsymbol);

      if (BIndexNachf) {                                     /* Wenn es schon einen Nachfolger zum Topsymbol gibt: */
        if (BK_IstBlatt(BIndexNachf)) {                               /* ist ein Blatt -> dann das Blatt aufteilen */
          BK_NachfolgenLassen(BIndex,Topsymbol,
            BlattAufgeteilt(BIndexNachf,TO_Schwanz(TermIndex),Regel,Baum,TiefeLinkerSeite));
          break;
        }
        else {            /* Nachfolger ist innerer Knoten -> dann weiter absteigen; vorher Sprungstapel behandeln */
          if (TO_NichtNullstellig(TermIndex)) {  /* Sprungausgaenge anmelden, falls Teilterm mehrstellige Funktion */             
            Push(BIndex,TermIndex);                 /* Sprungausgaenge von Konstanten koennen ignoriert werden, da */
          }                                            /* sie in einen schon existierenden inneren Knoten fuehren. */
          else          /* Ansonsten kommen womoeglich Sprungeingaenge. Gehen in existierenden Knoten BIndexNachf. */
            while (!(--TopZaehler))                                                           /* Daher ignorieren. */
              Pop(); /* Diese Schleife terminiert. Der Wurzelterm kann nicht vom Stapel genommen werden, weil noch */
            /* ein weiterer innerer Knoten folgt. Demnach hat die linke Seite noch mindestens ein weiteres Symbol. */
            /* Der Wurzelterm liegt auch tatsaechlich auf dem Stapel, da nullstellige linke Seiten unbesehen in    */
            /* BlattAufgeteilt oder in BlattEinfuegen landen. */
          BIndex = BIndexNachf;                                                                 /* jetzt absteigen */
          TermIndex = TO_Schwanz(TermIndex);
        }
      }
      else {                       /* oder kein adaequater Nachfolger vorhanden ist -> dann neues Blatt einhaengen */
        /* Blatt mit den sich aus dem Stapel ergebenden Sprungeingaengen.
           Die Praefixliste ist leer, da kein neuer innerer Knoten erzeugt worden ist. */
        NeuesBlattEinhaengen(BIndex,TermIndex,Regel,NULL,EintragEins,Baum,TiefeLinkerSeite);
        BK_MinMaxSymbAktualisieren(BIndex,Topsymbol);
        break;
      }
    } while (TRUE);
  }

  else {             /* Falls der Baum bisher leer ist: einen neuen Knoten erzeugen und genau ein Blatt einhaengen */
    PositionImTerm = 2;                    /* notwendig, damit im neuen Blatt Suffixposition korrekt angegeben ist */
    /* Die Praefixliste des Blattes ist leer, da es noch keine Sprungverweise gibt. */
    NeuesBlattEinhaengen(Baum->Wurzel =  BK_KnotenAnlegen(1),TP_LinkeSeite(Regel),
                         Regel,NULL,EintragEins,Baum,TiefeLinkerSeite);
    /* Verwaltungsinformationen im Wurzelknoten setzen */
    BK_VorgSei(Baum->Wurzel,NULL);
    BK_VorgSymbSei(Baum->Wurzel,SO_KleinstesFktSymb);
    BK_MinMaxSymbSei1(Baum->Wurzel,TO_TopSymbol(TP_LinkeSeite(Regel)));
    BK_MVorgZelleSei(Baum->Wurzel,NULL);
    BK_UVorgSei(Baum->Wurzel,NULL);
  }
}



/*=================================================================================================================*/
/*= VIII. Ueberarbeitung von Sprung- und Praefixlisten beim Loeschen                                              =*/
/*=================================================================================================================*/

static void AlleEingehendenSpruengeLoeschen(KantenT LoeschBlatt)
{
  SprungeintragsT SprungIndex = BK_Sprungeingaenge(LoeschBlatt);
 
  /* Sprungeingaenge in das zu loeschende Blatt ausgangsseitig entfernen und freigeben*/
  while (SprungIndex) {
    SprungeintragsT NaechsterStarteintrag = SprungIndex->NaechsterStarteintrag;

    /* ausgangsseitig aus der Verkettung herausnehmen */
    if (SprungIndex->VorigerZieleintrag) {                                                      /* nicht am Anfang */
      if ((SprungIndex->VorigerZieleintrag->NaechsterZieleintrag = SprungIndex->NaechsterZieleintrag)) /* nicht am */
        SprungIndex->NaechsterZieleintrag->VorigerZieleintrag = SprungIndex->VorigerZieleintrag;           /* Ende */
    }
    else                                                                           /* nun Loeschen am Listenanfang */
      if (BK_SprungausgaengeSei(SprungIndex->Startknoten,SprungIndex->NaechsterZieleintrag))
        SprungIndex->NaechsterZieleintrag->VorigerZieleintrag = NULL;

    /* Sprungeintrag loeschen */
    BK_SprungeintragFreigeben(SprungIndex);

    SprungIndex = NaechsterStarteintrag;
  }
  /* N. B. Davon ausgehend, dass LoeschBlatt geloescht werden soll, erhaelt LoeschBlatt.Sprungeingaenge nicht
     erst noch den Wert NULL zugewiesen, was bei Nichtbeachtung zu Unklarheiten fuehren kann. */
}


static void SprunglistenBereinigenEinfach(KantenT LoeschBlatt, KantenT NachbarBlatt)
{
  PraefixeintragsT PraefixIndex = BK_Praefixliste(LoeschBlatt);
  PraefixeintragsT PraefixIndexVorg = NULL;

  while (!BK_IstBlatt(NachbarBlatt))
    NachbarBlatt = BK_Nachf(NachbarBlatt,BK_minSymb(NachbarBlatt));

  AlleEingehendenSpruengeLoeschen(LoeschBlatt);

  /* Umbiegen der Termverweise der in den Praefixlisten vermerkten Spruenge */
  while (PraefixIndex) {
    UTermT TermIndexLoesch  = TO_Schwanz(BK_linkeSeite(LoeschBlatt));
    UTermT TermIndexNachbar = TO_Schwanz(BK_linkeSeite(NachbarBlatt));

    while (TO_PhysischUngleich(TermIndexLoesch,PraefixIndex->Sprungeintrag->Teilterm)) {
      TermIndexLoesch  = TO_Schwanz(TermIndexLoesch);
      TermIndexNachbar = TO_Schwanz(TermIndexNachbar);
    }
    PraefixIndex->Sprungeintrag->Teilterm = TermIndexNachbar;
    PraefixIndex = (PraefixIndexVorg = PraefixIndex)->NaechsterPraefixeintrag;
  }

  if (PraefixIndexVorg) {                                          /* ueberarbeitete Praefixlisten zusammenhaengen */
    PraefixIndexVorg->NaechsterPraefixeintrag = BK_Praefixliste(NachbarBlatt);
    BK_PraefixlisteSei(NachbarBlatt,BK_Praefixliste(LoeschBlatt));
  }
}


static void AlleEingehendenSpruengeUmsetzen(KantenT Knoten, KantenT NachbarBlatt)
{
  SprungeintragsT SprungIndex = BK_Sprungeingaenge(Knoten);
  SprungeintragsT SprungIndexVorg = NULL;

  while (SprungIndex) {
    SprungIndex->Zielknoten = NachbarBlatt;                      /* Eingehende Spruenge ins Nachbarblatt umbiegen. */
    SprungIndex->Praefixeintrag = NULL;                                                          /*  Da der Sprung */
    SprungIndex = (SprungIndexVorg = SprungIndex)->NaechsterStarteintrag;       /* jetzt in ein Blatt fuehrt, hier */
  }                                                       /*  den Praefixverweis im Sprungeintrag auf NULL setzen. */
  if (SprungIndexVorg) {                            /* ueberarbeitete Sprungliste mit der alten des Nachbarblattes */
    SprungIndexVorg->NaechsterStarteintrag = BK_Sprungeingaenge(NachbarBlatt);   /* eingangsseitig zusammenhaengen */
    BK_SprungeingaengeSei(NachbarBlatt,BK_Sprungeingaenge(Knoten));
  }
}


static void AlleAusgehendenSpruengeNullifizieren(KantenT Knoten)
{                                                              /* Die Spruenge gehen von Knoten nach NachbarBlatt. */
  SprungeintragsT SprungIndex = BK_Sprungausgaenge(Knoten);
 
  /* Sprungausgaenge vom zu loeschenden Knoten eingangsseitig markieren */
  while (SprungIndex) {
    SprungIndex->Zielknoten = NULL;
    SprungIndex = SprungIndex->NaechsterZieleintrag;
  }
}


static void NullifizierteEingehendeSpruengeLoeschen(KantenT NachbarBlatt)
{
  SprungeintragsT SprungIndex = BK_Sprungeingaenge(NachbarBlatt);
  SprungeintragsT SprungIndexVorg = NULL;

  /* Markierte Sprungeingaenge in das Nachbarblatt entfernen */
  while (SprungIndex)
    if (SprungIndex->Zielknoten)
      SprungIndex = (SprungIndexVorg = SprungIndex)->NaechsterStarteintrag;
    else {
      SprungeintragsT Loeschzeiger = SprungIndex;
      SprungIndex = SprungIndex->NaechsterStarteintrag;
      BK_SprungeintragFreigeben(Loeschzeiger);
      if (SprungIndexVorg)
        SprungIndexVorg->NaechsterStarteintrag = SprungIndex;
      else
        BK_SprungeingaengeSei(NachbarBlatt,SprungIndex);
    }
}


static void PraefixlistenKorrigieren(KantenT LoeschBlatt, KantenT NachbarBlatt)
{
  PraefixeintragsT PraefixIndex = BK_Praefixliste(NachbarBlatt);
  PraefixeintragsT PraefixIndexVorg = NULL;


  /* Ueberfluessige Eintraege der Praefixliste des NachbarBlatts entfernen */

  while (PraefixIndex)
    if (PraefixIndex->Sprungeintrag->Zielknoten &&                /* abfragen, ob Sprung ueberhaupt noch existiert */
        PraefixIndex->Sprungeintrag->Zielknoten!=NachbarBlatt)          /* und nicht ins verbleibende Blatt fuehrt */
      PraefixIndex = (PraefixIndexVorg = PraefixIndex)->NaechsterPraefixeintrag;   /* dann Praefixeintrag belassen */
    else {                                        /* ansonsten den Eintrag aus der Liste herausnehmen und loeschen */
      PraefixeintragsT Loeschzeiger = PraefixIndex;
      PraefixIndex = PraefixIndex->NaechsterPraefixeintrag;
      BK_PraefixeintragFreigeben(Loeschzeiger);
      if (PraefixIndexVorg)
        PraefixIndexVorg->NaechsterPraefixeintrag = PraefixIndex;
      else
        BK_PraefixlisteSei(NachbarBlatt,PraefixIndex);
    }


  /* Eintraege der Praefixliste des LoeschBlatts untersuchen:
     die zu nicht mehr existierenden Spruengen beseitigen, alle anderen termverweismaessig aktualisieren;
     fuer Spruenge, die jetzt in das NachbarBlatt gehen, die Praefixliste kuerzen */

  PraefixIndex = BK_Praefixliste(LoeschBlatt);
  PraefixIndexVorg = NULL;

  while (PraefixIndex) {

    if (PraefixIndex->Sprungeintrag->Zielknoten) {/* Umsetzen der Termverweise bei weiterhin bestehenden Spruengen */
      UTermT TermIndexLoesch  = TO_Schwanz(BK_linkeSeite(LoeschBlatt));
      UTermT TermIndexNachbar = TO_Schwanz(BK_linkeSeite(NachbarBlatt));
      while (TO_PhysischUngleich(TermIndexLoesch,PraefixIndex->Sprungeintrag->Teilterm)) {
        TermIndexLoesch  = TO_Schwanz(TermIndexLoesch);
        TermIndexNachbar = TO_Schwanz(TermIndexNachbar);
      }
      PraefixIndex->Sprungeintrag->Teilterm = TermIndexNachbar;
    }

    if (!PraefixIndex->Sprungeintrag->Zielknoten || PraefixIndex->Sprungeintrag->Zielknoten==NachbarBlatt) {
      PraefixeintragsT Loeschzeiger = PraefixIndex;                   /* Loeschen von unnoetigen Praefixeintraegen */
      PraefixIndex = PraefixIndex->NaechsterPraefixeintrag;
      BK_PraefixeintragFreigeben(Loeschzeiger);
      if (PraefixIndexVorg)
        PraefixIndexVorg->NaechsterPraefixeintrag = PraefixIndex;
      else
        BK_PraefixlisteSei(LoeschBlatt,PraefixIndex);
    }
    else
      PraefixIndex = (PraefixIndexVorg = PraefixIndex)->NaechsterPraefixeintrag;
  
  }


  /* Zusammenfuegen der beiden Praefixlisten */
  if (PraefixIndexVorg) {
    PraefixIndexVorg->NaechsterPraefixeintrag = BK_Praefixliste(NachbarBlatt);
    BK_PraefixlisteSei(NachbarBlatt,BK_Praefixliste(LoeschBlatt));
  }
}


static void AllenSprungkramLoeschen(KantenT LoeschBlatt, KantenT NachbarBlatt)
{
  PraefixeintragsT PraefixIndex = BK_Praefixliste(NachbarBlatt);

  AlleEingehendenSpruengeLoeschen(NachbarBlatt);
  BK_SprungeingaengeSei(NachbarBlatt,NULL);                        /* vgl. Anm. in AlleEingehendenSpruengeLoeschen */
  while (PraefixIndex) {
    PraefixeintragsT Loeschzeiger = PraefixIndex;
    PraefixIndex = PraefixIndex->NaechsterPraefixeintrag;
    BK_PraefixeintragFreigeben(Loeschzeiger);
  }
  PraefixIndex = BK_Praefixliste(LoeschBlatt);
  while (PraefixIndex) {
    PraefixeintragsT Loeschzeiger = PraefixIndex;
    PraefixIndex = PraefixIndex->NaechsterPraefixeintrag;
    BK_PraefixeintragFreigeben(Loeschzeiger);
  }
}

  
  

/*=================================================================================================================*/
/*= IX. Loeschen von Termen aus einem Baum                                                                        =*/
/*=================================================================================================================*/

static void BlattAuszeigernUndFreigeben(KantenT Blatt, DSBaumT *Baum)
#define /*Rumpf*/ ObjektAuszeigern(/*Objekt*/ Blatt, /*Objekttyp*/ KantenT,                                         \
  /*Vorgaenger lesen*/ BK_VorgBlatt, /*Nachfolger lesen*/ BK_NachfBlatt,                                            \
  /*Vorgaenger ueberschreiben*/ BK_VorgBlattSei, /*Nachfolger ueberschreiben*/ BK_NachfBlattSei,                    \
  /*Echter Kantentyp im Baum*/ EchterKantenT, /*Zugriff auf linken Term*/ BK_linkeSeite)                            \
{                                                                                                                   \
  unsigned int TiefeLoeschblatt = TO_Termtiefe(BK_linkeSeite(Blatt));                                               \
  KantenT Vorg = BK_VorgBlatt(Blatt),                                                                               \
          Nachf = BK_NachfBlatt(Blatt);                                                                             \
                                                                                                                    \
                                                                                                                    \
  /* 1. Aktualisieren des Feldes TiefeBlaetter und des Eintrages maxTiefeLinks */                                   \
                                                                                                                    \
  if ((KantenT) Baum->TiefeBlaetter[TiefeLoeschblatt] == Blatt) {                                                   \
    if (Nachf) {                                                      /* 1. Es existiert ein nachfolgendes Blatt. */\
      Baum->TiefeBlaetter[TiefeLoeschblatt] =                    /* Eintrag im Verweisfeld umbiegen oder loeschen */\
        TiefeLoeschblatt==TO_Termtiefe(BK_linkeSeite(Nachf)) ? (EchterKantenT) Nachf : NULL;                        \
    }                                                                                                               \
    else {                                                           /* 2. Es existiert kein nachfolgendes Blatt. */\
      Baum->maxTiefeLinks = Vorg ? TO_Termtiefe(BK_linkeSeite(Vorg)) : 0;                                           \
      Baum->TiefeBlaetter[TiefeLoeschblatt] = NULL;                                                                 \
    }                                                                                                               \
  }                                                                                                                 \
                                                                                                                    \
  /* 2. Aktualisieren des Eintrages ErstesBlatt und der Verkettung der Blaetter untereinander */                    \
                                                                                                                    \
  if (Nachf)                                                                                                        \
    BK_VorgBlattSei(Nachf,Vorg);                                                                                    \
  if (Vorg)                                                                                                         \
    BK_NachfBlattSei(Vorg,Nachf);                                                                                   \
  else                                                                                                              \
    Baum->ErstesBlatt = (EchterKantenT) Nachf;                                                                      \
}
{
  ObjektAuszeigern(/*Objekt*/ Blatt, /*Objekttyp*/ KantenT,
  /*Vorgaenger lesen*/ BK_VorgBlatt, /*Nachfolger lesen*/ BK_NachfBlatt,
  /*Vorgaenger ueberschreiben*/ BK_VorgBlattSei, /*Nachfolger ueberschreiben*/ BK_NachfBlattSei,
  /*Echter Kantentyp im Baum*/ KantenT, /*Zugriff auf linken Term*/ BK_linkeSeite)
  BK_BlattFreigeben(Blatt);
}  


void BO_ObjektEntfernen(DSBaumT *Baum, GNTermpaarT Regel)
{
  KantenT BaumIndex;

  IncAnzEntfernterRegeln();                                                            /* Statistik hochzaehlen */
  if (Regel->Nachf)     /* Loeschen aus Liste mit mehr als einem Element, nicht am Ende-> Baum bleibt unveraendert */
    if ((Regel->Nachf->Vorg = Regel->Vorg))               /* Vorgaengerzeiger umbiegen, falls Vorgaenger existiert */
      Regel->Vorg->Nachf = Regel->Nachf;
    else                        /* ansonsten wird erstes Element geloescht -> Blatt-Zeiger auf Liste aktualisieren */
      BK_RegelnSei(Regel->Blatt,Regel->Nachf);
  else if (Regel->Vorg) {                       /* Loeschen am Ende der Regelliste; Liste hat mehr als ein Element */
    Regel->Vorg->Nachf = NULL;             /* folglich Zeiger fuer linke Seiten umbiegen (vgl. Erlaeuterung zu IV) */
    Regel->linkeSeite = Regel->Vorg->linkeSeite;
    Regel->Vorg->linkeSeite = BK_linkeSeite(Regel->Blatt);
  }                                      /* alle folgenden Faelle: Loeschen aus Regelliste mit genau einem Element */
  else if ((!BK_Vorg(BaumIndex = BK_Vorg(Regel->Blatt))) && BK_genauEinNachf(BaumIndex)) {  /* Loeschen aus direkt */
          /* der Wurzel folgendem Blatt ist, wenn die Wurzel nur einen Nachfolger hat, Loeschen des ganzen Baumes. */
    BlattAuszeigernUndFreigeben(Regel->Blatt,Baum);    /* Jetzt Blatt herausnehmen aus  Verzeigerung der Blaetter. */
    BK_KnotenFreigeben(Baum->Wurzel);
    Baum->Wurzel = NULL;
  }
  else {                                                       /* Falls der Baum nun mehr als eine Regel enthaelt: */
    SymbolT VorgSymbol = BK_VorgSymb(Regel->Blatt);
    KantenT NachbarBlatt;
    BK_NachfSei(BaumIndex,VorgSymbol,NULL);                                          /* Blatt aus Knoten austragen */
    if (SO_SymbGleich(VorgSymbol,BK_minSymb(BaumIndex))) {
      /* Reorganisation kann nur dann erforderlich sein, wenn der vorhergehende Knoten genau zwei Nachfolger be-
         sitzt. (Wegen der Baumstruktur gibt es mindestens zwei.) Dann muss aus jenem Knoten mit dessen kleinstem
         oder dessen groesstem Symbol abgestiegen worden sein.
         Wurde mit dem kleinstem Symbol abgestiegen, so gibt es nur dann genau zwei Nachfolger, wenn die Nachfol-
         ger von Symbol+1 bis MaxSymb alle NULL sind.                                                              */
      SymbolT SymbolIndex;
      SO_forSymboleAufwaerts(SymbolIndex,SO_VorigesVarSymb(VorgSymbol),SO_NaechstesVarSymb(BK_maxSymb(BaumIndex)))
        if (BK_Nachf(BaumIndex,SymbolIndex)) {    /* Wenn es doch einen zusaetzlichen Nachfolger gibt, so muss nur */
          BK_minSymbSei(BaumIndex,SymbolIndex);          /* der Eintrag fuer das kleinste Symbol geaendert werden. */
          SprunglistenBereinigenEinfach(Regel->Blatt,BK_Nachf(BaumIndex,SymbolIndex));
          BlattAuszeigernUndFreigeben(Regel->Blatt,Baum);      /* Blatt herausnehmen aus Verzeigerung der Blaetter */
          return;
        }                            /* Wenn es genau zwei Nachfolger gibt, so wird der andere Nachfolger fixiert. */ 
      NachbarBlatt = BK_Nachf(BaumIndex,BK_maxSymb(BaumIndex));
    }
    else if (SO_SymbGleich(VorgSymbol,BK_maxSymb(BaumIndex))) {                     /* analog mit groesstem Symbol */
      SymbolT SymbolIndex;
      SO_forSymboleAbwaerts(SymbolIndex,SO_NaechstesVarSymb(VorgSymbol),SO_VorigesVarSymb(BK_minSymb(BaumIndex)))
        if (BK_Nachf(BaumIndex,SymbolIndex)) {
          BK_maxSymbSei(BaumIndex,SymbolIndex);
          SprunglistenBereinigenEinfach(Regel->Blatt,BK_Nachf(BaumIndex,SymbolIndex));
          BlattAuszeigernUndFreigeben(Regel->Blatt,Baum);      /* Blatt herausnehmen aus Verzeigerung der Blaetter */
          return;
        }
      NachbarBlatt = BK_Nachf(BaumIndex,BK_minSymb(BaumIndex));
    }
    else {/* Wenn das Symbol, mit dem abgestiegen wurde, weder das kleinste noch das groesste ist, so gibt es mehr */
      SprunglistenBereinigenEinfach(Regel->Blatt,BK_Nachf(BaumIndex,BK_minSymb(BaumIndex)));
      BlattAuszeigernUndFreigeben(Regel->Blatt,Baum);          /* Blatt herausnehmen aus Verzeigerung der Blaetter */
      return;              /* als zwei Nachfolger, und Min- sowie Max-Symb koennen ihre bisherigen Werte behalten. */
    }

    /* Noch zu befassende Situation: Der vorhergehende Knoten (Variable BaumIndex) hat genau zwei Nachfolger,
       naemlich Regel->Blatt und NachbarBlatt, von denen der erste allerdings schon entfernt wurde. Fuer den Fall,
       dass der andere Nachfolger (unbeschadet des Namens der auf ihn zeigenden Variable) kein Blatt ist, ist
       durch das Loeschen keine Einwegverzweigung zu einem Blatt entstanden und ausser der Anpassung von MinSymb
       und MaxSymb nichts zu unternehmen (else-Zweig). Ansonsten ist Aufstieg erforderlich bis zum naechsten Kno-
       tem mit mehr als einem Nachfolger bzw. bis zur Wurzel, wenn ein solcher nicht existiert. In den so erreich-
       ten Knoten ist das Blatt jetzt einzuhaengen, ausserdem der Eintrag Regelrest anzupassen.                    */

    if (BK_IstBlatt(NachbarBlatt)) {                    /* Anderer Nachfolger ist Blatt -> Schrumpfen erforderlich */
      UTermT TermIndex  = BK_linkeSeite(NachbarBlatt);
      unsigned int DeltaSuffixPosition = 0;

      IncAnzPfadschrumpfungen();                                                          /* Statistik hochzaehlen */
      AlleEingehendenSpruengeLoeschen(Regel->Blatt);                                          /* wie der Name sagt */

      do {             /* mit TermIndex fuer jeden aufgestiegenen Knoten um ein Symbol in linkeSeite weiterzaehlen */
        KantenT BaumIndexNachf = BaumIndex;
        VorgSymbol = BK_VorgSymb(BaumIndex);                           /* fuer Verkettung Vorgaenger-Symbol merken */
        if (!(BaumIndex = BK_Vorg(BaumIndex))) {                       /* Falls bis zur Wurzel aufgestiegen wurde: */
          BK_NachfolgenLassen(BaumIndexNachf,VorgSymbol = TO_TopSymbol(BK_linkeSeite(NachbarBlatt)),NachbarBlatt);
          BK_RestSei(NachbarBlatt,TO_Schwanz(BK_linkeSeite(NachbarBlatt)));    /* Blatt dort einhaengen, Regelrest */
          BK_SuffixPositionSei(NachbarBlatt,2);                                              /* und Suffixposition */
          BK_MinMaxSymbSei1(BaumIndexNachf,VorgSymbol);     /* aendern, MinMaxSymb aendern. Offensichtlich hat die */
          AllenSprungkramLoeschen(Regel->Blatt,NachbarBlatt);  /* Wurzel jetzt nur noch einen einzigen Nachfolger. */
          BlattAuszeigernUndFreigeben(Regel->Blatt,Baum);      /* Blatt herausnehmen aus Verzeigerung der Blaetter */
          return;
        }
        AlleAusgehendenSpruengeNullifizieren(BaumIndexNachf);   /* Spruenge von zu loeschendem Knoten ins Nachbar-
                   blatt markieren, jedoch noch nicht loeschen. Grund: Eingangsseitige Verkettung ist nur einfach. */
        AlleEingehendenSpruengeUmsetzen(BaumIndexNachf,NachbarBlatt);
        BK_KnotenFreigeben(BaumIndexNachf);  /* zurueckdurchstiegenen Knoten freigeben und TermIndex weiterzaehlen */
        TermIndex = TO_Schwanz(TermIndex);          /* Abbruch, wenn Knoten mit mehr als einem Nachfolger erreicht */
        DeltaSuffixPosition++;
      } while (BK_genauEinNachf(BaumIndex));     /* do-Schleife, da mindestens Blatt-Vorgaengerknoten zu entfernen */
      BK_NachfolgenLassen(BaumIndex,VorgSymbol,NachbarBlatt);     /* anderes Blatt in so erreichten Knoten haengen */
      {     /* Parallel TermIndex weiterzaehlen, bis Anfang vom alten Rest erreicht, und TermIndex2 vom Anfang von */
        UTermT TermIndex2 = BK_linkeSeite(NachbarBlatt);  /* linkeSeite aus weiterzaehlen. TermIndex2 haengt um so */
        do       /* viele Termzellen nach, wie Knoten nach oben durchstiegen wurden, ist dann also gueltiger Rest. */
          TermIndex2 = TO_Schwanz(TermIndex2);
        while (TO_PhysischUngleich(TermIndex = TO_Schwanz(TermIndex),BK_Rest(NachbarBlatt)));   
        BK_RestSei(NachbarBlatt,TermIndex2);
        BK_SuffixPositionSei(NachbarBlatt,BK_SuffixPosition(NachbarBlatt)-DeltaSuffixPosition);
      }
      PraefixlistenKorrigieren(Regel->Blatt,NachbarBlatt);
      NullifizierteEingehendeSpruengeLoeschen(NachbarBlatt);
      BlattAuszeigernUndFreigeben(Regel->Blatt,Baum);          /* Blatt herausnehmen aus Verzeigerung der Blaetter */
    }
    else {                              /* Anderer Nachfolger ist kein Blatt -> kein Schrumpfen erforderlich, s.o. */
      BK_MinMaxSymbSei1(BaumIndex,BK_VorgSymb(NachbarBlatt));
      SprunglistenBereinigenEinfach(Regel->Blatt,NachbarBlatt);
      BlattAuszeigernUndFreigeben(Regel->Blatt,Baum);          /* Blatt herausnehmen aus Verzeigerung der Blaetter */
    }
  }
}



/*=================================================================================================================*/
/*= X. Initialisierung und Statistik                                                                              =*/
/*=================================================================================================================*/

void BO_DSBaumInitialisieren(DSBaumT *Baum)
{
  unsigned int Index = 
               Baum->TiefeBlaetterDimension = BK_AnzahlErsterTieferBlaetter;
  Baum->Wurzel = Baum->ErstesBlatt = NULL;
  Baum->maxTiefeLinks = 0;
  Baum->TiefeBlaetter = ((KantenT *)our_alloc(BK_AnzahlErsterTieferBlaetter*sizeof(KantenT)))-1;
  do
    Baum->TiefeBlaetter[Index] = NULL;
  while (--Index);
}

void BO_InitApriori(void)
{
  if (xO == NULL) {
    xO = (SymbolT *) our_alloc(BO_AnzErsterVariablen*sizeof(SymbolT)) + BO_AnzErsterVariablen;
    xODimension = -BO_AnzErsterVariablen;
    Sprungstapel = array_alloc((StapelDimension = BO_ErsteSprungstapelhoehe)+1,StapeleintragsT);
  }
  BO_NeuesteBaumvariable = SO_VorErstemVarSymb; 
  BO_MaximaleTiefe = 0;
}


static unsigned int Knotenzahl(KantenT Knoten)
{
  if (!Knoten)
    return 0;
  else if (BK_IstBlatt(Knoten))
    return 1;
  else {
    unsigned int Summe = 1;
    SymbolT SymbolIndex;
    SO_forSymboleAufwaerts(SymbolIndex,BK_minSymb(Knoten),BK_maxSymb(Knoten))
      Summe += Knotenzahl(BK_Nachf(Knoten,SymbolIndex));
    return Summe;
  }
}


static unsigned int AnzahlGesparterKnoten(DSBaumT Baum)
{
  unsigned int Summe = 0;
  KantenT BlattIndex;
  BK_forBlaetter(BlattIndex,Baum)
    Summe += TO_Termlaenge(BK_linkeSeite(BlattIndex))+1-BK_SuffixPosition(BlattIndex);
  return Summe;
}


void BO_KnotenZaehlen(unsigned int *KnotenImBaum, unsigned int *KnotenGespart, DSBaumT Baum)
{
  if (Baum.Wurzel) {
    *KnotenImBaum = Knotenzahl(Baum.Wurzel);
    *KnotenGespart = AnzahlGesparterKnoten(Baum);
  }
  else
    *KnotenImBaum = *KnotenGespart = 0;
}
