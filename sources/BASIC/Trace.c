/* -------------------------------------------------------------------------- */
/* - Tracing Infrastructure ------------------------------------------------- */
/* -------------------------------------------------------------------------- */

#define DO_TRACE 1 /* 0 1 */
#include "Trace.h"   
#include "Ausgaben.h"

int TR_Tracelevel;

void TR_Traceindent (BOOLEAN indent, int level)
{
  if (indent){
    int i;
    for (i=0; i < level; i++){
      fputs("| ", stdout);
    }
  }
  else {
    printf("%3d:", level);
  }
}
