/* *************************************************************************
 *
 * KDHeapElements.h
 *
 * Typdefinition des ElementT fuer den KDHeap
 *
 * Bernd Loechner, 29.12.2001
 *
 * ************************************************************************* */

#ifndef KDHEAPELEMENTS_H
#define KDHEAPELEMENTS_H

#include "general.h"

#include "RUndEVerwaltung.h"

typedef enum  {a_entry, p_entry, i_entry, ir_entry, ax_entry} KDElementDescription;

typedef struct{
  KDElementDescription kindOf;
  WeightT              w1;
  WeightT              w2;
  RegelOderGleichungsT aP;
  RegelOderGleichungsT oP;
  TermpairT            tp;
  AbstractTime         t1;        /* i */
  AbstractTime         t2_0;      /* j  nach dimension 0 */
  long                 xp_0;      /* xp: p * 10 + 3 bits  - nach dimension 0 */
  AbstractTime         t2_1;      
  long                 xp_1;      
  AbstractTime         t2_2;      
  long                 xp_2;      
  AbstractTime         t2_3;      
  long                 xp_3;  
  /*deprecated!!!!*/
  AbstractTime         t2;      
  long                 xp;      
  AbstractTime         t2_;      
  long                 xp_;      
  unsigned long       *observer;

} ElementT;

#define EL_MAKE_ELEMENT(/*ElementT * */ x,                                        \
		        /*KDElementDescription*/ kind,                            \
	  	        /*WeightT*/ w1, /*WeightT*/ w2,                           \
		        /*RegelOderGleichungsT*/ aP, /*RegelOderGleichungsT*/ oP, \
		        /*TermpairT*/ tp,                                         \
		        /*AbstractTime*/ t1,                                      \
		        /*AbstractTime*/ t2_0, /*long*/ xp_0,                     \
		        /*AbstractTime*/ t2_1, /*long*/ xp_1,                     \
		        /*AbstractTime*/ t2_2, /*long*/ xp_2,                     \
		        /*AbstractTime*/ t2_3, /*long*/ xp_3,                     \
		        /*unsigned long * */ observer)                            \
  { (x)->kindOf = kind;                                                           \
    (x)->w1 = w1;                                                                 \
    (x)->w2 = w2;                                                                 \
    (x)->aP = aP;                                                                 \
    (x)->oP = oP;                                                                 \
    (x)->tp = tp;                                                                 \
    (x)->t1 = t1;                                                                 \
    (x)->t2_0 = t2_0;                                                             \
    (x)->xp_0 = xp_0;                                                             \
    (x)->t2_1 = t2_1;                                                             \
    (x)->xp_1 = xp_1;                                                             \
    (x)->t2_2 = t2_2;                                                             \
    (x)->xp_2 = xp_2;                                                             \
    (x)->t2_3 = t2_3;                                                             \
    (x)->xp_3 = xp_3;                                                             \
    (x)->observer = observer;                                                     \
  }

#define EL_IS_SET(/*ElementT* */ x)                       \
  ((a_entry == (x)->kindOf) ||                           \
   (p_entry == (x)->kindOf))

#define EL_IS_INDIVIDUAL(/*ElementT* */ x)                \
  ((ir_entry == (x)->kindOf) ||                          \
   (ax_entry == (x)->kindOf))

#define EL_IS_MEMBER(/*ElementT* */ x)                    \
  (i_entry == (x)->kindOf)

#define EL_EQUAL(/*ElementT* */ x, /*ElementT* */ y)      	\
  (((x)->w1 == (y)->w1) &&				\
   ((x)->w2 == (y)->w2) &&				\
   ((x)->aP == (y)->aP) &&				\
   ((x)->oP == (y)->oP) &&				\
   ((x)->tp == (y)->tp) &&				\
   ((x)->t1 == (y)->t1) &&				\
   ((x)->t2 == (y)->t2) &&				\
   ((x)->xp == (y)->xp)    )

/*#define EL_EXPAND(/ *ElementT* / x)                     \
  our_error("not yet implemented")*/

/*#define EL_DISPLACE(/ *ElementT* / x)                     \
  our_error("not yet implemented")*/

#define EL_MINIMIZE(/*ElementT * */ aset, /*ElementT* */ x)     \
  if ((aset)->w1 > (x)->w1) (aset)->w1 = (x)->w1;               \
  if ((aset)->w2 > (x)->w2) (aset)->w2 = (x)->w2;               \
  if ((aset)->t1 > (x)->t1) (aset)->t1 = (x)->t1;               \
  if ((aset)->t2_0 > (x)->t2_0) (aset)->t2_0 = (x)->t2_0;       \
  if ((aset)->xp_0 > (x)->xp_0) (aset)->xp_0 = (x)->xp_0;       \
  if ((aset)->t2_1 > (x)->t2_1) (aset)->t2_1 = (x)->t2_1;       \
  if ((aset)->xp_1 > (x)->xp_1) (aset)->xp_1 = (x)->xp_1;       \
  if ((aset)->t2_2 > (x)->t2_2) (aset)->t2_2 = (x)->t2_2;       \
  if ((aset)->xp_2 > (x)->xp_2) (aset)->xp_2 = (x)->xp_2;       \
  if ((aset)->t2_3 > (x)->t2_3) (aset)->t2_3 = (x)->t2_3;       \
  if ((aset)->xp_3 > (x)->xp_3) (aset)->xp_3 = (x)->xp_3;

