/*=================================================================================================================
  =                                                                                                               =
  =  TermOperationen.h                                                                                            =
  =                                                                                                               =
  =  Definition des Datentyps Term und Realisation elementarer Operationen                                        =
  =                                                                                                               =
  =  02.03.94 Thomas.                                                                                             =
  =  01.02.95 Thomas. Rekursionen eliminiert.                                                                     =
  =                                                                                                               =
  =================================================================================================================*/

#ifndef TERMOPERATIONEN_H
#define TERMOPERATIONEN_H


#include "general.h"
#include "parser.h"
#include "SpeicherVerwaltung.h"
#include "SymbolOperationen.h"



/*=================================================================================================================*/
/*= I. Typdeklarationen und Modulvariablen                                                                        =*/
/*=================================================================================================================*/

/* innermost Optimierung ? */
#define DO_FLAGS 1

/* Optimierung(?) fuer LPO */
#define VARSET_FOR_LPO 0
#if VARSET_FOR_LPO
typedef unsigned long VarsetT;
#endif

typedef struct TermzellenTS *TermzellenZeigerT;                  /* Zeiger- und Verbundtypen fuer Terme als Listen */
typedef struct TermzellenTS {
  TermzellenZeigerT Nachf;
  SymbolT Symbol;
#if 0 && DO_FLAGS
  char substFlag;
#endif
#if VARSET_FOR_LPO
  VarsetT vars;
#endif
  unsigned short id;
  TermzellenZeigerT Ende;                                       /* Der Ende-Zeiger ist immer besetzt (vgl. unten). */
} TermzellenT;

#define GroesseTermzellenT sizeof(TermzellenT)

typedef TermzellenZeigerT TermT;                                                 /* nach aussen sichtbarer Termtyp */
typedef TermzellenZeigerT UTermT;                                                      /* nicht beschnittener Term */
/* Die Menge der unbeschnittenen Terme ist echte Obermenge derjenigen der beschnittenen.                           */

typedef TermzellenZeigerT LiteralT;

extern SV_NestedSpeicherManagerT TO_FreispeicherManager;



/*=================================================================================================================*/
/*= II. Erzeugen, Besetzen und Loeschen von Termzellen                                                            =*/
/*=================================================================================================================*/

#define /*void*/ TO_TermzelleHolen(/*L-Ausdruck*/ Index)                                                            \
  SV_NestedAlloc(Index,TO_FreispeicherManager,TermzellenZeigerT)

#define /*TermzellenZeigerT*/ TO_HoleErsteNeueZelle()                                                               \
  (TermzellenZeigerT) SV_ErstesNextFree(TO_FreispeicherManager,TermzellenZeigerT)

#define /*void*/ TO_AufNaechsteNeueZelle(/*TermzellenZeigerT*/ TermIndex)                                           \
  SV_AufNextFree(TO_FreispeicherManager,TermIndex,TermzellenZeigerT)

#define /*void*/ TO_LetzteNeueZelleMelden(/*TermzellenZeigerT*/ TermIndex)                                          \
  SV_LetztesNextFreeMelden(TO_FreispeicherManager,(TermIndex->Nachf))

#define /*SymbolT*/ TO_SymbolBesetzen(/*TermzellenZeigerT*/ Termzelle, /*SymbolT*/ Symb)                            \
  ((Termzelle)->Symbol = (Symb))

#define /*TermzellenZeigerT*/ TO_NachfBesetzen(/*TermzellenZeigerT*/ Termzelle, /*TermzellenZeigerT*/ Nach)         \
  ((Termzelle)->Nachf = (Nach))

#define /*TermzellenZeigerT*/ TO_EndeBesetzen(/*TermzellenZeigerT*/ Termzelle, /*TermzellenZeigerT*/ End)           \
  ((Termzelle)->Ende = (End))

#define /*TermzellenZeigerT*/ TO_NULLBesetzen(/*TermzellenZeigerT*/ Termzelle, /*TermzellenZeigerT*/ End)           \
  ((Termzelle)->Ende->Nachf = (End))

#define /*void*/ TO_SetzeTermzelle(/*TermzellenZeigerT*/ target, /*TermzellenZeigerT*/ source)	\
  do {												\
     (target)->Symbol = (source)->Symbol;							\
     (target)->Nachf  = (source)->Nachf;							\
     if ((source)->Ende == (source))								\
       (target)->Ende = (target);								\
     else											\
       (target)->Ende = (source)->Ende;								\
  } while(0)

