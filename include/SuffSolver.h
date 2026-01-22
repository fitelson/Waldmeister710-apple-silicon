/* SuffSolver.h
 * 21.11.2002
 * Christian Schmidt
 */

#ifndef SUFFSOLVER_H
#define SUFFSOLVER_H

#include "general.h"
#include "Constraints.h"

/* Nummern fuer die Terme, $i */
typedef unsigned short ExtraConstT;

typedef struct SubstitutionT {
  unsigned short size;
  UTermT* bindings;
} Substitution;

BOOLEAN Suff_satisfiable(EConsT e);
BOOLEAN Suff_satisfiable2(EConsT e);
BOOLEAN Suff_satisfiable3(EConsT e);

#endif
