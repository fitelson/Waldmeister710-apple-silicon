/* *************************************************************************
 *
 * KDHeap.h
 *
 * Array-basierte Heap als PQ.
 *
 * Bernd Loechner, 12.10.2001
 *
 * ************************************************************************* */

#ifndef KDHEAP_H
#define KDHEAP_H

#include "general.h"
#include <limits.h>

#include "KDHeapElements.h"
/* ElementT muss eine Struct sein, die ein "unsigned long *observer"
 * enthaelt.
 */

#define KDH_UndefinedObserver ULONG_MAX

#define KDH_IsUndefinedObserver(x) ((x) == KDH_UndefinedObserver)

typedef enum {
  WTI_Nothing, WTI_Reweight, WTI_Stop, WTI_Delete
} KDH_WalkthroughInteractionT;

typedef KDH_WalkthroughInteractionT (*KDH_WalkthroughFct)(ElementT *xp);

/*
 * Prädikat, das TRUE liefert gdw e1 größer als e2 in der Dimension d ist.
 */
typedef BOOLEAN (*KDH_ComparisonFct)(unsigned dim, ElementT* x, ElementT* y, void* context );

/*
 * Prädikat, das TRUE liefert gdw e in der Dimension unendlich schwer ist.
 */
typedef BOOLEAN (*KDH_IsInftyFct)(unsigned dim, ElementT* e);

typedef struct {
  KDH_ComparisonFct greater;
  void*             greaterContext;
  KDH_IsInftyFct    isInfty;
} KDH_FunctionsT;

typedef struct KDHeapS *KDHeapT; /* Opak :-) */

KDHeapT KDH_mkHeap(unsigned long initial, unsigned k, KDH_FunctionsT fcts);
/* Eerzeugt einen KDHeap mit initial vielen Elementen und k Dimensionen.
 * Dazu wird der Verwaltungsblock und das Elementarray allokiert.
 */

void KDH_delHeap(KDHeapT h);
/* Gibt einen KDHeap wieder frei, d.h. das Elementarray und der 
 * Verwaltungsblock. Dinge, auf die Elemente zeigen, werden nicht 
 * freigegeben. */

void KDH_clear(KDHeapT h);
/* loescht alle Elemente in h, allerdings werden Dinge, auf die Elemente
 * zeigen, nicht freigegeben.
 */

BOOLEAN KDH_isEmpty(KDHeapT h);
/* Gibt an, ob der KDHeap leer ist. 
 */

unsigned long KDH_size(KDHeapT h);
/* Gibt die Anzahl der zur Zeit verwalteten Elemente an. 
 */

void KDH_insert (KDHeapT h, ElementT x);
/* Fuegt ein Element ein. Ggf. wird das Elementarray erweitert. 
 */

BOOLEAN KDH_getMin(KDHeapT h, ElementT *res, unsigned d);
/* Kopiert das bzgl. Dimension d minimale Element nach (*res). Das Element
 * wird nicht geloescht. Wenn kein Element gefunden wurde mit einem Gewicht kleiner
 * unendlich wird FALSE zurückgegeben sonst TRUE. */

BOOLEAN KDH_deleteMin(KDHeapT h, ElementT *res, unsigned d);
/* Kopiert das bzgl. Dimension d minimale Element nach (*res). Das Element
 * wird danach geloescht. Wenn kein Element gefunden wurde mit einem Gewicht kleiner
 * unendlich wird FALSE zurückgegeben sonst TRUE. */

void KDH_deleteElement(KDHeapT h, unsigned long index, ElementT *res);
/* Kopiert das durch index spezifizierte Element nach (*res) und loescht es
 * danach. index gibt die _Nummer_ des Elementes an, d.h. unabhaengig von der 
 * Variante muss gelten: 0 <= index < KDH_size(h)
 */

ElementT *KDH_getElement(KDHeapT h, unsigned long index);
/* Gibt einen Zeiger auf das durch index spezifizierte Element. Wird das 
 * Gewicht des Elementes von aussen veraendert, muss danach 
 * KDH_heapifyElement oder KDH_heapify aufgerufen werden.
 */

void KDH_heapifyElement(KDHeapT h, unsigned long index);
/* Stellt die Heapbedingung fuer das durch index spezifizierte Element her.
 */

void KDH_heapify(KDHeapT h);
/* Stellt die Heapbedingung fuer Heap wieder her. Sinnvoll, wenn zu oft
 * KDH_heapifyElement aufgerufen werden wuerde.
 */

void KDH_Walkthrough(KDHeapT h, KDH_WalkthroughFct f);
/* Iteriert in unspezifizierter Weise ueber alle Elemente in h.
 * liefert f WTI_Delete wird das Element geloescht (ggf. an Pointern haengende
 *   Werte muessen schon von f entsorgt werden!)
 * liefert f WTI_Reweight wird das Element neu einsortiert.
 * liefert f WTI_Stop wird das Element neu einsortiert, der Durchlauf
     wird danach beendet
 * liefert f WTI_Nothing passiert von Seiten des KDHeaps nichts!
 */

void KDH_ToScreen(KDHeapT h);
/* Ausgabe auf die Konsole
 */

#endif
