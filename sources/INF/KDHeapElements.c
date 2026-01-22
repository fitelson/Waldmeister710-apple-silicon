#include "KDHeapElements.h"
#include "KDHeap.h"


BOOLEAN KDE_greater (unsigned dim, ElementT* x, ElementT* y, void* context)
{
  return EL_GREATER(dim,x,y);
}

BOOLEAN KDE_isInfty(unsigned dim, ElementT* e) 
{
  return EL_IS_INFTY(dim,e);
}

