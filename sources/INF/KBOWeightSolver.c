/* ------------------------------------------------------------------------- */
/* KBOWeightSolver.c                                                         */
/*                                                                           */
/* Maps KBO-constraints to systems of linear inequations and then tests them */
/* for satisfiability in the rationals (not the integers!). Testing in the   */
/* rationals makes the test significantly easier (compared e.g. to full      */
/* omega-test) and faster -- missing only few oppertunities.                 */
/* This is anyway a sufficient test!                                         */
/*                                                                           */
/* Uses Fourier-Motzkin elimination. Handling of equality-constraints should */
/* be improved.                                                              */
/*                                                                           */
/* Bernd Loechner, 05.-10.05.04                                              */
/*                                                                           */
/* ------------------------------------------------------------------------- */

/* ------------------------------------------------------------------------- */
/* -- includes ------------------------------------------------------------- */
/* ------------------------------------------------------------------------- */

#include <limits.h>
#include <stdio.h>
#include <stdlib.h>

#include "KBOWeightSolver.h"
#include "Ordnungen.h"
#include "SpeicherVerwaltung.h"
#include "SymbolGewichte.h"

/* ------------------------------------------------------------------------- */
/* -- parameterization ----------------------------------------------------- */
/* ------------------------------------------------------------------------- */

#define HASH     0 /* 0, 1 */ /* should improve red elim, BUT EXPENSIVE !    */
#define DO_REDUN 0 /* 0, 1 */ /* eliminate redundancies during elimination   */
#define DO_SEL   0 /* 0, 1 */ /* modification of var selection heuristics    */
#define DO_FIRST 1 /* 0, 1 */ /* modification of var selection heuristics    */
#define DO_PRE   1 /* 0, 1 */ /* initial preprocessing: GCD/red elimination  */

#define VERBOSE  0 /* 0, 1 */ /* loads of output                             */

/* ------------------------------------------------------------------------- */
/* -- file local types ----------------------------------------------------- */
/* ------------------------------------------------------------------------- */

typedef long kw_EntryT;
#define KW_ENTRYT_MAX LONG_MAX

typedef kw_EntryT *kw_VectorT;
/* Representation of kw_VectorT vec: 
 * vec[0] = alpha_0, i.e. constant part
 * vec[i] = alpha_i for i = 1...kw_VecLen -1, i.e. all var coefficents
 * if HASH
 *    vec[kw_VecLen] = hash code of vec[1]...vec[kw_VecLen-1]
 */

typedef kw_EntryT *kw_chunkT;

#define KW_CHUNK_SIZE 1000

typedef struct {
  int size;         /* size of chunks (in no. of kw_chunkTs) */
  int alloced;      /* number of alloced chunks */
  int current;      /* number of current chunk */
  int used;         /* used part of current chunk (in no. of kw_EntryTs) */
  kw_chunkT *chunks;
} kw_VecPoolS;

typedef struct {
  int size;         /* alloced size of vecs */
  int len;          /* used part of vecs */
  kw_VectorT *vecs;
} kw_VecSetS;

typedef kw_VecSetS *kw_VecSetT;

typedef struct {
  int size;
  int *poss;
  int *negs;
} kw_VarStatS;

/* ------------------------------------------------------------------------- */
/* -- static variables ----------------------------------------------------- */
/* ------------------------------------------------------------------------- */

static int kw_VecLen;
static kw_VecPoolS kw_VecPool;
static kw_VecSetS kw_Set1, kw_Set2;
static kw_VarStatS kw_VarStat;
static int kw_VecAllocs;

/* ------------------------------------------------------------------------- */
/* -- static functions ----------------------------------------------------- */
/* ------------------------------------------------------------------------- */

static kw_EntryT kw_GCD (kw_EntryT n, kw_EntryT m)
/* there seems to be no gcd in standard libs ??? use Euclid's algorithm */
{
  /* assert(n>0); assert(m>0); */
  while (m != 0){
    kw_EntryT k = m;
    m = n % m;
    n = k;
  }
  return n;
}

static kw_EntryT kw_GCDn(kw_EntryT *ns, int len)
/* returns GCD(ABS(ns[0]),...ABS(ns[len-1])) */
{
  int i;
  kw_EntryT g = 0;
  for (i = 0; i < len; i++){
    kw_EntryT n = ns[i];
    if (n < 0) {
      n = -n;
    }
    if (n > 0){
      if (g != 0){
        g = kw_GCD(n,g);
        if (g == 1){
          return 1;
        }
      }
      else if (n == 1){
        return 1;
      }
      else {
        g = n;
      }
    }
  }
  return g;
}

