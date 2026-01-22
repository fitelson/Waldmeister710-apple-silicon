/* PreSolver.c
 * 7.7.2003
 * Christian Schmidt
 *
 * Benutzt die eingestellte Ordnung, um triviale Constraints herauszufiltern
 * oder zu vereinfachen.
 */


#include "PreSolver.h"
#include "Ausgaben.h"
#include "Ordnungen.h"
#include "SymbolOperationen.h"
#include "Constraints.h"
#include "LPO.h"
#include "KBO.h"

/* - Parameter -------------------------------------------------------------- */

#define ausgaben 0 /* 0, 1 */

/* - Macros ----------------------------------------------------------------- */

#if ausgaben
#define blah(x) { IO_DruckeFlex x ; }
#else
#define blah(x)
#endif

/* ------------------------------------------------------------------------- */

static EConsT PreSolve(EConsT e, BOOLEAN *result)
{
  EConsT cons;
  EConsT newcons = NULL;
  

  for (cons = e ; cons != NULL ; cons = cons->Nachf){
    if (ORD_TermGroesserRed(cons->t, cons->s)){ /* s < t */
      /* da nur >, = und >= als Constraint-Typ vorhanden folgt: Wiederspruch! */
      CO_deleteEConsList(newcons, NULL); /* bisherigen neuen cons wegwerfen */
      *result = FALSE;
      return NULL;
    }
    
    if (cons->tp != Eq_C){
      if (ORD_TermGroesserRed(cons->s, cons->t)){ 
        /* cons: > oder >=, LPO: > folgt: TRUE (ueberfluessig) */
        *result = TRUE;
        continue; /* diesen Teil des cons weglassen */
      }
    }
      
    if (cons->tp != Gt_C){ /* >= oder = */
      if (TO_TermeGleich(cons->s, cons->t)){
        *result = TRUE;
        continue; /* diesen Teil des cons weglassen */
      }
    }
    /* aktuellen Teil zum neuen cons hinzufuegen */
    newcons = CO_newEcons(cons->s, cons->t, newcons, cons->tp);
  }

  blah(("-----------------------------------------------\n"));
  blah(("+++ PreSolver vereinfacht: %E\n", e));
  
  if (newcons == NULL){
    if (*result){
      blah(("+++ Constraint war trivialerweise erfuellbar!\n"));
    }
    else {
      blah(("+++ Constraint war trivialerweise unerfuellbar!\n"));
    }
  }
  else {
    blah(("+++ zu: %E\n", newcons));
  }
  blah(("-----------------------------------------------\n"));

  return newcons;
}


/* ------------------------------------------------------------------------- */
/* -------- Interface nach aussen ------------------------------------------ */
/* ------------------------------------------------------------------------- */

/* Liefert einen vereinfachten Constraint zurueck:
 * - ist der Constraint NULL, und result == TRUE, so ist er trivial erfuellbar.
 * - ist der Constraint NULL, und result == FALSE, so ist er trivial unerfuellbar. */
EConsT PS_PreSolve(EConsT e, BOOLEAN *result) 
{
  return PreSolve(e, result);
}

/* ------------------------------------------------------------------------- */
/* -------- E O F ---------------------------------------------------------- */
/* ------------------------------------------------------------------------- */
