/*=================================================================================================================
  =                                                                                                               =
  =  TermOperationen.c                                                                                            =
  =                                                                                                               =
  =  Definition des Datentyps Term und Realisation elementarer Operationen                                        =
  =                                                                                                               =
  =  02.03.94 Thomas.                                                                                             =
  =  01.02.95 Thomas. Rekursionen eliminiert.                                                                     =
  =                                                                                                               =
  =================================================================================================================*/

#include "Ausgaben.h"
#include "DSBaumOperationen.h"
#include "FehlerBehandlung.h"
#include "general.h"

#ifndef CCE_Source
#include "LispLists.h"
#include "Parameter.h"
#include "PhilMarlow.h"
#include "SpezNormierung.h"
#else
#include "parse-util.h"
#endif

#include "SpeicherVerwaltung.h"
#include <stdarg.h>
#include <string.h>
#include "TermOperationen.h"
#include "TermIteration.h"
#include "RUndEVerwaltung.h"

TI_IterationDeklarieren(TO)


/*=================================================================================================================*/
/*= I. Typdeklarationen und Modulvariablen                                                                        =*/
/*=================================================================================================================*/

SV_NestedSpeicherManagerT TO_FreispeicherManager;
static SV_NestedSpeicherManagerT to_FrischzellenManager;
TermT *TO_Variablenterm;
static int VTermeDimension;

#ifndef CCE_Source
typedef struct VLZellenTS *VLZellenZeigerT;
typedef struct VLZellenTS {
  SymbolT VIntern;
  term VExtern;
  VLZellenZeigerT Vorg;
} VLZellenT;
static SV_SpeicherManagerT VLZellenSpeicherManager;
#endif

BOOLEAN TO_KombinatorapparatAktiviert;


/*=================================================================================================================*/
/*= II. Erzeugen,Besetzen und Loeschen von Termzellen                                                             =*/
/*=================================================================================================================*/

static TermT NullstelligBeschrieben(TermT t, SymbolT Symbol)
{
  TO_SymbolBesetzen(t,Symbol);
  TO_NachfBesetzen(t,NULL);
  TO_EndeBesetzen(t,t);
  return t;
}  


static TermT NullstelligerTermM(SymbolT Symbol)
{ 
  return NullstelligBeschrieben(struct_alloc(TermzellenT),Symbol);
}


TermT TO_NullstelligerTerm(SymbolT Symbol)
{
  TermT t;
  TO_TermzelleHolen(t);
  return NullstelligBeschrieben(t,Symbol);
}


TermT TO_(SymbolT Symbol, ...)                            /* ACHTUNG: Variablenzahl koennte Grenze ueberschreiten! */
{
  if (SO_Stelligkeit(Symbol)) {
    TermT TermNeu = TO_HoleErsteNeueZelle(),
          IndexNeu = TermNeu;
    va_list ap;
    TO_SymbolBesetzen(TermNeu,Symbol);
    TI_Push(TermNeu);
    va_start(ap,Symbol);
    do {
      TO_AufNaechsteNeueZelle(IndexNeu);
      Symbol = (SymbolT)(va_arg(ap,int));
      TO_SymbolBesetzen(IndexNeu,Symbol);
      if (SO_Stelligkeit(Symbol))
        TI_Push(IndexNeu);
      else {
        TO_EndeBesetzen(IndexNeu,IndexNeu);
        while (!(--TI_TopZaehler)) {
          TO_EndeBesetzen(TI_TopTeilterm,IndexNeu);
          if (TI_PopUndLeer()) {
            TO_LetzteNeueZelleMelden(IndexNeu);
            TO_NachfBesetzen(IndexNeu,NULL);
            va_end(ap);
            return TermNeu;
          }
        }
      }
    } while (TRUE);
  }
  else {
    TermT TermNeu;
    TO_TermzelleHolen(TermNeu);
    TO_SymbolBesetzen(TermNeu,Symbol);
    TO_NachfBesetzen(TermNeu,NULL);
    TO_EndeBesetzen(TermNeu,TermNeu);
    return TermNeu;
  }
}



/*=================================================================================================================*/
/*= IV. Einfache Praedikate auf Termen                                                                            =*/
/*=================================================================================================================*/


BOOLEAN TO_TermeGleich(UTermT Term1, UTermT Term2)
/* Prueft, ob UTerme Term1, Term2 denselben Praefix haben. Der Vergleich erfolgt von links nach rechts Symbol fuer
   Symbol. Abbruch, falls unterschiedliche Symbole oder Term1 abgearbeitet. Dass Term2 abgearbeitet ist und die-
   selben Symbole hat wie Term1 und Term1 noch nicht abgearbeitet ist, kommt wegen der Termstruktur nicht vor.     */
{
  TermzellenZeigerT Ende1;

  Ende1 = TO_NULLTerm(Term1);
  while (SO_SymbGleich(TO_TopSymbol(Term1),TO_TopSymbol(Term2))) {
    if (TO_PhysischGleich(Term1 = TO_Schwanz(Term1),Ende1))
      return TRUE;
    Term2 = TO_Schwanz(Term2);
  }
  return FALSE;
}


static BOOLEAN TopsymbolKorrekt(UTermT Term)
{
  SymbolT Symbol = TO_TopSymbol(Term);
  if (!(2*BO_NeuesteBaumvariable()<=Symbol && Symbol<=SO_GroesstesFktSymb)) {
    printf("\nFehler: Unzulaessiges Symbol: %d.\n", Symbol);
    return FALSE;
  }
  return TRUE;
}


BOOLEAN TO_TermKorrekt(TermT Term)                                       /* Prueft Termdarstellung auf Konsistenz. */
{                                                     /* Nur fuer Terme aus Regeln, Gleichungen, Kritischen Paaren */
  unsigned int TI_KSIndexAlt = TI_KSIndex;

  if (TO_NULLTerm(Term)) {
    printf("\nFehler: Beschnittener Term hat am Ende Nachfolger.\n");
    return FALSE;
  }
  if (!TopsymbolKorrekt(Term)) {
    printf("\nFehler: Term hat unzulaessiges Topsymbol.\n");
    return FALSE;
  }
  if (TO_Nullstellig(Term)) {
    if (TO_TermEnde(Term)!=Term) {
      printf("\nFehler: Ende-Zeiger von Konstanter weist nicht auf Konstante.\n");
      return FALSE;
    }
    return TRUE;
  }
  TI_Push(Term);
  do {
    if (!(Term = TO_Schwanz(Term))) {
      printf("\nFehler: Term endet vorzeitig.\n");
      TI_KSIndex = TI_KSIndexAlt;
      return FALSE;
    }
    if (!TopsymbolKorrekt(Term)) {
      printf("\nFehler: Term hat im Inneren unzulaessiges Symbol.\n");
      TI_KSIndex = TI_KSIndexAlt;
      return FALSE;
    }
    if (TO_Nullstellig(Term)) {
      if (TO_TermEnde(Term)!=Term) {
        printf("\nFehler: Ende-Zeiger von nullstelligem Teilterm weist nicht auf diesen Teilterm.\n");
        TI_KSIndex = TI_KSIndexAlt;
        return FALSE;
      }
      while (!(--TI_TopZaehler)) {
        if (Term!=TO_TermEnde(TI_TopTeilterm)) {
          printf("\nFehler: Ende-Zeiger weist nicht auf tatsaechliches (Teilterm-)Ende.\n");
          TI_KSIndex = TI_KSIndexAlt;
          return FALSE;
        }
        if (TI_PopUndLeer())
          return TRUE;
      }
    }
    else
      TI_Push(Term);
  } while (TRUE);
}

/* Frueher wurde implementiert |-|(n) := ={f(x_)=f(pi(x_)): x_ aus V^n, pi aus PI(n)} auf Term(F*,V)
   (repraesentiert durch Term(F,V)/=A mit rechtsgeklammerten Repraesentanten in Term(F,V))
   wobei in F* das Funktionssymbol f variable Stelligkeit hat, die f-Terme abgeflacht sein
   muessen und auf einen k-stelligen f-Term nur die Permutationen vom Grad k anwendbar sind,
   und damit s=(n)t := s =perm t  und nicht  s (|-|1 U ... U |-|n-1)+ t. */


