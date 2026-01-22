/* *************************************************************************
 *
 * CachedKDHeap.h
 *
 * KDHeap mit Caches nach in mehreren Dimensionen.
 *
 * Bernd Loechner, Jean-Marie Gaillourdet 13.1.2003
 *
 * ************************************************************************* */

#include "CachedKDHeap.h"
#include "SpeicherVerwaltung.h"
#include "FehlerBehandlung.h"
/*U*/#include "KPVEHeader.h"
/******************************************************************************
 * Konstanten
 *****************************************************************************/

#define CKD_INDEX_UNDEF ((unsigned long) (-1L))

/******************************************************************************
 * Typdefinitionen
 *****************************************************************************/

typedef unsigned long* DoublettenEintrag;

typedef struct {
  /* eigene Dimension ab 0 gezählt */
  DimensionT dimension;
  DimensionT anzahl;

  /* Größe des Caches */
  unsigned long size;

  CKD_FunctionsT fcts;
  NKDH_FunctionsT* kdhFcts;

  /* Heap mit gecachten Elementen, mit einer Dimension mehr als der CKDHeap */
  NKDHeapT h; 
} CKDCacheS, *CKDCacheT;

struct CKDHeapS {
  /* Zahl der Dimensionen des Heaps */
  unsigned int dimensionen; 

  /* Membercaches */
  CKDCacheT c;

  /* Heap mit Set- und Individualeinträgen */
  NKDHeapT h;

  /* hierin werden alle Doubletten angelegt um sie leicht wieder frei geben zu können */
  SV_SpeicherManagerT d;

  /* Callbacks */
  CKD_FunctionsT fcts;
};

typedef struct {
  NKDH_ComparisonFct clientGreater;
  void* clientContext;

  /* in dieser Dimension werden alle Elemente umgekehrt zur primären Dimension dieses Caches
   * sortiert. */
  DimensionT invertierteDim;

  /* dies ist die primäre / eigene Dimension dieses Caches */
  DimensionT eigeneDim;
} GreaterContextS, *GreaterContextT;

/******************************************************************************
 * interne Funktionen
 *****************************************************************************/

static BOOLEAN greater( unsigned dim, void* x, void* y, void* context ) {
  GreaterContextT c = (GreaterContextT) context;

  if (dim == c->invertierteDim) {
    return !c->clientGreater( c->eigeneDim, x, y, c->clientContext );
  } else {
    return c->clientGreater( dim, x, y, c->clientContext );
  }
}

static void zurueckInSet( CKDHeapT h, void* e )
{
  unsigned long setIndex;
  void * set;

  setIndex = h->fcts->getSet(e);
  set = NKDH_getElement(h->h, setIndex); 
  /*U*/printf("putting element in set:  ");
  /*U*/KPVE_PrintElement(set, TRUE);   
  h->fcts->minimize(set, e);
  CKD_heapifyElement(h, setIndex);
}

static void initCache( CKDCacheT c, unsigned long size, DimensionT anzahl, 
		       DimensionT eigene, CKD_FunctionsT fcts ) {

  GreaterContextT context = our_alloc( sizeof(GreaterContextS) );

  context->clientGreater = fcts->kdh.greater;
  context->clientContext = fcts->kdh.greaterContext;
  context->eigeneDim = eigene;
  context->invertierteDim = anzahl;

  c->dimension = eigene;
  c->anzahl = anzahl;
  c->size = size;
  c->fcts = fcts;

  c->kdhFcts = our_alloc(sizeof(NKDH_FunctionsT));
  c->kdhFcts->greater = greater;
  c->kdhFcts->greaterContext = context;
  /*U*//*printf("ckd112");*/
  c->kdhFcts->isInfty = c->fcts->kdh.isInfty;

  c->h = NKDH_mkHeap( size, anzahl+1, *c->kdhFcts );
}

static CKDCacheT mkCaches( unsigned long size, DimensionT anzahl, 
			   CKD_FunctionsT fcts ) 
{
  DimensionT i;
  CKDCacheT res;

  res = our_alloc( sizeof(CKDCacheS) * anzahl );
  for (i=0; i<anzahl; i++)
    initCache( &(res[i]), size, anzahl, i, fcts );

  return res;
}


