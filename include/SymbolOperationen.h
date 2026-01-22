/*=================================================================================================================
  =                                                                                                               =
  =  SymbolOperationen.h                                                                                          =
  =                                                                                                               =
  =  In diesem Modul sind die Operationen auf Symbolen, d.h. auf Variablen-,                                      =
  =  Konstanten- und Funktions-Symbolen, realisiert.                                                              =
  =                                                                                                               =
  =  23.02.94 Angelegt.                                                                                           =
  =  02.03.94 Weitergemacht. Getestet.                                                                            =
  =  02.03.94 Mit einigen Beispielen getestet, SO_TestAusgaben eingebaut, LAEUFT soweit.                          =
  =  16.03.94 Neue Schleifen-Makros eingebaut.                                                                    =
  =                                                                                                               =
  =================================================================================================================*/

#ifndef SYMBOLOPERATIONEN_H
#define SYMBOLOPERATIONEN_H


#include "general.h"

/*=================================================================================================================*/
/*= I. Typdefinitionen                                                                                            =*/
/*=================================================================================================================*/

typedef signed   short int SymbolT;

/*=================================================================================================================*/
/*= II. Globale Variablen mit Zugriffsmakros                                                                      =*/
/*=================================================================================================================*/

/* Arrangement der Funktionssymbole nach Normierung:
   SO_KleinstesFktSymb() <= SO_GroesstesSpezSymb <= SO_GroesstesFktSymb() <= SO_LetzteFVar()                       */

#define /*SymbolT*/   SO_ErstesVarSymb (-1)                                                      /* erste Variable */
#define /*SymbolT*/   SO_VorErstemVarSymb 0                    /* Ergebnis von SO_VorigesVarSymb(SO_ErstesVarSymb) */
#define /*SymbolT*/   SO_KleinstesFktSymb 0                                              /* erstes Funktionssymbol */
extern SymbolT        SO_GroesstesSpezSymb;                 /* groesstes Funktionssymbol nur aus der Spezifikation */
extern SymbolT        SO_GroesstesFktSymb;                      /* groesstes Funktionssymbol ggf. mit Exquant-Fkt. */
#define /*SymbolT*/   SO_ErsteFVar (SO_GroesstesFktSymb+1)                           /* Kleinste Funktionsvariable */
extern SymbolT        SO_LetzteFVar;    /* Groesste Funktionsvariable fuer ChaseManhattan oder SO_GroesstesFktSymb */
#define /*SymbolT*/   SO_Extrakonstante(/*unsigned int*/ n) (SO_LetzteFVar+(n))                  /* ab n>0 */

extern SymbolT        SO_EQN;
extern SymbolT        SO_EQ;
extern SymbolT        SO_TRUE;
extern SymbolT        SO_FALSE;
extern SymbolT        SO_const1;
extern SymbolT        SO_const2;

extern SymbolT        SO_NeuesteVariable;

extern unsigned int   SO_AnzahlFunktionen;                                              /* ==SO_GroesstesFktSymb+1 */
extern unsigned int   SO_MaximaleStelligkeit;                                         /* 0, falls keine Operatoren */
#define /*int*/       SO_AnzahlFunktionsvariablen() (SO_LetzteFVar-SO_ErsteFVar+1)

extern struct SO_SymbolInfoS {
  unsigned int        Stelligkeit;
  char *              Name;
  unsigned int        Namenslaenge;
  BOOLEAN             IstEquation;
  BOOLEAN             IstAC;
  BOOLEAN             Cancellation;
  SymbolT             DistribuiertUeber;
} *SO_SymbolInfo;

#define SO_Stelligkeit(Symbol)       (SO_IstExtrakonstante(Symbol) ?     0 : SO_SymbolInfo[Symbol].Stelligkeit)       /* SymbolT -> unsigned int */
#define SO_IstACSymbol(Symbol)       (SO_IstExtrakonstante(Symbol) ? FALSE : SO_SymbolInfo[Symbol].IstAC)                  /* SymbolT -> BOOLEAN */
#define SO_DistribuiertUeber(Symbol) (SO_IstExtrakonstante(Symbol) ? FALSE : SO_SymbolInfo[Symbol].DistribuiertUeber)    /* SymbolT -> GewichtsT */
#define SO_Cancellation(Symbol)      (SO_IstExtrakonstante(Symbol) ? FALSE : SO_SymbolInfo[Symbol].Cancellation)

#ifdef CCE_Source
#define SO_Symbolnamen(Symbol)       (SO_IstExtrakonstante(Symbol) ?    "@": SO_SymbolInfo[Symbol].Name)                             /* SymbolT -> char * */
#endif

/*=================================================================================================================*/
/*= III. Weitere elementare Praedikate und Funktionen; Iterationsmakros                                           =*/
/*=================================================================================================================*/

#define SO_SymbIstFkt(symbol)  ((symbol) >= 0)                                               /* SymbolT -> BOOLEAN */
#define SO_SymbIstKonst(symbol)  (SO_SymbIstFkt(symbol) && !SO_Stelligkeit(symbol))          /* SymbolT -> BOOLEAN */
#define SO_SymbIstVar(symbol) ((symbol) < 0)                                                 /* SymbolT -> BOOLEAN */
#define SO_SymbIstFVar(symbol) (((symbol)>=SO_ErsteFVar)&&((symbol)<=SO_LetzteFVar))
#define SO_IstExtrakonstante(/*SymbolT*/ c) ((c)>SO_LetzteFVar)

