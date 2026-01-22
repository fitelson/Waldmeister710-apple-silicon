
#ifndef CONFLUENCETREES_INTERN_H
#define CONFLUENCETREES_INTERN_H

#include "general.h"
#include "TermOperationen.h"


BOOLEAN CO_GroundJoinable1(TermT s, TermT t);
BOOLEAN CO_GroundJoinableWithWitnesses1(TermT s, TermT t, 
                                        TermT *s0, TermT *t0);

BOOLEAN CO_GroundJoinable2(TermT s, TermT t);
BOOLEAN CO_GroundJoinableWithWitnesses2(TermT s, TermT t, 
                                        TermT *s0, TermT *t0);

BOOLEAN CO_GroundJoinable3(TermT s, TermT t);
BOOLEAN CO_GroundJoinableWithWitnesses3(TermT s, TermT t, 
                                        TermT *s0, TermT *t0);

BOOLEAN CO_GroundJoinable4(TermT s, TermT t);
BOOLEAN CO_GroundJoinableWithWitnesses4(TermT s, TermT t, 
                                        TermT *s0, TermT *t0);

BOOLEAN CO_GroundJoinable5(TermT s, TermT t);
BOOLEAN CO_GroundJoinableWithWitnesses5(TermT s, TermT t, 
                                        TermT *s0, TermT *t0);

BOOLEAN CO_GroundJoinable6(TermT s, TermT t);
BOOLEAN CO_GroundJoinableWithWitnesses6(TermT s, TermT t, 
                                        TermT *s0, TermT *t0);

BOOLEAN CO_GroundJoinable7(TermT s, TermT t);
BOOLEAN CO_GroundJoinableWithWitnesses7(TermT s, TermT t, 
                                        TermT *s0, TermT *t0);

BOOLEAN CO_GroundJoinable8(TermT s, TermT t);
BOOLEAN CO_GroundJoinableWithWitnesses8(TermT s, TermT t, 
                                        TermT *s0, TermT *t0);

BOOLEAN CO_GroundJoinable30(TermT s, TermT t);
BOOLEAN CO_GroundJoinableWithWitnesses30(TermT s, TermT t, 
                                        TermT *s0, TermT *t0);

BOOLEAN CO_GroundJoinable31(TermT s, TermT t);
BOOLEAN CO_GroundJoinableWithWitnesses31(TermT s, TermT t, 
                                        TermT *s0, TermT *t0);

BOOLEAN CO_GroundJoinable32(TermT s, TermT t);
BOOLEAN CO_GroundJoinableWithWitnesses32(TermT s, TermT t, 
                                        TermT *s0, TermT *t0);

BOOLEAN CO_GroundJoinable40(TermT s, TermT t);
BOOLEAN CO_GroundJoinableWithWitnesses40(TermT s, TermT t, 
                                        TermT *s0, TermT *t0);

BOOLEAN CO_GroundJoinable41(TermT s, TermT t);
BOOLEAN CO_GroundJoinableWithWitnesses41(TermT s, TermT t, 
                                        TermT *s0, TermT *t0);

BOOLEAN CO_GroundJoinable42(TermT s, TermT t);
BOOLEAN CO_GroundJoinableWithWitnesses42(TermT s, TermT t, 
                                        TermT *s0, TermT *t0);

BOOLEAN CO_GroundJoinable50(TermT s, TermT t);
BOOLEAN CO_GroundJoinableWithWitnesses50(TermT s, TermT t, 
                                        TermT *s0, TermT *t0);

BOOLEAN CO_GroundJoinable51(TermT s, TermT t);
BOOLEAN CO_GroundJoinableWithWitnesses51(TermT s, TermT t, 
                                        TermT *s0, TermT *t0);

BOOLEAN CO_GroundJoinable52(TermT s, TermT t);
BOOLEAN CO_GroundJoinableWithWitnesses52(TermT s, TermT t, 
                                        TermT *s0, TermT *t0);

BOOLEAN CO_GroundJoinable60(TermT s, TermT t);
BOOLEAN CO_GroundJoinableWithWitnesses60(TermT s, TermT t, 
                                        TermT *s0, TermT *t0);

BOOLEAN CO_GroundJoinable61(TermT s, TermT t);
BOOLEAN CO_GroundJoinableWithWitnesses61(TermT s, TermT t, 
                                        TermT *s0, TermT *t0);

BOOLEAN CO_GroundJoinable62(TermT s, TermT t);
BOOLEAN CO_GroundJoinableWithWitnesses62(TermT s, TermT t, 
                                        TermT *s0, TermT *t0);

BOOLEAN CO_GroundJoinable70(TermT s, TermT t);
BOOLEAN CO_GroundJoinableWithWitnesses70(TermT s, TermT t, 
                                        TermT *s0, TermT *t0);

BOOLEAN CO_GroundJoinable71(TermT s, TermT t);
BOOLEAN CO_GroundJoinableWithWitnesses71(TermT s, TermT t, 
                                        TermT *s0, TermT *t0);

BOOLEAN CO_GroundJoinable72(TermT s, TermT t);
BOOLEAN CO_GroundJoinableWithWitnesses72(TermT s, TermT t, 
                                        TermT *s0, TermT *t0);

BOOLEAN CO_GroundJoinable80(TermT s, TermT t);
BOOLEAN CO_GroundJoinableWithWitnesses80(TermT s, TermT t, 
                                        TermT *s0, TermT *t0);

/* -------------------------------------------------------------------------- */
/* - parameterization ------------------------------------------------------- */
/* -------------------------------------------------------------------------- */

#define DO_TRACE          0 /* 0, 1 */
#include "Trace.h"

#if 1
#define TEX_OUTPUT        (DO_TRACE || 1) /* 0, 1 */
#else

extern BOOLEAN CO_texOutput;

#define TEX_OUTPUT        (DO_TRACE || CO_texOutput)
#endif
#define TEX_OUTPUT_F      0 /* 0, 1 */
#define VERBOSE           0 /* 0, 1 */
#define LIMIT_DEPTH       0 /* 0, 1 */
#define TRY_ALL_EQNS      0 /* 0, 1 */
#define GIVE_UP_BY_LENGTH_LIMIT 1 /* 0, 1 */
#define PREFER_D1         1 /* 0, 1 */

/* Einschraenkungen zwecks Vermeiden zu langer Wartereien... */

#define DO_NODE_COUNTS    0 /* 0, 1 */
#define MAX_NODE_COUNT    100 

#define DEPTH_LIMIT            2000

extern unsigned int length_limit;
extern unsigned int depth_limit;
extern BOOLEAN do_witnesses;
extern TermT witness_l;
extern TermT witness_r;




#endif
