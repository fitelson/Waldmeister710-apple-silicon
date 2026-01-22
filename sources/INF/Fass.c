/* **************************************************************************
 * Fass.c
 * 
 * Jedes Fass steht für eine Instanz, der Menge P sortiert nach einer 
 * individuellen Heuristik. Damit es zur Selektion von Fakten genutzt werden 
 * kann, stellt es die Schnittstelle Zapfhahn zur Verfügung. Zum Empfangen von
 * neuen kritischen Paaren wird die Schnittstelle Tankstutzen implementiert.
 * 
 * Jean-Marie Gaillourdet, 02.09.2003
 *
 * ************************************************************************** */


#include "Fass.h"

#include "Ausgaben.h"
#include "DSBaumOperationen.h"
#include "FehlerBehandlung.h"
#include "general.h"
#include "Hauptkomponenten.h"
#include "Id.h"
#include "KissKDHeap.h"
#include "KissKDHeapElements.h"
#include "KPVerwaltung.h"
#include "NFBildung.h"
#include "Parameter.h"
#include "RUndEVerwaltung.h"
#include "SpeicherVerwaltung.h"
#include "TankAnlage.h"
#include "Tankstutzen.h"
#include "Termpaare.h"
#include "Unifikation1.h"
#include "ZapfAnlage.h"
#include "Zielverwaltung.h"

typedef struct {
  unsigned long observer;
  WId klein_w; /* Die Id des größten bisher selektierten Elementes des
		* zugehörigen ASets */
  unsigned ir_handled : 1;
} ObserverEntry;

typedef struct {
  ObserverEntry** observerArray;
  unsigned long capacity;
} ObserverArrayS, *ObserverArrayT;


struct FassS {
  HeapType type;
  KKDHeapT heap;
  KKDHeapT irHeap;
  KKDHeapT cache;
  KKDHeapT buffer; 
  unsigned long cacheSize;
  ObserverArrayT setObserver;
  ClassifyProcT classify;
};

/*undefinierter Observerwert*/
#define OBS_IDX_UNDEF ULONG_MAX

#define OBSERVER_BLOCK_SIZE 256

/** Funktion zum Auslesen einer Observeraddresse.
 *  Gibt es eine Addresse mit dem angegebenen Index nicht,
 *  so wird ein Fehler generiert.
 *
 *  @param array Die Adresse des ObserverArrayS aus dem gelesen werden soll.
 *  @param index Der Index des angeforderten Observers.
 *  @return die Adresse des angeforderten Observers.
 */
static ObserverEntry* getLookupEntryAddress(ObserverArrayT array, unsigned long index){
  if (array->capacity <= index) {
    return NULL;      
  }
  return &(array->observerArray[index/OBSERVER_BLOCK_SIZE][index%OBSERVER_BLOCK_SIZE]);
}

/** Funktion zur Beschaffung einer Observeraddresse zur Verwendung in einem Heapelement.
 *
 *  @param array Die Adresse des ObserverArrayS auf dem gearbeitet werden soll.
 *  @param index Der Index des angeforderten Observers.
 *  @return die Adresse des angeforderten ObserverEntry.
 */
static ObserverEntry* getNewLookupEntryAddress(ObserverArrayT array, unsigned long index) 
{
  ObserverEntry* nextBlock;
  long int i;

  while (array->capacity <= index) {
    nextBlock = our_alloc(OBSERVER_BLOCK_SIZE * sizeof(ObserverEntry));
    for (i=0; i < OBSERVER_BLOCK_SIZE; i++) {
      nextBlock[i].observer = OBS_IDX_UNDEF;
      WID_SET_ZERO(nextBlock[i].klein_w);	    
      nextBlock[i].ir_handled = 0;
    }

    array->capacity += OBSERVER_BLOCK_SIZE;
    array->observerArray = our_realloc(array->observerArray, 
				       (array->capacity/OBSERVER_BLOCK_SIZE) * sizeof(ObserverEntry), 
				       "extending ObserverArray");
    
    array->observerArray[(array->capacity-1)/OBSERVER_BLOCK_SIZE] = nextBlock;
  }
  return &(array->observerArray[index/OBSERVER_BLOCK_SIZE][index%OBSERVER_BLOCK_SIZE]);
}