static void delCaches( CKDCacheT cs, DimensionT anzahl ) 
     /* cs[i].fcts wird mit dem CKDHeap freigegeben */
{
  DimensionT i;

  for (i=0; i<anzahl; i++) {
    NKDH_delHeap( cs[i].h );
    our_dealloc( cs[i].kdhFcts );
  }

  our_dealloc( cs );
}
  
static void displaceFromCache( CKDHeapT h, CKDCacheT cs, void* e, unsigned long* e_observer ) {
  DoublettenEintrag doub;
  DimensionT i;
  BOOLEAN nochInCacheVorhanden = FALSE;
  void* dummy;

  doub = &(e_observer[-(cs->dimension)]); /* Anfang des DoublettenEintrages bestimmen */

  NKDH_deleteElement(cs->h,*e_observer,&dummy,NULL);
  doub[cs->dimension] = CKD_INDEX_UNDEF;

  for (i = 0; i<cs->anzahl; i++) {
    if (doub[i] != CKD_INDEX_UNDEF) {
      nochInCacheVorhanden |= TRUE;
    }
  }

  if (!nochInCacheVorhanden) {
    SV_Dealloc(h->d,doub);
    /*U*/printf("zurueckInSet() called by displace() \n");
    zurueckInSet(h,e);
  }
}

/* löscht ein Element aus allen Caches.
 * dim gibt an in welcher Dimension e und e_observer gefunden wurden.
 */
static void removeFromCaches( CKDHeapT h, unsigned long* e_observer, DimensionT dim ) {
  DoublettenEintrag doub;
  DimensionT i;
  void* dummy;

  doub = &(e_observer[-dim]); /* Anfang des DoublettenEintrages bestimmen */


  for (i = 0; i<h->dimensionen; i++) {
    if (doub[i] != CKD_INDEX_UNDEF) {    
      /*U*//*printf("        removeFromCaches:   removing element number %lu from Cache number %lu\n", (unsigned long) doub[i], (unsigned long) i);*/
      /* NKDH_deleteElement(h->c[i].h,*e_observer,&dummy,NULL);
	 doub[dim] = CKD_INDEX_UNDEF;*/
      /*im Cacheheap i das Element Nr doub[i] löschen, den Inhalt des Elements in dummy und den Observer in den NULLpointer schreiben*/
      NKDH_deleteElement(h->c[i].h, doub[i], &dummy, NULL);
      doub[i] = CKD_INDEX_UNDEF;/*Den doubletteneintrag löschen (auf "nicht im Cache" setzten)*/
    }
  }

  SV_Dealloc(h->d,doub);
}

static void insertInCache( CKDHeapT h, CKDCacheT cs, void* e, DoublettenEintrag doub ) {
  void* max;
  unsigned long* max_observer;
  unsigned long* observer;

  observer = &(doub[cs->dimension]);

  if (NKDH_size(cs->h) < cs->size) {
    NKDH_insert(cs->h,e, observer );
  } else {
    /*if (NKDH_getMin(cs->h,&max,&max_observer,cs->dimension)) {*/
    if (NKDH_getMin(cs->h, &max, &max_observer, cs->anzahl)) {
      if (cs->fcts->kdh.greater(cs->dimension,max,e,cs->fcts->kdh.greaterContext)) {
	displaceFromCache(h,cs,max,max_observer);
	NKDH_insert(cs->h,e, observer );
	/*U*/printf("element cached (dimension %lu)\n", (unsigned long) cs->dimension);
      } else {
	/*U*/printf("element ");
	/*U*/KPVE_PrintElement(e, FALSE);
	/*U*/printf(" NOT cached (dimension %lu) as not smaller than ", (unsigned long) cs->dimension);
	/*U*/KPVE_PrintElement((void*) max, TRUE);
	/* nicht einfügen, da das schlechteste Element im Cache besser ist */
	*observer = CKD_INDEX_UNDEF;
      }
    } else {
      /* nicht einfügen, da alle Elemente im Cache optimal sind */
      *observer = CKD_INDEX_UNDEF;
    }
  }

}


