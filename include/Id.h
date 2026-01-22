/* **************************************************************************
 * Id 
 * 
 * Identitäten von Fakten
 *
 * 2.9.2003 Jean-Marie Gaillourdet
 * ***************************************************************************/

/* XXX To do: Position an Id angleichen */
#ifndef ID_H
#define ID_H

#include "general.h"
#include "Position.h"
#include "TermOperationen.h"

/*  * = MAX_INT
 *  i,j > 0
 *  xp >= 0
 *
 *  Axiom:        0,0,n
 *  CP:           i,j,xp     i >= j > 0
 *  ACGOverlap:   i,0,xp
 *  IROpfer:      i,j,xp     i <  j
 *  Lemma:        0,f,n      f >  0      
 *  P-Entry:      i,j,*
 *  A-Entry:      i,*,*
 */

#define ID_WILDCARD ULONG_MAX 

typedef struct IdS {
  unsigned long i;
  unsigned long j;
  Position xp;
} Id;
/*  Original JMG:
  unsigned i : 32;
  unsigned j : 32;
  braeuchte ID_WILDCARD als 0xFFFFFFFF=ULONG_MAX 32 Bit
  hakt aber andernorts */


#define ID_i(id)  ((id).i)
#define ID_j(id)  ((id).j)
#define ID_xp(id) ((id).xp)

#define ID_isAxiom(id)      (((id).i == 0) && ((id).j == 0) && ((id).xp > 0))
#define ID_isCP(id)         (((id).i > 0)  && ((id).j > 0)   && ((id).j <= (id).i))
#define ID_isACGOverlap(id) (((id).i > 0)  && ((id).j == 0))
#define ID_isIR(id)         (((id).i > 0)  &&                   ((id).j > (id).i))
#define ID_isPSet(id)       (((id).i > 0)  && ((id).j > 0)            && ((id).xp == ID_WILDCARD))
#define ID_isASet(id)       (((id).i > 0)  && ((id).j == ID_WILDCARD) && ((id).xp == ID_WILDCARD))
#define ID_isLemma(id)      (((id).i == 0) && ((id).j > 0))

typedef struct WIdS {
    WeightT w;
    Id id;
} WId;

#define WID_w(  /* WId */ wid )  ((wid).w)
#define WID_i(  /* WId */ wid )  (ID_i((wid).id))
#define WID_j(  /* WId */ wid )  (ID_j((wid).id))
#define WID_xp( /* WId */ wid )  (ID_xp((wid).id))
#define WID_id( /* WId */ wid )  ((wid).id)

#define WID_GREATER( /*WId*/ x, /*WId*/ y )									\
     ((WID_w(x) > WID_w(y))											\
      || ((WID_w(x) == WID_w(y)) && (WID_i(x) > WID_i(y)))							\
      || ((WID_w(x) == WID_w(y)) && (WID_i(x) == WID_i(y)) && (WID_j(x) > WID_j(y)))				\
      || ((WID_w(x) == WID_w(y)) && (WID_i(x) == WID_i(y)) && (WID_j(x) == WID_j(y)) && (WID_xp(x) > WID_xp(y))))

#define WID_EQUAL( /*WId*/ x, /*WId*/ y ) 	\
     ((WID_w(x) == WID_w(y))			\
      && (WID_i(x) == WID_i(y))			\
      && (WID_j(x) == WID_j(y))			\
      && (WID_xp(x) == WID_xp(y)))
 
#define WID_MAXIMIZE( /*WId*/ aset, /*WId*/ x )	\
if (WID_GREATER((x),(aset)))			\
{						\
    WID_w(aset) = WID_w(x);			\
    WID_id(aset) = WID_id(x);			\
}

#define WID_MINIMIZE( /*WId*/ aset, /*WId*/ x )						\
if (WID_GREATER((aset),(x)))								\
{											\
  if (WID_i(aset) != WID_i(x)) our_error("WID_MINIMIZE Vorbedingung verletzt\n");	\
    WID_w(aset) = WID_w(x);								\
    WID_id(aset) = WID_id(x);								\
}

#define WID_SET_INFTY(/*WId*/ wid)		\
{						\
    WID_w(wid) = maximalWeight();		\
    WID_i(wid) = maximalAbstractTime();		\
    WID_j(wid) = maximalAbstractTime();		\
    WID_xp(wid) = maximalPosition();		\
}

#define WID_SET_ZERO(/*WId*/ wid)		\
{						\
    WID_w(wid) = 0;				\
    WID_i(wid) = 0;				\
    WID_j(wid) = 0;				\
    WID_xp(wid) = 0;				\
}



#define WID_isAxiom(wid)       ID_isAxiom((wid).id)
#define WID_isCP(wid)          ID_isCP((wid).id)
#define WID_isACGOverlap(wid)  ID_isACGOverlap((wid).id)
#define WID_isIR(wid)          ID_isIR((wid).id)
#define WID_isPSet(wid)        ID_isPSet((wid).id)
#define WID_isASet(wid)        ID_isASet((wid).id)
#define WID_isLemma(wid)       ID_isLemma((wid).id)

typedef struct {
    TermT lhs; 
    TermT rhs;
    Id id;
    BOOLEAN isCP;
} IdMitTermenS, *IdMitTermenT;

#endif /* ID_H */