TermT TO_NullstelligerTerm(SymbolT f);

TermT TO_(SymbolT Symbol, ...);
/* Term aus Symbolfolge aufbauen. Die Korrektheit der Folge wird vorausgesetzt! */


#if DO_FLAGS
#if 0
#define /* void */ TO_SetSubstFlagTo(/*TermzellenZeigerT*/ Termzelle, /*char*/ val)\
  ((Termzelle)->substFlag = (val), (void) 0)
#define /* Boolean */ TO_SubstFlagSet(/*TermzellenZeigerT*/ Termzelle)\
  ((Termzelle)->substFlag)

/* Achtung: DUAL USE */
#define /*void*/ TO_SetFlag(/*TermzellenZeigerT*/ Termzelle, /*BOOLEAN*/ val)\
  ((Termzelle)->substFlag = (val), (void) 0)
#define /* Boolean */ TO_GetFlag(/*TermzellenZeigerT*/ Termzelle)\
  ((Termzelle)->substFlag)

#else

#define /* void */ TO_SetSubstFlagTo(/*TermzellenZeigerT*/ Termzelle, /*char*/ val)\
  ((Termzelle)->id = (val), (void) 0)
#define /* Boolean */ TO_SubstFlagSet(/*TermzellenZeigerT*/ Termzelle)\
  ((Termzelle)->id)

/* Achtung: DUAL USE */
#define /*void*/ TO_SetFlag(/*TermzellenZeigerT*/ Termzelle, /*BOOLEAN*/ val)\
  ((Termzelle)->id = (val), (void) 0)
#define /* Boolean */ TO_GetFlag(/*TermzellenZeigerT*/ Termzelle)\
  ((Termzelle)->id)
#endif
#else
#define /* void */ TO_SetSubstFlagTo(/*TermzellenZeigerT*/ Termzelle, /*char*/ val)\
  ((void) 0)
#define /* Boolean */ TO_SubstFlagSet(/*TermzellenZeigerT*/ Termzelle)\
  (0)

#define /*void*/ TO_SetFlag(/*TermzellenZeigerT*/ Termzelle, /*BOOLEAN*/ val)\
  ((void) 0)
#define /* Boolean */ TO_GetFlag(/*TermzellenZeigerT*/ Termzelle)\
  (0)
#endif

#define TO_SetId(/*TermzellenZeigerT*/t, /*unsigned short*/ i) ((t)->id = (i))
#define TO_GetId(/*TermzellenZeigerT*/t) ((t)->id)

#define /* void */ TO_SetSubstFlag(/*TermzellenZeigerT*/ Termzelle)\
  TO_SetSubstFlagTo(Termzelle,1)
#define /* void */ TO_ClearSubstFlag(/*TermzellenZeigerT*/ Termzelle)\
  TO_SetSubstFlagTo(Termzelle,0)

#if VARSET_FOR_LPO
#define /*BOOLEAN*/ VS_EQUAL(/*VarsetT*/s1, /*VarsetT*/s2) ((s1)==(s2))
#define /*VarsetT*/ VS_EMPTY() (0L)
#define /*VarsetT*/ VS_TIP(/* pos int */ VarNummer) (1 << VarNummer)
#define /*VarsetT*/ VS_UNION(/*VarsetT*/s1, /*VarsetT*/s2) ((s1)|(s2))
#define /*VarsetT*/ VS_IS_SUBSET(/*VarsetT*/s1, /*VarsetT*/s2) (!((s1)&~(s2)))
#define /*void*/ TO_SetVarset(/*TermzellenZeigerT */t, /*VarsetT*/ v) ((t)->vars = (v))
#define /*VarsetT*/ TO_GetVarset(/*TermzellenZeigerT */t) ((t)->vars)
#endif

#define /* void */ TO_mkLit(/* LiteralT */ lit, /*TermT */ lhs, /*TermT*/ rhs)  \
{                                                                               \
  TO_TermzelleHolen(lit);                                                       \
  TO_SymbolBesetzen(lit,SO_EQN);                                                \
  TO_NachfBesetzen(lit,lhs);                                                    \
  TO_EndeBesetzen(lit,TO_TermEnde(rhs));                                        \
  TO_NULLBesetzen(lhs,rhs);                                                     \
}

