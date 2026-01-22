/**********************************************************************
* Filename : Hash.c
*
* Kurzbeschreibung : ADT Hash
*
* Autoren : Andreas Jaeger
*
* 20.08.96: Erstellung
* 22.02.97: Mehrfachaufrufclean. AJ.
*
* Funktionalitaet:
* Abstrakter Datentype "Hash". Es werden Pointer verwaltet
*
* Benoetigt folgende Module:
*   - SpeicherVerwaltung
*   - FehlerBehandlung
*   - general: fuer BOOLEAN
*   - Statistiken: fuer Statistik
*   - Ausgaben: fuer Statistikausgaben
*
* Benutzte Globale Variablen aus anderen Modulen:
*   zur Zeit keine
*
**********************************************************************/

#include "compiler-extra.h"
#include "Hash.h"
#include "general.h"
#include "SpeicherVerwaltung.h"
#include "FehlerBehandlung.h"
#include "Statistiken.h"
#include "Ausgaben.h"

#define HA_TEST 0


static SV_NestedSpeicherManagerT       HA_Speicher;


void HA_InitAposteriori (void)
{
  SV_NestedManagerInitialisieren (HA_Speicher, HashEntryT,
				  HA_SpeicherBlock, "HashEntries");
  /* ist mehrfachaufrufclean */
}


void HA_Cleanup (void)
{
}


void HA_New (HashT *hash, HA_CmpFuncT CmpFunc, HA_DeallocFuncT DeallocFunc, HashSizeT size) 
{
   unsigned int       i;

   hash->size = size;
   hash->countElems = 0;
   hash->maxLengthBucket = 0;
   hash->lastMaxLengthBucket = 0;
   hash->CmpFunc = CmpFunc;
   hash->DeallocFunc = DeallocFunc;
   hash->FirstEntry = NULL;
   hash->LastEntry = NULL;

   hash->FirstInteresting = NULL;
   hash->LastInteresting = NULL;
   hash->Entry = (HashEntryPtrT *) our_alloc (sizeof (HashEntryPtrT) * size);
   for (i=0;i<size;i++) {
      hash->Entry[i] = NULL;
   }

#if HA_STATISTIK
   if (size > HA_MaxHashSize)
      HA_MaxHashSize = size;
#endif /* HA_STATISTIK */
}


static void HA_DeallocData (HashT *hash) 
{
   HashEntryPtrT      Ptr;

   if (hash->FirstEntry == NULL)
      /* Nothing to do */
      return;
   
   hash->countElems = 0;
   hash->maxLengthBucket = 0;
   hash->lastMaxLengthBucket = 0;

   /* Speicher in Einfuegereihenfolge freigeben */
   Ptr = hash->FirstEntry;
   while (Ptr != NULL) {
      hash->DeallocFunc (Ptr->Data);
      Ptr = Ptr->Nachf;
   }
   
   SV_NestedDealloc (HA_Speicher, hash->FirstEntry, hash->LastEntry);

   hash->FirstEntry = NULL;
   hash->LastEntry = NULL;

   hash->LastInteresting = NULL;
   hash->FirstInteresting = NULL;
}


void HA_Delete (HashT *hash)
{
   HA_DeallocData (hash);
   our_dealloc(hash->Entry);
   hash->Entry = NULL;
}


void HA_Clear (HashT *hash) 
{
   unsigned int      i;
   
   HA_DeallocData (hash);
   for (i = 0; i < hash->size; i++) {
      hash->Entry[i] = NULL;
   }
}


void HA_Stat (HashT *hash, double *avgLength, unsigned int  *maxLength,
              HashSizeT *bucketsUsed) 
{
   unsigned int       i;
   HashSizeT          anzBelegt = 0;
   unsigned int       lengthListe;
   unsigned int       sumLength = 0;
   
   HashSizeT          maxBelegt = 0;
   HashEntryPtrT      STPtr;

   for (i=0; i < hash->size; i++) {
      STPtr = hash->Entry[i];
      lengthListe = 0;
      if (STPtr != NULL) {
         do {
            lengthListe++;
            STPtr = STPtr->nextInBucket;
         } while (STPtr != NULL);
         Maximieren (&maxBelegt,  lengthListe);
         anzBelegt++;
         sumLength += lengthListe;
      }
   }
   if (anzBelegt > 0)
      *avgLength = (double) sumLength /  (double)anzBelegt;
   else *avgLength = 0.0;
   *maxLength = maxBelegt;
   *bucketsUsed = anzBelegt;
}


