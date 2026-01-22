/* PreSolver.h
 * 7.7.2003
 * Christian Schmidt
 *
 * Benutzt die eingestellte Ordnung, um triviale Constraints herauszufiltern
 * oder zu vereinfachen.
 */

#ifndef PRESOLVER_H
#define PRESOLVER_H

#include "general.h"
#include "Constraints.h"


/* Liefert einen vereinfachten Constraint zurueck:
 * - ist der Constraint NULL, und result == TRUE, so ist er trivial erfuellbar.
 * - ist der Constraint NULL, und result == FALSE, so ist er trivial unerfuellbar. */
EConsT PS_PreSolve(EConsT e, BOOLEAN *result);

#endif
