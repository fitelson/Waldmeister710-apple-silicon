/* *************************************************************************
 *
 * KKDHeap.h
 *
 * Array-basierte Heap als PQ.
 *
 * basiert auf KDHeap von Bernd Loechner, 12.10.2001 
 * angepasst auf KissKDHeapElements Hendrik Spies 22.05.2003
 *
 * ************************************************************************* */

#ifndef KISS_KDHEAP_H
#define KISS_KDHEAP_H

#include "general.h"
#include <limits.h>

#include "KissKDHeapElements.h"
/* KElementT muss eine Struct sein, die ein "unsigned long *observer"
 * enthaelt.
 */

#define KKDH_UndefinedObserver ULONG_MAX

#define KKDH_IsUndefinedObserver(x) ((x) == KKDH_UndefinedObserver)

typedef enum {
  WTI_Nothing, WTI_Reweight, WTI_Stop, WTI_Delete
} KKDH_WalkthroughInteractionT;

typedef KKDH_WalkthroughInteractionT (*KKDH_WalkthroughFct)(KElementT *xp);

/*
 * Prädikat, das TRUE liefert gdw e1 größer als e2 in der Dimension d ist.
 */
typedef BOOLEAN (*KKDH_ComparisonFct)(unsigned dim, KElementT* x, KElementT* y, void* context );

/*
 * Prädikat, das TRUE liefert gdw e in der Dimension unendlich schwer ist.
 */
typedef BOOLEAN (*KKDH_IsInftyFct)(unsigned dim, KElementT* e);

typedef struct {
  KKDH_ComparisonFct greater;
  void*             greaterContext;
  KKDH_IsInftyFct    isInfty;
} KKDH_FunctionsS, *KKDH_FunctionsT;

typedef struct KKDHeapS *KKDHeapT; /* Opak :-) */

KKDHeapT KKDH_mkHeap(unsigned long initial, unsigned k);
/* Eerzeugt einen KKDHeap mit initial vielen Elementen und k Dimensionen.
 * Dazu wird der Verwaltungsblock und das Elementarray allokiert.
 */

void KKDH_delHeap(KKDHeapT h);
/* Gibt einen KKDHeap wieder frei, d.h. das Elementarray und der 
 * Verwaltungsblock. Dinge, auf die Elemente zeigen, werden nicht 
 * freigegeben. */

void KKDH_clear(KKDHeapT h);
/* loescht alle Elemente in h, allerdings werden Dinge, auf die Elemente
 * zeigen, nicht freigegeben.
 */

BOOLEAN KKDH_isEmpty(KKDHeapT h);
/* Gibt an, ob der KKDHeap leer ist. 
 */

unsigned long KKDH_size(KKDHeapT h);
/* Gibt die Anzahl der zur Zeit verwalteten Elemente an. 
 */

void KKDH_insert (KKDHeapT h, KElementT x);
/* Fuegt ein Element ein. Ggf. wird das Elementarray erweitert. 
 */

BOOLEAN KKDH_getMin(KKDHeapT h, KElementT *res, unsigned d);
/* Kopiert das bzgl. Dimension d minimale Element nach (*res). Das Element
 * wird nicht geloescht. Wenn kein Element gefunden wurde mit einem Gewicht kleiner
 * unendlich wird FALSE zurückgegeben sonst TRUE. */

BOOLEAN KKDH_deleteMin(KKDHeapT h, KElementT *res, unsigned d);
/* Kopiert das bzgl. Dimension d minimale Element nach (*res). Das Element
 * wird danach geloescht. Wenn kein Element gefunden wurde mit einem Gewicht kleiner
 * unendlich wird FALSE zurückgegeben sonst TRUE. */

void KKDH_deleteElement(KKDHeapT h, unsigned long index, KElementT *res);
/* Kopiert das durch index spezifizierte Element nach (*res) und loescht es
 * danach. index gibt die _Nummer_ des Elementes an, d.h. unabhaengig von der 
 * Variante muss gelten: 0 <= index < KKDH_size(h)
 */

KElementT *KKDH_getElement(KKDHeapT h, unsigned long index);
/* Gibt einen Zeiger auf das durch index spezifizierte Element. Wird das 
 * Gewicht des Elementes von aussen veraendert, muss danach 
 * KKDH_heapifyElement oder KKDH_heapify aufgerufen werden.
 */

BOOLEAN KKDH_isInHeap(KKDHeapT h, KElementT x);
/* Durchsucht den Heap nach dem uebergebenen Element und liefert
 * TRUE, wenn es gefunden wird, ansonsten FALSE.
 */

void KKDH_heapifyElement(KKDHeapT h, unsigned long index);
/* Stellt die Heapbedingung fuer das durch index spezifizierte Element her.
 */

void KKDH_heapify(KKDHeapT h);
/* Stellt die Heapbedingung fuer Heap wieder her. Sinnvoll, wenn zu oft
 * KKDH_heapifyElement aufgerufen werden wuerde.
 */

void KKDH_Walkthrough(KKDHeapT h, KKDH_WalkthroughFct f);
/* Iteriert in unspezifizierter Weise ueber alle Elemente in h.
 * liefert f WTI_Delete wird das Element geloescht (ggf. an Pointern haengende
 *   Werte muessen schon von f entsorgt werden!)
 * liefert f WTI_Reweight wird das Element neu einsortiert.
 * liefert f WTI_Stop wird das Element neu einsortiert, der Durchlauf
     wird danach beendet
 * liefert f WTI_Nothing passiert von Seiten des KKDHeaps nichts!
 */

void KKDH_ToScreen(KKDHeapT h);
/* Gibt den heap auf die Konsole aus (zu debugging Zwecken)
 */
#endif
