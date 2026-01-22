#ifndef VARIABLENDIFFMMENGEN_H
#define VARIABLENDIFFMMENGEN_H

#include "general.h"

typedef struct VarDMSetS *VarDMSetT;

VarDMSetT VMS_alloc(int lastId);
VarDMSetT VMS_enlarge(VarDMSetT s, int lastId);
void VMS_dealloc(VarDMSetT s);
void VMS_clear(VarDMSetT s);
void VMS_inc(VarDMSetT s, int x);
void VMS_dec(VarDMSetT s, int x);
VergleichsT VMS_compare(VarDMSetT s);
BOOLEAN VMW_noNeg(VarDMSetT s);
BOOLEAN VMW_noPos(VarDMSetT s);
#endif