/*=================================================================================================================*/
/*= III. Zugriffsfunktionen auf Terme und UTerme                                                                  =*/
/*=================================================================================================================*/

#define /*SymbolT*/ TO_TopSymbol(/*UTermT*/ UTerm)                                                                  \
  ((UTerm)->Symbol)

#define /*UTermT*/ TO_Schwanz(/*UTermT*/ UTerm)                                                                     \
  ((UTerm)->Nachf)

#define /*UTermT*/ TO_ErsterTeilterm(/*UTermT*/ UTerm)                                                              \
  ((UTerm)->Nachf)

#define /*UTermT*/ TO_NaechsterTeilterm(/*UTermT*/ UTerm)                                                           \
  ((UTerm)->Ende->Nachf)

#define /*UTermT*/ TO_ZweiterTeilterm(/*UTermT*/ UTerm)                                                              \
  TO_NaechsterTeilterm(TO_ErsterTeilterm(UTerm))

#define /*UTermT*/ TO_TermEnde(/*UTermT*/ UTerm)                                                                    \
  ((UTerm)->Ende)

#define /*UTermT*/ TO_NULLTerm(/*UTermT*/ UTerm)                                                                    \
  ((UTerm)->Ende->Nachf)

#define /*unsigned int*/ TO_AnzahlTeilterme(/*UTermT*/ UTerm)                                                       \
  SO_Stelligkeit(TO_TopSymbol(UTerm))

#define /* UTermT */ TO_LitLHS(/*LiteralT */ lit) (TO_ErsterTeilterm(lit))
#define /* UTermT */ TO_LitRHS(/*LiteralT */ lit) (TO_ZweiterTeilterm(lit))



/*=================================================================================================================*/
/*= IV. Einfache Praedikate auf Termen                                                                            =*/
/*=================================================================================================================*/

#define /*BOOLEAN*/ TO_PhysischGleich(/*UTermT*/ UTerm1, /*UTermT*/ UTerm2)                                         \
  ((UTerm1) == (UTerm2))

#define /*BOOLEAN*/ TO_PhysischUngleich(/*UTermT*/ UTerm1, /*UTermT*/ UTerm2)                                       \
  ((UTerm1) != (UTerm2))

#define /*BOOLEAN*/ TO_TermIstVar(/*UTermT*/ UTerm)                                                                 \
  SO_SymbIstVar(TO_TopSymbol(UTerm))

#define /*BOOLEAN*/ TO_TermIstNichtVar(/*UTermT*/ UTerm)                                                            \
  SO_SymbIstFkt(TO_TopSymbol(UTerm))

#define /*BOOLEAN*/ TO_Nullstellig(/*UTermT*/ UTerm)                                                                \
  TO_PhysischGleich(UTerm,TO_TermEnde(UTerm))

#define /*BOOLEAN*/ TO_NichtNullstellig(/*UTermT*/ UTerm)                                                           \
  TO_PhysischUngleich(UTerm,TO_TermEnde(UTerm))

#define /*BOOLEAN*/ TO_MehrstelligerTermEinstellig(/*UTermT*/ UTerm)                       /* lies: "mehr" = ">0" */\
  TO_PhysischGleich(TO_TermEnde(UTerm),TO_TermEnde(TO_ErsterTeilterm(UTerm)))

#define /*BOOLEAN*/ TO_TermIstKonst(/*UTermT*/ UTerm)                                                               \
  (TO_TermIstNichtVar(UTerm) && TO_Nullstellig(UTerm))

#define /*BOOLEAN*/ TO_TopSymboleGleich(/*UTermT*/ UTerm1, /*UTermT*/ UTerm2)                                       \
  (SO_SymbGleich(TO_TopSymbol(UTerm1),TO_TopSymbol(UTerm2)))

#define /*BOOLEAN*/ TO_TopSymboleUngleich(/*UTermT*/ UTerm1, /*UTermT*/ UTerm2)                                     \
  (SO_SymbUngleich(TO_TopSymbol(UTerm1),TO_TopSymbol(UTerm2)))

#define /*unsigned int*/ TO_Stelligkeit(/*UTermT*/ UTerm)                                                           \
  (SO_Stelligkeit(TO_TopSymbol(UTerm)))