static BOOLEAN fa_IdIstWaise(Id id) 
{
  RegelOderGleichungsT parent1;
  RegelOderGleichungsT parent2;
   
  if (ID_i(id) > 0) {
    parent1 = RE_getActiveWithBirthday(ID_i(id));
    if (!parent1->lebtNoch) {
      return TRUE;
    }
  }
  if (ID_j(id) > 0) {
    parent2 = RE_getActiveWithBirthday(ID_j(id));
    if (!parent2->lebtNoch) {
      return TRUE;
    }
  }

  return FALSE;
}

static BOOLEAN fa_IdPasstZuFasstyp(FassT f, Id id) 
{
  if (f->type == CP) {
    if (ID_isCP(id)) {
      return !RE_IstUngleichung(RE_getActiveWithBirthday(ID_i(id)))
        && !RE_IstUngleichung(RE_getActiveWithBirthday(ID_j(id)));
    } else if (ID_isACGOverlap(id)) {
      return !RE_IstUngleichung(RE_getActiveWithBirthday(ID_i(id)));
    } else {
      return FALSE;
    }
  } else {
    if (ID_isCP(id)) {
      if (POS_into_i(ID_xp(id))) {
        return RE_IstUngleichung(RE_getActiveWithBirthday(ID_i(id)));
      } else {
        return RE_IstUngleichung(RE_getActiveWithBirthday(ID_i(id)));
      }
    } else if (ID_isACGOverlap(id)) {
      return RE_IstUngleichung(RE_getActiveWithBirthday(ID_i(id)));
    } else {
      return FALSE;
    }
  }
}

static void zuASetHinzufeugen(FassT f, KElementT* x)
{
  KElementT* set;
  ObserverEntry* oe = getLookupEntryAddress(f->setObserver,WID_i(x->wid));

  set = KKDH_getElement(f->heap, oe->observer);
  KEL_MINIMIZE(*set,*x);
  KKDH_heapifyElement(f->heap, oe->observer);
}

static void fa_verdraengen(FassT f, KElementT* x)
{
  ObserverEntry* oe = getNewLookupEntryAddress(f->setObserver,WID_i(x->wid));

  if (oe->observer == OBS_IDX_UNDEF) {
    KElementT newset;
	
    WID_SET_INFTY(newset.wid);
    WID_i(newset.wid) = WID_i(x->wid);
    newset.observer = &(oe->observer);

    KKDH_insert(f->heap,newset);
  }
  zuASetHinzufeugen(f,x);
}

	
    


static void FA_CPinCacheOrHeap(FassT f, KElementT x) 
{
  KElementT maxCache;

  if ((KKDH_size(f->cache) == f->cacheSize) && KKDH_getMin(f->cache,&maxCache,1)) {
    if ((f->cacheSize != 0) && KEL_GREATER(0,&maxCache,&x)) {	    
      KKDH_deleteMin(f->cache,&maxCache,1);
      /* 	    IO_DruckeFlex("put %w into cache mit Verdrängung\n",&(x.wid)); */
      fa_verdraengen(f,&maxCache);
      KKDH_insert(f->cache,x);
    } else {	 
      /* 	    IO_DruckeFlex("put %w into aset\n",&(x.wid)); */
      zuASetHinzufeugen(f,&x);
    }       
  } else {
    /* 	IO_DruckeFlex("put %w into cache\n",&(x.wid)); */
    KKDH_insert(f->cache,x);
  }
}

