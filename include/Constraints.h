/* Constraints.h
 * 7.12.2000 
 * Bernd Loechner
 */

#ifndef CONSTRAINTS_H
#define CONSTRAINTS_H

#include "SymbolOperationen.h"
#include "TermOperationen.h"

typedef enum {Gt_C, Eq_C, Ge_C} Constraint;
typedef enum {Abbrechen, Weitersuchen} decModeT; 

typedef struct ECons {
  struct ECons *Nachf;
  UTermT s, t;
  Constraint tp;
} *EConsT, EConsZellenT;

typedef UTermT *Bindings;
typedef struct {
  Bindings bindings;
  EConsT   econses;
  unsigned long id;
} *ContextT, ContextStruct;

typedef struct DCons {
  struct DCons *Nachf;
  ContextT c;
} *DConsT, DConsZellenT;

BOOLEAN CO_cEQ (ContextT c, UTermT s, UTermT t);
VergleichsT CO_cLPOCMP (ContextT c, UTermT s, UTermT t);
VergleichsT CO_cLPOCMPGTEQ (ContextT c, UTermT s, UTermT t);

/* folgende Fassungen verwenden/veraendern den currentContext */
VergleichsT CO_LPOCMP (UTermT s, UTermT t);
VergleichsT CO_LPOCMPGTEQ (UTermT s, UTermT t);
ContextT CO_getCurrentContext (void);
void CO_setCurrentContext (ContextT c);

BOOLEAN CO_decompose(UTermT s, UTermT t);

/*
  BOOLEAN CO_GroundJoinable(int n, TermT s, TermT t);
  BOOLEAN CO_GroundJoinableWithWitnesses(int n, TermT s, TermT t, 
                                       TermT *s0, TermT *t0);
*/

BOOLEAN CO_TermGroesserUnif(TermT s, TermT t, TermT u, TermT v);
BOOLEAN CO_HandleGoal(TermT s, TermT t);

void CO_printECons(FILE *strom, EConsT e, BOOLEAN all);
void CO_printContext(FILE *strom, ContextT c);
void CO_printDCons(FILE *strom, DConsT d);
unsigned int CO_DConsLength(DConsT d);
void CO_sortDCons(DConsT d);
DConsT CO_minimizeDCons(DConsT d);

void CO_DruckeVergleichStrom(FILE *strom, VergleichsT v);

void CO_ConstraintsInitialisieren(void);


/* fuer Split von Constraints.c in Constraints.c und ConfluenceTrees.c */
Bindings CO_allocBindings(void);
EConsT CO_newEcons (UTermT s, UTermT t, EConsT next, Constraint tp);
void CO_deepDelete(ContextT c);
BOOLEAN CO_somethingBound(Bindings bs);
int CO_getAllSolutionsInBothDirections(UTermT s, UTermT t, ContextT c,
				                  DConsT *d1, DConsT *d2);
BOOLEAN CO_satisfiable(EConsT e);
BOOLEAN CO_satisfiableUnder(EConsT e, ContextT c);
ContextT CO_solveEQPart (ContextT c);
void CO_deleteDConsList (DConsT d);
void CO_deleteEcons (EConsT e);
void CO_flatDelete(ContextT c);
EConsT CO_constructECons(UTermT s);
void CO_deleteEConsList (EConsT e, EConsT next);
TermT CO_substitute(Bindings bs, UTermT s);
DConsT CO_getAllSolutions(EConsT e, ContextT c);
ContextT CO_newContext (Bindings bs);

void CO_deepDeleteECons(EConsT e);
EConsT CO_substituteECons(Bindings bs, EConsT e);

SymbolT CO_neuesteVariable(EConsT e);       
/* neuestes (= als ganze Zahl kleinstes) Variablensymbol in e 
 * bzw. SO_VorErstemVarSymb(), falls e grund */

#endif