BOOLEAN TO_TermeGleich(UTermT Term1, UTermT Term2);
/* Prueft, ob UTerme Term1, Term2 denselben Praefix haben. Der Vergleich erfolgt von links nach rechts Symbol fuer
   Symbol. Abbruch, falls unterschiedliche Symbole oder Term1 abgearbeitet. Dass Term2 abgearbeitet ist und die-
   selben Symbole hat wie Term1 und Term1 noch nicht abgearbeitet ist, kommt wegen der Termstruktur nicht vor.     */

BOOLEAN TO_TermKorrekt(TermT Term);                                      /* Prueft Termdarstellung auf Konsistenz. */
                                                     /* Nur fuer Terme aus Regeln, Gleichungen, Kritischen Paaren. */
BOOLEAN TO_HomoeomorphDrin(UTermT s, UTermT t);
/* Ist s homoeomorph in t eingebettet? Das ganze inklusive Aequivalenzanteil (==syntaktischer Gleichheit). */

BOOLEAN TO_SymbolEnthalten(SymbolT f, UTermT t);
/* Ist die f-Laenge von t positiv? */

unsigned int TO_Symbollaenge(SymbolT f, UTermT t);
/* Wie oft ist f in t enthalten? */

BOOLEAN TO_VarGroesserGleich(UTermT s, UTermT t);
/* Ist Var(s) Obermenge von Var(t)? */

BOOLEAN TO_DefinierendeGleichung(UTermT s, UTermT t);
/* Ist s=t eine definierende Gleichung?
     Gdw. s==f(x(i1), ..., x(in)); |t|(f)=0; Var(s)>=Var(t).
     Zwecks Variablennormierung wird nur auf Kopien von s und t gearbeitet. */

BOOLEAN TO_TermEnthaeltFVar(TermT t);
/* Enthaelt t eine Funktionsvariable? */

BOOLEAN TO_PhysischTeilterm(UTermT si, UTermT s);
/* Ist die Topzelle von si Element der Liste s? */

#define /*BOOLEAN*/ TO_IstSkolemTerm(/*UTermT*/ Term)                                                               \
  (SO_IstSkolemSymbol(TO_TopSymbol(Term)))

BOOLEAN TO_TermIstGrund(TermT t);                                                          /* Ist t ein Grundterm? */

#define /*BOOLEAN*/ TO_TermIstNichtGrund(/*TermT*/ t)                                                               \
  (!TO_TermIstGrund(t))

UTermT TO_ElterntermVonIn(UTermT t, UTermT s);
/* t muss physikalisch Teilterm von s sein. Das Ergebnis ist dann NULL gdw. t==s,
   ansonsten der kleinste t echt physikalisch enthaltende Teilterm von s;
   bei baumweiser Termdarstellung also gerade der Vorgaenger. */



/*=================================================================================================================*/
/*= V. Schleifenkonstrukte                                                                                        =*/
/*=================================================================================================================*/

/*--- 1. Teiltermorientierte Konstrukte ---------------------------------------------------------------------------*/

#define TO_forTeilterme(/*UTermT*/ UTerm, /*UTermT*/ Teilterm, /*Anweisungen*/ Block)                               \
{ TermzellenZeigerT _forTeilterme_Exit = TO_NULLTerm(UTerm);                                                        \
  Teilterm = TO_ErsterTeilterm(UTerm);                                                                              \
  while (TO_PhysischUngleich(Teilterm,_forTeilterme_Exit)) {                                                        \
    Block                                                                                                           \
    Teilterm = TO_NaechsterTeilterm(Teilterm);                                                                      \
  }                                                                                                                 \
}
/* Durchlaufmakro; Terme koennen dann mit Schleifen der Gestalt
   forTeilterme(Term,Teilterm,
     Anweisung1;
     Anweisung2;
     ...
     AnweisungN;
   )
   durchlaufen werden.
*/

/*--- 2. Stellenorientierte Konstrukte ----------------------------------------------------------------------------*/

#define TO_doStellen(/*UTermT*/ UTerm, /*UTermT*/ Index, /*Anweisungen*/ Block)                                     \
{ TermzellenZeigerT _doStellen_Exit = TO_NULLTerm(UTerm);                                                           \
  Index = UTerm;                                                                                                    \
  do {                                                                                                              \
    Block                                                                                                           \
  } while (TO_PhysischUngleich(Index = TO_Schwanz(Index),_doStellen_Exit));                                         \
}