static DimensionT doublettenGroesse( DoublettenEintrag doub, DimensionT anzahl ) {
  DimensionT i,counter = 0;

  for (i=0; i<anzahl; i++) {
    if (!(doub[i] == CKD_INDEX_UNDEF)) 
      counter++;
  }

  return counter;
}


/******************************************************************************
 * externes API
 *****************************************************************************/

/*
 */
CKDHeapT CKD_mkHeap( unsigned long initial, unsigned long cacheSize, 
		    DimensionT k, CKD_FunctionsT fcts ) {
  CKDHeapT newCKD;

  newCKD = our_alloc( sizeof(struct CKDHeapS) );
  newCKD->dimensionen = k;
  newCKD->fcts = our_alloc( sizeof(CKD_FunctionsS) );
  *(newCKD->fcts) = *fcts;
  newCKD->c = mkCaches( cacheSize, k, newCKD->fcts );

  SV_BeliebigenManagerInitialisieren( &(newCKD->d), sizeof(unsigned long) * k, 
				      cacheSize, FALSE, "CKD Doublettenverwaltung" );

  /*  SV_BeliebigenManagerInitialisieren( &(newCKD->d), sizeof(unsigned long), cacheSize, FALSE, "CKD_Doublettenverwaltung");*/
  newCKD->h = NKDH_mkHeap(initial,k,newCKD->fcts->kdh);

  return newCKD;
}

/*
 * XXX Doubletten werden nicht freigegeben
 */
void CKD_delHeap( CKDHeapT h ) {
  delCaches( h->c, h->dimensionen );
  our_dealloc( h->fcts );
  NKDH_delHeap( h->h );
}

/*
 * XXX Doubletten werden nicht freigegeben
 */
void CKD_clearHeap( CKDHeapT h ) {
  DimensionT i;

  NKDH_clear( h->h );

  for (i=0; i<h->dimensionen; i++) {
    NKDH_clear( h->c[i].h );
  }

}

BOOLEAN CKD_isEmpty( CKDHeapT h ) {
  DimensionT i;

  for (i=0; i<h->dimensionen; i++)
    if (!CKD_isEmptyInDimension( h, i ))
      return FALSE;

  return TRUE;
}

BOOLEAN CKD_isEmptyInDimension( CKDHeapT h, DimensionT d ) {
  void* e;
  DimensionT i;
  
  for (i=0; i<h->dimensionen; i++) {
    if (NKDH_getMin(h->c[i].h,&e,NULL,d)) {
      /*U*//*printf("ckd296");*/
      if (!(h->fcts->kdh.isInfty(d,e))) {
	return FALSE;
      }
    }
  }

  if (NKDH_getMin(h->h,&e,NULL,d)) {
    /*U*//*printf("ckd304");*/
    if (!(h->fcts->kdh.isInfty(d,e))) {
      return FALSE;
    }
  }
  
  return TRUE;
}

void CKD_insert( CKDHeapT h, void* x, unsigned long* observer ) {
  DimensionT i;

  if (h->fcts->isSet(x)) {
    NKDH_insert( h->h, x, observer );
  } else if (h->fcts->isIndividual(x)) {
    /*if (observer != NULL) {
      our_error( "CKD_insert: Individualeintrag mit non-NULL Observer" );
      }*/
    NKDH_insert( h->h, x, observer );
  } else { /* EXPAND Callback */
    DoublettenEintrag doub;
    
    if (observer != NULL) {
      our_error( "CKD_insert: Membereintrag mit non-NULL Observer" );
    }

    SV_Alloc( doub, h->d, void* );
    
    for (i=0; i<h->dimensionen; i++) {
      insertInCache( h, &(h->c[i]), x, doub );
    }
    
    if (doublettenGroesse( doub, h->dimensionen ) == 0) {
      /*U*/printf("zurueckInSet() called by insert for ");    
      /*U*/KPVE_PrintElement(x, TRUE);
      zurueckInSet( h, x );
    }
  }
}

/*
 */