BOOLEAN TO_HomoeomorphDrin(UTermT s, UTermT t)
{  /* Ist s homoeomorph in t eingebettet? Das ganze inklusive Aequivalenzanteil (==syntaktischer Gleichheit). */
  if (TO_TermIstVar(s)) {
    SymbolT x = TO_TopSymbol(s);
    UTermT NULLt = TO_NULLTerm(t);
    do {
      if (SO_SymbGleich(x,TO_TopSymbol(t)))
        return TRUE;
    } while (TO_PhysischUngleich(t = TO_Schwanz(t),NULLt));
    return FALSE;
  }
  else if (TO_TermIstVar(t))
    return FALSE;
  else {
      UTermT NULLt = TO_NULLTerm(t),
             j = TO_ErsterTeilterm(t);

    /* alpha */
    if TO_TopSymboleGleich(s,t) {
      UTermT i = TO_ErsterTeilterm(s);
      while (TO_PhysischUngleich(j,NULLt)) {
        if (!TO_HomoeomorphDrin(i,j))
          break;
        i = TO_NaechsterTeilterm(i);
        j = TO_NaechsterTeilterm(j);
      }
      if (TO_PhysischGleich(j,NULLt))
        return TRUE;
      j = TO_ErsterTeilterm(t);
    }

    /* beta */
    while (TO_PhysischUngleich(j,NULLt)) {
      if (TO_HomoeomorphDrin(s,j))
        return TRUE;
      j  = TO_NaechsterTeilterm(j);
    }

    return FALSE;
  }
}


BOOLEAN TO_SymbolEnthalten(SymbolT f, UTermT t)
{ /* Ist die f-Laenge von t positiv? */
  SymbolT g;
  TO_doSymbole(t, g, {
    if (SO_SymbGleich(f,g))
      return TRUE;
  });
  return FALSE;
}

unsigned int TO_Symbollaenge(SymbolT f, UTermT t)
{ /* Wie oft ist f in t enthalten? */
  unsigned int n = 0;
  SymbolT g;
  TO_doSymbole(t, g, {
    if (SO_SymbGleich(f,g))
      n++;
  });
  return n;
}

BOOLEAN TO_VarGroesserGleich(UTermT s, UTermT t)
{ /* Ist Var(s) Obermenge von Var(t)? -- Effizientere Basisoperationen dafuer in VariablenMengen.h! */
  SymbolT x;
  TO_doSymbole(t, x, {
    if (SO_SymbIstVar(x) && !TO_SymbolEnthalten(x,s))
      return FALSE;
  });
  return TRUE;
}


BOOLEAN TO_DefinierendeGleichung(UTermT s_, UTermT t_)
{ /* Ist s=t eine definierende Gleichung?
     Gdw. s==f(x(i1), ..., x(in)); |t|(f)=0; Var(s)>=Var(t)
     Zwecks Variablennormierung wird nur auf Kopien von s und t gearbeitet. */
  if (TO_TermIstVar(s_))
    return FALSE;
  else {
    TermT s = TO_Termkopie(s_),
          t = TO_Termkopie(t_);
    BOOLEAN Resultat = TRUE;
    UTermT si;
    SymbolT f = TO_TopSymbol(s);
    unsigned int i=1;
    BO_TermpaarNormierenAlsKP(s,t);
    TO_forTeilterme(s, si, {
      SymbolT fi = TO_TopSymbol(si);
      SymbolT xi = SO_NummerVar(i++);
      if (SO_SymbUngleich(fi,xi)) {
        Resultat = FALSE;
        break;
      }
    });
    Resultat = Resultat && !TO_SymbolEnthalten(f,t) && TO_VarGroesserGleich(s,t);
    TO_TermeLoeschen(s,t);
    return Resultat;
  }
}

BOOLEAN TO_TermEnthaeltFVar(TermT t)
{
  do
    if (SO_SymbIstFVar(TO_TopSymbol(t)))
      return TRUE;
  while ((t = TO_Schwanz(t)));
  return FALSE;
}


BOOLEAN TO_PhysischTeilterm(UTermT si, TermT s)
{
  UTermT NULLs = TO_NULLTerm(s);
  do
    if (TO_PhysischGleich(si,s))
      return TRUE;
  while (TO_PhysischUngleich(s = TO_Schwanz(s),NULLs));
  return FALSE;
}


BOOLEAN TO_TermIstGrund(TermT t)
{
  SymbolT f;
  TO_doSymbole(t, f, {
    if (SO_SymbIstVar(f))
      return FALSE;
  });
  return TRUE;
}


UTermT TO_ElterntermVonIn(UTermT t, UTermT s)
{
  UTermT s0 = NULL;
  while (TO_PhysischUngleich(t,s)) {           /* Beachte: s ist komplexer Term f(s1..s(n+1)). Suchen t in s1..sn. */
    UTermT sNULL = TO_NULLTerm(s);
    s0 = s, s = TO_ErsterTeilterm(s);
    while (TO_PhysischUngleich(TO_NaechsterTeilterm(s),sNULL) && !TO_PhysischTeilterm(t,s))
      s = TO_NaechsterTeilterm(s);
  }
  return s0;
}



/*=================================================================================================================*/
/*= VI. Funktionen zum Kopieren                                                                                   =*/
/*=================================================================================================================*/

/* ------- 1. Allgemeine Kopierfunktion -------------------------------------------------------------------------- */

TermT TO_TermkopieXX(UTermT TermAlt)
{                               /* Erzeugt Kopie des Praefixes von TermAlt. Nachf- und Ende-Zeiger werden besetzt. */
  if (TO_Nullstellig(TermAlt)) {
    TermT TermNeu;
    TO_TermzelleHolen(TermNeu);
    TO_SymbolBesetzen(TermNeu,TO_TopSymbol(TermAlt));
    TO_NachfBesetzen(TermNeu,NULL);
    TO_EndeBesetzen(TermNeu,TermNeu);
    return TermNeu;
  }
  else {
    TermT TermNeu = TO_HoleErsteNeueZelle(),
          IndexNeu = TermNeu;
    TO_SymbolBesetzen(TermNeu,TO_TopSymbol(TermAlt));
    TI_Push(TermNeu);
    do {
      TO_AufNaechsteNeueZelle(IndexNeu);
      TO_SymbolBesetzen(IndexNeu,TO_TopSymbol(TermAlt = TO_Schwanz(TermAlt)));
      if (TO_Nullstellig(TermAlt)) {
        TO_EndeBesetzen(IndexNeu,IndexNeu);
        while (!(--TI_TopZaehler)) {
          TO_EndeBesetzen(TI_TopTeilterm,IndexNeu);
          if (TI_PopUndLeer()) {
            TO_LetzteNeueZelleMelden(IndexNeu);
            TO_NachfBesetzen(IndexNeu,NULL);
            return TermNeu;
          }
        }
      }
      else
        TI_Push(IndexNeu);
    } while (TRUE);
  }
}


/* ------- 2. Besondere Funktionen fuer MatchOperatioen ---------------------------------------------------------- */

TermT TO_TermkopieEZ(UTermT TermAlt, TermzellenZeigerT ersteTermzelle)     /* Erzeugt eine Kopie des Praefixes von */
{                /* TermAlt. Nachf- und Ende-Zeiger werden besetzt. Erste Termzelle der Kopie wird ersteTermzelle. */
  UTermT IndexNeu = TO_HoleErsteNeueZelle();                                           /* TermAlt ist mehrstellig. */
  TO_SymbolBesetzen(ersteTermzelle,TO_TopSymbol(TermAlt));
  TO_ClearSubstFlag(ersteTermzelle);
  TI_Push(ersteTermzelle);
  {              /* gesonderter erster Schleifendurchlauf, da einmal Aufruf von TO_HoleErsteNeueZelle erforderlich */
    TO_NachfBesetzen(ersteTermzelle,IndexNeu);
    TO_SymbolBesetzen(IndexNeu,TO_TopSymbol(TermAlt = TO_Schwanz(TermAlt)));
    TO_ClearSubstFlag(IndexNeu);
    if (TO_Nullstellig(TermAlt)) {
      TO_EndeBesetzen(IndexNeu,IndexNeu);
      while (!(--TI_TopZaehler)) {
        TO_EndeBesetzen(TI_TopTeilterm,IndexNeu);
        if (TI_PopUndLeer()) {
          TO_LetzteNeueZelleMelden(IndexNeu);
          TO_NachfBesetzen(IndexNeu,NULL);
          return ersteTermzelle;
        }
      }
    }
    else
      TI_Push(IndexNeu);
  }
  do {
    TO_AufNaechsteNeueZelle(IndexNeu);
    TO_SymbolBesetzen(IndexNeu,TO_TopSymbol(TermAlt = TO_Schwanz(TermAlt)));
    TO_ClearSubstFlag(IndexNeu);
    if (TO_Nullstellig(TermAlt)) {
      TO_EndeBesetzen(IndexNeu,IndexNeu);
      while (!(--TI_TopZaehler)) {
        TO_EndeBesetzen(TI_TopTeilterm,IndexNeu);
        if (TI_PopUndLeer()) {
          TO_LetzteNeueZelleMelden(IndexNeu);
          TO_NachfBesetzen(IndexNeu,NULL);
          return ersteTermzelle;
        }
      }
    }
    else
      TI_Push(IndexNeu);
  } while (TRUE);
}


