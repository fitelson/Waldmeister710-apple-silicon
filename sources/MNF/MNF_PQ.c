/**********************************************************************
* Filename : MNF_PQ.c
*
* Kurzbeschreibung : Verwalten der Priority-Queue fuer MNF
*
* Autoren : Andreas Jaeger
*
* 03.09.96: Erstellung
*
**********************************************************************/

#include "compiler-extra.h"
#include "general.h"
#include "MNF_PQ.h"
#include "MNF_intern.h"
#include "Stack.h"
#include "Deque.h"
#include "SpeicherVerwaltung.h"

static void * MNFPQ_ST_Pop (void *stack, BOOLEAN dum MAYBE_UNUSEDPARAM) {

   return (ST_Pop ((StackT *)stack));
}

static void * MNFPQ_DQ_PopFifo (void *deque, 
                                BOOLEAN LastWasIrred MAYBE_UNUSEDPARAM) {
   /* reine Breitensuche */
   return DQ_PopR ((DequeT *)deque);
}

static void * MNFPQ_DQ_Pop (void *deque, BOOLEAN LastWasIrred) {
   /* Mix zwischen Fifo und Lifo */
   if (LastWasIrred)
      return DQ_PopR ((DequeT *)deque);
   else
      return DQ_PopL ((DequeT *)deque);
}

static void *MNFPQ_ST_GetNewPQ (void) {
   StackPtrT   stack;
   
   stack = (StackPtrT)our_alloc (sizeof(StackT));
   ST_New (stack);
   return stack;
}

static void *MNFPQ_DQ_GetNewPQ (void) {
   DequePtrT deque;
   
   deque = (DequePtrT) our_alloc(sizeof(DequeT));
   DQ_New (deque);
   return deque;
}

static void MNFPQ_ST_Delete (void *stack) {
   
   ST_Delete ((StackT *)stack);
   our_dealloc (stack);
}

static void MNFPQ_DQ_Delete (void *deque) {
   
   DQ_Delete ((DequeT *)deque);
   our_dealloc (deque);
}

void MNFPQ_InitMenge (MNF_MengeT *menge)
{
   switch (menge->MNFPQ_Art) {
   case MNFPQ_stack:
      menge->MNFPQ_GetNewPQ = MNFPQ_ST_GetNewPQ;
      menge->MNFPQ_Push   = (MNFPQ_PushT) ST_Push;
      menge->MNFPQ_Pop    = MNFPQ_ST_Pop;
      menge->MNFPQ_Delete = MNFPQ_ST_Delete;
      menge->MNFPQ_Clear  = (MNFPQ_ClearT)ST_Clear;
      menge->MNFPQ_Empty  = (MNFPQ_EmptyT)ST_Empty;
      break;
   case MNFPQ_deque:
      menge->MNFPQ_GetNewPQ = MNFPQ_DQ_GetNewPQ;
      menge->MNFPQ_Push   = (MNFPQ_PushT)DQ_PushL;
      menge->MNFPQ_Pop    = MNFPQ_DQ_Pop;
      menge->MNFPQ_Clear  = (MNFPQ_ClearT)DQ_Clear;
      menge->MNFPQ_Delete = MNFPQ_DQ_Delete;
      menge->MNFPQ_Empty  = (MNFPQ_EmptyT)DQ_Empty;
      break;
   case MNFPQ_fifo:
      menge->MNFPQ_GetNewPQ = MNFPQ_DQ_GetNewPQ;
      menge->MNFPQ_Push   = (MNFPQ_PushT)DQ_PushL;
      menge->MNFPQ_Pop    = MNFPQ_DQ_PopFifo;
      menge->MNFPQ_Clear  = (MNFPQ_ClearT)DQ_Clear;
      menge->MNFPQ_Delete = MNFPQ_DQ_Delete;
      menge->MNFPQ_Empty  = (MNFPQ_EmptyT)DQ_Empty;
      break;
   }
}