static void HA_ResizeHash (HashT *hash) 
{
   HashEntryPtrT      TraversePtr;
   HashSizeT          Bucket;
   HashSizeT          newSize;
   unsigned int       i;
   unsigned int       lengthListe;

   /* Wir wollen nicht zuviel - aber auch nicht viel zuwenig Elemente
      im Hash drin haben, zuviel entscheidet der Aufrufer,
      hier wird zuwenig entschieden: */
   if ((hash->countElems * 5 < hash->size) &&
       (hash->maxLengthBucket < 10))
      {
         hash->lastMaxLengthBucket++;
         return;
      }
   
   newSize = hash->size * 2 + 1;

#if HA_STATISTIK   
   if (newSize > HA_MaxHashSize)
      HA_MaxHashSize = newSize;
#endif
   
   /* Da der Hash mehrfach verkettet ist, koennen wir unsortiert durch
      den Hash durchlaufen und diese Verzeigerung nicht aufbrechen -
      und dabei gleichzeitig  eine neue Verzeigerung definieren.
      Es wird also in Entstehungsreihenfolge durchgegangen (Nachf).
      */
   hash->Entry = (HashEntryPtrT *)our_realloc (hash->Entry, sizeof (HashEntryPtrT) * newSize, "HashEntry");
   hash->size = newSize;
   for (i = 0;i < newSize;i++) {
      hash->Entry[i] = NULL;
   }
   
   /* Durchgehen durch alten Hash und einfuegen in neuen Hash */
   TraversePtr = hash ->FirstEntry;

   while (TraversePtr != NULL) {
      /* Element vorne in Bucket einfuegen */
      Bucket = (HashSizeT) (TraversePtr->Key1 % newSize);
      TraversePtr->nextInBucket = hash->Entry[Bucket];
      hash->Entry[Bucket] = TraversePtr;
      /* und zum naechsten Element */
      TraversePtr = TraversePtr->Nachf;
   }

   /* maxLengthBucket aktualisieren */
   hash->maxLengthBucket = 0;
   for (i = 0; i < hash->size; i++) {
      TraversePtr = hash->Entry[i];
      lengthListe = 0;
      if (TraversePtr != NULL) {
         do {
            lengthListe++;
            TraversePtr = TraversePtr->nextInBucket;
         } while (TraversePtr != NULL);
         if (lengthListe > hash->maxLengthBucket)
            hash->maxLengthBucket = lengthListe;
      }
   }
   hash->lastMaxLengthBucket = hash->maxLengthBucket;
}


void HA_AddData (HashT *hash, HashKeyT Key1, HashKeyT Key2, void *Data, void **OldData)
{
   HashSizeT          Bucket = Key1 % hash->size;
   HashSizeT          maxLength = 0;
   HashEntryPtrT      Ptr;

   Ptr = hash->Entry[Bucket];
   *OldData = NULL;
   
   while (Ptr != NULL) {
      INC_AnzBucketGleich;
      if (Key2 == Ptr->Key2) {
         INC_AnzKey2Gleich;
         if (Key1 == Ptr->Key1) {
            INC_AnzKey1Gleich;
            if (hash->CmpFunc(Ptr->Data, Data)) {
               *OldData = Ptr->Data;
               return;
            }
         }
      }
      maxLength++;
      Ptr = Ptr->nextInBucket;
   }
   SV_NestedAlloc(Ptr,HA_Speicher,HashEntryPtrT);
   Ptr->Key1 = Key1;
   Ptr->Key2 = Key2;
   Ptr->Data = Data;
   /* Element vorne in Bucket einfuegen */
   Ptr->nextInBucket = hash->Entry[Bucket];
   hash->Entry[Bucket] = Ptr;

   /* Pointer zum unsortierten Durchgehen in Einfuegereihenfolge setzen;
      hinten anfuegen */
   Ptr->Nachf = NULL;
   if (hash->FirstEntry == NULL) {
      hash->FirstEntry = Ptr;
      hash->LastEntry = Ptr;
   } else {
      hash->LastEntry->Nachf = Ptr;
      hash->LastEntry = Ptr;
   }
   
   /* Pointer zum Durchgehen durch interessante Elemente setzen */
   Ptr->nextInteresting = NULL;
   if (hash->LastInteresting != NULL) {
      (hash->LastInteresting)->nextInteresting = Ptr;
      hash->LastInteresting = Ptr;
   }
   else {
      hash->LastInteresting = Ptr;
      hash->FirstInteresting = Ptr;
   }
   /* Statistiken updaten und evtl. Hash reorganisieren */
   hash->countElems++;
   if (hash->maxLengthBucket <= maxLength) {
      /* Element selber wurde noch nicht gezaehlt, deswegen plus 1 */
      hash->maxLengthBucket = maxLength+1;
#if HA_STATISTIK
      if (maxLength >= HA_MaxElemsInBucket)
         HA_MaxElemsInBucket = maxLength + 1;
#endif
      if ((hash->maxLengthBucket > hash->lastMaxLengthBucket + 5) ||
          (hash->countElems > hash->size * 10))
         HA_ResizeHash (hash);
   }
}