TermT TO_TermkopieEZx1Subst(UTermT TermAlt, TermzellenZeigerT ersteTermzelle)            /* Erzeugt eine Kopie des */
{  /* Praefixes von TermAlt. Nachf- und Ende-Zeiger werden besetzt. Erste Termzelle der Kopie wird ersteTermzelle. */
                         /* TermAlt ist mehrstellig. Ausserdem wird x1 durch TO_Topsymbol(ersteTermzelle) ersetzt. */
  SymbolT Substitut = TO_TopSymbol(ersteTermzelle);
  UTermT IndexNeu = TO_HoleErsteNeueZelle();
  TO_SymbolBesetzen(ersteTermzelle,TO_TopSymbol(TermAlt));
  TO_ClearSubstFlag(ersteTermzelle);
  TI_Push(ersteTermzelle);
  {
    TO_NachfBesetzen(ersteTermzelle,IndexNeu);
    TermAlt = TO_Schwanz(TermAlt);
    if (TO_Nullstellig(TermAlt)) {
      SymbolT Topsymbol = TO_TopSymbol(TermAlt);
      TO_SymbolBesetzen(IndexNeu,SO_SymbGleich(Topsymbol,SO_ErstesVarSymb) ? Substitut : Topsymbol);
      if (SO_SymbGleich(Topsymbol,SO_ErstesVarSymb))
	TO_SetSubstFlag(IndexNeu);
      else
	TO_ClearSubstFlag(IndexNeu);
      TO_EndeBesetzen(IndexNeu,IndexNeu);
      while (!(--TI_TopZaehler)) {
        TO_EndeBesetzen(TI_TopTeilterm,IndexNeu);
        if (TI_PopUndLeer()) {
          TO_LetzteNeueZelleMelden(IndexNeu);
          TO_NachfBesetzen(IndexNeu,NULL);
          return ersteTermzelle;
        }
      }
    }
    else {
      TO_SymbolBesetzen(IndexNeu,TO_TopSymbol(TermAlt));
      TO_ClearSubstFlag(IndexNeu);
      TI_Push(IndexNeu);
    }
  }
  do {
    TO_AufNaechsteNeueZelle(IndexNeu);                                                                              \
    TermAlt = TO_Schwanz(TermAlt);
    if (TO_Nullstellig(TermAlt)) {
      SymbolT Topsymbol = TO_TopSymbol(TermAlt);
      TO_SymbolBesetzen(IndexNeu,SO_SymbGleich(Topsymbol,SO_ErstesVarSymb) ? Substitut : Topsymbol);
      if (SO_SymbGleich(Topsymbol,SO_ErstesVarSymb))
	TO_SetSubstFlag(IndexNeu);
      else
	TO_ClearSubstFlag(IndexNeu);
      TO_EndeBesetzen(IndexNeu,IndexNeu);
      while (!(--TI_TopZaehler)) {
        TO_EndeBesetzen(TI_TopTeilterm,IndexNeu);
        if (TI_PopUndLeer()) {
          TO_LetzteNeueZelleMelden(IndexNeu);
          TO_NachfBesetzen(IndexNeu,NULL);
          return ersteTermzelle;
        }
      }
    }
    else {
      TO_SymbolBesetzen(IndexNeu,TO_TopSymbol(TermAlt));
      TO_ClearSubstFlag(IndexNeu);
      TI_Push(IndexNeu);
    }
  } while (TRUE);
}


TermT TO_TermkopieLZm(UTermT TermAlt, TermzellenZeigerT letzteTermzelle)
/* Erzeugt eine Kopie des Praefixes von TermAlt. Letzte Termzelle der Kopie wird letzteTermzelle. Deren Nachfol-
   ger-Zeiger wird nicht neu besetzt. Ebensowenig deren Ende-Zeiger; dieser muss also schon auf letzteTermzelle    */
          /* weisen. Aufruf erfolgt nur in MatchOperationen; Zusicherung gilt dort. TermAlt muss mehrstellig sein. */

#ifndef CCE_Source
#define RumpfTO_TermkopieLZm                                                                                        \
{                                                                                                                   \
  TermT TermNeu = TO_HoleErsteNeueZelle(),                                                                          \
        TermAltEnde = TO_TermEnde(TermAlt);                                                                         \
  UTermT IndexNeu = TermNeu;                                                                                        \
                                                                                                                    \
  TO_SymbolBesetzen(TermNeu,TO_TopSymbol(TermAlt));                                                                 \
  TO_ClearSubstFlag(TermNeu);\
  TI_Push(TermNeu);                                                                                                 \
  while (TO_PhysischUngleich(TermAlt = TO_Schwanz(TermAlt),TermAltEnde)) {                                          \
    TO_AufNaechsteNeueZelle(IndexNeu);                                                                              \
    TO_SymbolBesetzen(IndexNeu,TO_TopSymbol(TermAlt));                                                              \
    TO_ClearSubstFlag(IndexNeu);\
    if (TO_Nullstellig(TermAlt)) {                                                                                  \
      TO_EndeBesetzen(IndexNeu,IndexNeu);                                                                           \
      while (!(--TI_TopZaehler)) {                                                                                  \
        TO_EndeBesetzen(TI_TopTeilterm,IndexNeu);                                                                   \
        TI_Pop();                                                                                                   \
      }                                                                                                             \
    }                                                                                                               \
    else                                                                                                            \
      TI_Push(IndexNeu);                                                                                            \
  }                                                                                                                 \
                                                                                                                    \
  TO_LetzteNeueZelleMelden(IndexNeu);                                                                               \
  TO_NachfBesetzen(IndexNeu,letzteTermzelle);                                                                       \
  TO_SymbolBesetzen(letzteTermzelle,TO_TopSymbol(TermAltEnde));                                                     \
  TO_ClearSubstFlag(letzteTermzelle);\
  do                                                                                                                \
    TO_EndeBesetzen(TI_TopTeilterm,letzteTermzelle);                                                                \
  while (TI_PopUndNichtLeer());                                                                                     \
                                                                                                                    \
  return TermNeu;                                                                                                   \
}
RumpfTO_TermkopieLZm

#else

#define RumpfTO_TermkopieLZm                                                                                        \
{                                                                                                                   \
  TermT TermNeu = TO_HoleErsteNeueZelle(),                                                                          \
        TermAltEnde = TO_TermEnde(TermAlt);                                                                         \
  UTermT IndexNeu = TermNeu;                                                                                        \
                                                                                                                    \
  TO_SymbolBesetzen(TermNeu,TO_TopSymbol(TermAlt));                                                                 \
  TI_Push(TermNeu);                                                                                                 \
  while (TO_PhysischUngleich(TermAlt = TO_Schwanz(TermAlt),TermAltEnde)) {                                          \
    TO_AufNaechsteNeueZelle(IndexNeu);                                                                              \
    TO_SymbolBesetzen(IndexNeu,TO_TopSymbol(TermAlt));                                                              \
    if (TO_Nullstellig(TermAlt)) {                                                                                  \
      TO_EndeBesetzen(IndexNeu,IndexNeu);                                                                           \
      while (!(--TI_TopZaehler)) {                                                                                  \
        TO_EndeBesetzen(TI_TopTeilterm,IndexNeu);                                                                   \
        TI_Pop();                                                                                                   \
      }                                                                                                             \
    }                                                                                                               \
    else                                                                                                            \
      TI_Push(IndexNeu);                                                                                            \
  }                                                                                                                 \
                                                                                                                    \
  TO_LetzteNeueZelleMelden(IndexNeu);                                                                               \
  TO_NachfBesetzen(IndexNeu,letzteTermzelle);                                                                       \
  TO_SymbolBesetzen(letzteTermzelle,TO_TopSymbol(TermAltEnde));                                                     \
  do                                                                                                                \
    TO_EndeBesetzen(TI_TopTeilterm,letzteTermzelle);                                                                \
  while (TI_PopUndNichtLeer());                                                                                     \
                                                                                                                    \
  return TermNeu;                                                                                                   \
}
RumpfTO_TermkopieLZm

#endif

TermT TO_TermkopieLZ(UTermT TermAlt, TermzellenZeigerT letzteTermzelle)
/* Erzeugt eine Kopie des Praefixes von TermAlt. Letzte Termzelle der Kopie wird letzteTermzelle. Deren Nachfol-
   ger-Zeiger wird nicht neu besetzt. Ebensowenig deren Ende-Zeiger; dieser muss also schon auf letzteTermzelle    */
{                                        /* weisen. Aufruf erfolgt nur in MatchOperationen; Zusicherung gilt dort. */
  if (TO_Nullstellig(TermAlt)) {
    TO_SymbolBesetzen(letzteTermzelle,TO_TopSymbol(TermAlt));
    TO_ClearSubstFlag(letzteTermzelle);
    return letzteTermzelle;
  }
  else
    RumpfTO_TermkopieLZm;
}     