#define EL_MEMBER_VALID(/*ElementT*/ x)                                          \
  ((TP_lebtZumZeitpunkt_M(RE_getActiveWithBirthday((x)->t1),HK_getAbsTime_M())) &&  \
   (TP_lebtZumZeitpunkt_M(RE_getActiveWithBirthday((x)->t2),HK_getAbsTime_M())))    

#define EL_IS_INFTY(/*DimensionT*/ d, /*ElementT* */ x)   \
  ((0 == d) ? (maximalWeight() == (x)->w1) :             \
  ((1 == d) ? (maximalAbstractTime() == (x)->t1) :       \
  ((2 == d) ? (maximalWeight() == (x)->w2) :             \
  (maximalAbstractTime() == (x)->t1))))
  
#define /*long*/ EL_GET_SET(/*ElementT* */ e)             \
  ((long) (RE_getActiveWithBirthday((e)->t1),HK_getAbsTime_M()).aEntryObserver)

#define /*BOOLEAN*/ EL_IS_MEMBER_OF(/*ElementT* */ member, /*ElementT* */ set)   \
  ((a_entry == (set)->kindOf) ? ((member)->t1 == (set)->t1) :                      \
   ((p_entry) ? (((set)->t1 == (member)->t1) && ((set)->t2_0 == (member)->t2_0)) : \
    (FALSE)))

#define EL_GREATER_CONTEXT(/*DimensionT*/ d, /*ElementT* */ x, /*ElementT* */ y, /*void * */ context)         \
  ((4 == d) ?                                                      \
   (!(EL_INTERNAL_GREATER((DimensionT) context, *(x), *(y)))) :    \
   ((EL_INTERNAL_GREATER(d, *(x), *(y) ))))                        \

/* hier wird in Dimension
 *  0 nach w1 gewichtet(lexikographisch verfeinert mit i, j[0] und xp[0])
 *  1 nach Alter gewichtet (juenger ist groesser) lex. verf. j[1], xp[1]
 *  2 wie dim 0, nur mit w1 fuer kritische Ziele (mit j[2], xp[2])
 *  3 wie dim 2, nur mit j[3] und xp[3] fuer krit. Ziele
 */
#define EL_INTERNAL_GREATER(/*DimensionT*/ d, /*ElementT* */ x, /*ElementT* */ y)    \
((0 == d) ? ((((x)->w1 > (y)->w1) ||		                                 \
             (((x)->w1 == (y)->w1) && 				                 \
              (((x)->t1 > (y)->t1) ||				                 \
               (((x)->t1 == (y)->t1) &&			                         \
                (((x)->t2_0 > (y)->t2_0) ||			                 \
	         (((x)->t2_0 == (y)->t2_0) &&			                 \
	          ((x)->xp_0 > (y)->xp_0)))))))) :                                 \
 ((1 == d) ? ((((x)->t1 > (y)->t1) ||				                 \
	       (((x)->t1 == (y)->t1) &&			                         \
		(((x)->t2_1 > (y)->t2_1) ||			                 \
		 (((x)->t2_1 == (y)->t2_1) &&			                 \
		  ((x)->xp_1 > (y)->xp_1)))))) :                                   \
  ((2 == d) ? ((((x)->w2 > (y)->w2) ||				                 \
		(((x)->w2 == (y)->w2) && 				                 \
		 (((x)->t1 > (y)->t1) ||				                 \
		  (((x)->t1 == (y)->t1) &&			                 \
		   (((x)->t2_2 > (y)->t2_2) ||			                 \
		    (((x)->t2_2 == (y)->t2_2) &&			                 \
		     ((x)->xp_2 > (y)->xp_2)))))))) :                              \
   (/*3 == d*/(((x)->t1 > (y)->t1) ||				                 \
	       (((x)->t1 == (y)->t1) &&			                         \
		(((x)->t2_3 > (y)->t2_3) ||			                 \
		 (((x)->t2_3 == (y)->t2_3) &&			                 \
		  ((x)->xp_3 > (y)->xp_3)))))))))

#define EL_GREATER(/*int*/ d, /*ElementT* */ x, /*ElementT* */ y)    \
((d == 0) ?					        \
   (((x)->w1 > (y)->w1) ||				\
    (((x)->w1 == (y)->w1) && 				\
     (((x)->t1 > (y)->t1) ||				\
      (((x)->t1 == (y)->t1) &&			        \
       (((x)->t2 > (y)->t2) ||			        \
	(((x)->t2 == (y)->t2) &&			        \
	 ((x)->xp > (y)->xp))))))) :                      \
 ((1 == d)?                                             \
 (((x)->w1 == maximalWeight()) ? TRUE :                  \
 (((y)->w1 == maximalWeight()) ? FALSE :                 \
 (((x)->t1 > (y)->t1) ||				        \
  (((x)->t1 == (y)->t1) &&				\
   (((x)->t2_ > (y)->t2_) ||				\
    (((x)->t2_ == (y)->t2_) &&				\
     ((x)->xp_ > (y)->xp_))))))):                         \
  (((x)->w1 < (y)->w1) ||				        \
    (((x)->w1 == (y)->w1) && 				\
     (((x)->t1 < (y)->t1) ||				\
      (((x)->t1 == (y)->t1) &&			        \
       (((x)->t2 < (y)->t2) ||			        \
	(((x)->t2 == (y)->t2) &&			        \
	 ((x)->xp <= (y)->xp)))))))))

#define PRINT_ELEMENT(/*ElementT* */ x)			        \
  printf("(%ld, %ld)", (x)->w1, (x)->w2);		        \


BOOLEAN KDE_greater (unsigned dim, ElementT* x, ElementT* y, void* context);
BOOLEAN KDE_isInfty( unsigned dim, ElementT* e );
#endif
