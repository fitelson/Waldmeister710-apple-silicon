/* -------------------------------------------------------------------------------------------------------------*/
/* -                                                                                                            */
/* - NewClassification.h                                                                                        */
/* - ****************                                                                                           */
/* -                                                                                                            */
/* - Schnittstelle der Klassifikationsroutinen fuer SUEM.                                                       */
/* -                                                                                                            */
/* - 14.03.96 Arnim. Angelegt.                                                                                  */
/* - 24.04.98 Andreas: Umbeannt in NewClassification von Classifikation.h                                       */
/* -                                                                                                            */
/* -                                                                                                            */
/* -------------------------------------------------------------------------------------------------------------*/

#ifndef NEWCLASSIFICATION_H
#define NEWCLASSIFICATION_H


#include "TermOperationen.h"
#include "general.h"
#include "RUndEVerwaltung.h"
#include "InfoStrings.h"
#include "KPVerwaltung.h"
#include "ClasFunctions.h"

typedef enum  {Crit_KillerR,
               Crit_KillerRE,
               Crit_KillerE,
               Crit_EChild,
               Crit_LastMarker /* Ende Marker */
} CriteriaEnumType;


/* Act_normal ist nur der Vollstaendigkeit halber in der Liste - es wird nichts gemacht */
typedef enum {Act_ultimate, Act_normal, Act_double, Act_half, Act_never,
              Act_LastMarker
} ActionEnumType;


typedef enum {Heu_AddWeight, Heu_CP_In_Goal, Heu_GLAM, Heu_Goal_In_CP, Heu_GtWeight,
              Heu_MixWeight2, Heu_MixWeight, Heu_MaxWeight, Heu_OrdWeight, Heu_PolyWeight, Heu_PolyWeight2,
              Heu_Unifikationsmass,
	      Heu_CPPatch, Heu_CGPatch,
              Heu_Fifo, Heu_Golem,
	      Heu_LastMarker
} HeuristicsEnumType;


typedef struct 
{
  CriteriaEnumType criterion;
  ActionEnumType action;
  CriteriaProcType critProc;
  CounterT fullfilled;
} CriterionType;


typedef struct 
{
  CPHistoryT context;
  ActionEnumType action;
} ContextType;

/* ------------------------------------------------------------------------*/
/* -------- globale Variablen ---------------------------------------------*/
/* ------------------------------------------------------------------------*/

extern CriterionType *ClasCriteria;
extern CriterionType *ReClasCriteria;

/* -------------------------------------------------------------------------------------------------------------*/
/* ------- Initialize ------------------------------------------------------------------------------------------*/
/* -------------------------------------------------------------------------------------------------------------*/

void C_ParamInfo(ParamInfoArtT what, int whichModule, 
                 InfoStringT clasDEF, InfoStringT clasUSR, 
                 InfoStringT reclasDEF, InfoStringT reclasUSR, 
                 InfoStringT cgDEF, InfoStringT cgUSR);
/* whichModule: 1 - CLAS
                2 - RECLAS
                3 - CGCLAS
*/

BOOLEAN C_InitAposteriori(InfoStringT clasDEF, InfoStringT clasUSR, 
                          InfoStringT reclasDEF, InfoStringT reclasUSR, 
                          InfoStringT cgDEF, InfoStringT cgUSR);

/* -------------------------------------------------------------------------------------------------------------*/
/* ------- Classify --------------------------------------------------------------------------------------------*/
/* -------------------------------------------------------------------------------------------------------------*/

void
C_Classify (WeightEntryT *wtPtr, TermT lhs, TermT rhs,
	    RegelOderGleichungsT actualParent, RegelOderGleichungsT otherParent,
	    UTermT actualPosition,
	    CPHistoryT History);

void C_ReClassify (WeightEntryT *wtPtr, TermT lhs, TermT rhs,
		   RegelOderGleichungsT actualParent, RegelOderGleichungsT otherParent);
        
void
C_CG_Classify (WeightEntryT *wtPtr, TermT lhs, TermT rhs,
	       RegelOderGleichungsT actualParent, RegelOderGleichungsT otherParent);

/* -------------------------------------------------------------------------------------------------------------*/
/* ------- Prototypen ------------------------------------------------------------------------------------------*/
/* -------------------------------------------------------------------------------------------------------------*/

const char *get_criterion (CriteriaEnumType singleCrit);

#endif

