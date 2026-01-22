/*=================================================================================================================
  =                                                                                                               =
  =  SymbolOperationen.c                                                                                          =
  =                                                                                                               =
  =  In diesem Modul sind die Operationen auf Symbolen, d.h. auf Variablen-,                                      =
  =  Konstanten- und Funktions-Symbolen, realisiert.                                                              =
  =                                                                                                               =
  =  24.02.94 Angelegt. Arnim.                                                                                    =
  =  25.02.94 1 Fehler beseitigt, IntegerVergleich. Thomas.                                                       =
  =  25.02.94 Weitergemacht. Arnim.                                                                               =
  =  04.03.94 Kommentiert, etc.                                                                                   =
  =                                                                                                               =
  =================================================================================================================*/

#include "Ausgaben.h"
#include "FehlerBehandlung.h"
#include "general.h"
#include "KBO.h"
#include <stdlib.h>
#include <string.h>
#include "SymbolOperationen.h"

#ifdef CCE_Source
#include "parse-util.h"
#else
#include "parser.h"
#include "Parameter.h"
#include "PhilMarlow.h"
#include "SpezNormierung.h"
#endif

/*=================================================================================================================*/
/*= II. Globale Variablen                                                                                         =*/
/*=================================================================================================================*/

SymbolT SO_GroesstesSpezSymb;
SymbolT SO_GroesstesFktSymb;
SymbolT SO_LetzteFVar;
SymbolT SO_EQN;
SymbolT SO_EQ;
SymbolT SO_TRUE;
SymbolT SO_FALSE;
SymbolT SO_const1;
SymbolT SO_const2;
SymbolT SO_NeuesteVariable;

unsigned int SO_AnzahlFunktionen;
unsigned int SO_MaximaleStelligkeit;
unsigned int SO_AnzahlVariablen;

struct SO_SymbolInfoS *SO_SymbolInfo;
static unsigned int so_GroesseSymbolInfo;

static void so_SymbolInfoSei(SymbolT i, char *Name, unsigned int Stelligkeit, BOOLEAN IstEquation)
{
  SO_SymbolInfo[i].Name = Name;
  SO_SymbolInfo[i].Namenslaenge = strlen(Name);
  SO_SymbolInfo[i].Stelligkeit = Stelligkeit;
  SO_SymbolInfo[i].IstEquation = IstEquation;
  SO_SymbolInfo[i].IstAC = FALSE;
  SO_SymbolInfo[i].DistribuiertUeber = SO_ErstesVarSymb;
  SO_SymbolInfo[i].Cancellation = FALSE;

  Maximieren(&SO_MaximaleStelligkeit, Stelligkeit);
}

/*=================================================================================================================*/
/*= IV. Symbolindizierte Felder                                                                                   =*/
/*=================================================================================================================*/

static void ArraysMitFunktionsInformationenAnlegen(void)
{
  so_GroesseSymbolInfo = SO_AnzahlFunktionen+SO_AnzErsterVariablen;
  SO_AnzahlVariablen = SO_AnzErsterVariablen;
  SO_SymbolInfo = negarray_alloc(so_GroesseSymbolInfo, struct SO_SymbolInfoS, SO_AnzahlVariablen);
}


void SO_VariablenfeldAnpassen(SymbolT NeuesteBaumvariable)                   /* Anpassen des Feldes SO_Stelligkeit */
{                                                                            /* und des Feldes SO_FunktionsGewicht */
  if (SO_VariableIstNeuer(NeuesteBaumvariable,SO_NummerVar(SO_AnzahlVariablen))) {
    unsigned int AnzVariablenNeu = minKXgrY(SO_AnzErsterVariablen,-NeuesteBaumvariable);
    unsigned int GroesseSymbolInfoNeu = AnzVariablenNeu+(SO_LetzteFVar+1);
    struct SO_SymbolInfoS *SymbolInfoNeu = negarray_alloc(GroesseSymbolInfoNeu, struct SO_SymbolInfoS, AnzVariablenNeu);
    SymbolT i;
    SO_forSymboleAbwaerts(i,SO_LetzteFVar,SO_NummerVar(SO_AnzahlVariablen))
      SymbolInfoNeu[i] = SO_SymbolInfo[i];
    negarray_dealloc(SO_SymbolInfo,SO_AnzahlVariablen);
    SO_SymbolInfo = SymbolInfoNeu;
    SO_forSymboleAbwaerts(i,SO_NaechstesVarSymb(SO_NummerVar(SO_AnzahlVariablen)),SO_NummerVar(AnzVariablenNeu))
      so_SymbolInfoSei(i, "",0,FALSE);
    so_GroesseSymbolInfo = GroesseSymbolInfoNeu;
    SO_AnzahlVariablen = AnzVariablenNeu;
    SO_NeuesteVariable = SO_NummerVar(SO_AnzahlVariablen);
  }
}

SymbolT SO_VarDelta(SymbolT Variable, unsigned int Faktor)
{
  return -minKXgrY(Faktor,-Variable);
}