static void fa_beendeReexpand(FassT f, AbstractTime i) 
{
  int produzierteFakten = 0;
  KElementT *set,x;
  WId border;
  ObserverEntry *oe;

  oe = getLookupEntryAddress(f->setObserver,i);
  set = KKDH_getElement(f->heap,oe->observer);
  border = set->wid;

  /*     IO_DruckeFlex("changing set %w ",&(set->wid)); */

  WID_SET_INFTY(set->wid);
  WID_i(set->wid) = i;

  /*     IO_DruckeFlex("to %w\n",&(set->wid)); */
  KKDH_heapifyElement(f->heap,oe->observer);
    
  while (KKDH_deleteMin(f->buffer, &x, 0)) {
    produzierteFakten += 1;
    if (!WID_GREATER(border,x.wid)) {
      /* 	    IO_DruckeFlex("took %w from buffer\n",&(x.wid)); */
      FA_CPinCacheOrHeap(f,x);
    }
  }

  if (produzierteFakten == 0) {
    KKDH_deleteElement(f->heap,oe->observer,&x);
  }
}


static void FA_leereBuffer(FassT f) 
{
  KElementT x;
  /*     KElementT *res; */
  ObserverEntry* oe;
  int produzierteFakten = 0;
    
  if (KKDH_getMin(f->buffer,&x,0)) {      

    oe = getNewLookupEntryAddress(f->setObserver,WID_i(x.wid));
    if (oe->observer != OBS_IDX_UNDEF) {
      fa_beendeReexpand(f,WID_i(x.wid));
      return;
    } else {
      KElementT newset;

      WID_SET_INFTY(newset.wid);
      WID_i(newset.wid) = WID_i(x.wid);
      newset.observer = &(oe->observer);

      /* 	    IO_DruckeFlex("inserting new set %w",&(newset.wid)); */
      KKDH_insert(f->heap,newset);
    }

    /* 	if (WID_i(x.wid)==105) { */
    /* 	    IO_DruckeFlex("Hallo\n"); */
    /* 	} */
	
    
    while (KKDH_deleteMin(f->buffer, &x, 0)) {
      produzierteFakten += 1;
      /* 	    IO_DruckeFlex("took %w from buffer\n",&(x.wid)); */
      FA_CPinCacheOrHeap(f,x);
    }
	
    if (produzierteFakten == 0) {
      KKDH_deleteElement(f->heap,oe->observer,&x);
      oe->observer = OBS_IDX_UNDEF;
    }
  } else {
    /* XXX Eine Reexpansion die zu keinem kritischen Paar führte und von
     * einem anderen Fass angestoßen wurde. Eigentlich könnte ich jetzt
     * dieses Aset löschen, allerdings weiß ich nicht welches.
     */
  }
}



static void selectMinFromHeap(KKDHeapT h, KElementT* x)
{
  if (!KKDH_getMin(h,x,0)) {
    KEL_SET_INFTY(x);	
  }    
}

static BOOLEAN selectMinimum(FassT f, WId* x)
{
  KElementT xcache,xheap,xir;
  KElementT* minimum;    
  KKDHeapT minHeap;
    
  selectMinFromHeap(f->irHeap,&xir);
 nach_expansion:    
  selectMinFromHeap(f->cache,&xcache);
  selectMinFromHeap(f->heap,&xheap);

  /*     IO_DruckeFlex("sM: cache(%d):%w ir:%w heap:%w",KKDH_size(f->cache),&(xcache.wid),&(xir.wid),&(xheap.wid)); */

  if (KEL_GREATER(0,&xheap,&xcache)) {
    if (KEL_GREATER(0,&xir,&xcache)) {
      minHeap = f->cache;	    
      minimum = &xcache;
      /* 	    IO_DruckeFlex(" ... selected cache\n"); */
    } else {
      minHeap = f->irHeap;	    
      minimum = &xir;	    
      /* 	    IO_DruckeFlex(" ... selected ir\n"); */
    }
  } else if (KEL_GREATER(0,&xir,&xheap)) {
    /* 	    minHeap = f->heap;	     */
    /* 	    minimum = &xheap; */
    /* 	    IO_DruckeFlex(" ... selected heap\n"); */

    /* 	    IO_DruckeFlex("Reexpansion von %w bei cachesize=%d... \n",&xheap.wid,KKDH_size(f->cache)); */
	    
    TA_Expandiere(WID_id(xheap.wid));
    fa_beendeReexpand(f,WID_i(xheap.wid));
    /* 	    IO_DruckeFlex("... beendet bei cachesize=%d\n",KKDH_size(f->cache)); */
    goto nach_expansion;	    
  } else {
    minHeap = f->irHeap;	    
    minimum = &xir;	    
    /* 	    IO_DruckeFlex(" ... selected ir\n"); */
  }

  if (KEL_IS_INFTY(0,minimum)) {
    return FALSE;
  } else {
    KKDH_deleteMin(minHeap,&xcache,0);
    *x = minimum->wid;
  }
  /*     IO_DruckeFlex("### selected %w\n",&minimum->wid); */
  return TRUE;
}