TermT TO_TermkopieLZx1Subst(UTermT TermAlt, TermzellenZeigerT letzteTermzelle)
{                     /* Wie TO_TermkopieLZm, aber zusaetzlich wird x1 durch TO_Topsymbol(ersteTermzelle) ersetzt. */
  SymbolT Substitut = TO_TopSymbol(letzteTermzelle);
  TermT TermNeu = TO_HoleErsteNeueZelle(),
        TermAltEnde = TO_TermEnde(TermAlt);
  UTermT IndexNeu = TermNeu;
  
  TO_SymbolBesetzen(TermNeu,TO_TopSymbol(TermAlt));
  TO_ClearSubstFlag(TermNeu);
  TI_Push(TermNeu);
  while (TO_PhysischUngleich(TermAlt = TO_Schwanz(TermAlt),TermAltEnde)) {
    TO_AufNaechsteNeueZelle(IndexNeu);
    if (TO_Nullstellig(TermAlt)) {
      SymbolT Topsymbol = TO_TopSymbol(TermAlt);
      TO_SymbolBesetzen(IndexNeu,SO_SymbGleich(Topsymbol,SO_ErstesVarSymb) ? Substitut : Topsymbol);
      if (SO_SymbGleich(Topsymbol,SO_ErstesVarSymb))
	TO_SetSubstFlag(IndexNeu);
      else
	TO_ClearSubstFlag(IndexNeu);
      TO_EndeBesetzen(IndexNeu,IndexNeu);
      while (!(--TI_TopZaehler)) {
        TO_EndeBesetzen(TI_TopTeilterm,IndexNeu);
        TI_Pop();
      }
    }
    else {
     TO_SymbolBesetzen(IndexNeu,TO_TopSymbol(TermAlt));
     TO_ClearSubstFlag(IndexNeu);
     TI_Push(IndexNeu);
    }
  }
  
  TO_LetzteNeueZelleMelden(IndexNeu);
  TO_NachfBesetzen(IndexNeu,letzteTermzelle);
  TO_SymbolBesetzen(letzteTermzelle,TO_TopSymbol(TermAltEnde));
  TO_ClearSubstFlag(letzteTermzelle);
  do
    TO_EndeBesetzen(TI_TopTeilterm,letzteTermzelle);
  while (TI_PopUndNichtLeer());
  
  return TermNeu;
}     


/* ------- 3. Besondere Funktion fuer Unifikation ---------------------------------------------------------------- */

TermT TO_TermkopieVarUmbenannt(UTermT TermAlt, SymbolT NeuesteVar)
{                          /* Erzeugt eine Kopie des Praefixes von TermAlt. Nachf- und Ende-Zeiger werden besetzt. */
  if (TO_Nullstellig(TermAlt)) {
    TermT TermNeu;
    SymbolT Topsymbol = TO_TopSymbol(TermAlt);
    TO_TermzelleHolen(TermNeu);
    TO_SymbolBesetzen(TermNeu,SO_SymbIstVar(Topsymbol) ? Topsymbol+NeuesteVar : Topsymbol);
    TO_NachfBesetzen(TermNeu,NULL);
    TO_EndeBesetzen(TermNeu,TermNeu);
    return TermNeu;
  }
  else {
    TermT TermNeu = TO_HoleErsteNeueZelle(),
          IndexNeu = TermNeu;
    TO_SymbolBesetzen(TermNeu,TO_TopSymbol(TermAlt));
    TI_Push(TermNeu);
    do {
      TO_AufNaechsteNeueZelle(IndexNeu);
      TermAlt = TO_Schwanz(TermAlt);
      if (TO_Nullstellig(TermAlt)) {
        SymbolT Topsymbol = TO_TopSymbol(TermAlt);
        TO_SymbolBesetzen(IndexNeu,SO_SymbIstVar(Topsymbol) ? Topsymbol+NeuesteVar : Topsymbol);
        TO_EndeBesetzen(IndexNeu,IndexNeu);
        while (!(--TI_TopZaehler)) {
          TO_EndeBesetzen(TI_TopTeilterm,IndexNeu);
          if (TI_PopUndLeer()) {
            TO_LetzteNeueZelleMelden(IndexNeu);
            TO_NachfBesetzen(IndexNeu,NULL);
            return TermNeu;
          }
        }
      }
      else {
        TO_SymbolBesetzen(IndexNeu,TO_TopSymbol(TermAlt));
        TI_Push(IndexNeu);
      }
    } while (TRUE);
  }
}

/* ------- 4. Besondere Funktion fuer Frischzellenkur ------------------------------------------------------------ */

#define /*void*/ to_FrischeTermzelleHolen(/*L-Ausdruck*/ Index)                                                            \
  SV_NestedAlloc(Index,to_FrischzellenManager,TermzellenZeigerT)

#define /*TermzellenZeigerT*/ to_HoleErsteNeueFrischZelle()                                                               \
  (TermzellenZeigerT) SV_ErstesNextFree(to_FrischzellenManager,TermzellenZeigerT)

#define /*void*/ to_AufNaechsteNeueFrischZelle(/*TermzellenZeigerT*/ TermIndex)                                           \
  SV_AufNextFree(to_FrischzellenManager,TermIndex,TermzellenZeigerT)

#define /*void*/ to_LetzteNeueFrischeZelleMelden(/*TermzellenZeigerT*/ TermIndex)                                          \
  SV_LetztesNextFreeMelden(to_FrischzellenManager,(TermIndex->Nachf))

static TermT to_FrischeTermkopie(UTermT TermAlt)
/* Code nahezu (bis auf obige 4 Routinen) identisch zu TO_Termkopie */
{
  if (TO_Nullstellig(TermAlt)) {
    TermT TermNeu;
    to_FrischeTermzelleHolen(TermNeu);
    TO_SymbolBesetzen(TermNeu,TO_TopSymbol(TermAlt));
    TO_NachfBesetzen(TermNeu,NULL);
    TO_EndeBesetzen(TermNeu,TermNeu);
    return TermNeu;
  }
  else {
    TermT TermNeu = to_HoleErsteNeueFrischZelle(),
          IndexNeu = TermNeu;
    TO_SymbolBesetzen(TermNeu,TO_TopSymbol(TermAlt));
    TI_Push(TermNeu);
    do {
      to_AufNaechsteNeueFrischZelle(IndexNeu);
      TO_SymbolBesetzen(IndexNeu,TO_TopSymbol(TermAlt = TO_Schwanz(TermAlt)));
      if (TO_Nullstellig(TermAlt)) {
        TO_EndeBesetzen(IndexNeu,IndexNeu);
        while (!(--TI_TopZaehler)) {
          TO_EndeBesetzen(TI_TopTeilterm,IndexNeu);
          if (TI_PopUndLeer()) {
            to_LetzteNeueFrischeZelleMelden(IndexNeu);
            TO_NachfBesetzen(IndexNeu,NULL);
            return TermNeu;
          }
        }
      }
      else
        TI_Push(IndexNeu);
    } while (TRUE);
  }
}

TermT TO_FrischzellenTermkopie(TermT t)
{
  if (!PA_Frischzellenkur()){
    return TO_Termkopie(t);
  }
  else if (1){
    return to_FrischeTermkopie(t);
  }
  else { /* funktioniert leider nicht */
    TermT res;
    SV_NestedSpeicherManagerT tmp = TO_FreispeicherManager;
    TO_FreispeicherManager = to_FrischzellenManager;
    res = TO_Termkopie(t);
    TO_FreispeicherManager = tmp;
    return res;
  }
}

void TO_FrischzellenTermLoeschen(TermT t)
{
  if (PA_Frischzellenkur()){
    SV_NestedDealloc(to_FrischzellenManager,t,TO_TermEnde(t));
  }
  else {
    TO_TermLoeschen(t);
  }
}

void TO_FrischzellenTermeLoeschen(TermT s, TermT t)
{
  if (PA_Frischzellenkur()){
    TO_NachfBesetzen(TO_TermEnde(s),t);
    SV_NestedDealloc(to_FrischzellenManager,s,TO_TermEnde(t));
  }
  else {
    TO_TermeLoeschen(s,t);
  }
}



/*=================================================================================================================*/
/*= VII. Destruktive Funktionen                                                                                   =*/
/*=================================================================================================================*/

void TO_TermLoeschenM(TermT Term)                 /* Loeschen von mit TO_PlattklopfenLRMitMalloc erzeugten Termen */
{
  do {
    UTermT Alt = Term;
    Term = TO_Schwanz(Term);
    our_dealloc(Alt);
  } while (Term);
}



/*=================================================================================================================*/
/*= VIII. Funktionen zur Umwandlung von externer in interne Termdarstellung                                       =*/
/*=================================================================================================================*/

#ifndef CCE_Source

typedef VLZellenZeigerT VListenT;
static VListenT Variablenliste;                  /* Erlaeuterung der Struktur siehe unter Funktion PlattklopfenRek */