#ifndef CCE_Source
static void InTermErmitteln(term t)
{
  if (!variablep(t)) {
    func_symb f = term_func(t);
    SymbolT g = SN_WMvonDISCOUNT(f);
    short int i;
    SO_SymbolInfo[g].IstEquation = TRUE;
    for (i=sig_arity(term_func(t))-1; i>=0; i--)
      InTermErmitteln(term_arg(t,i));
  }
}


static void EquationSymboleErmitteln(void)
{
  eq_list e;
  for (e=get_equations(); e; e=eq_rest(e)) {
    InTermErmitteln(eq_first_lhs(e));
    InTermErmitteln(eq_first_rhs(e));
  }
} /* Gehoert bei Revision "on the fly" erledigt. */
#endif


/*=================================================================================================================*/
/*= VI. Symbolinformationen holen                                                                                 =*/
/*=================================================================================================================*/

static unsigned int so_AnzahlFunktionssymboleInSpez(void)
{
  func_symb_list p;
  unsigned int res = 0;

  for (p = get_sig(); p; p = sig_rest(p)){                                             /* Funktionssymbole zaehlen */
    res++;
  }
  return res;
}

static void so_FunktionssymboleVonParserUebernehmen(void)
{
  func_symb_list p;
  SO_MaximaleStelligkeit = 0;
  for (p = get_sig(); p; p = sig_rest(p)) {
    func_symb f = sig_first(p);
    SymbolT F = SN_WMvonDISCOUNT(f);
    so_SymbolInfoSei(F, map_func(f),sig_arity(f),FALSE);
  }
}

static void so_SonstigeSymboleEintragen(void)
{
  SymbolT i;
  SO_forSymboleAbwaerts(i,SO_ErstesVarSymb,SO_NummerVar(SO_AnzErsterVariablen))
    so_SymbolInfoSei(i, "",0,FALSE);

  SO_const1 = SO_GroesstesSpezSymb - 1;
  SO_const2 = SO_GroesstesSpezSymb;
  so_SymbolInfoSei(SO_const1, "const1",0,TRUE);
  so_SymbolInfoSei(SO_const2, "const2",0,TRUE);

#ifndef CCE_Source
  if (PM_Existenzziele()) {
    SO_EQ    = SO_GroesstesSpezSymb + 1;
    SO_TRUE  = SO_GroesstesSpezSymb + 2;
    SO_FALSE = SO_GroesstesSpezSymb + 3;
    so_SymbolInfoSei(SO_EQ, "eq",2,FALSE);
    so_SymbolInfoSei(SO_TRUE, "true",0,FALSE);
    so_SymbolInfoSei(SO_FALSE, "false",0,FALSE);
  }
#endif
#define EQN_SYM 1 /* 0 */
#if EQN_SYM
  SO_EQN = SO_GroesstesFktSymb;
  so_SymbolInfoSei(SO_EQN, "===",2,TRUE);
#endif
}

void SO_InitAposteriori(void)
{
  SO_AnzahlFunktionen = so_AnzahlFunktionssymboleInSpez();
  if (TRUE){
    SO_GroesstesSpezSymb = SO_AnzahlFunktionen - 1 + 2; /* const1, const2 */
    SO_AnzahlFunktionen += 2; /* const1, const2 */
  }
#ifndef CCE_Source
  SO_AnzahlFunktionen += PM_Existenzziele ? 3 : 0;
#endif
  if (EQN_SYM){
    SO_AnzahlFunktionen += 1;
  }
  SO_GroesstesFktSymb = SO_AnzahlFunktionen - 1;
  SO_LetzteFVar = SO_GroesstesFktSymb;
  ArraysMitFunktionsInformationenAnlegen();

  so_FunktionssymboleVonParserUebernehmen();
#ifndef CCE_Source
  EquationSymboleErmitteln();
#endif
  so_SonstigeSymboleEintragen();
}

static unsigned int AnzahlFunktionsvariablen(void)
{
  unsigned int Anzahl = 0;
  func_symb_list sig;
  for (sig = get_sig(); sig; sig = sig_rest(sig))
    Anzahl += sig_is_fvar(sig_first(sig)) ? 1 : 0;
  return Anzahl;
}
  
void SO_FunktionsvariablenEintragen(void)
{
  func_symb_list sig = get_sig();
  SymbolT i;
  SO_LetzteFVar = SO_ErsteFVar+AnzahlFunktionsvariablen()-1;
  so_GroesseSymbolInfo = SO_LetzteFVar+1+SO_AnzErsterVariablen;
  negarray_realloc(SO_SymbolInfo, so_GroesseSymbolInfo, struct SO_SymbolInfoS, SO_AnzErsterVariablen);
  SO_forFktVarSymbole(i) {
    func_symb f;
    while (!sig_is_fvar(f = sig_first(sig)))
      sig = sig_rest(sig);
    so_SymbolInfoSei(i, "",sig_arity(f),FALSE);
    sig = sig_rest(sig);
  }
}




/*=================================================================================================================*/
/*= VIII. Cleanup                                                                                                 =*/
/*=================================================================================================================*/

void SO_CleanUp(void)
{
  negarray_dealloc(SO_SymbolInfo,SO_AnzahlVariablen);
}