#define TO_forStellenOT(/*UTermT*/ UTerm, /*UTermT*/ Index, /*Anweisungen*/ Block)                                  \
{ TermzellenZeigerT _forStellenOT_Exit = TO_NULLTerm(UTerm);                                                        \
  Index = TO_Schwanz(UTerm);                                                                                        \
  while (TO_PhysischUngleich(Index,_forStellenOT_Exit)) {                                                           \
    Block                                                                                                           \
    Index = TO_Schwanz(Index);                                                                                      \
  }                                                                                                                 \
}

#define TO_doStellenOT(/*UTermT*/ UTerm, /*UTermT*/ Index, /*Anweisungen*/ Block)                                   \
{ TermzellenZeigerT _doStellenOT_Exit = TO_NULLTerm(UTerm);                                                         \
  Index = TO_Schwanz(UTerm);                                                                                        \
  do {                                                                                                              \
    Block                                                                                                           \
  } while (TO_PhysischUngleich(Index = TO_Schwanz(Index),_doStellenOT_Exit));                                       \
}

/*--- 3. Symbolorientierte Konstrukte -----------------------------------------------------------------------------*/

#define TO_doSymbole(/*UTermT*/ UTerm, /*SymbolT*/ Symbol, /*Anweisungen*/ Block)                                   \
{ TermzellenZeigerT _doSymbole_Exit = TO_NULLTerm(UTerm);                                                           \
  UTermT _doSymbole_Index = UTerm;                                                                                  \
  do {                                                                                                              \
    Symbol = TO_TopSymbol(_doSymbole_Index);                                                                        \
    Block                                                                                                           \
    _doSymbole_Index = TO_Schwanz(_doSymbole_Index);                                                                \
  } while (TO_PhysischUngleich(_doSymbole_Index,_doSymbole_Exit));                                                  \
}

#define TO_doSymboleOT(/*UTermT*/ UTerm, /*SymbolT*/ Symbol, /*Anweisungen*/ Block)                                 \
{ TermzellenZeigerT _doSymboleOT_Exit = TO_NULLTerm(UTerm);                                                         \
  UTermT _doSymboleOT_Index = TO_Schwanz(UTerm);                                                                    \
  do {                                                                                                              \
    Symbol = TO_TopSymbol(_doSymboleOT_Index);                                                                      \
    Block                                                                                                           \
    _doSymboleOT_Index = TO_Schwanz(_doSymboleOT_Index);                                                            \
  } while (TO_PhysischUngleich(_doSymboleOT_Index,_doSymboleOT_Exit));                                              \
}

#define TO_forSymboleOT(/*UTermT*/ UTerm, /*SymbolT*/ Symbol, /*Anweisungen*/ Block)                                \
{ TermzellenZeigerT _forStellenOT_Exit = TO_NULLTerm(UTerm);                                                        \
  UTermT _forSymboleOT_Index = TO_Schwanz(UTerm);                                                                   \
  while (TO_PhysischUngleich(_forSymboleOT_Index,_forStellenOT_Exit))   {                                           \
    Symbol = TO_TopSymbol(_forSymboleOT_Index);                                                                     \
    Block                                                                                                           \
    _forSymboleOT_Index = TO_Schwanz(_forSymboleOT_Index);                                                          \
  }                                                                                                                 \
}
/* Makro fuer symbolweises Durchlaufen von links nach rechts. Begonnen wird mit dem Topsymbol des ersten Teilterms,
   daher OT =  ohne Topsymbol. Der Test auf Termende erfolgt am Schleifenanfang -> auch geeignet fuer nullstellige
   Terme. Fuer nichtnullstellige, deren Stelligkeit bekannt ist, ist der Test beim ersten Durchlauf ueberfluessig.
   Aus diesem Grund gleich noch ein Makro mit annehmender Schleife.                                                */



/*=================================================================================================================*/
/*= VI. Funktionen zum Kopieren                                                                                   =*/
/*=================================================================================================================*/

/* ------- 1. Allgemeine Kopierfunktion -------------------------------------------------------------------------- */

#define TO_Termkopie(TermAlt) \
  (ON_TERMZELLEN_JAGD(fprintf(stderr,"Termkopie %s %d -> %d (%ld / %ld)\n", __FILE__, __LINE__, TO_Termlaenge(TermAlt),TO_FreispeicherManager.Anforderungen,SV_AnzNochBelegterElemente(TO_FreispeicherManager))), TO_TermkopieXX(TermAlt) )

