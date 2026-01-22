/* *************************************************************************
 *
 * CachedKDHeap.h
 *
 * KDHeap mit Caches nach in mehreren Dimensionen.
 *
 * Bernd Loechner, Jean-Marie Gaillourdet 13.1.2003
 *
 * ************************************************************************* */

#ifndef CACHED_KDHEAP
#define CACHED_KDHEAP

#include "NKDHeap.h"

typedef struct CKDHeapS *CKDHeapT;
typedef unsigned DimensionT;
#define MAX_DIMENSION ((DimensionT) -1L)

/******************************************************************************
 * Callback API des Elementtyps 
 *****************************************************************************/


/*
 * BOOLEAN EL_IS_SET( void* e )
 * 
 * Prädikat, das TRUE liefert gdw e ein Mengeneintrag ist.
 */
typedef BOOLEAN (*CKD_IsSetFct)( void* e );

/*
 * BOOLEAN EL_IS_INDIVIDUAL( void* e )
 *
 * Prädikat, das TRUE liefert gdw e ein Individuum ist, welches NICHT in einen
 * Mengeneintrag eingefügt werden kann.
 */
typedef BOOLEAN (*CKD_IsIndividualFct)(void* e);

/*
 * BOOLEAN EL_IS_MEMBER( void* e )
 *
 * Prädikat, das TRUE liefert gdw e ein Individuum ist, welches in einen 
 * Mengeneintrag eingefügt werden kann.
 */
typedef BOOLEAN (*CKD_IsMemberFct)(void* e);

/*
 * void EL_EXPAND( CKDHeapT h, void* set )
 * 
 * PRECONDITION: EL_IS_SET( set ) == TRUE
 * 
 * Expandiere alle Einträge des Mengeneintrages e zu Members in h
 * 
 * AENDERUNG(BL&HS): Diese Funktion wird beim Erzeugen des CKDHeaps als Argument
 * mitgegeben. Der CKDHeap ruft dann den Funktionspointer auf.
 * Hierzu ein passender Typ:
 *
 * AENDERUNG HS2003-05-05: Nicht alle Einträge werden expandiert, sondern nur die
 * guten, damit nicht allzuoft wiederverpackt werden muss. Zum Verpacken müssen
 * wir die Members ohnehin sortieren.
 * D.h. nach Expand() sind ein Teil (mglweise alle) in einem ASet enthaltenen 
 * Membereinträge ausgepackt. Die - aus welchem Grund auch immer - nicht aus-
 * gepackten sind durch ein A-Set im Heap vertreten.
 *
 * ein Heapyfy auf dem geänderten Set muss außerhalb von expand durchgeführt werden
 */
typedef void (*CKD_ExpandFct)(CKDHeapT h, void* e);

/*
 * unsigned long EL_GET_SET( void* e )
 *
 * PRECONDITION: EL_IS_MEMBER( e ) == TRUE
 * 
 * Liefert den Index des Elterneintrag im Heap.
 */
typedef unsigned long (*CKD_GetSetFct)(void* e);

/*
 * void CKD_MinimizeFct( void* aset, void* member )
 * 
 */
typedef void (*CKD_MinimizeFct)(void* aset, void* member);

/*
 * deepcopy
 *
 * void* CKD_CloneFct( void* element)
 *
 */
typedef void* (*CKD_CloneFct) (void* element);

/*
 * setzt einen a-Set Eintrag auf "leer"
 * 
 * PRECONDITION: EL_IS_SET( aset ) == TRUE
 *
 * void CKD_MakeEmptyFct (void * aset);
 *
 */
typedef void (*CKD_MakeEmptyFct) (void* aset);

/*
 * BOOLEAN EL_IS_MEMBER_OF( void* member, void* set )
 * 
 * TRUE gdw member aus set erzeugt wurde
 */
typedef BOOLEAN (*CKD_IsMemberOfFct)(void* member, void* set);

/* 
 * BOOLEAN EL_MEMBER_VALID( void* e )
 *
 * Prädikat, das TRUE liefert gdw e noch gültig ist, d.h. nicht verworfen wurde.
 * Im Fall der KDHeapElements bedeutet das, dass das Element keine Waise ist, 
 * d.h. beide Eltern leben noch.
 */
typedef BOOLEAN (*CKD_MemberValidFct)(void* e);

/*
 * gibt den Speicher von e frei
 * 
 * void CKD_DeleteFct (void* e)
 */
typedef void (*CKD_DeleteFct) (void* e);

typedef struct {
  NKDH_FunctionsT             kdh;
  CKD_IsSetFct                isSet;
  CKD_IsIndividualFct         isIndividual;
  CKD_IsMemberFct             isMember;
  CKD_ExpandFct               expand;
  CKD_GetSetFct               getSet;
  CKD_MinimizeFct             minimize;
  CKD_IsMemberOfFct           isMemberOf;
  CKD_MemberValidFct          memberValid;
  CKD_CloneFct                clone;
  CKD_MakeEmptyFct            makeEmpty;
  CKD_DeleteFct               delete;
} CKD_FunctionsS, *CKD_FunctionsT;

