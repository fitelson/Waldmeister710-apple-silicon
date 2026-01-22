/* *************************************************************************
 *
 * KissKDHeapElements.h
 *
 * Typdefinition des ElementT fuer den KDHeap
 *
 * Original KDHeapElements.h von Bernd Loechner, 29.12.2001
 * Angepasst Hendrik Spies 22.05.2002
 *
 * ************************************************************************* */

#ifndef KISS_KDHEAPELEMENTS_H
#define KISS_KDHEAPELEMENTS_H

#include "general.h"
#include "Position.h"


#include "RUndEVerwaltung.h"
#include "Id.h"

typedef struct{
    WId wid;
    unsigned long       *observer;
} KElementT;




#define KEL_EQUAL(/*KElementT* */ x, /*KElementT* */ y)      	               \
  (WID_EQUAL((x)->wid,(y)->wid))

/*#define EL_EXPAND(/ *KElementT* / x)                                           \
  our_error("not yet implemented")*/

/*#define EL_DISPLACE(/ *KElementT* / x)                                         \
  our_error("not yet implemented")*/

/* #define KEL_MINIMIZE(/\*KElementT * *\/ aset, /\*KElementT* *\/ x)			\ */
/*   if (WID_i((aset)->wid)!= WID_i((x)->wid)) {					\ */
/*     our_error("KKDHElements: KEL_MINIMIZE: non-corresponding elements");	\ */
/*   }										\ */
/*   if (WID_w((aset)->wid)> WID_w((x)->wid)) {					\ */
/*     WID_w((aset)->wid) = WID_w((x)->wid);					\ */
/*     WID_i((aset)->wid) = WID_i((x)->wid);					\ */
/*     WID_j((aset)->wid) = WID_j((x)->wid);					\ */
/*     WID_xp((aset)->wid)= WID_xp((x)->wid);					\ */
/*   } else if (WID_w((aset)->wid)== WID_w((x)->wid)) {				\ */
/*     if (WID_i((aset)->wid)> WID_i((x)->wid)) {					\ */
/*       WID_i((aset)->wid)  = WID_i((x)->wid);					\ */
/*       WID_j((aset)->wid)  = WID_j((x)->wid);					\ */
/*       WID_xp((aset)->wid) = WID_xp((x)->wid);					\ */
/*     } else if (WID_i((aset)->wid)== WID_i((x)->wid)) {				\ */
/*       if (WID_j((aset)->wid)> WID_j((x)->wid)) {				\ */
/* 	WID_j((aset)->wid) = WID_j((x)->wid);					\ */
/* 	WID_xp((aset)->wid)= WID_xp((x)->wid);					\ */
/*       } else if (WID_xp((aset)->wid)> WID_xp((x)->wid)){			\ */
/* 	WID_xp((aset)->wid)= WID_xp((x)->wid);					\ */
/*       }										\ */
/*     }										\ */
/*   } */

#define KEL_MINIMIZE( /* KElementT*/ aset, /*KElementT*/ x )	\
    WID_MINIMIZE((aset).wid,(x).wid)


#define KEL_SET_INFTY(/*KElementT*/ x)	 WID_SET_INFTY((x)->wid)

#define KEL_MEMBER_VALID(/*KElementT*/ x)                                         \
  ((TP_lebtZumZeitpunkt_M(RE_getActiveWithBirthday((x)->i),HK_getAbsTime_M())) &&\
   (TP_lebtZumZeitpunkt_M(RE_getActiveWithBirthday((x)->j),HK_getAbsTime_M())))    

#define KEL_IS_INFTY(/*DimensionT*/ d, /*KElementT* */ x)	\
  ((0 == d) ? (maximalWeight() == WID_w((x)->wid)) :		\
  ((1 == d) ? (maximalAbstractTime() == WID_i((x)->wid)) :	\
  TRUE))
  
#define /*long*/ KEL_GET_SET_INDEX(/*KElementT* */ e)             \
  ((long) (RE_getActiveWithBirthday((e)->i),HK_getAbsTime_M()).aEntryObserver)

#define /*BOOLEAN*/ KEL_IS_MEMBER_OF(/*KElementT* */ member, /*KElementT* */ set)                                    \
  ((K_ASET == (set)->type) ?                                                                                         \
      ((K_MEMBER == (member)->type) ? ((member)->i == (set)->i) : FALSE) :                                           \
      ((K_PSET == (set)->type) ?                                                                                     \
          ((K_MEMBER == (member)->type) ? (((set)->t1 == (member)->t1) && ((set)->t2_0 == (member)->t2_0)) : FALSE) :\
          (FALSE)))