static void kw_allocNextChunk(void)
{
  if (kw_VecPool.alloced + 1 >= kw_VecPool.size){
    kw_VecPool.size += 10;
    kw_VecPool.chunks = our_realloc(kw_VecPool.chunks,
                                    kw_VecPool.size * sizeof(kw_chunkT),
                                    "kw_VecPool");
  }
  kw_VecPool.chunks[kw_VecPool.alloced] = our_alloc(sizeof(kw_EntryT) *
                                                    KW_CHUNK_SIZE);
  kw_VecPool.alloced++;
}

static void kw_gotoNextChunk(void)
{
  if (kw_VecPool.current + 1 >= kw_VecPool.alloced){
    kw_allocNextChunk();
  }
  kw_VecPool.current++;
  kw_VecPool.used = 0;
}

static void kw_zeroVec(kw_VectorT vec)
{
  int i;
  for (i = 0; i < kw_VecLen + HASH; i++){ 
    vec[i] = 0;
  }
}

static kw_VectorT kw_allocVec(void)
{
  kw_VectorT res;
  if (kw_VecPool.used + kw_VecLen + HASH >= KW_CHUNK_SIZE){
    kw_gotoNextChunk();
  }
  res = &(kw_VecPool.chunks[kw_VecPool.current][kw_VecPool.used]);
  kw_VecPool.used = kw_VecPool.used + kw_VecLen + HASH;
  kw_VecAllocs++;
  kw_zeroVec(res);
  return res;
}

static void kw_printVec(kw_VectorT vec)
{
  int i;
  printf("Vector %p:", vec);
  for (i = 0; i < kw_VecLen; i++){
    printf(" %5ld", vec[i]);
  }
  if (HASH){
    printf(" Hash = %x", vec[kw_VecLen]);
  }
  printf("\n");
}

static void kw_setHash(kw_VectorT vec)
{
  if (HASH){
    if (vec[kw_VecLen] == 0){
      kw_EntryT h = 0;
      int i;
      for (i = 1; i < kw_VecLen; i++){
        h = (h << 3) | (vec[i] & 0x7);
      }
      vec[kw_VecLen] = h;
    }
  }
}

static BOOLEAN kw_hashedVecsEqual(kw_VectorT vec1, kw_VectorT vec2)
{
  int i;
  if (HASH && (vec1[kw_VecLen] != vec2[kw_VecLen])){
    return FALSE;
  }
  for (i = 1; i < kw_VecLen; i++){
    if (vec1[i] != vec2[i]){
      return FALSE;
    }
  }
  return TRUE;
}

static BOOLEAN kw_VecsEqual(kw_VectorT vec1, kw_VectorT vec2)
{
  int i;
  for (i = 1; i < kw_VecLen; i++){
    if (vec1[i] != vec2[i]){
      return FALSE;
    }
  }
  return TRUE;
}

static void kw_VecSetReset(kw_VecSetT vs)
{
  vs->len = 0;
}

static void kw_VecSetPush(kw_VecSetT vs, kw_VectorT v)
{
  if (vs->len == vs->size){
    vs->size += 100;
    vs->vecs = our_realloc(vs->vecs,
                           vs->size * sizeof(kw_VectorT),
                           "kw_VecSetPush");
  }
  vs->vecs[vs->len] = v;
  vs->len++;
}

static void kw_printVecSet(kw_VecSetT vs)
{
  int i;
  printf("\nVecSet %p, Length = %d, Size = %d\n", vs, vs->len, vs->size);
  for (i = 0; i < vs->len; i++){
    kw_printVec(vs->vecs[i]);
  }
  printf("VecSet %p =====================\n\n", vs);
}

static void kw_VarStatInit(void)
{
  if (kw_VarStat.size < kw_VecLen){
    kw_VarStat.size += 10;
    kw_VarStat.poss = our_realloc(kw_VarStat.poss,
                                  kw_VarStat.size * sizeof(int),
                                  "kw_VarStat");
    kw_VarStat.negs = our_realloc(kw_VarStat.negs,
                                  kw_VarStat.size * sizeof(int),
                                  "kw_VarStat");
  }
}

