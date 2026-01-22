
#ifndef CONFLUENCETREES_H
#define CONFLUENCETREES_H

#include "general.h"
#include "TermOperationen.h"


BOOLEAN CO_GroundJoinable(int n, TermT s, TermT t);
BOOLEAN CO_GroundJoinableWithWitnesses(int n, TermT s, TermT t, 
                                       TermT *s0, TermT *t0);




#endif
