/* ACGrundreduzibilitaet.c
 * 23.10.2002
 * Christian Schmidt
 */

#include "general.h"
#include "ACGrundreduzibilitaet.h"
#include "Ausgaben.h"
#include "MissMarple.h"
#include "Ordnungen.h"
#include "SymbolOperationen.h"
#include "Constraints.h"

/* fuer Test von Suffsolver */
#include "SuffSolver.h"

/* fuer Test von KBOSuffsolver */
#include "KBOSuffSolver.h"

#include "PreSolver.h"

#ifndef CCE_Source
#include "Parameter.h"
#else
#include "parse-util.h"
#define PA_acgRechts() (1)
#define PA_skRechts() (1)
/*#define PA_doACG() (1)*/
#endif

/* - Parameter -------------------------------------------------------------- */

#define ausgaben 0 /* 0, 1 */

static BOOLEAN ACG_flag = TRUE;

/* - Macros ----------------------------------------------------------------- */

#if ausgaben
#define blah(x) {if (ACG_flag) { IO_DruckeFlex x ; } }
#else
#define blah(x)
#endif


static BOOLEAN  ac_DoItReally = FALSE;

static EConsT erzeugeECons(UTermT Term, EConsT econs) 
{

  UTermT NULLt = TO_NULLTerm(Term);  /* stellt das Ende des Terms dar */
  UTermT teilterm = Term;


  while(teilterm != NULLt) {

    if (MM_ACC_aktiviert(TO_TopSymbol(teilterm))) { 

      if (TO_TopSymbol(TO_ZweiterTeilterm(teilterm)) == TO_TopSymbol(teilterm)) {

        /* top(t1) element AC _und_ top(t2) == top(t1) */
        econs = CO_newEcons(TO_ErsterTeilterm(TO_ZweiterTeilterm(teilterm)), 
                            TO_ErsterTeilterm(teilterm), econs, Ge_C);
	
      } else {
	
        /* top(t1) element AC _und_ top(t2) =/= top(t1) */
        econs = CO_newEcons(TO_ZweiterTeilterm(teilterm), 
                            TO_ErsterTeilterm(teilterm), econs, Ge_C);

      }
    }
    teilterm = TO_Schwanz(teilterm);
  }
  return econs;
}

static EConsT erzeugeSECons(UTermT Term, EConsT econs) 
{
  UTermT NULLt = TO_NULLTerm(Term);  /* stellt das Ende des Terms dar */
  UTermT teilterm = Term;
  UTermT t1, t2;

  while(teilterm != NULLt) {
    if (MM_ACC_aktiviert(TO_TopSymbol(teilterm))) { 
      t1 = TO_ErsterTeilterm(teilterm);
      t2 = TO_ZweiterTeilterm(teilterm);
      if (TO_TopSymbol(teilterm) == TO_TopSymbol(t2)) {
        t2 = TO_ErsterTeilterm(t2);
        if (!TO_TermeGleich(t1,t2)){
          econs = CO_newEcons(t2,t1, econs, Gt_C);
        }
      }
      else {
        if (!TO_TermeGleich(t1,t2)){
          econs = CO_newEcons(t2,t1, econs, Gt_C);
        }
      }
    }
    teilterm = TO_Schwanz(teilterm);
  }
  return econs;
}

static BOOLEAN isSatisfiable(EConsT e)
{
  BOOLEAN satisfiable = TRUE;
  BOOLEAN suffsatisfiable = TRUE;
  BOOLEAN suffsatisfiable2 = TRUE;
  BOOLEAN suffsatisfiable3 = TRUE;

  /* ------------- PreSolver aufrufen ----------------- */
  BOOLEAN result;
  EConsT econs = PS_PreSolve(e, &result);

  if (econs == NULL){
    if(0)IO_DruckeFlex("### %b\n", result);
    /*(void) Suff_satisfiable3(e); */
    if (result){
      return TRUE;
    }
    else {
      return FALSE;
    }
  } 
  /* ------------- PreSolver aufrufen ------ ENDE ------ */

#if 0
  if (econs != NULL){
    if (ORD_OrdIstLPO){
      satisfiable      = CO_satisfiable(econs);
      suffsatisfiable2 = Suff_satisfiable2(econs);
      suffsatisfiable3 = Suff_satisfiable3(econs);
      if (suffsatisfiable3 != suffsatisfiable2){
        blah(("###########\n#########\n########\n 2vs3: %b vs %b %E\n###########\n#########\n########\n", 
             suffsatisfiable2, suffsatisfiable3, econs));
        our_error("murks");
      }
      if (1) {
        if ((satisfiable == TRUE) && (suffsatisfiable2 == FALSE)){
          blah(("Full: %b, Suff: %b  %E Arrrrgh\n", 
                satisfiable, suffsatisfiable2, econs));
        }
        if ((satisfiable == FALSE) && (suffsatisfiable2 == TRUE)){
          blah(("Full1: %b, Suff: %b  %E !\n", 
                satisfiable, suffsatisfiable2, econs));
        }
        else if (1) {
          blah(("Full2: %b, Suff: %b \n", satisfiable, suffsatisfiable2));
        }
      }
    }
    else if (ORD_OrdIstKBO){
      /*      suffsatisfiable = Suff_satisfiable(econs);*/
      /*      satisfiable = suffsatisfiable;*/ /* dreaming of decision procedure */
#if 0
      satisfiable      = KBOSuff_isSatisfiable(econs);
      suffsatisfiable  = Suff_satisfiable(econs);
      if (satisfiable != suffsatisfiable2){
        blah(("### Achtung! KBOSuff: %b Suff2: %b cons: %E\n",
              satisfiable, suffsatisfiable2, econs));
      }
//      blah(("### Vergleich: KBOSuff: %b Suff: %b Suff2: %b cons: %E\n",
//            satisfiable, suffsatisfiable, suffsatisfiable2, econs));
//      if (suffsatisfiable != suffsatisfiable2){
//        blah(("### Achtung! Suff: %b Suff2: %b cons: %E\n",
//              suffsatisfiable, suffsatisfiable2, econs));
//      }
#endif
      suffsatisfiable2 = Suff_satisfiable2(econs);
      suffsatisfiable3 = Suff_satisfiable3(econs);
      if (suffsatisfiable3 != suffsatisfiable2){
        blah(("###########\n#########\n########\n 2vs3: %b vs %b %E\n###########\n#########\n########\n", 
             suffsatisfiable2, suffsatisfiable3, econs));
        our_error("murks");
      }
      satisfiable = suffsatisfiable2;
    }
  }
  if (0){
    blah(("###(%E,%b)\n", econs,satisfiable));
  }
#endif
  if (econs != NULL){
    satisfiable = Suff_satisfiable3(econs);
  }
  CO_deleteEConsList(econs,NULL);
  return satisfiable;
}