static void kw_printVarStat(void)
{
  int i;
  printf("VarStat: Size = %d\n", kw_VarStat.size);
  printf("VarStat.poss: ");
  for (i = 0; i < kw_VecLen; i++){
    printf(" %5ld", kw_VarStat.poss[i]);
  }
  printf("\nVarStat.negs: ");
  for (i = 0; i < kw_VecLen; i++){
    printf(" %5ld", kw_VarStat.negs[i]);
  }
  printf("\n");
}

static void kw_determineVarStat(kw_VecSetT vs)
{
  int i,j;
  for (i = 0; i < kw_VecLen; i++){
    kw_VarStat.poss[i] = 0;
    kw_VarStat.negs[i] = 0;
  }
  for (j = 0; j < vs->len; j++){
    kw_VectorT vec = vs->vecs[j];
    for (i = 1; i < kw_VecLen; i++){
      if (vec[i] > 0){
        kw_VarStat.poss[i]++;
      }
      else if (vec[i] < 0){
        kw_VarStat.negs[i]++;
      }
    }
  }
}

static void kw_InitTables(SymbolT maxVar, kw_VecSetT from, kw_VecSetT to)
{
  kw_VecLen = SO_VarNummer(maxVar)+1;
  if (kw_VecPool.alloced == 0){
    kw_allocNextChunk();
  }
  kw_VecPool.current = 0;
  kw_VecPool.used = 0;
  kw_VecAllocs = 0;
  kw_VecSetReset(from);
  kw_VecSetReset(to);
  kw_VarStatInit();
}

static void kw_InsertVarConditions(kw_VecSetT vs)
/* add for x = x1,...,x_maxVar
 *   x >= mu
 */
{
  int i;
  for (i = 1; i < kw_VecLen; i++){
    kw_VectorT vec = kw_allocVec();
    vec[i] = 1;
    vec[0] = -SG_KBO_Mu_Gewicht();
    kw_VecSetPush(vs, vec);
  }
}

static void kw_modifyVec(kw_VectorT vec, UTermT t, int sign)
{
  UTermT t0 = TO_NULLTerm(t);
  UTermT t1 = t;
  while (t0 != t1){
    if (TO_TermIstVar(t1)){
      vec[SO_VarNummer(TO_TopSymbol(t1))] += sign;
    } 
    else if (sign > 0){
      vec[0] += SG_SymbolGewichtKBO(TO_TopSymbol(t1));
    }
    else {
      vec[0] -= SG_SymbolGewichtKBO(TO_TopSymbol(t1));
    }
    t1 = TO_Schwanz(t1);
  }
}

static void kw_InsertECons(EConsT e, kw_VecSetT vs)
/* s :>: t   is mapped to  phi s - phi t >= 0
 * s :>= t   is mapped to  phi s - phi t >= 0
 * s :=: t   is mapped to  phi s - phi t >= 0 and
 *                         phi t - phi s >= 0
 */
{
  while (e != NULL){
    kw_VectorT vec = kw_allocVec();
    kw_modifyVec(vec,e->s,1);
    kw_modifyVec(vec,e->t,-1);
    kw_VecSetPush(vs, vec);
    if (e->tp == Eq_C){
      vec = kw_allocVec();
      kw_modifyVec(vec,e->t,1);
      kw_modifyVec(vec,e->s,-1);
      kw_VecSetPush(vs, vec);
    }
    e = e->Nachf;
  }
}

static int kw_determineVar(kw_VecSetT vs)
/* returns 0 iff there is no var left 
 * otherwise: 
 * if there is some var i with poss[i] == 0, negs[i] > 0 then return i
 * let I the set of vars with poss[i]>0, negs[i] == 0 
 * if I not empty then return i\in I that makes poss[i] maximal
 * return i that minimizes poss[i]*negs[i]-poss[i]-negs[i]
*/
{
  int i;
  int res = 0;
  kw_EntryT p = KW_ENTRYT_MAX;
  int pres = 0;
  kw_EntryT pval = 0;
  kw_determineVarStat(vs);
  for (i = 1; i < kw_VecLen; i++){
    if (kw_VarStat.poss[i] == 0 && kw_VarStat.negs[i] > 0){
      return i;
    }
    if (kw_VarStat.poss[i] > 0 && kw_VarStat.negs[i] == 0){
      if (DO_FIRST){
        return i;
      }
      if (kw_VarStat.poss[i] > pval){
        pval = kw_VarStat.poss[i];
        pres = i;
      }
    }
    if (kw_VarStat.poss[i] > 0 && kw_VarStat.negs[i] > 0){
      kw_EntryT q = kw_VarStat.poss[i] * kw_VarStat.negs[i];
      if (DO_SEL){
         q = q - kw_VarStat.poss[i] - kw_VarStat.negs[i];
      }
      if (p > q){
        p = q;
        res = i;
      }
    }
  }
  if (pres != 0){
    return pres;
  }
  return res;
}