static VLZellenZeigerT VLZelleHolen(SymbolT VIntern, term VExtern, VLZellenZeigerT Vorg)
{
  VLZellenZeigerT Index;
  SV_Alloc(Index,VLZellenSpeicherManager,VLZellenZeigerT);
  Index->VIntern = VIntern;
  Index->VExtern = VExtern;
  Index->Vorg = Vorg;
  return Index;
}


static void VariablenlisteLoeschen(void)
{
  VLZellenZeigerT Vorg;
  do {
    Vorg = Variablenliste->Vorg;
    SV_Dealloc(VLZellenSpeicherManager,Variablenliste);
  } while ((Variablenliste=Vorg));
}


static TermT PlattklopfenRek(term Term)
/* Interne Darstellung von Term erzeugen und zurueckgeben. Variablen werden mit Hilfe der dateiglobalen Variable
   Variablenliste negativen ganzen Zahlen zugeordnet; die Zuordnung der Funktionssymbole wird aus SymbolOperationen
   uebernommen. Variablenliste ist ein Stapel der Gestalt
   ((n. Variable extern | Nummer der n. Variable intern) ...(1. Variable extern | Nummer der 1. Variable intern)),
   so dass die interne Nummer beim obersten Stapelelement vermindert um eins die Nummer der jeweils naechsten
   neuen Variable ist. Beim ersten Aufruf von PlattklopfenRek muss Variablenliste daher initialisiert sein auf
   ((neue externe Variable, die nirgends sonst vorkommt | 0)).                                                     */
{
  if (variablep(Term)) {                                                                        /* fuer Variablen: */
    VLZellenZeigerT Index;
    Index = Variablenliste;
    while (Index && !term_eq(Index->VExtern,Term))                 /* nachsehen, ob Variable schon aufgetreten ist */
      Index = Index->Vorg;
    if (Index)                                         /* falls ja: Termzelle mit zugeordneter Nummer zurueckgeben */
      return NullstelligerTermM(Index->VIntern);
    else {                                       /* sonst neue Nummer vergeben und entsprechende Termzelle zurueck */
      SymbolT neueNummer;
      neueNummer = Variablenliste->VIntern-1;
      Variablenliste = VLZelleHolen(neueNummer,Term,Variablenliste);         /* ausserdem Variablenliste erweitern */
      return NullstelligerTermM(neueNummer);
    } 
 }
  else {                                                                                       /* fuer Konstanten: */
    short int i;
    UTermT Flunder=NULL, Teilterm=NULL, Ende=NULL;        /* alle Teilterme in umgekehrter Reihenfolge durchlaufen */
    for(i=sig_arity(term_func(Term))-1; i>=0; i--) {   /* Invariante: Flunder zeigt auf zuletzt erzeugten Teilterm */
      TO_NachfBesetzen(TO_TermEnde(Teilterm = PlattklopfenRek(term_arg(Term,i))),Flunder);     /* Nachf-Verkettung */
      if (!Ende)                       /* vermerkt Endzeiger des letzten Teilterms; wird Endzeiger des Gesamtterms */
        Ende = TO_TermEnde(Teilterm);
      Flunder = Teilterm;
    }
    Flunder = struct_alloc(TermzellenT);                                        /* Termzelle fuer Topsymbol bilden */
    TO_NachfBesetzen(Flunder,Teilterm);
    TO_EndeBesetzen(Flunder,Ende ? Ende : Flunder);   /* Ende-Zeiger mit Ende aus Schleife besetzen bzw. mit Term- */
    TO_SymbolBesetzen(Flunder,SN_WMvonDISCOUNT(term_func(Term)));          /* zelle selbst, falls Konstantensymbol */
    return Flunder;
  }
}

void TO_PlattklopfenLRMitMalloc(term Term1, term Term2, TermT *Flunder1, TermT *Flunder2)
/* Aufruf von PlattklopfenRek mit Initialisierung der Variablenliste; danach Loeschen derselbigen.
   Fuer Term2 mit bereits in Term1 erklaerten Variablenersetzungen. */
{
  Variablenliste = VLZelleHolen(SO_VorErstemVarSymb,create_var(sorts_first(get_sorts())),NULL);
  *Flunder1 = PlattklopfenRek(Term1);
  *Flunder2 = PlattklopfenRek(Term2);
  SO_VariablenfeldAnpassen(Variablenliste->VIntern);
  TO_FeldVariablentermeAnpassen(Variablenliste->VIntern);
  VariablenlisteLoeschen();
}
#endif


/*=================================================================================================================*/
/*= IX. AC-Gleichheit                                                                                             =*/
/*=================================================================================================================*/

/* ------- 1. Hochstapeleien ------------------------------------------------------------------------------------- */

typedef enum {S, T} StapeladrT;
#define forST(/*StapeladrT*/ I)                                                                                     \
  for (I=S; I<=T; I++)

typedef union {
  UTermT s;
  unsigned int i;
} TermIntT;

static struct StapelTS {
  unsigned int Ausdehnung,
               Gesamthoehe[2],
               letzteHoeheAnte;
  TermIntT *Terme[2];
} Stapel = {0,{0,0},0,{NULL,NULL}};

static void Push_(TermIntT si, StapeladrT I);

static void StapelNeu(void)
{
  TermIntT si;
  si.i = Stapel.letzteHoeheAnte;
  Push_(si,S);
  Stapel.letzteHoeheAnte = Stapel.Gesamthoehe[T]++;
}

static void StapelAlt(void)
{
  StapeladrT I;
  forST(I)
    Stapel.Gesamthoehe[I] = Stapel.letzteHoeheAnte;
  Stapel.letzteHoeheAnte = Stapel.Terme[S][Stapel.letzteHoeheAnte+1].i;
}

static BOOLEAN True(void)
{
  StapelAlt();
  return TRUE;
}

static BOOLEAN False(void)
{
  StapelAlt();
  return FALSE;
}

static unsigned int Hoehe(StapeladrT I)
{
  return Stapel.Gesamthoehe[I]-Stapel.letzteHoeheAnte-1;
}

static BOOLEAN Empty(StapeladrT I)
{
  return !Hoehe(I);
}

static void Pop(void)
{
  StapeladrT I;
  forST(I)
    Stapel.Gesamthoehe[I]--;
}

static UTermT Top(StapeladrT I)
{
  unsigned int Gesamthoehe = Stapel.Gesamthoehe[I];
  return Stapel.Terme[I][Gesamthoehe].s;
}

static void Push_(TermIntT si, StapeladrT I)
{
  if (Stapel.Ausdehnung==Stapel.Gesamthoehe[I]) {
    unsigned int Ausdehnung = minKXgrY(SPP_AnzErsterVariablen,Stapel.Ausdehnung);
    StapeladrT J;
    forST(J)
      if (Stapel.Ausdehnung)
        posarray_realloc(Stapel.Terme[J],Ausdehnung,TermIntT,1);
      else
        Stapel.Terme[J] = posarray_alloc(Ausdehnung,TermIntT,1);
    Stapel.Ausdehnung = Ausdehnung;
  }
  Stapel.Terme[I][++Stapel.Gesamthoehe[I]] = si;
}

static void Push(UTermT s, StapeladrT I)
{
  TermIntT si;
  si.s = s;
  Push_(si,I);
}


/* ------- 2. Anwendung ------------------------------------------------------------------------------------------ */

static void Abflachen_(SymbolT f, UTermT s, StapeladrT I)
{
  if (SO_SymbGleich(f,TO_TopSymbol(s))) {
    UTermT s1 = TO_ErsterTeilterm(s),
           s2 = TO_NaechsterTeilterm(s1);
    Abflachen_(f,s1,I);
    Abflachen_(f,s2,I);
  }
  else
    Push(s,I);
}

static void Abflachen(UTermT s, StapeladrT I)
{
  SymbolT f = TO_TopSymbol(s);
  UTermT s1 = TO_ErsterTeilterm(s),
         s2 = TO_NaechsterTeilterm(s1);
  Abflachen_(f,s1,I);
  Abflachen_(f,s2,I);
}


static int cmp(const void *s_, const void *t_)
{
  UTermT s = ((TermIntT*)s_)->s,
         t = ((TermIntT*)t_)->s,
         s0 = TO_NULLTerm(s);
  do {
    SymbolT f = TO_TopSymbol(s),
            g = TO_TopSymbol(t);
    if (f<g)
      return +1;
    else if (f>g)
      return -1;
  } while (TO_PhysischUngleich((t = TO_Schwanz(t),s = TO_Schwanz(s)),s0));
  return 0;
}

static void my_qsort(TermIntT *Feld, unsigned int n)
{
  if (n>2)
    qsort(Feld,n,sizeof(UTermT),cmp);
  else if (cmp(&(Feld[0]),&(Feld[1]))>0) {
    TermIntT tmp = Feld[0];
    Feld[0] = Feld[1];
    Feld[1] = tmp;
  }
}

static void Sortieren(void)
{
  StapeladrT I;
  forST(I) {
    TermIntT *Feld = Stapel.Terme[I]+1+Stapel.letzteHoeheAnte+1;
    my_qsort(Feld,Hoehe(I));
  }
}

