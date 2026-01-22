#include "KissKDHeapElements.h"
#include "KissKDHeap.h"
#include "FehlerBehandlung.h"


/* Dimension 0 ist die Heuristic lex.comb.(w,i,j,xp)
 *  und Dimension 1 ist die Heuristic ^-1
 */
BOOLEAN KKDE_greater (unsigned dim, KElementT* x, KElementT* y)
{
  if (0 == dim) {
    return KEL_GREATER(0,x,y);
  } else if (1 == dim) {
    return KEL_GREATER(0,y,x);
  } else {
    our_error("KissKDHeapElements: KKDE_greater: dimension > 1 not implemented");
    return FALSE;
  }
}


BOOLEAN KKDE_equal(unsigned dim, KElementT* x, KElementT* y) {
  if (0 == dim) {
    return KEL_EQUAL(x, y);
  } else { /*egal, gleich ist gleich*/
    return KEL_EQUAL(x, y);
  }
}

BOOLEAN KKDE_isInfty(unsigned dim, KElementT* e) 
{
  if (0 == dim ) {
    return KEL_IS_INFTY(0, e);
  } else if (1 == dim) {
    return KEL_IS_INFTY(0, e);
  } else {
    our_error("KissKDHeapElements: KKDE_greater: dimension > 1 not implemented");
    return FALSE;
  }
}

