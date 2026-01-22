/**********************************************************************
* Filename : Deque.h
*
* Kurzbeschreibung : ADT Deque (Double ended queue)
*
* Autoren : Andreas Jaeger
*
* 02.09.96: Erstellung
*
**********************************************************************/

#ifndef DEQUE_H
#define DEQUE_H

#include "general.h"

typedef struct DequeEntryTS 
{
   struct DequeEntryTS   *toLeft;
   struct DequeEntryTS   *toRight;
   void                  *Data;
} DequeEntryT, *DequeEntryPtrT;

typedef struct DequeTS 
{
   DequeEntryPtrT   leftEnd;
   DequeEntryPtrT   rightEnd;
} DequeT, *DequePtrT;

void DQ_InitAposteriori (void);

void DQ_Cleanup (void);

void DQ_New (/*@out@*/ DequeT *deque);

void DQ_Delete (DequeT *deque);

void DQ_Clear (DequeT *deque);

void DQ_PushL (DequeT *deque, void *ptr);

void DQ_PushR (DequeT *deque, void *ptr);

void *DQ_PopL (DequeT *deque);

void *DQ_PopR (DequeT *deque);

void *DQ_TopL (DequeT *deque);

void *DQ_TopR (DequeT *deque);

BOOLEAN DQ_Empty (DequeT *deque);

#endif /* DEQUE_H */