#define SO_NaechstesVarSymb(var) ((var)-1)                                          /* Liefert die naechste Variable */
#define SO_NtesNaechstesVarSymb(var,n) ((var)-((signed)n))                                            /* dito n-fach */
#define SO_VorigesVarSymb(var) ((var)+1)                                              /* Liefert die vorige Variable */
#define SO_NtesVorigesVarSymb(var,n) ((var)+((signed)n))                                              /* dito n-fach */
#define SO_NaechstesVarSymbD(var) (--(var))                              /* Liefert destruktiv die naechste Variable */
#define SO_VorigesVarSymbD(var) (++(var))                                  /* Liefert destruktiv die vorige Variable */

#define SO_SymbGleich(symbol1, symbol2) ((symbol1)==(symbol2))                       /* SymbolT SymbolT -> BOOLEAN */
#define SO_SymbUngleich(symbol1, symbol2) ((symbol1)!=(symbol2))                     /* SymbolT SymbolT -> BOOLEAN */
#define SO_VariableIstNeuer(x1,x2) ((x1)<(x2))
#define SO_VariableIstNichtNeuer(x1,x2) ((x1)>=(x2))
#define SO_VarNummer(var)    ((unsigned)(-(var)))           /* SymbolT -> Nat; liefert die Nummer einer Variablen. */
#define SO_NummerVar(nummer) (-((signed)(nummer)))              /* Nat -> SymbolT; liefert soundsovielte Variable. */
#define SO_StelligkeitenGleich(symbol1, symbol2) (SO_Stelligkeit(symbol1)==SO_Stelligkeit(symbol2))
#define SO_IstSkolemSymbol(symbol)                                                                                  \
  (SO_SymbIstFkt(symbol) && !SO_IstExtrakonstante(symbol) && !SO_SymbolInfo[symbol].IstEquation)


#define SO_forSymboleAufwaerts(Index,MinSymb,MaxSymb)                                                               \
  for(Index=(MinSymb);Index<=(MaxSymb);Index++)

#define SO_forSymboleAbwaerts(Index,MaxSymb,MinSymb)                                                                \
  for(Index=(MaxSymb);Index>=(MinSymb);Index--)

#define SO_forFktSymbole(Index)                                                                                     \
  SO_forSymboleAufwaerts(Index,SO_KleinstesFktSymb,SO_GroesstesFktSymb)                         /* alle Operatoren */

#define SO_forSpezSymbole(Index)                                                                                    \
  SO_forSymboleAufwaerts(Index,SO_KleinstesFktSymb,SO_GroesstesSpezSymb)   /* alle Operatoren ausser Existenzquant */

#define SO_forEquationSymbole(Index)                                                                                \
  SO_forSymboleAufwaerts(Index,SO_KleinstesFktSymb,SO_GroesstesSpezSymb)         /* alle Operatoren aus EQUATIONS */\
    if (SO_SymbolInfo[Index].IstEquation)

#define SO_forConclusionSymbole(Index)                                                                              \
  SO_forSymboleAufwaerts(Index,SO_KleinstesFktSymb,SO_GroesstesSpezSymb)        /* alle Operatoren aus CONCLUSION */\
    if (!SO_SymbolInfo[Index].IstEquation)                                                     /* sowie unbenutzte */

#define SO_forParaSymbole(Index)                                                                                    \
  SO_forSymboleAufwaerts(Index,SO_GroesstesSpezSymb+1,SO_GroesstesFktSymb)                       /* falls benutzt, */
                                                                        /* alle Existenzquantifizierungsoperatoren */
#define SO_forKonstSymbole(Index)                                                                                   \
  SO_forFktSymbole(Index)                                                                                           \
    if (!SO_Stelligkeit(Index))                                          /* von allen Operatoren die nullstelligen */

#define SO_forUnkonstSymbole(Index)                                                                                 \
  SO_forFktSymbole(Index)                                                                                           \
    if (SO_Stelligkeit(Index))                                     /* von allen Operatoren die nicht nullstelligen */

#define SO_forFktVarSymbole(Index)                                                                                  \
  SO_forSymboleAufwaerts(Index,SO_ErsteFVar,SO_LetzteFVar)                   /* Datenbank: alle Funktionsvariablen */

#define SO_forFktKonstVarSymbole(Index)                                                                             \
  SO_forSymboleAufwaerts(Index,SO_KleinstesFktSymb,SO_LetzteFVar)        /* alle Operatoren und Funktionsvariablen */
  


/*=================================================================================================================*/
/*= IV. Symbolindizierte Felder                                                                                   =*/
/*=================================================================================================================*/

void SO_VariablenfeldAnpassen(SymbolT NeuesteBaumvariable);                  /* Anpassen des Feldes SO_Stelligkeit */

SymbolT SO_VarDelta(SymbolT Variable, unsigned int Faktor);    /* naechstgroesserer Variablenindex, der Vielfaches */
                                                                                                 /* von Faktor ist */


/*=================================================================================================================*/
/*= VI. Symbolinformationen holen                                                                                 =*/
/*=================================================================================================================*/

void SO_InitAposteriori(void);
/* Besetzt die noetigen Strukturen wie Name, Stelligkeit, etc. mit den Daten aus der Spezifikation */

void SO_FunktionsvariablenEintragen(void);
/* Kommt noch nach InitAposteriori, wenn gerade Datenbank-Spezifikation geparst wurde */


/*=================================================================================================================*/
/*= VIII. Cleanup                                                                                                 =*/
/*=================================================================================================================*/

void SO_CleanUp(void);

#endif