BOOLEAN HA_CheckData (HashT *hash, HashKeyT Key1, HashKeyT Key2, void *Data, void **OldData)
{
   HashEntryPtrT      Ptr;
   Ptr = hash->Entry[Key1 % hash->size];
   *OldData = NULL;
   
   while (Ptr != NULL) {
      INC_AnzBucketGleich;
      if (Key2 == Ptr->Key2) {
         INC_AnzKey2Gleich;
         if (Key1 == Ptr->Key1) {
            INC_AnzKey1Gleich;
            if (hash->CmpFunc(Ptr->Data, Data)) {
               *OldData = Ptr->Data;
               return TRUE;
            }
         }
      }
      Ptr = Ptr->nextInBucket;
   }
   return FALSE;
}


void HA_InitTraverseN (HashT *hash, HashEntryPtrT *TraversePtr)
{
   *TraversePtr = hash->FirstEntry;
}


void HA_NextTraverseN (HashEntryPtrT *TraversePtr, void **Data)
{
#if HA_TEST
   if (*TraversePtr == NULL)
      our_error ("HA_NextTraverse: TraversePtr is NULL");
#endif /* HA_TEST */
   *Data=(*TraversePtr)->Data;
   *TraversePtr = (*TraversePtr)->Nachf;
}


BOOLEAN HA_EndTraverseN (HashEntryPtrT *TraversePtr)
{
   return (*TraversePtr == NULL);
}

/* Traverse fuer "interessante" Elemente */
/* etwas andere Implementierung als normales Traverse, um Loeschen zu ermoeglichen:
   TraversePtrCurrent zeigt auf das aktuelle Element,
   TraversePtrPrev zeigt auf den Vorgaenger
   */
void HA_InitTraverseI (HashT *hash MAYBE_UNUSEDPARAM, HashEntryPtrT *TraversePtrCurrent, HashEntryPtrT *TraversePtrPrev)
{
   *TraversePtrCurrent = NULL;
   *TraversePtrPrev = NULL;
}


void HA_NextTraverseI (HashT *hash, HashEntryPtrT *TraversePtrCurrent, HashEntryPtrT *TraversePtrPrev, void **Data)
{
   if (*TraversePtrCurrent == NULL) {
      *TraversePtrCurrent = hash->FirstInteresting;
   }
   else {
      *TraversePtrPrev = *TraversePtrCurrent;
      *TraversePtrCurrent = (*TraversePtrCurrent)->nextInteresting;
   }
   if (*TraversePtrCurrent != NULL)
      *Data=(*TraversePtrCurrent)->Data;
   else
      *Data = NULL;
}


void HA_DelAndNextTraverseI (HashT *hash, HashEntryPtrT *TraversePtrCurrent, HashEntryPtrT *TraversePtrPrev,
                             void **Data)
{
#if HA_TEST
   if (*TraversePtrCurrent == NULL)
      our_error ("HA_DelTraverseI: TraversePtrCurrent is NULL");
#endif /* HA_TEST */
   
   /* erstes Element ? */
   if (TraversePtrPrev == NULL) {
      /* testen ob erstes auch letztes ist? */
      if (hash->FirstInteresting == hash->LastInteresting) {
         hash->LastInteresting = NULL;
         hash->FirstInteresting = NULL;
         *TraversePtrCurrent = NULL;
         *Data = NULL;
      }
      else {
         hash->FirstInteresting = (*TraversePtrCurrent)->nextInteresting;
         (*TraversePtrCurrent)->nextInteresting = NULL;
         *TraversePtrCurrent = hash->FirstInteresting;
         *Data=(*TraversePtrCurrent)->Data;
      }
   }
   /* Struktur umbiegen */
   else {
      (*TraversePtrPrev)->nextInteresting = (*TraversePtrCurrent)->nextInteresting;
      (*TraversePtrCurrent)->nextInteresting = NULL;
      *TraversePtrCurrent = (*TraversePtrPrev)->nextInteresting;
      if (*TraversePtrCurrent != NULL)
         *Data=(*TraversePtrCurrent)->Data;
      else { /* zeigt auf letztes Element */
         hash->LastInteresting = *TraversePtrPrev;
         *Data = NULL;
      }
   }
}


BOOLEAN HA_EndTraverseI (HashT *hash, HashEntryPtrT *TraversePtrCurrent,
                         HashEntryPtrT *TraversePtrPrev)
{
   if (*TraversePtrPrev == NULL)
      return (hash->FirstInteresting == NULL);
   else
      return (*TraversePtrCurrent == NULL);
}
