/**********************************************************************
* Filename : Stack.c
*
* Kurzbeschreibung : ADT Stack
*
* Autoren : Andreas Jaeger
*
* 20.08.96: Erstellung
* 22.02.97: Mehrfachaufrufclean. AJ.
*
* Funktionalitaet:
* Abstrakter Datentype "Stack". Es werden Pointer verwaltet
*
* Benoetigt folgende Module:
*   - SpeicherVerwaltung
*   - FehlerBehandlung
*   - general: fuer BOOLEAN
*
* Benutzte Globale Variablen aus anderen Modulen:
*   zur Zeit keine
*
**********************************************************************/

#include "Stack.h"
#include "SpeicherVerwaltung.h"
#include "FehlerBehandlung.h"

/* interne Konsistenztest,
   Werte: 0 aus, 1 an */
#define ST_TEST 0

#define ST_ALLOC_ELEMS 50

void ST_New (StackT *stack)
{
   stack->topElem = 0;
   stack->maxElem = ST_ALLOC_ELEMS;
   stack->Data = (void **)our_alloc (ST_ALLOC_ELEMS * sizeof (void *));
}


void ST_Delete (StackT *stack) 
{
   our_dealloc(stack->Data);
}


void ST_Clear (StackT *stack) 
{
   stack->topElem = 0;
}


void ST_Push (StackT *stack, void *ptr)
{
   if (stack->topElem == stack->maxElem) {
      stack->maxElem += ST_ALLOC_ELEMS;
      stack->Data = (void **)our_realloc (stack->Data, stack->maxElem * sizeof (void *), "Stack erweitern");
   }
   (stack->Data[stack->topElem]) = ptr;
   (stack->topElem)++;
}


void *ST_Pop (StackT *stack)
{
#if ST_TEST   
   if (ST_Empty (stack)) {
      our_error("ST_Pop auf leeren Stack!");
   }
#endif /* ST_TEST */   
   (stack->topElem)--;
   return (stack->Data[stack->topElem]);
}


BOOLEAN ST_Empty (StackT *stack) 
{
   return (stack->topElem == 0);
}


void *ST_Top (StackT *stack) 
{
#if ST_TEST   
   if (ST_Empty (stack)) {
      our_error("ST_Top auf leeren Stack!");
   }
#endif /* ST_TEST */   
   return (stack->Data[stack->topElem-1]); 
}
