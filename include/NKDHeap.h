/* *************************************************************************
 *
 * NKDHeap.h
 *
 * Array-basierte Heap als PQ.
 *
 * Bernd Loechner, 12.10.2001
 *
 * ************************************************************************* */

#ifndef NKDHEAP_H
#define NKDHEAP_H

#include "general.h"
#include <limits.h>

#define NKDH_UndefinedObserver ULONG_MAX

#define NKDH_IsUndefinedObserver(x) ((x) == NKDH_UndefinedObserver)


typedef enum {
  NWTI_Nothing, NWTI_Reweight, NWTI_Stop, NWTI_Delete
} NKDH_WalkthroughInteractionT;

typedef NKDH_WalkthroughInteractionT (*NKDH_WalkthroughFct)(void* xp);

/*
 * Prädikat, das TRUE liefert gdw e1 größer als e2 in der Dimension d ist.
 */
typedef BOOLEAN (*NKDH_ComparisonFct)(unsigned dim, void* x, void* y, void* context );

/*
 * Prädikat, das TRUE liefert gdw e in der Dimension unendlich schwer ist.
 */
typedef BOOLEAN (*NKDH_IsInftyFct)(unsigned dim, void* e);

typedef struct {
  NKDH_ComparisonFct greater;
  void*             greaterContext;
  NKDH_IsInftyFct    isInfty;
} NKDH_FunctionsT;

typedef struct NKDHeapS *NKDHeapT; /* Opak :-) */

NKDHeapT NKDH_mkHeap(unsigned long initial, unsigned k, NKDH_FunctionsT fcts);
/* Eerzeugt einen NKDHeap mit initial vielen Elementen und k Dimensionen.
 * Dazu wird der Verwaltungsblock und das Elementarray allokiert.
 */

void NKDH_delHeap(NKDHeapT h);
/* Gibt einen NKDHeap wieder frei, d.h. das Elementarray und der 
 * Verwaltungsblock. Dinge, auf die Elemente zeigen, werden nicht 
 * freigegeben. */

void NKDH_clear(NKDHeapT h);
/* loescht alle Elemente in h, allerdings werden Dinge, auf die Elemente
 * zeigen, nicht freigegeben.
 */

BOOLEAN NKDH_isEmpty(NKDHeapT h);
/* Gibt an, ob der NKDHeap leer ist. 
 */

unsigned long NKDH_size(NKDHeapT h);
/* Gibt die Anzahl der zur Zeit verwalteten Elemente an. 
 */

void NKDH_insert (NKDHeapT h, void* x, unsigned long *observer );
/* Fuegt ein Element ein. Ggf. wird das Elementarray erweitert. 
 */

BOOLEAN NKDH_getMin(NKDHeapT h, void** res, unsigned long** observer, unsigned d);
/* Kopiert das bzgl. Dimension d minimale Element nach (*res). Das Element
 * wird nicht geloescht. Wenn kein Element gefunden wurde mit einem Gewicht kleiner
 * unendlich wird FALSE zurückgegeben sonst TRUE. */

BOOLEAN NKDH_deleteMin(NKDHeapT h, void** res, unsigned long** observer, unsigned d);
/* Kopiert das bzgl. Dimension d minimale Element nach (*res). Das Element
 * wird danach geloescht. Wenn kein Element gefunden wurde mit einem Gewicht kleiner
 * unendlich wird FALSE zurückgegeben sonst TRUE. */

void NKDH_deleteElement(NKDHeapT h, unsigned long index, void** res, unsigned long** observer);
/* Kopiert das durch index spezifizierte Element nach (*res) und loescht es
 * danach. index gibt die _Nummer_ des Elementes an, d.h. unabhaengig von der 
 * Variante muss gelten: 0 <= index < NKDH_size(h)
 */

void* NKDH_getElement(NKDHeapT h, unsigned long index);
/* Gibt einen Zeiger auf das durch index spezifizierte Element. Wird das 
 * Gewicht des Elementes von aussen veraendert, muss danach 
 * NKDH_heapifyElement oder NKDH_heapify aufgerufen werden.
 */

void NKDH_heapifyElement(NKDHeapT h, unsigned long index);
/* Stellt die Heapbedingung fuer das durch index spezifizierte Element her.
 */

void NKDH_heapify(NKDHeapT h);
/* Stellt die Heapbedingung fuer Heap wieder her. Sinnvoll, wenn zu oft
 * NKDH_heapifyElement aufgerufen werden wuerde.
 */

void NKDH_Walkthrough(NKDHeapT h, NKDH_WalkthroughFct f);
/* Iteriert in unspezifizierter Weise ueber alle Elemente in h.
 * liefert f WTI_Delete wird das Element geloescht (ggf. an Pointern haengende
 *   Werte muessen schon von f entsorgt werden!)
 * liefert f WTI_Reweight wird das Element neu einsortiert.
 * liefert f WTI_Stop wird das Element neu einsortiert, der Durchlauf
     wird danach beendet
 * liefert f WTI_Nothing passiert von Seiten des NKDHeaps nichts!
 */

typedef void (*printElementFunc)(void* e, BOOLEAN newline);

void NKDH_ToScreen(NKDHeapT h, printElementFunc printE);
  /* Iteriert ueber alle Elemente und wendet die printElementFunc
   * darauf an.
   */
#endif