BOOLEAN TO_ACGleich(UTermT s, UTermT t)
{
  /*IO_DruckeFlex("Aufruf mit %t und %t\n",s,t);*/
  if (!TO_TopSymboleGleich(s,t))
    return FALSE;
  else {
    SymbolT f = TO_TopSymbol(s);
    if (!SO_IstACSymbol(f)) {
      UTermT s0 = TO_NULLTerm(s);
      s = TO_ErsterTeilterm(s); t = TO_ErsterTeilterm(t); 
      while (TO_PhysischUngleich(s,s0)) {
        if (!TO_ACGleich(s,t))
          return FALSE;
        s = TO_NaechsterTeilterm(s); t = TO_NaechsterTeilterm(t);
      }
      return TRUE;
    }
    else {
      StapelNeu();
      Abflachen(s,S); Abflachen(t,T);
      if (Hoehe(S)!=Hoehe(T))
        return False();
      Sortieren();
      do {
        s = Top(S); t = Top(T);
        if (!TO_ACGleich(s,t))
          return False();
        Pop();
      } while (!Empty(S));
      return True();
    }
  }
}



/*=================================================================================================================*/
/*= X. Sonstiges                                                                                                  =*/
/*=================================================================================================================*/

unsigned long int TO_StelleInTerm(UTermT Stelle, TermT Term)                 /* Stelle des Teiltermzeigers als int */
{
   UTermT Index;
   unsigned long int zaehlen = 0;

   TO_doStellen(Term, Index,{
      if (TO_PhysischGleich(Index, Stelle))
         return zaehlen;
      zaehlen++;
   });
   IO_DruckeFlex("\nFEHLER: Stelle nicht im Term!\nStelle %t, Term %t\n", Stelle, Term);
   return 0;
}

UTermT TO_TermAnStelle(TermT Term, unsigned long int Stellennr)                          /* Teiltermzeigers von int */
{
  UTermT Stelle = Term;
  while (Stellennr--)
    Stelle = TO_Schwanz(Stelle);
  return Stelle;
}

unsigned long int TO_Termlaenge(UTermT Term)             /* Laenge (= Anzahl von Symbolen) eines Termes bestimmen. */
{
  unsigned long Laenge = 1;
  UTermT NULLTerm = TO_NULLTerm(Term);
  while (TO_PhysischUngleich(Term = TO_Schwanz(Term),NULLTerm))
    Laenge++;
  return Laenge;
}


unsigned int TO_Termtiefe(UTermT Term)
{
  if (TO_Nullstellig(Term))
    return 1;
  else {
    unsigned int maximaleTiefeMinus1 = 1;
    TI_PushOT(Term);        /* TI_Stapelhoehe()+1 ist aktuelle Termtiefe fuer den Fall, dass Term gerade auf einen */
    do {                            /* nullstelligen Teilterm zeigt. Da nur dort Maxima angenommen werden koennen, */
      Term = TO_Schwanz(Term);
      if (TO_Nullstellig(Term)) {                              /* muss maximaleTiefe nur dort aktualisiert werden. */
        if (TI_Stapelhoehe()>maximaleTiefeMinus1)
          maximaleTiefeMinus1 = TI_Stapelhoehe();
        while (!(--TI_TopZaehler))
          if (TI_PopUndLeer())
            return maximaleTiefeMinus1+1;
      }
      else
        TI_PushOT(Term);
    } while (TRUE);
  }
}


SymbolT TO_SymbolNeuesterVariablenterm(void)
{
  return VTermeDimension;
}


void TO_FeldVariablentermeAnpassen(SymbolT DoppelteNeuesteBaumvariable)
{
  if (SO_VariableIstNeuer(DoppelteNeuesteBaumvariable,VTermeDimension)) {
    int VTermeDimensionNeu = SO_VarDelta(DoppelteNeuesteBaumvariable,TO_AnzErsterVariablenterme);
    UTermT *VariablentermeNeu = (UTermT *) our_alloc(-VTermeDimensionNeu*sizeof(TermT)) - VTermeDimensionNeu;
    SymbolT VarIndex;                             /* Addition von -VTermeDimensionNeu wegen negativer Adressierung */
    SO_forSymboleAbwaerts(VarIndex,SO_ErstesVarSymb,VTermeDimension)
      VariablentermeNeu[VarIndex] = TO_Variablenterm[VarIndex];
    SO_forSymboleAbwaerts(VarIndex,VTermeDimension-1,VTermeDimensionNeu)
      VariablentermeNeu[VarIndex] = NullstelligerTermM(VarIndex);
    our_dealloc(TO_Variablenterm+VTermeDimension);
    VTermeDimension = VTermeDimensionNeu;
    TO_Variablenterm = VariablentermeNeu;
  }
 }


UTermT TO_VorigeTermzelle(UTermT Teilterm, UTermT Term)
/* Zeiger auf die der Termzelle Teilterm im Term Term vorangehende Termzelle */
{
  if (TO_PhysischGleich(Term,Teilterm))
    return NULL;
  else {
    UTermT TermVorg;
    while (TO_PhysischUngleich(Term = TO_Schwanz(TermVorg = Term),Teilterm));
    return TermVorg;
  }
}


UTermT TO_VorletzteZelle(UTermT Term)    /* Zeiger auf vorletzte Zelle von Term. Term darf nicht nullstellig sein. */
{
  do
    if (TO_MehrstelligerTermEinstellig(Term))
      if (TO_Nullstellig(TO_ErsterTeilterm(Term)))
        return Term;
      else
        Term = TO_ErsterTeilterm(Term);
    else {
      UTermT vorletzterTeilterm = TO_ErsterTeilterm(Term),
             letzterTeilterm = TO_NaechsterTeilterm(vorletzterTeilterm);
      while (TO_PhysischUngleich(TO_TermEnde(letzterTeilterm),TO_TermEnde(Term)))
        letzterTeilterm = TO_NaechsterTeilterm(vorletzterTeilterm = letzterTeilterm);
      if (TO_Nullstellig(letzterTeilterm))
        return TO_TermEnde(vorletzterTeilterm);
      else
        Term = letzterTeilterm;
    }
  while (TRUE);
}


void TO_VariablenUmbenennen(TermT Term, signed int Delta)
{                                               /* Sei Delta = q. Jede Variable x(i) wird ersetzt durch x(i+q). */
  do
    if (TO_TermIstVar(Term))
      TO_SymbolBesetzen(Term,SO_NtesNaechstesVarSymb(TO_TopSymbol(Term),Delta));
  while ((Term = TO_Schwanz(Term)));
}

SymbolT TO_neuesteVariable(TermT Term)
{
  SymbolT x = SO_VorErstemVarSymb;
  do {
    SymbolT y = TO_TopSymbol(Term);
    if (SO_SymbIstVar(y) && SO_VariableIstNeuer(y,x))
      x = y;
  } while ((Term = TO_Schwanz(Term)));
  return x;
}


VergleichsT TO_EntscheidungSymbLex(TermT Term1, TermT Term2)        /* stellt Verhaeltnis von Term1 und Term2 beim */
{                 /* Vergleich als Listen ueber ganzen Zahlen mittels lexikographischer Erweiterung von >(Z) fest. */
  do {
    SymbolT Symbol1 = TO_TopSymbol(Term1),
            Symbol2 = TO_TopSymbol(Term2);
    if (Symbol1>Symbol2)
      return Groesser;
    else if (Symbol1<Symbol2)
      return Kleiner;
    Term1 = TO_Schwanz(Term1);
    Term2 = TO_Schwanz(Term2);
  } while (Term1);
  return Gleich;
}


BOOLEAN TO_TestSymbLex(TermT Term1, TermT Term2)                                     /* Test, ob Term1 >(Z)* Term2 */
{
  do {
    SymbolT Symbol1 = TO_TopSymbol(Term1),
            Symbol2 = TO_TopSymbol(Term2);
    if (Symbol1>Symbol2)
      return TRUE;
    else if (Symbol1<Symbol2)
      return FALSE;
    Term1 = TO_Schwanz(Term1);
    Term2 = TO_Schwanz(Term2);
  } while (Term1);
  return FALSE;
}


VergleichsT TO_TPEntscheidungSymbLex(TermT TP1Links, TermT TP1Rechts, TermT TP2Links, TermT TP2Rechts)
{                 /* Vergleich als Listen ueber ganzen Zahlen mittels lexikographischer Erweiterung von >(Z) fest. */
  do {
    SymbolT Symbol1 = TO_TopSymbol(TP1Links),
            Symbol2 = TO_TopSymbol(TP2Links);
    if (Symbol1>Symbol2)
      return Groesser;
    else if (Symbol1<Symbol2)
      return Kleiner;
    TP1Links = TO_Schwanz(TP1Links);
    TP2Links = TO_Schwanz(TP2Links);
  } while (TP1Links);
  do {
    SymbolT Symbol1 = TO_TopSymbol(TP1Rechts),
            Symbol2 = TO_TopSymbol(TP2Rechts);
    if (Symbol1>Symbol2)
      return Groesser;
    else if (Symbol1<Symbol2)
      return Kleiner;
    TP1Rechts = TO_Schwanz(TP1Rechts);
    TP2Rechts = TO_Schwanz(TP2Rechts);
  } while (TP2Links);
  return Gleich;
}