#define KEL_GREATER_CONTEXT(/*DimensionT*/ d, /*KElementT* */ x, /*KElementT* */ y, /*void * */ context)         \
  ((4 == d) ?                                                      \
   (!(EL_INTERNAL_GREATER((DimensionT) context, *(x), *(y)))) :    \
   ((EL_INTERNAL_GREATER(d, *(x), *(y) ))))                        \

/* hier wird in Dimension
 *  0 nach w1 gewichtet(lexikographisch verfeinert mit i, j[0] und xp[0])
 *  1 nach Alter gewichtet (juenger ist groesser) lex. verf. j[1], xp[1]
 *  2 wie dim 0, nur mit w1 fuer kritische Ziele (mit j[2], xp[2])
 *  3 wie dim 2, nur mit j[3] und xp[3] fuer krit. Ziele
 */
#define KEL_INTERNAL_GREATER(/*DimensionT*/ d, /*KElementT* */ x, /*KElementT* */ y)\
((0 == d) ? ((((x)->w > (y)->w) ||		                                    \
             (((x)->w == (y)->w) && 				                    \
              (((x)->i > (y)->i) ||				                    \
               (((x)->i == (y)->i) &&			                            \
                (((x)->j > (y)->j) ||			                            \
	         (((x)->j == (y)->j) &&			                            \
	          ((x)->xp > (y)->xp)))))))) :                                      \
 ((1 == d) ? ((((x)->i > (y)->i) ||				                    \
	       (((x)->i == (y)->i) &&			                            \
		(((x)->j > (y)->j) ||			                            \
		 (((x)->j == (y)->j) &&			                            \
		  ((x)->xp > (y)->xp)))))) :                                        \
FALSE

#define KEL_GREATER(/*int*/ d, /*KElementT* */ x, /*KElementT* */ y)           	\
      (((d)==0) ? (WID_GREATER((x)->wid,(y)->wid)) :				\
       (((d)==1) ? (WID_GREATER((y)->wid,(x)->wid)) : 				\
	our_error("KEL_GREATER: ungültige Dimension\n"),FALSE))

/* #define KEL_GREATER(/\*int*\/ d, /\*KElementT* *\/ x, /\*KElementT* *\/ y)           \ */
/* ((d == 0) ?					                               \ */
/*    (((x)->w > (y)->w) ||				                       \ */
/*     (((x)->w == (y)->w) && 				                       \ */
/*      (((x)->i > (y)->i) ||				                       \ */
/*       (((x)->i == (y)->i) &&			                               \ */
/*        (((x)->j > (y)->j) ||			                               \ */
/* 	(((x)->j == (y)->j) &&			                               \ */
/* 	 ((x)->xp > (y)->xp))))))) :                                           \ */
/*  ((1 == d)?                                                                    \ */
/*  (((x)->w == maximalWeight()) ? TRUE :                                         \ */
/*  (((y)->w == maximalWeight()) ? FALSE :                                        \ */
/*  (((x)->i > (y)->i) ||				                               \ */
/*   (((x)->i == (y)->i) &&				                       \ */
/*    (((x)->j > (y)->j) ||				                       \ */
/*     (((x)->j == (y)->j) &&				                       \ */
/*      ((x)->xp > (y)->xp))))))):                                                \ */
/*   (((x)->w < (y)->w) ||				                               \ */
/*     (((x)->w == (y)->w) && 				                       \ */
/*      (((x)->i < (y)->i) ||				                       \ */
/*       (((x)->i == (y)->i) &&			                               \ */
/*        (((x)->j < (y)->j) ||			                               \ */
/* 	(((x)->j == (y)->j) &&			                               \ */
/* 	 ((x)->xp <= (y)->xp))))))))) */

#define PRINT_KELEMENT(/*KElementT* */ x)			               \
  printf("(%ld, %lu, %lu, %lu)", (x)->w, (x)->i, (x)->j, (x)->xp)             \


BOOLEAN KKDE_greater ( unsigned dim, KElementT* x, KElementT* y );
BOOLEAN KKDE_isInfty ( unsigned dim, KElementT* e );
BOOLEAN KKDE_equal   ( unsigned dim, KElementT* x, KElementT* y );

#endif