static BOOLEAN kw_isBottomVar(int var)
{
  return kw_VarStat.poss[var] == 0 && kw_VarStat.negs[var] > 0;
}

static BOOLEAN kw_isTrivialVar(int var)
{
  return kw_VarStat.poss[var] > 0 && kw_VarStat.negs[var] == 0;
}

static kw_VectorT kw_addScalarTimesVec(kw_VectorT res, int n, kw_VectorT vec)
{
  int i;
  for (i = 0; i < kw_VecLen; i++){
    res[i] += n*vec[i];
  }
}

static kw_VectorT kw_combineVecs(kw_VectorT vecp, kw_VectorT vecn, int var)
{
  kw_EntryT p = vecp[var];
  kw_EntryT n = -vecn[var];
  kw_EntryT g = kw_GCD(p,n);
  if (g > 1){
    if (VERBOSE){
      printf("GCD: %d\n", g);
    }
    p /= g;
    n /= g;
  }
  kw_VectorT res = kw_allocVec();
  kw_addScalarTimesVec(res,n,vecp);
  kw_addScalarTimesVec(res,p,vecn);
  if (VERBOSE){
    printf("combining  "); kw_printVec(vecp);
    printf("and        "); kw_printVec(vecn);
    printf("results in "); kw_printVec(res);
  }
  return res;
}

static BOOLEAN kw_isBotVec(kw_VectorT vec)
/* vec != zeroVec and for all i: vec[i] <= 0 */
{
  int i, n = 0;
  for (i = 0; i < kw_VecLen; i++){
    if (vec[i] > 0){
      return FALSE;
    }
    else if (vec[i] < 0){
      n++;
    }
  }
  return n > 0;
}

static BOOLEAN kw_isTrivVec(kw_VectorT vec)
/* vec is zeroVec or 
 * there is only one i with vec[i] > 0 and for all j vec[j] >= 0
 */
{
  int i, n = 0;
  for (i = 0; i < kw_VecLen; i++){
    if (vec[i] < 0){
      return FALSE;
    }
    else if (vec[i] > 0){
      if (n > 0){
        return FALSE;
      }
      n++;
    }
  }
  return TRUE;
}

static BOOLEAN kw_Combines(kw_VecSetT from, kw_VecSetT to, int var)
{
  int i, j;
  for (i = 0; i < from->len; i++){
    kw_VectorT vecp = from->vecs[i];
    if (vecp[var] > 0){
      for (j = 0; j < from->len; j++){
        kw_VectorT vecn = from->vecs[j];
        if (vecn[var] < 0){
          kw_VectorT vec = kw_combineVecs(vecp,vecn,var);
          if (kw_isBotVec(vec)){
            if (VERBOSE){
              printf("Bottom: "); kw_printVec(vec);
            }
            return TRUE;
          }
          if (!kw_isTrivVec(vec)){
            kw_VecSetPush(to,vec);
          }
          else if (VERBOSE){
            printf("Trivial: "); kw_printVec(vec);
          }
        }
      }
    }
  }
  return FALSE;
}

static BOOLEAN kw_CopyWithZeroEntry(kw_VecSetT from, kw_VecSetT to, int var)
{
  int i;
  for (i = 0; i < from->len; i++){
    kw_VectorT vec = from->vecs[i];
    if (vec[var] == 0){
      kw_VecSetPush(to,vec);
    }
  }
}

static void kw_VectorCancelGCD(kw_VectorT vec)
{
  kw_EntryT g = kw_GCDn(vec,kw_VecLen);
  if (g > 1){
    int i;
    for (i = 0; i < kw_VecLen; i++){
      if (vec[i] != 0){
        vec[i] /= g;
      }
    }
    if (VERBOSE){
      printf("CancelGCD g = %ld ", g);
      kw_printVec(vec);
    }
  }
}

