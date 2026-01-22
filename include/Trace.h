#ifndef TRACE_H
#define TRACE_H

#include "general.h"

#ifndef DO_TRACE
#error "For using Trace.h DO_TRACE must be defined, either to 0 or to 1"
#endif

#ifndef TRACE_INDENT /* 0,1 */
#define TRACE_INDENT 1 /* Default: 1 */
#endif 

#ifndef TRACE_GUARD
#define TRACE_GUARD TRUE
#endif
/* -------------------------------------------------------------------------- */
/* - Tracing Infrastructure ------------------------------------------------- */
/* -------------------------------------------------------------------------- */

#if DO_TRACE == 1

extern int TR_Tracelevel;

void TR_Traceindent (BOOLEAN indent, int level);


#define OPENTRACE(x)  \
 {if(TRACE_GUARD){TR_Traceindent(TRACE_INDENT, TR_Tracelevel++);IO_DruckeFlex x;}}
#define CLOSETRACE(x) \
 {if(TRACE_GUARD){TR_Traceindent(TRACE_INDENT, --TR_Tracelevel);IO_DruckeFlex x;}}
#define RESTRACE(x)   \
 {if(TRACE_GUARD){TR_Traceindent(TRACE_INDENT, TR_Tracelevel);IO_DruckeFlex x;}}

#else

#define OPENTRACE(x)  /* nix */
#define CLOSETRACE(x) /* nix */
#define RESTRACE(x)   /* nix */

#endif

#endif
