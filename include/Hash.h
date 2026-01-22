/**********************************************************************
* Filename : Hash.h
*
* Kurzbeschreibung : ADT Hash
*
* Autoren : Andreas Jaeger
*
* 20.08.96: Erstellung
*
**********************************************************************/

#ifndef HASH_H
#define HASH_H

#include "general.h"
#include "compiler-extra.h"

typedef unsigned long HashKeyT;

typedef struct HashEntryTS 
{
   struct HashEntryTS *Nachf; /* fuer Traverse durch alle Elemente
                                 und fuer Benutzung des Nested SV */
   HashKeyT          Key1;
   HashKeyT          Key2;
   void              *Data;
   struct HashEntryTS *nextInBucket; /* weitergehen innerhalb eines Buckets */
   struct HashEntryTS *nextInteresting; /* fuer Traverse durch "interesseante" Elemente */
   struct HashEntryTS *prevInteresting; /* fuer Traverse durch "interesseante" Elemente */
} HashEntryT, *HashEntryPtrT;

typedef BOOLEAN (*HA_CmpFuncT)(void *, void *);
typedef void (*HA_DeallocFuncT)(void *);

typedef unsigned int HashSizeT;

typedef struct HashTS 
{
   /* Groesse des Arrays Entry */
   HashSizeT          size;
   /* Anzahl von Elementen in Hash */
   HashSizeT          countElems;
   /* maximale Anzahl von Elementen in einem Bucket, die Information
      wird benoetigt um den Hash einfacher zu reorganisieren */
   HashSizeT          maxLengthBucket;
   /* maximale Anzahl von Elemente in einem Bucket bei der letzten Reorg
      um immerwiederkehrende Reorganisationen zu vermeiden */
   HashSizeT          lastMaxLengthBucket;
   /* CmpFunc wird aufgerufen mit zwei Elementen, deren Key gleich ist
      und es muss ueberprueft werden, ob die Elemente auch gleich sind */
   HA_CmpFuncT        CmpFunc;
   /* DeallocFunc wird aufgerufen, wenn der Hash leer geraeumt wird,
      DeallocFunc muss dafuer sorgen, dass die Daten (Data) auf die
      verwiesen wird, bei Bedarf, freigegeben werden
      (die Funktion darf nichts machen, wenn die Daten noch weiter benutzt
      werden) */
   HA_DeallocFuncT   DeallocFunc;
   /* FirstEntry/LastEntry sind Anfang/Ende der Faedelung in Einfuegereihenfolge
      (ueber HashEntryT->Nachf). Sie dienen zum unsortierten Durchgehen durch
      alle Elemente und zum Einfuegen am Ende.
      */
   HashEntryPtrT     FirstEntry;
   HashEntryPtrT     LastEntry;
   HashEntryPtrT     FirstInteresting; /* zum Unsortierten Durchgehen durch "interessante" Elemente */
   HashEntryPtrT     LastInteresting; /* zum Unsortierten Durchgehen durch "interessante" Elemente */
   HashEntryPtrT     *Entry;   /* wird als Array verwaltet */
} HashT, *HashPtrT;

void HA_InitAposteriori (void);

void HA_Cleanup (void);

void HA_New (HashT *hash, HA_CmpFuncT CmpFunc, HA_DeallocFuncT DeallocFunc, HashSizeT size);

void HA_Delete (HashT *hash);

/* Freigeben der einzelnen Eintraege und Reinitialisieren der Struktur,
   so dass hash nochmal neu benutzt werden kann (anstelle von Delete/New)
   */
void HA_Clear (HashT *hash);


/* Statistik der HashTabelle: durchschnittliche Laenge der belegten (!)
   Eintraege und maximale Laenge */
void HA_Stat (HashT *hash, double *avgLength, HashSizeT *maxLength,
              unsigned int *bucketsUsed);