TermT TO_TermkopieXX(UTermT TermAlt);
/* Erzeugt eine Kopie des Praefixes von TermAlt. Nachf- und Ende-Zeiger werden besetzt. */


/* ------- 2. Besondere Funktionen fuer MatchOperatioen ---------------------------------------------------------- */

TermT TO_TermkopieEZ(UTermT TermAlt, TermzellenZeigerT ersteTermzelle);
/* Erzeugt eine Kopie des Praefixes von TermAlt. Nachf- und Ende-Zeiger werden besetzt. 
   Erste Termzelle der Kopie wird ersteTermzelle. TermAlt muss mehrstellig sein. */

 TermT TO_TermkopieEZx1Subst(UTermT TermAlt, TermzellenZeigerT ersteTermzelle);
/* Wie vor, aber zusaetzlich wird x1 durch TO_Topsymbol(ersteTermzelle) ersetzt.
   Ausserdem muss TermAlt mehrstellig sein. */

 TermT TO_TermkopieLZ(UTermT TermAlt, TermzellenZeigerT letzteTermzelle);
/* Erzeugt eine Kopie des Praefixes von TermAlt. Letzte Termzelle der Kopie wird letzteTermzelle. Deren Nachfol-
   ger-Zeiger wird nicht neu besetzt. Ebensowenig deren Ende-Zeiger; dieser muss also schon auf letzteTermzelle
   weisen. */

 TermT TO_TermkopieLZm(UTermT TermAlt, TermzellenZeigerT letzteTermzelle);
/* Wie vor, aber TermAlt muss mehrstellig sein. */

 TermT TO_TermkopieLZx1Subst(UTermT TermAlt, TermzellenZeigerT letzteTermzelle);
/* Wie TO_TermkopieLZ, aber zusaetzlich wird x1 durch TO_Topsymbol(ersteTermzelle) ersetzt.
   Ausserdem muss TermAlt mehrstellig sein. */


/* ------- 3. Besondere Funktion fuer Unifikation ---------------------------------------------------------------- */

TermT TO_TermkopieVarUmbenannt(UTermT TermAlt, SymbolT NeuesteVar);
/* Termkopie, wobei, wenn NeuesteVar=x(n), aus Variable x(i) in der Kopie Variable x(i+n) wird */

/* ------- 4. Besondere Funktion fuer Frischzellenkur ------------------------------------------------------------ */

TermT TO_FrischzellenTermkopie(TermT t);

void TO_FrischzellenTermLoeschen(TermT t);

void TO_FrischzellenTermeLoeschen(TermT s, TermT t);

/*=================================================================================================================*/
/*= VII. Destruktive Funktionen                                                                                   =*/
/*=================================================================================================================*/

#define TO_TermzellenlisteLoeschen(/*UTermT*/ ErsteZelle, /*UTerm*/ LetzteZelle) /* Ueber Nachf-Zeiger verkettete */\
  SV_NestedDealloc(TO_FreispeicherManager,ErsteZelle,LetzteZelle)   /* Liste von Erster bis LetzterZelle freigeben */


#define TO_TermzelleLoeschen(/*UTermT*/ Term)                                                                       \
  TO_TermzellenlisteLoeschen(Term,Term)


#define TO_TermLoeschen(/*UTermT*/ Term)                                                                            \
  (ON_TERMZELLEN_JAGD(fprintf(stderr,"TermLoeschen %s %d -> %d (%ld / %ld)\n", __FILE__, __LINE__, TO_Termlaenge(Term),TO_FreispeicherManager.Anforderungen,SV_AnzNochBelegterElemente(TO_FreispeicherManager))),\
  TO_TermzellenlisteLoeschen(Term,TO_TermEnde(Term)))


void TO_TermLoeschenM(TermT Term);                 /* Loeschen von mit TO_PlattklopfenLRMitMalloc erzeugten Termen */