static void kw_VecSetCancelGCD(kw_VecSetT vs)
{
  int i;
  for (i = 0; i< vs->len; i++){
    kw_VectorCancelGCD(vs->vecs[i]);
  }
}

static void kw_VecSetHashAll(kw_VecSetT vs)
{
  if (HASH){
    int i;
    for (i = 0; i< vs->len; i++){
      kw_setHash(vs->vecs[i]);
    }
  }
}

static void kw_RemoveRedundantVecs(kw_VecSetT vs)
{
  int i = 0;
  kw_VecSetHashAll(vs);
  if (VERBOSE){
    printf("Search for redundancies in\n");
    kw_printVecSet(vs);
  }
  while (i < vs->len){
    kw_VectorT vec1 = vs->vecs[i];
    int j = i+1;
    while (j < vs->len){
      kw_VectorT vec2 = vs->vecs[j];
      if (kw_hashedVecsEqual(vec1,vec2)){
        if (VERBOSE){
          printf("Equal: Vec1: "); kw_printVec(vec1);
          printf("       Vec2: "); kw_printVec(vec2);
        }
        vs->len--;
        if (vec2[0] >= vec1[0]){/*vec 2 redundant */
          vs->vecs[j] = vs->vecs[vs->len];
        }
        else {/* vec1 redundant */
          vec1 = vec2;
          vs->vecs[i] = vs->vecs[j];
          vs->vecs[j] = vs->vecs[vs->len];
        }
      }
      else {
        j++;
      }
    }
    i++;
  }
  if (VERBOSE){
    printf("after eliminating redundancies \n");
    kw_printVecSet(vs);
  }
}

static BOOLEAN kw_EliminationStep(kw_VecSetT from, kw_VecSetT to, int var)
{
  if (kw_Combines(from,to,var)){
    return TRUE;
  }
  if(DO_REDUN) kw_VecSetCancelGCD(to);
  kw_CopyWithZeroEntry(from,to,var);
  if(DO_REDUN) kw_RemoveRedundantVecs(to);
  return FALSE;
}

static BOOLEAN kw_isSatisfiable(kw_VecSetT from, kw_VecSetT to)
{
  int i;
  if (DO_PRE){
    kw_VecSetCancelGCD(from);    
    kw_RemoveRedundantVecs(from);
  }
  for (i = 0; i < kw_VecLen; i++){
    int var = kw_determineVar(from);
    if (VERBOSE){
      printf("Set size = %d\n", from->len);
      kw_printVecSet(from);
      kw_printVecSet(to);
      kw_printVarStat();
      printf("Next var: %d\n", var);
    }
    if (var == 0){
      return TRUE;
    }
    if (kw_isBottomVar(var)){
      return FALSE;
    }
    if (kw_isTrivialVar(var)){
      kw_CopyWithZeroEntry(from,to,var);
    }
    else if (kw_EliminationStep(from,to,var)){
      return FALSE;
    }
    {kw_VecSetT tmp = to; to = from; from = tmp;}
    kw_VecSetReset(to);
  }
  /* should not be reached: */
  our_warning("kw_isSatisfiable: outer loop");
  return TRUE;
}

/* ------------------------------------------------------------------------- */
/* -- interface to outside ------------------------------------------------- */
/* ------------------------------------------------------------------------- */

BOOLEAN KW_isSatisfiable(EConsT e/*, SymbolT maxVar*/)
{
  if (ORD_OrdIstKBO){
    BOOLEAN res;
    SymbolT maxVar = CO_neuesteVariable(e);
    kw_VecSetT from = &kw_Set1;
    kw_VecSetT to = &kw_Set2;
    kw_InitTables(maxVar,from,to);
    if (kw_VecLen + HASH > KW_CHUNK_SIZE){
      our_warning("kw_VecLen > KW_CHUNK_SIZE");
      return TRUE;
    }
    kw_InsertVarConditions(from);
    kw_InsertECons(e,from);
    res = kw_isSatisfiable(from,to);
    if (0){
      printf("Allocs %d\n", res ? kw_VecAllocs : -kw_VecAllocs);
    }
    return res;
  }
  return TRUE;
}

/* ------------------------------------------------------------------------- */
/* -- EOF ------------------------------------------------------------------ */
/* ------------------------------------------------------------------------- */