/******************************************************************************
 * externes API
  *****************************************************************************/

/*
 * ACHTUNG: für Member- und Individual- Einträge muss der observer NULL sein !!!
 */

/*
 * Erzeugt einen neuen mehrdimensionalen Heap mit einem Cache pro Dimension. 
 * initial   die Anfangsgröße des Heaps
 * cacheSize Größe jedes einzelnen Caches
 * k         Anzahl der Dimensionen
 */
CKDHeapT CKD_mkHeap( unsigned long initial, unsigned long cacheSize,
		 DimensionT k, CKD_FunctionsT fcts);

/*
 * Löscht einen Heap
 * h         der zu löschende Heap
 */
void CKD_delHeap( CKDHeapT h );

/*
 * Leert einen Heap und setzt alles auf die selben Werte, wie direkt nach der 
 * Erzeugung.
 * h         der zu leerende Heap
 */
void CKD_clearHeap( CKDHeapT h );

/*
 * Prüft, ob der Heap Elemente enthält, die in mindestens einer Dimension minimal 
 * sein können. (Elemente können in einer Dimension minimal sein, wenn ihr Gewicht
 * ungleich unendlich ist.
 * h         dieser Heap wird untersucht
 */
BOOLEAN CKD_isEmpty( CKDHeapT h );

/*
 * Prüft, ob der Heap Elemente enthält, die in der d-ten Dimension minimal sein
 * können. (s. CKD_isEmpty() )
 * h         dieser Heap wird untersucht
 * d         die zu berücksichtigende Dimension
 */
BOOLEAN CKD_isEmptyInDimension( CKDHeapT h, DimensionT d );


/*
 * Fügt ein weiteres Element in den Heap ein.
 * h         in diesen Heap wird eingefügt 
 * x         dieses Element wird eingefügt
 * ACHTUNG: für Member- und Individual- Einträge muss der observer NULL sein !!!
 */
void CKD_insert( CKDHeapT h, void* x, unsigned long* observer ); 


/*
 * Kopiert das in der Dimension d minimale Element in res zurück.
 * h         aus diesem Heap wird das minimale Element ausgewählt
 * res       Zeiger auf den Speicher, in dem das minimale Element kopiert wird
 * d         in dieser Dimension wird das minimale Element ausgewählt        
 * ACHTUNG: für Member- und Individual- Einträge muss der observer NULL sein !!!
 */
BOOLEAN CKD_getMin( CKDHeapT h, void** res, DimensionT d ); 

/*
 * Löscht das in der Dimension d minimale Element und gibt es in res zurück.
 * h         aus diesem Heap wird das minimale Element ausgewählt
 * res       Zeiger auf den Speicher, in dem das minimale Element kopiert wird
 * d         in dieser Dimension wird das minimale Element ausgewählt        
 * ACHTUNG: für Member- und Individual- Einträge muss der observer NULL sein !!!
 */
BOOLEAN CKD_deleteMin( CKDHeapT h, void** res, DimensionT d); 

/*
 * Löscht ein spezielles Element aus dem CKDHeap und kopiert es in res.
 * h         aus diesem Heap wird ein Element gelöscht
 * index     dieses Element ist aus dem Heap zu löschen
 * res       Zeiger auf den Speicher, in dem das aus dem Heap entfernte Element gespeichert wird
 * ACHTUNG: für Member- und Individual- Einträge muss der observer NULL sein !!!
 */
void CKD_deleteElement( CKDHeapT h, unsigned long index, void** res, unsigned long** observer ); 


/*
 * Führe ein heapify auf e des Heaps h durch. Dabei muss EL_IS_SET( e ) == TRUE gelten.
 * h         in diesem Heap wird das Heapify durchgeführt
 * index     an dieser Stelle liegende Element wird ein heapify durchgeführt
 */
void CKD_heapifyElement( CKDHeapT h, unsigned long index );


/*
 * Liefert das größte in der Dimension dim gecachte Element zurück
 * h         in diesem Heap wird im Cache mit der Dimension dim das nach dim größte Element gesucht
 * dim       in dieser Dimension wird gesucht
 */
void * CKD_getWorstCached( CKDHeapT h, DimensionT dim);

/*
 * Liefert true, wenn der cache für die in dim angegebene Dimension die Zielgröße erreicht hat
 * h        in diesem Heap wird der Cache untersucht
 * dim      der Cache dieser Dimension wird auf die Zielgröße untersucht
 */
BOOLEAN CKD_cacheFull( CKDHeapT h, DimensionT dim);

/*
 * Gibt die Informationen über den Heap auf die Konsole aus.
 * h        der Heap auf dem gearbeitet werden soll
 * printE   Funktion zur ausgabe der Elemente
 */
void CKD_ToScreen(CKDHeapT h, printElementFunc printE);

#endif /* CACHED_KDHEAP */