#define TO_TermeLoeschen(/*UTermT*/ Term1, /*UTermT*/ Term2)                                                        \
  (ON_TERMZELLEN_JAGD(fprintf(stderr,"TermeLoeschen %s %d -> %d (%ld / %ld)\n", __FILE__, __LINE__, TO_Termlaenge(Term1)+TO_Termlaenge(Term2),TO_FreispeicherManager.Anforderungen,SV_AnzNochBelegterElemente(TO_FreispeicherManager))),\
   TO_TermzellenlisteLoeschen(Term1,(TO_NachfBesetzen(TO_TermEnde(Term1),Term2),TO_TermEnde(Term2))))


#define TO_TermeLoeschen3(/*UTermT*/ Term1, /*UTermT*/ Term2, /*UTermT*/ Term3)                                     \
  TO_TermzellenlisteLoeschen(Term1,                                                                                 \
    (TO_NachfBesetzen(TO_TermEnde(Term1),Term2),TO_NachfBesetzen(TO_TermEnde(Term2),Term3),TO_TermEnde(Term3)))



/*=================================================================================================================*/
/*= VIII. Funktionen zur Umwandlung von externer in interne Termdarstellung                                       =*/
/*=================================================================================================================*/

#ifndef CCE_Source
void TO_PlattklopfenLRMitMalloc(term Term1, term Term2, TermT *Flunder1, TermT *Flunder2);
/* Interne Darstellung von Term1 und Term2 erzeugen. Fuer Term2 mit Variablenersetzungen aus Term1. 
   Wegen Wiederaufsetzen Terme aus permanentem Malloc-Speicher aufbauen. */
#endif


/*=================================================================================================================*/
/*= IX. AC-Gleichheit                                                                                             =*/
/*=================================================================================================================*/

BOOLEAN TO_ACGleich(UTermT s, UTermT t);



/*=================================================================================================================*/
/*= X. Sonstiges                                                                                                  =*/
/*=================================================================================================================*/

unsigned long int TO_StelleInTerm(UTermT Stelle, TermT Term);                 /* Stelle des Teiltermzeigers als int */

UTermT TO_TermAnStelle(TermT Term, unsigned long int Stelle);                            /* Teiltermzeigers von int */

unsigned long int TO_Termlaenge(UTermT Term);             /* Laenge (= Anzahl von Symbolen) eines Termes bestimmen.*/


unsigned int TO_Termtiefe(UTermT Term);


unsigned int TO_AusdehnungKopierstapel(void);                                                    /* fuer Statistik */


extern TermT *TO_Variablenterm;

#define /*TermT*/ TO_Variablenterm(/*SymbolT*/ VarSymbol)                                                           \
  (TO_Variablenterm[VarSymbol])       /* Zeiger auf nicht zu beschreibende Termzelle, die VarSymbol als Symbol hat */

SymbolT TO_SymbolNeuesterVariablenterm(void);                              /* Ausdehnung des entsprechenden Feldes */

void TO_FeldVariablentermeAnpassen(SymbolT DoppelteNeuesteBaumvariable);                /* Aufruf aus BO_TermpaarNormieren */


UTermT TO_VorigeTermzelle(UTermT Teilterm, UTermT Term);
/* Zeiger auf die der Termzelle Teilterm im Term Term vorangehende Termzelle */


UTermT TO_VorletzteZelle(UTermT Term);
/* Zeiger auf vorletzte Zelle von Term. Term darf nicht nullstellig sein. */


void TO_VariablenUmbenennen(TermT Term, signed int Delta);    /* Jede Variable x(i) wird ersetzt durch x(i+Delta). */

SymbolT TO_neuesteVariable(TermT Term);           /* neuestes (= als ganze Zahl kleinstes) Variablensymbol in Term */
                                                               /* bzw. SO_VorErstemVarSymb(), falls Term Grundterm */
VergleichsT TO_EntscheidungSymbLex(TermT Term1, TermT Term2);       /* stellt Verhaeltnis von Term1 und Term2 beim */
                  /* Vergleich als Listen ueber ganzen Zahlen mittels lexikographischer Erweiterung von >(Z) fest. */

BOOLEAN TO_TestSymbLex(TermT Term1, TermT Term2);                                    /* Test, ob Term1 >(Z)* Term2 */


VergleichsT TO_TPEntscheidungSymbLex(TermT TP1Links, TermT TP1Rechts, TermT TP2Links, TermT TP2Rechts);
/* lexikographische Erweiterung von TO_EntscheidungSymbLex auf Termpaare */

BOOLEAN TO_TPTestSymbLex(TermT TP1Links, TermT TP1Rechts, TermT TP2Links, TermT TP2Rechts);              /* analog */