/* ------------------------------------------------------------------------- */
/* -------- Interface nach aussen ------------------------------------------ */
/* ------------------------------------------------------------------------- */

BOOLEAN AC_FaktumGrundreduzibel(RegelOderGleichungsT Objekt) 
{
  return AC_TermeGrundreduzibel(Objekt->linkeSeite, Objekt->rechteSeite);
}

BOOLEAN AC_TermeGrundreduzibel(UTermT Term1, UTermT Term2) 
{
  if (ac_DoItReally){
    EConsT econs = NULL;
    BOOLEAN satisfiable;

    econs = erzeugeECons(Term1, econs);  

    if (PA_acgRechts()){
      econs = erzeugeECons(Term2, econs);
    }

    if (econs != NULL){
      satisfiable = isSatisfiable(econs);
      CO_deleteEConsList(econs,NULL);
      return !satisfiable; /* wenn econs nicht erfuellbar,
                              dann existiert keine AC_ir_reduzibele Grundinstanz,
                              dann sind die Terme AC-Grundreduzibel
                              -> bilde _nur_ ACKPs */
    }
  }
  return FALSE;
}

void AC_lohntSich(void)
{
  ac_DoItReally = PA_doACG();
}

BOOLEAN AC_FaktumStrengKonsistent(RegelOderGleichungsT Objekt)
{
  return AC_TermeStrengKonsistent(Objekt->linkeSeite, Objekt->rechteSeite);
}

BOOLEAN AC_TermeStrengKonsistent(UTermT Term1, UTermT Term2)
{
  if (1&&ac_DoItReally){
    EConsT econs = NULL;
    BOOLEAN satisfiable;

    econs = erzeugeSECons(Term1, econs);  
    
    if (PA_skRechts()){
      econs = erzeugeSECons(Term2, econs);
    }

    if (econs != NULL){
      satisfiable = isSatisfiable(econs);
      CO_deleteEConsList (econs,NULL);
      return !satisfiable; /* wenn econs nicht erfuellbar,
                              dann existiert keine AC_ir_reduzibele Grundinstanz,
                              dann sind die Terme AC-Grundreduzibel
                              -> bilde _nur_ ACKPs */
    }
  }
  return FALSE;
}

static BOOLEAN stelleACReduzibel(UTermT t)
{
  if (MM_ACC_aktiviert(TO_TopSymbol(t))){
    UTermT t1 = TO_ErsterTeilterm(t);
    UTermT t2 = TO_ZweiterTeilterm(t);
    if (TO_TopSymbol(t1) == TO_TopSymbol(t)){ /* A-reduzibel */
      return TRUE;
    }
    if (TO_TopSymbol(t2) == TO_TopSymbol(t)){ /* C'-reduzibel? */
      return ORD_TermGroesserRed(t1,TO_ErsterTeilterm(t2));
    }
    /* C-reduzibel? */
    return ORD_TermGroesserRed(t1,t2);
  }
  return FALSE;
}

BOOLEAN AC_Reduzibel(UTermT t)
{
  if (ac_DoItReally){
    UTermT t0 = TO_NULLTerm(t);
    UTermT t1 = t;
    while (t1 != t0){
      if (stelleACReduzibel(t1)){
        return TRUE;
      }
      t1 = TO_Schwanz(t1);
    }
  }
  return FALSE;
}

/* ------------------------------------------------------------------------- */
/* -------- E O F ---------------------------------------------------------- */
/* ------------------------------------------------------------------------- */