BOOLEAN TO_TPTestSymbLex(TermT TP1Links, TermT TP1Rechts, TermT TP2Links, TermT TP2Rechts)               /* analog */
{
  do {
    SymbolT Symbol1 = TO_TopSymbol(TP1Links),
            Symbol2 = TO_TopSymbol(TP2Links);
    if (Symbol1>Symbol2)
      return TRUE;
    else if (Symbol1<Symbol2)
      return FALSE;
    TP1Links = TO_Schwanz(TP1Links);
    TP2Links = TO_Schwanz(TP2Links);
  } while (TP1Links);
  do {
    SymbolT Symbol1 = TO_TopSymbol(TP1Rechts),
            Symbol2 = TO_TopSymbol(TP2Rechts);
    if (Symbol1>Symbol2)
      return TRUE;
    else if (Symbol1<Symbol2)
      return FALSE;
    TP1Rechts = TO_Schwanz(TP1Rechts);
    TP2Rechts = TO_Schwanz(TP2Rechts);
  } while (TP1Rechts);
  return FALSE;
}


BOOLEAN TO_TermEntspricht(TermT Term, unsigned int AnzSymbole, ...)
{                                                                  /* Die Folge muss einen korrekten Term ergeben. */
  va_list Argumente;
  unsigned int i;
  SymbolT SymbolLinks, SymbolRechts;
  va_start(Argumente,AnzSymbole);
  for(i=1; i<=AnzSymbole; i++) {
            SymbolLinks = TO_TopSymbol(Term);
            SymbolRechts = (SymbolT)va_arg(Argumente,int);
    if (SO_SymbUngleich(SymbolLinks,SymbolRechts)) {
      va_end(Argumente);
      return FALSE;
    }
    Term = TO_Schwanz(Term);
  }
  va_end(Argumente);
  return TRUE;
}


BOOLEAN TO_IstAssoziativitaet(TermT links, TermT rechts)
/* Sei links=rechts variablennormiert von links nach rechts mit Status l.
   sei f=TO_TopSymbol(links); f aus F U V. Test, ob links=rechts aus
   { A1: f(f(x1,x2),x3)=f(x1,f(x2,x3));  A2: f(x1,f(x2,x3))=f(f(x1,x2),x3) } */
{
  SymbolT f = TO_TopSymbol(links);
  return 
    SO_Stelligkeit(f)==2 && (
      (TO_TermEntspricht(links,5,f,f,-1,-2,-3) && TO_TermEntspricht(rechts,5,f,-1,f,-2,-3)) ||
      (TO_TermEntspricht(links,5,f,-1,f,-2,-3) && TO_TermEntspricht(rechts,5,f,f,-1,-2,-3))
    );
}


BOOLEAN TO_IstKommutativitaet(TermT links, TermT rechts)
/* Sei links=rechts variablennormiert von links nach rechts mit Status l.
   sei f=TO_TopSymbol(links); f aus F U V. Test, ob links=rechts aus
   { C: f(x1,x2)=f(x2,x1) } */
{
  SymbolT f = TO_TopSymbol(links);
  return 
    SO_Stelligkeit(f)==2 &&
    TO_TermEntspricht(links,3,f,-1,-2) && 
    TO_TermEntspricht(rechts,3,f,-2,-1);
}


BOOLEAN TO_IstKommutativitaet_(TermT l, TermT r)
/* Sei links=rechts variablennormiert von links nach rechts mit Status l.
   sei f=TO_TopSymbol(links); f aus F U V. Test, ob links=rechts aus
   { C': f(x1,f(x2,x3))=f(x3,f(x1,x2))} 
   parallel. */
{
  SymbolT f = TO_TopSymbol(l);
  return 
    SO_Stelligkeit(f)==2 &&
    TO_TermEntspricht(l,5, f,-1,f,-2,-3) && 
       TO_TermEntspricht(r,5, f,-2,f,-1,-3);
}

BOOLEAN TO_IstErweiterteKommutativitaet(TermT l, TermT r)
/* Sei links=rechts variablennormiert von links nach rechts mit Status l.
   sei f=TO_TopSymbol(links); f aus F U V. Test, ob links=rechts aus
   { C': f(x1,f(x2,x3))=f(x3,f(x1,x2)), =f(x2,f(x1,x3)), =f(x3,f(x2,x1)), =f(x2,f(x3,x1))}
   parallel oder ueber Kreuz. */
{
  SymbolT f = TO_TopSymbol(l);
  return 
    SO_Stelligkeit(f)==2 &&
    TO_TermEntspricht(l,5, f,-1,f,-2,-3) && 
      (TO_TermEntspricht(r,5, f,-3,f,-1,-2) ||
       TO_TermEntspricht(r,5, f,-2,f,-1,-3) ||
       TO_TermEntspricht(r,5, f,-3,f,-2,-1) ||
       TO_TermEntspricht(r,5, f,-2,f,-3,-1));
}


BOOLEAN TO_IstDistribution(TermT links, TermT rechts)
/* Sei links=rechts variablennormiert in der Reihenfolge links, rechts mit Status l;
   sei f=TO_TopSymbol(links); f aus F U V. Test, ob links=rechts aus
   { DL: f(x1,g(x2,x3))=g(f(x1,x2),f(x1,x3)), DR: f(g(x1,x2),x3)=g(f(x1,x3),f(x2,x3))} */
{
  SymbolT f = TO_TopSymbol(links),
          g = TO_TopSymbol(rechts);
  return 
    SO_Stelligkeit(f)==2 && SO_Stelligkeit(g)==2 &&
    ( (TO_TermEntspricht(links,5,f,-1,g,-2,-3) && TO_TermEntspricht(rechts,7,g,f,-1,-2,f,-1,-3)) ||
      (TO_TermEntspricht(links,5,f,g,-1,-2,-3) && TO_TermEntspricht(rechts,7,g,f,-1,-3,f,-2,-3)) );
}


BOOLEAN TO_IstAntiDistribution(TermT links, TermT rechts)
/* Sei links=rechts variablennormiert in der Reihenfolge links, rechts mit Status l;
   sei f=TO_TopSymbol(links); f aus F U V. Test, ob links=rechts aus
   { dL: g(f(x1,x2),f(x1,x3))=f(x1,g(x2,x3)), dR: g(f(x1,x2),f(x3,x2))=f(g(x1,x3),x2)} */
{
  SymbolT f = TO_TopSymbol(rechts),
          g = TO_TopSymbol(links);
  return 
    SO_Stelligkeit(f)==2 && SO_Stelligkeit(g)==2 &&
    ( (TO_TermEntspricht(links,7,g,f,-1,-2,f,-1,-3) && TO_TermEntspricht(rechts,5,f,-1,g,-2,-3)) ||
      (TO_TermEntspricht(links,7,g,f,-1,-2,f,-3,-2) && TO_TermEntspricht(rechts,5,f,g,-1,-3,-2)) );
}

BOOLEAN TO_IstIdempotenz(TermT links, TermT rechts)
/* Sei links=rechts variablennormiert von links nach rechts mit Status l.
   sei f=TO_TopSymbol(links); f aus F U V. Test, ob links=rechts aus
   { I: f(x1,x1)=x1 } */
{
  SymbolT f = TO_TopSymbol(links);
  return 
    SO_Stelligkeit(f)==2 &&
    TO_TermEntspricht(links,3,f,-1,-1) && 
    TO_TermEntspricht(rechts,1,-1);
}

BOOLEAN TO_IstIdempotenz_(TermT links, TermT rechts)
/* Sei links=rechts variablennormiert von links nach rechts mit Status l.
   sei f=TO_TopSymbol(links); f aus F U V. Test, ob links=rechts aus
   { I': f(x1,f(x1,x2))=f(x1,x2) } */
{
  SymbolT f = TO_TopSymbol(links);
  return 
    SO_Stelligkeit(f)==2 &&
    TO_TermEntspricht(links,5,f,-1,f,-1,-2) && 
    TO_TermEntspricht(rechts,3,f,-1,-2);
}



BOOLEAN TO_KombinatorapparatAktiviert;
SymbolT TO_ApplyOperator;