BOOLEAN TO_TermEntspricht(TermT Term, unsigned int AnzSymbole, ...);

BOOLEAN TO_IstAssoziativitaet(TermT links, TermT rechts);
/* Sei links=rechts variablennormiert in der Reihenfolge links, rechts mit Status l;
   sei f=TO_TopSymbol(links); f aus F U V. Test, ob links=rechts aus
   { A1: f(f(x1,x2),x3)=f(x1,f(x2,x3));  A2: f(x1,f(x2,x3))=f(f(x1,x2),x3) } */

BOOLEAN TO_IstKommutativitaet(TermT links, TermT rechts);
/* Sei links=rechts variablennormiert in der Reihenfolge links, rechts mit Status l;
   sei f=TO_TopSymbol(links); f aus F U V. Test, ob links=rechts aus
   { C: f(x1,x2)=f(x2,x1) } */

BOOLEAN TO_IstKommutativitaet_(TermT l, TermT r);
/* Sei links=rechts variablennormiert von links nach rechts mit Status l.
   sei f=TO_TopSymbol(links); f aus F U V. Test, ob links=rechts gleich
    C': f(x1,f(x2,x3))=f(x3,f(x1,x2)) */

BOOLEAN TO_IstErweiterteKommutativitaet(TermT l, TermT r);
/* Sei links=rechts variablennormiert von links nach rechts mit Status l.
   sei f=TO_TopSymbol(links); f aus F U V. Test, ob links=rechts aus
   { C': f(x1,f(x2,x3))=f(x3,f(x1,x2)), =f(x2,f(x1,x3)), =f(x3,f(x2,x1)), =f(x2,f(x3,x1))}
   parallel oder ueber Kreuz. */

BOOLEAN TO_IstDistribution(TermT links, TermT rechts);
/* Sei links=rechts variablennormiert in der Reihenfolge links, rechts mit Status l;
   sei f=TO_TopSymbol(links); f aus F U V. Test, ob links=rechts aus
   { DL: f(x1,g(x2,x3))=g(f(x1,x2),f(x1,x3)), DR: f(g(x1,x2),x3)=g(f(x1,x3),f(x2,x3))} */

BOOLEAN TO_IstAntiDistribution(TermT links, TermT rechts);
/* Sei links=rechts variablennormiert in der Reihenfolge links, rechts mit Status l;
   sei f=TO_TopSymbol(links); f aus F U V. Test, ob links=rechts aus
   { dL: g(f(x1,x2),f(x1,x3))=f(x1,g(x2,x3)), dR: g(f(x1,x2),f(x3,x2))=f(g(x1,x3),x2)} */

BOOLEAN TO_IstIdempotenz(TermT links, TermT rechts);
/* Sei links=rechts variablennormiert von links nach rechts mit Status l.
   sei f=TO_TopSymbol(links); f aus F U V. Test, ob links=rechts aus
   { I: f(x1,x1)=x1 } */

BOOLEAN TO_IstIdempotenz_(TermT links, TermT rechts);
/* Sei links=rechts variablennormiert von links nach rechts mit Status l.
   sei f=TO_TopSymbol(links); f aus F U V. Test, ob links=rechts aus
   { I': f(x1,f(x1,x2))=f(x1,x2) } */

extern BOOLEAN TO_KombinatorapparatAktiviert;
extern SymbolT TO_ApplyOperator;
BOOLEAN TO_IstKombinatorTupel(TermT s, TermT t);                                        /* s== C x[n] = @(x[n])==t */

void TO_TermeTauschen(TermT *Term1, TermT *Term2);
/* jeweilige Inhalte vertauschen */


void TO_TeiltermeTauschen(UTermT Vorg1, UTermT Vorg2);
/* Tauscht zwei Teilterme. Diese duerfen nicht ueberlappen oder ineinander verschachtelt sein! */

/*=================================================================================================================*/
/*= XI. Initialisierung und Deinitialisierung                                                                     =*/
/*=================================================================================================================*/

void TO_InitAposteriori(void);

void TO_KombinatorapparatAktivieren(void);
/* in der Spezifikation apply-Symbol suchen */

void TO_FeldVariablentermeLeeren(void);
/* kann moduleigenes Termzellenfeld beseitigen, damit Speicherstatistik stimmt. */

#endif