BOOLEAN CKD_getMin( CKDHeapT h, void** res, DimensionT d ) {
  void* min_e = NULL;
  void* e;
  DimensionT min_d = MAX_DIMENSION;
  DimensionT i;
  
  /*U*/ printf("ckd getmin entered\n");

  for (i=0; i<h->dimensionen; i++) {
    if (!NKDH_isEmpty( h->c[i].h ) && NKDH_getMin( h->c[i].h, &e, NULL, d )) {
      if (h->fcts->kdh.greater(d,min_e,e,h->fcts->kdh.greaterContext)) {
	min_e = e; 
	min_d = i;       
      }
    }
  }

  if (!NKDH_isEmpty( h->h ) && NKDH_getMin( h->h, &e, NULL , d )) {
    if (min_e != NULL) {
      if (h->fcts->kdh.greater(d, min_e, e,h->fcts->kdh.greaterContext)) { /* Minimum ist im Set-Heap */
	if (h->fcts->isIndividual(e)) { 
	  *res = e;
	  /*U*//*printf("ckd getmin left with true\n");*/
	  return TRUE;
	} else if (h->fcts->isSet(e)) {
	  h->fcts->expand( h, e );
	  /*NKDH_heapifyElement( h->h, e_observer );*/
	  our_error("hier muss ein heapify her");
	  /*U*/printf("ckd getmin recursing\n");
	  return CKD_getMin( h, res, d );
	  }
      } else { /* Minimum ist im Cache */
	*res = min_e;
	/*U*//*printf("ckd getmin left with true\n");*/
	return TRUE;
      }
    } else { /* Minimum ist im Set-Heap */
      if (h->fcts->isIndividual(e)) {
	*res = e;
	/*U*//*printf("ckd getmin left with true\n");*/
	return TRUE;
      } else if (h->fcts->isSet(e)) {
	h->fcts->expand(h,e);
	our_error("hier muss ein heapify her");
	/*U*/printf("ckd getmin recursing\n");
	return CKD_getMin( h, res, d );
      }      
    }      
  } else { /* kein Minimum im Set-Heap */
    if (min_e != NULL) {
      *res = min_e;
      /*U*//*printf("ckd getmin left with true\n");*/
      return TRUE;
    } else { /* alle Heaps leer */
      *res = NULL;
      /*U*//*printf("ckd getmin left with false\n");*/
      return FALSE;
    }
  }
  return FALSE; /* nur um ein Warning des GCC   zu vermeiden */
}

BOOLEAN CKD_deleteMin( CKDHeapT h, void** res, DimensionT d) {
  /*U*//*printf("ckd deletemin entered\n");*/
  void* min_e = NULL;
  void* e;
  unsigned long* e_observer;
  unsigned long* min_e_observer = NULL;
  DimensionT min_d = MAX_DIMENSION;
  DimensionT i;
  

  for (i=0; i<h->dimensionen; i++) {
    if (!NKDH_isEmpty( h->c[i].h)) {
      if (NKDH_getMin( h->c[i].h, &e, &e_observer, d )) {
	if (NULL != min_e) {
	  if (h->fcts->kdh.greater(d,min_e,e,h->fcts->kdh.greaterContext)) {
	    min_e_observer = e_observer;
	    min_e = e; 
	    min_d = i;
	  }
	} else { /*erstes gefundenes -> setzte Werte*/
	  min_e_observer = e_observer;
	  min_e = e;
	  min_d = i;
	}
      }
    }
  }

  if (!NKDH_isEmpty( h->h ) && NKDH_getMin( h->h, &e, &e_observer , d )) {
    if ((NULL == min_e) || (h->fcts->kdh.greater(d, min_e, e,h->fcts->kdh.greaterContext))) { /* Minimum ist im Set-Heap */
      if (h->fcts->isIndividual(e)) { 
	/*U*//*printf("individual to be returned\n");*/
	*res = e;
	/*U*//*printf("ckd 440");*/
	NKDH_deleteMin(h->h,&e,&e_observer,d);
	/*U*//*printf("ckd deletemin left with true1\n");*/
	return TRUE;
      } else if (h->fcts->isSet(e)) {
	/*U*//*printf("set has to be expanded\n");*/
	h->fcts->expand( h, e );
	/*printf("                               @@@@@@@@@@@@ observer(this will be heapified) %lu\n", *e_observer);*/
	NKDH_heapifyElement( h->h, *e_observer );
	/*U*//*printf("ckd deletemin recursing1\n");*/
	return CKD_deleteMin( h, res, d );
      }
    } else { /* Minimum ist im Cache */
      *res = min_e;
      removeFromCaches(h,min_e_observer,min_d);
      /*U*//*printf("ckd deletemin left with true2\n");*/
      return TRUE;
    }
 
  } else { /* kein Minimum im Set-Heap (weil leer oder nur unendliche Elemente)*/
    if (min_e != NULL) {
      *res = min_e;
      removeFromCaches(h,min_e_observer,min_d);
      /*U*//*printf("ckd deletemin left with true4\n");*/
      return TRUE;
    } else { /* alle Heaps leer */
      *res = NULL;
      /*U*//*printf("ckd deletemin left with false1 - size of heap 0:%lu 1:%lu 2:%lu 3:%lu sets:%lu\n",
		  NKDH_size((h->c[0]).h),
		  NKDH_size((h->c[1]).h),
		  NKDH_size((h->c[2]).h), 
		  NKDH_size((h->c[3]).h), 
		  NKDH_size(h->h));*/
      return FALSE;
    }
  }
  /*U*//*printf("ckd deletemin left with false2\n");*/
  return FALSE; /* nur um ein Warning des GCC   zu vermeiden */
}

