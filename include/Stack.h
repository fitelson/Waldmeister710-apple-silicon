/**********************************************************************
* Filename : Stack.h
*
* Kurzbeschreibung : ADT Stack
*
* Autoren : Andreas Jaeger
*
* 20.08.96: Erstellung
*
**********************************************************************/

#ifndef STACK_H
#define STACK_H

#include "general.h"

typedef struct StackTS 
{
   unsigned long         topElem;
   unsigned long         maxElem;
   void                  **Data;
} StackT, *StackPtrT;

void ST_New (/*@out@*/StackT *stack);

void ST_Delete (StackT *stack);

/* Loeschen der Daten, so dass der Stack wieder neu benutzt werden kann,
   Alternative zu ST_Delete/ST_New */
void ST_Clear (StackT *stack);

void ST_Push (StackT *stack, void *ptr);

void *ST_Pop (StackT *stack);

BOOLEAN ST_Empty (StackT *stack);

void *ST_Top (StackT *stack);

#endif /* STACK_H */