/* Wählt so lange Elemente aus, bis es eine nicht Waise gefunden hat,
 * oder alle Elemente aufgebraucht und die Heaps leer sind.
 */
static BOOLEAN selectNonOrphan(FassT f, WId* x) {    
  BOOLEAN result;

  do {
    result = selectMinimum(f, x);
  } while (result && !WID_isIR(*x) && fa_IdIstWaise(WID_id(*x)));
  return result;
}



static BOOLEAN FA_Selektiere(ZapfhahnT z, SelectRecT* selRec) 
{
  WId x;
  WId udef = {0, {0,0,0}};
  BOOLEAN tryAgain;
  FassT f = (FassT)z->privat;


  /*     IO_DruckeFlex("### Zapfhahn(%s,%d)\n",f->type == CP ? "CP" : "CG",f->classify); */
    
  /*     IO_DruckeFlex("###1 Selektiere Fass %s\n",f->type == CP ? "CP" : "CG");     */
  /*     printf("FA_Selektiere cache:%u ir:%u heap:%u buffer:%u\n", */
  /* 	   KKDH_size(f->cache),KKDH_size(f->irHeap),KKDH_size(f->heap),KKDH_size(f->buffer));     */
  FA_leereBuffer(f);

  /*     IO_DruckeFlex("Selektiere von %s\n",f->type == CP ? "CP" : "CG"); */
    

#if 1
  selRec->lhs = NULL;
  selRec->rhs = NULL;
  selRec->actualParent = NULL;
  selRec->otherParent = NULL;
  selRec->tp = NULL;
  selRec->result = 0;
  selRec->wasFIFO = FALSE;
  selRec->istGZFB = FALSE;
  selRec->wid = udef;
#endif    
  do {
    tryAgain = FALSE;
    /* 	IO_DruckeFlex("actual cachesize 1 = %d\n",KKDH_size(f->cache)); */

    if (PA_Waisenmord() ? selectNonOrphan(f, &x) : selectMinimum(f, &x)) 
      {
	/* 	    IO_DruckeFlex("actual cachesize 2 = %d\n",KKDH_size(f->cache)); */
	if (!ZPA_BereitsSelektiertWId( &x ) ) {
	  ObserverEntry* oe;
	  KPV_reconstruct(&x, selRec, TRUE);

	  oe = getNewLookupEntryAddress(f->setObserver,WID_i(x));	
	  if (WID_isIR(x)) {
	    oe->ir_handled = 1;
	  } else {
	    WID_MAXIMIZE(oe->klein_w,x);
	  }    

	  /* 		IO_DruckeFlex("Selektion %s lieferte %w\n",f->type == CP ? "CP" : "CG", &x); */
	  return TRUE;		
	} else {
	  tryAgain = TRUE;
	}
      }
  } while (tryAgain);
  /*     IO_DruckeFlex("Selektion lieferte nichts\n"); */
  return FALSE;
}

static BOOLEAN FA_BereitsSelektiertWId(ZapfhahnT z, WId* id) 
{
  FassT f = (FassT)z->privat;    
  ObserverEntry* oe;    
    

  if (!fa_IdPasstZuFasstyp(f,WID_id(*id))) {
    return FALSE;
  }    

  oe = getLookupEntryAddress(f->setObserver,WID_i(*id));
  if (oe == NULL) {
    return FALSE;	
  } else {
    if (WID_isIR(*id)) {
      return oe->ir_handled;
    } else {
      return WID_GREATER(oe->klein_w,*id);    
    }
  }
}