void CKD_deleteElement( CKDHeapT h, unsigned long index, void** res, unsigned long** observer ) {
  NKDH_deleteElement( h->h, index, res, observer );
}


void CKD_heapifyElement( CKDHeapT h, unsigned long index ) {
  NKDH_heapifyElement( h->h, index );
}

/*
 * Liefert das größte in der Dimension dim gecachte Element zurück
 * existiert keins, wird NULL zurückgeliefert
 * h         in diesem Heap wird im Cache mit der Dimension dim das nach dim größte Element gesucht
 * dim       in dieser Dimension wird gesucht
*/
void * CKD_getWorstCached( CKDHeapT h, DimensionT dim) {
  void * worstElement;
  unsigned long * someObserverNobodyCaresAbout;
  if (NKDH_getMin( (h->c[dim]).h, &worstElement, &someObserverNobodyCaresAbout, h->dimensionen)) {
    return worstElement;
  } else {
    return NULL;
  }
}


/*
 * Liefert true, wenn der cache für die in dim angegebene Dimension die Zielgröße erreicht hat
 * h        in diesem Heap wird der Cache untersucht
 * dim      der Cache dieser Dimension wird auf die Zielgröße untersucht
 */
BOOLEAN CKD_cacheFull( CKDHeapT h, DimensionT dim) {
  /*U*/if (((h->c[dim]).size) > (NKDH_size((h->c[dim]).h))){
  /*U*/  printf("cache number %lu not yet full: actual size is : %lu, aim size : %lu\n", (unsigned long) dim, NKDH_size((h->c[dim]).h), (h->c[dim]).size);
  return ((((h->c[dim]).size) == (NKDH_size((h->c[dim]).h))) || (((h->c[dim]).size) < (NKDH_size((h->c[dim]).h))));
  /*U*/} else {
  /*U*/  printf("cache number %lu FULL: actual size is : %lu, aim size : %lu\n", (unsigned long) dim, NKDH_size((h->c[dim]).h), (h->c[dim]).size);
  return ((((h->c[dim]).size) == (NKDH_size((h->c[dim]).h))) || (((h->c[dim]).size) < (NKDH_size((h->c[dim]).h))));
  /*U*/}
}

void CKD_ToScreen(CKDHeapT h, printElementFunc printE) {
  unsigned long int i;
  printf("_________________________________________________________________________________________\n");
  printf("CachedKDHeap: dimensions: %u\n", h->dimensionen);
  printf("Membercaches:\n");
  for (i = 0; i < h->dimensionen; ++i) {
    printf("Cache number %lu:\n", (unsigned long) h->c[i].dimension);
    NKDH_ToScreen(h->c[i].h, printE);
  }
  printf("\nSet and individual cache:\n");
  NKDH_ToScreen(h->h, printE);
  printf("\nDoubletten:\nTODO!!\n");
  printf("_________________________________________________________________________________________\n");
}
 
