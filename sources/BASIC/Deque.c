/**********************************************************************
* Filename : Deque.c
*
* Kurzbeschreibung : ADT Deque (Double ended queue)
*
* Autoren : Andreas Jaeger
*
* 02.09.96: Erstellung
* 22.02.97: Mehrfachaufrufclean. AJ.
*
**********************************************************************/

#include "general.h"
#include "compiler-extra.h"
#include "Deque.h"
#include "SpeicherVerwaltung.h"
#include "FehlerBehandlung.h"

static SV_SpeicherManagerT       DQ_Speicher;


void DQ_InitAposteriori (void)
{
   SV_ManagerInitialisieren (&DQ_Speicher, DequeEntryT, DQ_SpeicherBlock, "DequeEntries");
}


void DQ_Cleanup (void)
{
   /* nothing to do at the moment */
   ; 
}


void DQ_New (/*@out@*/ DequeT *deque) {

   deque->leftEnd=NULL;
   deque->rightEnd=NULL;
}


static void
DQ_Dealloc (DequeT *deque) 
{
   DequeEntryPtrT       entry = deque->leftEnd;
   DequeEntryPtrT       prev  = NULL;
   
   while (entry != NULL) {
      prev = entry;
      entry = entry->toRight;
      SV_Dealloc (DQ_Speicher, prev);
   }
   deque->leftEnd=NULL;
   deque->rightEnd=NULL;
}


void DQ_Delete (DequeT *deque) {
   
   DQ_Dealloc(deque);
}


void DQ_Clear (DequeT *deque) {
   
   DQ_Dealloc(deque);
}


static void DQ_FirstElem (DequeT *deque, void *ptr) {
   DequeEntryPtrT       entry;
   
   SV_Alloc(entry, DQ_Speicher, DequeEntryPtrT);
   entry->toLeft=NULL;
   entry->toRight=NULL;
   entry->Data=ptr;
   deque->leftEnd=entry;
   deque->rightEnd=entry;
}


void DQ_PushL (DequeT *deque, void *ptr) {

   if (deque->leftEnd == NULL) {
      /* dann muss rightEnd auch NULL sein! */
      DQ_FirstElem (deque, ptr);
   }
   else {
      DequeEntryPtrT       entry;
   
      SV_Alloc(entry,DQ_Speicher,DequeEntryPtrT);
      entry->Data = ptr;
      entry->toLeft = NULL;
      entry->toRight= deque->leftEnd;
      deque->leftEnd->toLeft = entry;
      deque->leftEnd = entry;
      
   }
}


void DQ_PushR (DequeT *deque, void *ptr) {

   if (deque->leftEnd == NULL) {
      /* dann muss rightEnd auch NULL sein! */
      DQ_FirstElem (deque, ptr);
   }
   else {
      DequeEntryPtrT       entry;
   
      SV_Alloc(entry,DQ_Speicher,DequeEntryPtrT);
      entry->Data = ptr;
      entry->toRight = NULL;
      entry->toLeft= deque->rightEnd;
      deque->rightEnd->toRight = entry;
      deque->rightEnd = entry;
      
   }
}


void *DQ_PopL (DequeT *deque)
{
   DequeEntryPtrT       entry;
   void                 *retPtr;
   
   entry = deque->leftEnd;

   retPtr = entry->Data;
   
   deque->leftEnd = entry->toRight;
   SV_Dealloc (DQ_Speicher, entry);
   
   if (deque->leftEnd == NULL) {
      deque->rightEnd = NULL;
   }
   else {
      deque->leftEnd->toLeft=NULL;
   }
   return retPtr;
}


void *DQ_PopR (DequeT *deque)
{
   DequeEntryPtrT       entry;
   void                 *retPtr;
   
   entry = deque->rightEnd;

   retPtr = entry->Data;
   
   deque->rightEnd = entry->toLeft;
   SV_Dealloc (DQ_Speicher, entry);
   
   if (deque->rightEnd == NULL) {
      deque->leftEnd = NULL;
   }
   else {
      deque->rightEnd->toRight=NULL;
   }
   return retPtr;
}


BOOLEAN DQ_Empty (DequeT *deque) 
{
   return (deque->leftEnd == NULL);
}