static BOOLEAN FA_BereitsSelektiert(ZapfhahnT z, IdMitTermenT tid) 
{
  FassT f = (FassT)z->privat;    
  ObserverEntry* oe;
  WId id;

  if (!fa_IdPasstZuFasstyp(f,tid->id)) {
    return FALSE;
  }    

  WID_w(id) = f->classify(tid);
  WID_id(id) = tid->id;    
    
  oe = getLookupEntryAddress(f->setObserver,WID_i(id));    
  if (oe == NULL) {
    return FALSE;	
  } else {
    if (WID_isIR(id)) {
      return oe->ir_handled;
    } else {
      return WID_GREATER(oe->klein_w,id);    
    }
  }
}


static void FA_FreigebenZ(ZapfhahnT z)
{
  if (z == 0) {}; /* nothing to do */
}



static void FA_FaktenEinfuegen(TankstutzenT t, TermT l, TermT r, Id id )
{
  TermT dummy_s, dummy_t;
  FassT f = (FassT) t->privat;
  KElementT x;

  /*     IO_DruckeFlex("FA_FaktenEinfuegen %w\n", &(x.wid)); */

  if (KKDH_getMin(f->buffer,&x,0) && (ID_i(id) != WID_i(x.wid))) {
    FA_leereBuffer(f);
  }
  
  if (Z_UngleichungEntmantelt(&dummy_s,&dummy_t,l,r) == (f->type == CG)
      && !fa_IdIstWaise(id)) {
    KElementT x;
	    
    IdMitTermenS tid = {l,r,id,f->type == CP};
    WID_w(x.wid) = f->classify(&tid);
    WID_id(x.wid) = id;
    x.observer = NULL;

    if(IO_level4())    IO_DruckeFlex("%s %w %t # %t\n", (f->type == CG) ? "CG" : "CP", &(x.wid), l, r);
      
    KKDH_insert(f->buffer, x);
  }
}


static void FA_IROpferEinfuegen(TankstutzenT t, IdMitTermenT tid )
{
  KElementT x;
  FassT f = (FassT) t->privat;

  if (tid->isCP == (f->type == CP)) {
    WID_w(x.wid) = ((FassT)t->privat)->classify(tid);
    WID_id(x.wid) = tid->id;
    x.observer = NULL;
    KKDH_insert(f->irHeap,x);

    /* XXX Waisenmordbehandlung einfügen ??? */
  }
}

static void FA_FreigebenTS(TankstutzenT t)
{
  if (t == 0) {}; /* nothing to do */
}


TankstutzenT FA_mkTankstutzen( FassT fass )
{
  TankstutzenT t = our_alloc( sizeof(struct TankstutzenS) );
  t->faktenEinfuegen = FA_FaktenEinfuegen;
  t->iROpferEinfuegen = FA_IROpferEinfuegen;
  t->freigeben = FA_FreigebenTS;
  t->privat = fass;
  return t;
}


ZapfhahnT FA_mkZapfhahn( FassT fass )
{
  ZapfhahnT z = our_alloc(sizeof(struct ZapfhahnS));
  z->selektiere = FA_Selektiere;
  z->bereitsSelektiert = FA_BereitsSelektiert;
  z->bereitsSelektiertWId = FA_BereitsSelektiertWId;
  z->freigeben = FA_FreigebenZ;
  z->privat = fass;
  return z;
}


FassT FA_mkFass( HeapType type, ClassifyProcT classify )
{
  FassT fass = (FassT) our_alloc( sizeof( struct FassS ) );
  fass->type = type;
  fass->cacheSize = PA_cacheSize();
  fass->classify = classify;

  fass->heap = KKDH_mkHeap(10, 1);
  fass->irHeap = KKDH_mkHeap(10, 1);
  fass->cache = KKDH_mkHeap(fass->cacheSize, 2);
  fass->buffer = KKDH_mkHeap(10, 1);

  fass->setObserver = our_alloc(sizeof(ObserverArrayS));
  fass->setObserver->observerArray = NULL;
  fass->setObserver->capacity = 0;

  SetKPV_AnzCritGoalsActual(0); 

  return fass;
};



void FA_changeClass(FassT fass, ClassifyProcT classifcation)
{
  fass->classify = classifcation;
}