static BOOLEAN IstKombinatorTupel(TermT s, TermT t, SymbolT apply)                      /* s== C x[n] = @(x[n])==t */
{
  if (SO_SymbGleich(apply,TO_TopSymbol(s))) {
    UTermT si = s;
    SymbolT C;
    do
      C = TO_TopSymbol(si = TO_Schwanz(si));
    while (SO_SymbGleich(C,apply));
    if (SO_SymbIstKonst(C) && TO_TermIstVar(si = TO_Schwanz(si))) {
      while ((si = TO_Schwanz(si)) && TO_TermIstVar(si)) {
        UTermT sj = TO_Schwanz(s);
        while (TO_PhysischUngleich(si,sj = TO_Schwanz(sj)))
          if (TO_TopSymboleGleich(si,sj))
            return FALSE;
      }
      if (!si) {
        do
          if (SO_SymbIstFkt(C = TO_TopSymbol(t)) ? SO_SymbUngleich(C,apply) : !TO_SymbolEnthalten(C,s))
            return FALSE;
        while ((t = TO_Schwanz(t)));
        return TRUE;
      }
    }
  }
  return FALSE;
}

BOOLEAN TO_IstKombinatorTupel(TermT s, TermT t)                               /* dito mit fixiertem Apply-Operator */
{
  BOOLEAN Resultat = TO_KombinatorapparatAktiviert && IstKombinatorTupel(s,t,TO_ApplyOperator);
  /*IO_DruckeFlex("TO_IstKombinatorTupel(%t,%t)=%d\n",s,t,Resultat);*/
  return Resultat;
}

void TO_TermeTauschen(TermT *Term1, TermT *Term2)
/* jeweilige Inhalte vertauschen */
{
  TermT Term1Komma5 = *Term1;
  *Term1 = *Term2;
  *Term2 = Term1Komma5;
}

static void topSymboleTauschen(UTermT s, UTermT t)
{
  SymbolT sym = TO_TopSymbol(s);
  TO_SymbolBesetzen(s,TO_TopSymbol(t));
  TO_SymbolBesetzen(t,sym);
}

static void zeigerVerbiegen(UTermT ti, UTermT e1, UTermT e2)
/* funktioniert nur, wenn e1 im gleichen Flachterm echt hinter ti vorkommt  */
{
  while (TO_Schwanz(ti) != e1){
    /* Alle Zellen vor der vorletzten beackern */
    if (TO_TermEnde(ti) == e1){
      TO_EndeBesetzen(ti,e2);
    }
    ti = TO_Schwanz(ti);
  }
  /* Vorletzte Zelle beackern: */
  if (TO_TermEnde(ti) == e1){
    TO_EndeBesetzen(ti,e2);
  }
  TO_NachfBesetzen(ti,e2);
}

static void tauscheKurzGegenLang(UTermT vk, UTermT vl)
{
  UTermT tk = TO_Schwanz(vk);
  UTermT tl = TO_Schwanz(vl);
  UTermT el = TO_TermEnde(tl);

  TO_NachfBesetzen(vl,el);
  TO_NachfBesetzen(vk,tl);

  topSymboleTauschen(el,tk);
    
  zeigerVerbiegen(tl,el,tk);
}

static void tauscheLangGegenLang(UTermT v1, UTermT v2)
{
  UTermT t1 = TO_Schwanz(v1);
  UTermT t2 = TO_Schwanz(v2);
  UTermT e1 = TO_TermEnde(t1);
  UTermT e2 = TO_TermEnde(t2);

  TO_NachfBesetzen(v1,t2);
  TO_NachfBesetzen(v2,t1);

  topSymboleTauschen(e1,e2);
    
  zeigerVerbiegen(t1,e1,e2);
  zeigerVerbiegen(t2,e2,e1);
}

/* Einfacher Fall:
 * Es gibt genau dann einen Zeiger auf die letzte Zelle eines Teilterms,
 * wenn sein Vorgaenger auf diese letzte Zelle zeigt.
 * (anschaulich: TO_TermEnde-Zeiger "ueberkreuzen sich nicht"!)
 *
 * Zeigen nun beide Vorgaenger _nicht_ auf die jeweils letzte Zelle der
 * zu tauschenden Teilterme, so reicht diese Prozedur zum Tauschen aus.
 *
 * ACHTUNG: notwendige Zusatzbedingungen!
 * + Die jeweiligen Vorgaenger duerfen _nicht_ nullstellig sein! Sonst
 *   gibt es mit dieser Prozedur Murks.
 * + e1 darf nicht gleich v2 sein.
 */
static void tauscheEinfachenFall(UTermT v1, UTermT v2) 
{
  UTermT t1 = TO_Schwanz(v1);
  UTermT t2 = TO_Schwanz(v2);
  UTermT e1 = TO_TermEnde(t1);
  UTermT e2 = TO_TermEnde(t2);
  UTermT n1 = TO_Schwanz(e1);
  UTermT n2 = TO_Schwanz(e2);

  TO_NachfBesetzen(v1,t2);
  TO_NachfBesetzen(v2,t1);  
  TO_NachfBesetzen(e1,n2);
  TO_NachfBesetzen(e2,n1);
}


void TO_TeiltermeTauschen(UTermT Vorg1, UTermT Vorg2)
{
  UTermT t1 = TO_Schwanz(Vorg1);
  UTermT t2 = TO_Schwanz(Vorg2);

  if (TO_Nullstellig(t1) && TO_Nullstellig(t2)){
    topSymboleTauschen(t1,t2);
  } 
  else if ((TO_TermEnde(Vorg1) != TO_TermEnde(t1)) && 
	   (TO_TermEnde(Vorg2) != TO_TermEnde(t2)) &&
	   !(TO_Nullstellig(Vorg1))                &&
	   !(TO_Nullstellig(Vorg2))                &&
	   (TO_TermEnde(t1) != Vorg2)       	   ){
    tauscheEinfachenFall(Vorg1, Vorg2);
  }
  else if (TO_Nullstellig(t1)){
    tauscheKurzGegenLang(Vorg1, Vorg2);
  }
  else if (TO_Nullstellig(t2)){
    tauscheKurzGegenLang(Vorg2, Vorg1);
  }
  else { 
    tauscheLangGegenLang(Vorg1, Vorg2);
  }
}



/*=================================================================================================================*/
/*= XI. Initialisierung und Deinitialisierung                                                                     =*/
/*=================================================================================================================*/

static BOOLEAN TO_SchonInitialisiert = FALSE;

void TO_InitAposteriori(void)
{
  SymbolT VarIndex;
  if (!TO_SchonInitialisiert) {
    TO_Variablenterm = negarray_alloc(TO_AnzErsterVariablenterme,TermT,TO_AnzErsterVariablenterme);
    VTermeDimension = -TO_AnzErsterVariablenterme;
    TO_KombinatorapparatAktiviert = FALSE; 
  }
  TO_SchonInitialisiert = TRUE;
  SV_NestedManagerInitialisieren(TO_FreispeicherManager, TermzellenT, TO_AnzahlTermZellenJeBlock, "Termzellen");
  SV_NestedManagerInitialisieren(to_FrischzellenManager, TermzellenT, TO_AnzahlTermZellenJeBlock, "Frische Termzellen");
#ifndef CCE_Source
  SV_ManagerInitialisieren(&VLZellenSpeicherManager, VLZellenT, TO_AnzVLZellenJeBlock, "TO_VLZellen");
#endif
  SO_forSymboleAbwaerts(VarIndex,SO_ErstesVarSymb,VTermeDimension)
    TO_Variablenterm[VarIndex] = NullstelligerTermM(VarIndex);
}

#ifndef CCE_Source
void TO_KombinatorapparatAktivieren(void)
{
  if (!TO_KombinatorapparatAktiviert) {
    SymbolT f;
    TO_ApplyOperator = SO_ErstesVarSymb;
    SO_forEquationSymbole(f)
      if (SO_Stelligkeit(f)==2) {
        ListenT Fakten;
        for (Fakten = PM_Gleichungen; Fakten; Fakten = cdr(Fakten)) {
          RegelOderGleichungsT Faktum = car(Fakten);
          TermT u = TP_LinkeSeite(Faktum),
                v = TP_RechteSeite(Faktum);
          if (IstKombinatorTupel(u,v,f) || IstKombinatorTupel(v,u,f)) {
            if (SO_SymbGleich(TO_ApplyOperator,SO_ErstesVarSymb) || SO_SymbGleich(TO_ApplyOperator,f))
              TO_ApplyOperator = f;
            else
              our_error("ambiguous apply operator for option(s) -kern / -back");
          }
        }
      }
    if (SO_SymbGleich(TO_ApplyOperator,SO_ErstesVarSymb))
      our_error("no apply operator for combinatory logic option");
    TO_KombinatorapparatAktiviert = TRUE; 
  }
}
#endif

void TO_FeldVariablentermeLeeren(void)
{
  SymbolT VarIndex;
  SO_forSymboleAbwaerts(VarIndex,SO_ErstesVarSymb,VTermeDimension)
    TO_TermLoeschenM(TO_Variablenterm[VarIndex]);
}