/* OldData: wenn CmpFunc TRUE liefert, dann enthaelt OldData,
   die entsprechenden Daten. OldData ist somit der bereits vorhandene
   Eintrag. Wenn CmpFunc immer FALSE liefert, dann ist OldData = NULL */
void HA_AddData (HashT *hash, HashKeyT Key1, HashKeyT Key2, void *Data, void **OldData);

/* OldData: wenn CmpFunc TRUE liefert, dann enthaelt OldData,
   die entsprechenden Daten. OldData ist somit der bereits vorhandene
   Eintrag. Wenn CmpFunc immer FALSE liefert, dann ist OldData = NULL.
   HA_CheckData liefert TRUE zurueck, wenn CmpFunc TRUE zurueckliefert.
   */
BOOLEAN HA_CheckData (HashT *hash, HashKeyT Key1, HashKeyT Key2, void *Data, void **OldData);

/* Die Traverse Funktionen benoetigen einen Pointer zur Verwaltung */
/* TraversePtr wird intern (!) zum Durchgehen
   des Hashes mit den Funktionen HA_*Traverse benutzt
   dabei zeigt TraversePtr immer auf das Element, welches
   bei HA_NextTraverse genommen wird! */

/* HA_InitTraverse intialisiert Traversial */
void HA_InitTraverseN (HashT *hash, HashEntryPtrT *TraversePtr);

/* HA_NextTraverse holt naechsten Eintrag,
   vor jedem Aufruf von HA_NextTraverse muss HA_EndTraverse
   aufgerufen werden und FALSE liefern !! */
void HA_NextTraverseN (HashEntryPtrT *TraversePtr, void **Data);

/* HA_EndTraverse liefert TRUE, wenn der Durchlauf beendet ist,
   HA_NextTraverse darf nur aufgerufen werden, wenn
   HA_EndTraverse FALSE liefert! */
BOOLEAN HA_EndTraverseN (HashEntryPtrT *TraversePtr);


/* Traverse fuer "interessante" Elemente */
/* siehe Ha_InitTraverseN etc. - es wird aber ein zweiter Pointer benutzt um Elemente loeschen
   zu koennen aus der Liste der "interessante" Elemente (Elemente sind also
   nach wie vor im Hash, aber sind nicht mehr "interessant") */
/* Programmskelet:
   HashEntryPtrT       TraversePtr1, TraversePtr2;
   void                *Data;
   BOOLEAN             deleteCurrent;
   
   
   HA_InitTraverseI (&menge->HashTabelle, &TraversePtr1, &TraversePtr2);
   if (!HA_EndTraverseI (&menge->HashTabelle, &TraversePtr1, &TraversePtr2))
     HA_NextTraverseI (&menge->HashTabelle, &TraversePtr1, &TraversePtr2, &Data);
   while (!HA_EndTraverseI (&menge->HashTabelle, &TraversePtr1, &TraversePtr2)) {
      mach was mit Data, da Daten ok sind und setze deleteCurrent
      if (deleteCurrent)
	 HA_DelAndNextTraverseI (&menge->HashTabelle, &TraversePtr1, &TraversePtr2, &Data);
      else
	 HA_NextTraverseI (&menge->HashTabelle, &TraversePtr1, &TraversePtr2, &Data);
   }
*/

void HA_InitTraverseI (HashT *hash, HashEntryPtrT *TraversePtr1, HashEntryPtrT *TraversePtr2);
void HA_NextTraverseI (HashT *hash, HashEntryPtrT *TraversePtr1, HashEntryPtrT *TraversePtr2,
		       void **Data);
BOOLEAN HA_EndTraverseI (HashT *hash, HashEntryPtrT *TraversePtr1, HashEntryPtrT *TraversePtr2);
void HA_DelAndNextTraverseI (HashT *hash, HashEntryPtrT *TraversePtr1, HashEntryPtrT *TraversePtr2,
			     void **Data);

#endif /* HASH_H */
