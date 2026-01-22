/* NKDHeap.c: Mehrdimensionale Prioritaetswarteschlangen
 * 
 * Bernd Loechner, 12.10.01
 * Hendrik Spies, Jean-Marie Gaillourdet 25.3.2003
 *
 * Z.T. etwas haesslich wg. effektiv drei Fassungen fuer k == 1,
 * kleines k und grosses k, sowie Elimination von Div/Modulo-Operationen
 * und Aufrufen von level(...) oder block(...). 
 * Dadurch aber Leistungssteigerungen um den Faktor 2 - 5 !!!
 *
 * Literatur: 
 *   Ding, Weiss: The K-D Heap: An Efficient Multi-dimensional
 *   Priority Queue, in Dehne et.al. (Eds): Algorithms and Data
 *   Structures WADS '93, Springer Verlag, LNCS 709, pp. 302-313, 1993.
 *
 *    Messungen (100 * (1000;100;100))
 *
 *     1    0.050u    0.160u
 *     2    0.110u    0.200u
 *     3    0.110u    0.250u
 *     4    0.160u    0.360u
 *     5    0.210u    0.440u
 *     6    0.320u    0.530u
 *     7    0.510u    0.610u
 *     8    0.900u    0.700u
 *     9    1.490u    0.790u
 *    10    2.600u    0.900u
 *    
 *    Messungen (1000 * (1000;30;30))
 *    
 *     1    0.420u    1.500u
 *     2    0.870u    1.980u
 *     3    0.940u    2.370u
 *     4    1.140u    3.360u
 *     5    1.370u    3.880u
 *     6    1.790u    4.440u
 *     7    2.610u    4.940u
 *     8    4.130u    5.570u
 *     9    6.870u    6.120u
 *    10   12.420u    6.740u
 *
 *    Messungen (1000 * (1000;30;30));Del*
 *    
 *     1    2.890u    5.540u
 *     2    5.670u    5.100u
 *     3    6.680u    5.570u
 *     4    8.100u    7.180u
 *     5   10.150u    8.180u
 *     6   14.720u    9.020u
 *     7   18.630u   11.200u
 *     8   33.470u   12.440u
 *     9   59.410u   13.630u
 *    10   72.970u   15.180u
 */

#include "NKDHeap.h"
#include <math.h>

#define TESTHEAP 0  /* 0, 1, 2 */
#define MAGIC_K 8

#if UNITTEST
#define KAH 2
#define PRINT 0
#include <stdlib.h>
#include <stdio.h>
#define our_alloc(n) malloc(n)
#define our_realloc(x,n,m) realloc(x,n)
#define our_dealloc(x) free(x)
static void our_error(char *s)
{
  printf("ERROR: %s\n", s);
  exit(EXIT_FAILURE);
}
#else
#include "SpeicherVerwaltung.h"
#include "FehlerBehandlung.h"
#endif

#define ENLARGEMENT(x) (((x) < 128 * 1024) ? ((x)+(x)) : ((x)+(x)/4))

#define GREATER(h,d,x,y) (h->fcts.greater(d,(x)->data,(y)->data,(h)->fcts.greaterContext))

/* Es macht zeitlich ganz schoen was aus, welche Variante von GREATER
 * verwendet wird: Zum Bsp. 8.68u 0.01s vs 22.31u 0.02s. Das sollte
 * man also ganz genau entscheiden.  
 * Durch die Änderung der Signatur zu ElementT Pointern wird der Zeitverlust 
 * weitegehend vermieden. 
 * z.B.: ROB006-1.pr -auto
 * mit Macros:                          37.250s
 * mit Funktionspointer:                50.450s
 * mit Funktionspointern und ElementT*: 38.970s
 * JMG 17.1.2003
 */

#if TESTHEAP >= 1
#include <assert.h>
#else
#define assert(x) /* nix tun */
#endif
#if TESTHEAP == 2
#define HEAPTEST(h) {checkHeap(h); /*printHeap(h);*/}
#else
#define HEAPTEST(h) /* nix tun */
#endif

#define DONT_CARE 0

typedef struct InternalElementS {
  void* data;
  unsigned long* observer;
} InternalElementT;

struct NKDHeapS {
  InternalElementT *xs;
  unsigned long e;
  /* e gibt die Zahl der allokierten Element an. D.h unabhaengig von der
   * Variante sind die Array-Elemente xs[0]...xs[e-1] allokiert.
   */
  unsigned long i;
  /* i gibt die Zahl der Elemente an. Fuer k > MAGIC_K liegen die Elemente
   * in xs[0]...xs[i-1], ansonsten in xs[1]...xs[i].
   */
  unsigned k; /* die Dimension des Heaps. */
  unsigned level_i; /* Invariante: level_i == level(i) */
  NKDH_FunctionsT fcts;
};

/* ------------------------------------------------------------------------- */
/* ------------------------------------------------------------------------- */

/* i gibt die Zahl der Elemente an. Fuer k > MAGIC_K liegen die Elemente
 * in xs[0]...xs[i-1], ansonsten in xs[1]...xs[i]. Dazu gibt es die beiden
 * folgenden Funktionen: die Elemente liegen in xs[firstIndex]..xs[lastIndex]
 */

static unsigned int firstIndex(NKDHeapT h)
{
  return (h->k > MAGIC_K) ? 0 : 1;
}
static unsigned int lastIndex(NKDHeapT h)
{
  return (h->k > MAGIC_K) ? h->i - 1 : h->i;
}

static void setObserver(InternalElementT *x, unsigned long index)
{
  if (x->observer != NULL){
    *(x->observer) = index;
  }
}

static void swap(InternalElementT *x, InternalElementT *y)
{
  InternalElementT t = *x;
  *x = *y;
  *y = t;
  if ((x->observer != NULL) && (y->observer != NULL)){
    unsigned long t = *(x->observer);
    *(x->observer) = *(y->observer);
    *(y->observer) = t;
  }
}

static unsigned level(unsigned long n)
/* fuer n > 0 ist level(n) = floor(log_2(n)), level(0) = 0 */
/* liese sich zur Not mit binaerer Suche machen */
{
  unsigned l = 1;
  while (n >= (1U << l)){
    l++;
  }
  assert ( ((n == 0) && ((l - 1) == 0)) || 
	   ((n > 0) && ((1U << (l-1)) <= n) && (n < (1U <<l))) );
  return l - 1;
}

static unsigned dmodk(unsigned d, unsigned k)
/* Berechnet d % k fuer 0 <= d < 2k, nur deutlich effizienter als d % k */
{
  unsigned res;
  assert ((0 <= d) && (d < 2 * k));
  res = (d >= k) ? d - k : d;
  assert (res == d % k);
  return res;
}

static void decreaseI(NKDHeapT h)
{
  assert( h->level_i == level(h->i) );
  h->i--;    
  if (h->i == 0){
    h->level_i = 0;
  }
  else if ((h->i & (h->i + 1)) == 0){
    h->level_i--;
  }
  assert( h->level_i == level(h->i) );
}

static void extendHeap(NKDHeapT h)
/* Vergroessert den Heap */
{
  h->e  = ENLARGEMENT(h->e);
  h->xs = our_realloc(h->xs, h->e * sizeof(InternalElementT), "Extending NKDHeap");
}

#if TESTHEAP == 2
static void printHeap(NKDHeapT h)
{
  unsigned long i;
  printf("Heap: i = %ld, e = %ld, k = %d, level_i = %d\n", 
         h->i, h->e, h->k, h->level_i);
  for (i = 1; i < h->e; i++){
    printf("heap[%ld] = ", i);
    PRINT_ELEMENT(h->xs[i]);
    printf("\n");
  }
}

static void checkNode(NKDHeapT h, unsigned long index)
/* Checkt die Heap-Bedingung fuer ein Element 
 * fuer ein- und wenigdimensionalen Heap (1 <= k <= MAGIC_K) */
{
  unsigned level_diff = 1;
  unsigned d = level(index) % h->k;
  if ((h->xs[index].observer != NULL) &&
      (*(h->xs[index].observer) != index - 1)){
    printf("observer value = %ld, should be = %ld\n", 
	   *(h->xs[index].observer), index - 1);
    our_error("checkNode: Heap property violated!");
  }
  for(;;){
    unsigned long delta;
    for (delta = 0; delta < (1U << level_diff); delta++){
      unsigned long j = (index << level_diff) + delta;
      if (j > h->i) {
        return;
      }
      /*      printf("checking %ld %ld\n", index, j);*/
      if (GREATER(h,d,&(h->xs[index]),&(h->xs[j]))){
        printf("%ld > %ld\n", index, j);
        our_error("checkNode: Heap property violated!");
      }
    }
    level_diff++;
  }
}

static void checkNode2(NKDHeapT h, unsigned long index)
/* Checkt die Heap-Bedingung fuer ein Element 
 * fuer vieldimensionalen Heap (k > MAGIC_K) */
{
  our_error("Sigh: checkNode2: Not yet implemented!");
}

static void checkHeap(NKDHeapT h)
/* Checkt die Heap-Bedingung fuer alle Elemente */
{
  unsigned long i;
  if (h->k > MAGIC_K){
    for (i = 0; i < h->i; i++){
      checkNode2(h,i);
    }
  }
  else {
    if (level(h->i) != h->level_i){
      printf("level(h->i) = %d != %d = h->level_i\n", level(h->i), h->level_i);
      our_error("CheckHeap: Violated NKDHeap invariant!");
    }
    for (i = 1; i <= h->i; i++){
      checkNode(h,i);
    }
  }
}

#endif

static void find_min(NKDHeapT h, unsigned long index, 
                     unsigned d, unsigned levels,
                     unsigned long *min, unsigned *min_level)
/* Bestimmt das Minimum bzgl. Ordnung d im Subheap ab index in dessen obersten
 * levels+1 Ebenen .
 * min ist der Index des Minimums, min_level der level-Unterschied zwischen
 * index und min, also 0 <= min_level <= levels.
 * fuer wenigdimensionalen Heap (1 < k <= MAGIC_K) 
 */
{
  unsigned long delta, j;
  unsigned level_diff;

  *min = index; 
  *min_level = 0;
  for (level_diff = 1; level_diff <= levels; level_diff++){
    for (delta = 0; delta < (1U << level_diff); delta++){
      j = (index << level_diff) + delta;
      if (j > h->i) {
        return;
      }
      if (GREATER(h,d,&(h->xs[*min]),&(h->xs[j]))){
        *min = j; 
        *min_level = level_diff;
      }
    }
  }
}

static void normalize_up(NKDHeapT h, unsigned long index, 
                         unsigned level_diff, unsigned d)
/* fuer wenigdimensionalen Heap (1 < k <= MAGIC_K) */
{
  assert ((d == level(index >> level_diff) % h->k) || 
	  ((d == h->k) && (level(index >> level_diff) % h->k == 0)));
  while (level_diff > 0){
    d = dmodk(d,h->k);
    if (GREATER(h,d,&(h->xs[index >> level_diff]),&(h->xs[index]))){
      swap(&(h->xs[index >> level_diff]),&(h->xs[index]));
    }
    level_diff--;
    d++;
  }
}

static void normalize_down(NKDHeapT h, unsigned long index, 
                           unsigned level_index)
/* fuer wenigdimensionalen Heap (1 < k <= MAGIC_K) */
{
  unsigned long min;
  unsigned d, min_level;

  assert(level_index == level(index));
  d = (level_index < h->k) ? level_index       /* Haeufigen Fall optimieren */
                           : level_index % h->k; 
  find_min(h,index,d,h->k,&min,&min_level);
  while (min != index){
    swap(&(h->xs[min]),&(h->xs[index]));
    normalize_up(h, min, min_level - 1, d + 1);
    index = min;
    d = dmodk(d + min_level, h->k);
    find_min(h,index,d,h->k,&min,&min_level);
  }
}

/* sons (i) | (i+1)%k == 0  = {2(i+1)-k,2(i+1)}
            | otherwise     = {i+1}
*/ 
static unsigned long block(NKDHeapT h, unsigned long i)
{
  return i / h->k + 1;
}

static unsigned slot (NKDHeapT h, unsigned long i)
{
  return i % h->k;
}

static void find_min2(NKDHeapT h, 
		      unsigned long index, unsigned s, 
		      unsigned d, unsigned levels,
                      unsigned long *min, unsigned *smin)
/* Bestimmt das Minimum bzgl. Ordnung d im Subheap ab index in dessen obersten
 * levels+1 Ebenen .
 * min ist der Index des Minimums.
 * fuer vieldimensionalen Heap (k > MAGIC_K) 
 */
{
  unsigned long j, j1;
  unsigned s1,s1max,s2limit;

  assert(s == slot(h,index));
  assert(levels <= h->k);

  *min = index; *smin = s;
  s1max = (s + levels >= h->k) ? h->k - 1 : s + levels;
  s2limit = s + levels - s1max;
  j1 = index - s + h->k;
  for (s1 = s+1, j = index + 1; s1 <= s1max; s1++, j++){
    if (j > h->i) {
      goto end;
    }
    if (GREATER(h,d,&(h->xs[*min]),&(h->xs[j]))){
      *min = j; *smin = s1;
    }
  }
  for (s1 = 0, j = 2*j1 - h->k; s1 < s2limit; s1++, j++){
    if (j > h->i) {
      goto end;
    }
    if (GREATER(h,d,&(h->xs[*min]),&(h->xs[j]))){
      *min = j; *smin = s1;
    }
  }
  for (s1 = 0, j = 2*j1; s1 < s2limit; s1++, j++){
    if (j > h->i) {
      goto end;
    }
    if (GREATER(h,d,&(h->xs[*min]),&(h->xs[j]))){
      *min = j; *smin = s1;
    }
  }
 end:
  assert(*smin == slot(h,*min));

}

static void normalize_up2(NKDHeapT h, 
                          unsigned long index, unsigned long bi,
                          unsigned long sub_root,unsigned sr,
			  unsigned long bldiff)
/* fuer vieldimensionalen Heap (k > MAGIC_K) */
{
  unsigned long j = sub_root;

  assert(sr == slot(h,sub_root));
  assert(bi == block(h,index));
  assert(bldiff == level(bi) - level(block(h,sub_root)));

  while (j < index){
    if (GREATER(h,sr,&(h->xs[j]),&(h->xs[index]))){
      swap(&(h->xs[j]),&(h->xs[index]));
    }
    sr++;
    j++;
    if (sr == h->k){
      bldiff--;
      sr = 0;
      j = ((bi >> bldiff) & 1) ? 2*j : 2*j - h->k;
    }
  }
}

static void normalize_up2Speziale(NKDHeapT h, 
				  unsigned long index, unsigned si,
				  unsigned long sub_root, unsigned sr)
/* fuer vieldimensionalen Heap (k > MAGIC_K) */
{
  unsigned long j = sub_root;

  assert(sr == slot(h,sub_root));
  assert(si == slot(h,index));

  for (;;){
    j++;
    sr++;
    if (sr == h->k){
      j = index - si;
    }
    if (j == index){
      return;
    }
    if (GREATER(h,sr,&(h->xs[j]),&(h->xs[index]))){
      swap(&(h->xs[j]),&(h->xs[index]));
    }
  }
}


static void normalize_down2(NKDHeapT h, unsigned long index, unsigned sindex)
/* fuer vieldimensionalen Heap (k > MAGIC_K) */
{
  unsigned long min;
  unsigned smin;

  for (;;){
    assert (sindex == slot(h,index));
    find_min2(h,index,sindex, sindex,h->k, &min,&smin);
    if (min == index){
      return;
    }
    swap(&(h->xs[index]),&(h->xs[min]));
    normalize_up2Speziale(h,min,smin,index,sindex); 
    index = min; sindex = smin;
  }
}

static void upheap(NKDHeapT h, unsigned long hole, InternalElementT x)
/* Schiebt solange das Loch nach oben, bis das Element x so passt, dass
 * die Heap-Bedingung erfuellt ist.  Nur fuer eindimensionalen Heap. 
 */
{
  assert((1 <= hole) && (hole <= h->i));
  while ((hole > 1) && GREATER(h,0,&(h->xs[hole/2]),&x)){
    h->xs[hole] = h->xs[hole/2];
    setObserver(&(h->xs[hole]),hole - 1);
    hole = hole/2;
  }
  h->xs[hole] = x;
  setObserver(&(h->xs[hole]),hole - 1);
}

static void downheap(NKDHeapT h, unsigned long hole, InternalElementT x)
/* Schiebt solange das Loch nach unten, bis das Element x so passt, dass
 * die Heap-Bedingung erfuellt ist.  Nur fuer eindimensionalen Heap.
 */
{
  unsigned long child = 2 * hole;

  assert((1 <= hole) && (hole <= h->i));
  while (child <= h->i){/* es gibt mind. ein Kind */
    if (child < h->i){  /* es gibt zwei Kinder */
      if (GREATER(h,0,&(h->xs[child]), &(h->xs[child+1]))){
        child++;
      }
    }
    if (GREATER(h,0,&x,&(h->xs[child]))){ /* falls groesser als kleineres Kind */
      h->xs[hole] = h->xs[child]; /* Loch nach unten schieben */
      setObserver(&(h->xs[hole]),hole - 1);
      hole = child;
      child = 2 * hole;
    }
    else {
      break; /* Schleife abbrechen!! */
    }
  }
  h->xs[hole] = x;
  setObserver(&(h->xs[hole]),hole - 1);
}

static void deleteElement(NKDHeapT h, void** res, unsigned long** observer,
			  unsigned long index, unsigned level_index,
			  unsigned long bindex, unsigned long lbindex)
{
  assert(!((1 < h->k) && (h->k < MAGIC_K)) || (level_index == level(index)));
  *res = h->xs[index].data;
  if (observer != NULL) {
    *observer = h->xs[index].observer;
  }
  decreaseI(h);
  if (h->i > 0){ /* Heap nicht leer -> restrukturieren */
    if (index > lastIndex(h)){/* letztes Element geloescht */
      /* Nix tun*/
    }
    else {
      if (h->k > MAGIC_K){           /* vieldimensionaler Heap */
	assert(bindex == block(h,index));
	assert(lbindex == level(bindex));
	assert(level_index == slot(h,index));
	h->xs[index] = h->xs[h->i];
	setObserver(&(h->xs[index]),index);
	normalize_up2(h,index,bindex,0,0,lbindex);
	normalize_down2(h,index,level_index);    /* level_index ist hier eigentlich slot */
      }
      else if (h->k == 1){            /* eindimensionaler Heap */
	if ((index > 1) && (GREATER(h,0,&(h->xs[index/2]), &(h->xs[h->i+1])))){
	  upheap(h,index,h->xs[h->i+1]);
	}
	else {
	  downheap(h,index,h->xs[h->i+1]);
	}
      }
      else {                          /* wenigdimensionaler Heap */
	h->xs[index] = h->xs[h->i+1];
	setObserver(&(h->xs[index]),index - 1);
	normalize_up(h,index,level_index,0);
	normalize_down(h,index,level_index);    
      }
    }
  }
}

static void ReorgHeap(NKDHeapT h)
{
  unsigned long i;
  if (h->k > MAGIC_K){
    for (i = h->i; i >= 1; i--){
      normalize_down2(h,i,slot(h,i));
    }
  }
  else if (h->k == 1){
    for (i = h->i/2; i >= 1; i--){
      downheap(h,i,h->xs[i]);
    }
  }
  else {
    for (i = h->i/2; i >= 1; i--){
      normalize_down(h,i,level(i));
    }
  }
}

/* ------------------------------------------------------------------------- */
/* ------------------------------------------------------------------------- */

NKDHeapT NKDH_mkHeap(unsigned long initial, unsigned k, NKDH_FunctionsT fcts)
/* Erzeugt Verwaltungsblock und allokiert Raum fuer Elemente */
{
  NKDHeapT res = our_alloc(sizeof(*res));
  if (!((1 <= k) && (k <= 50))){
    our_error("NKDH_mkHeap: k out of sensible bounds!");
  }
  res->k       = k;
  res->i       = 0;
  res->level_i = 0;
  res->fcts    = fcts;
  res->e  = (initial > 16) ? initial : 16;
  res->xs = our_alloc(res->e * sizeof(InternalElementT));

  if (k > MAGIC_K){
    our_error("NKDH: Implementierung fuer grosse k fehlerhaft (level_i != level(i)) -- wo wird es benutzt? wo muss es gepflegt werden?");
  }

  HEAPTEST(res);
  return res;
}

void NKDH_delHeap(NKDHeapT h)
/* Loeschen des Elemente-Arrays und des Verwaltungsblockes. */
/* Sind die Elemente Zeiger, werden die bezeigten Objekte nicht geloescht. */
{
  our_dealloc(h->xs);
  our_dealloc(h);
}

void NKDH_clear(NKDHeapT h)
/* loescht alle Elemente in h, allerdings werden Dinge, auf die Elemente
 * zeigen, nicht freigegeben.
 */
{
  HEAPTEST(h);
  h->i = 0;
  h->level_i = 0;
  HEAPTEST(h);
}

BOOLEAN NKDH_isEmpty(NKDHeapT h)
{
  return h->i == 0;
}

unsigned long NKDH_size(NKDHeapT h)
{
  return h->i;
}

void NKDH_insert (NKDHeapT h, void* x, unsigned long* observer)
{
  HEAPTEST(h);
  if (h->k > MAGIC_K){ /* vieldimensionaler Heap */
    unsigned long bi;
    if (h->i == h->e){
      extendHeap(h);
    }
    h->xs[h->i].data = x;
    h->xs[h->i].observer = observer;
    setObserver(&(h->xs[h->i]),h->i);
    bi = block(h,h->i); 
    /* Optimierungspotenzial: bi, level(bi) mitziehen, wie mit level_i */
    normalize_up2(h,h->i,bi,0,0,level(bi));
    h->i++;
  }
  else { /* ein- und wenigdimensionaler Heap */
    if (h->i == 0){ /* Spezialfall insb. wg. level_i gesondert behandeln. */
      h->i = 1;
      h->level_i = 0;
      h->xs[1].data = x;
      h->xs[1].observer = observer;
      setObserver(&(h->xs[1]), 1 - 1);
    }
    else {
      h->i++;
      if ((h->i & (h->i - 1)) == 0){
        h->level_i++;
      }
      if (h->i == h->e){
        extendHeap(h);
      }
      if (h->k == 1){                 /* eindimensionaler Heap */
	InternalElementT newInternalElement = {x,observer};
        upheap(h,h->i,newInternalElement);
      }
      else {                          /* wenigdimensionaler Heap */
        h->xs[h->i].data = x;
        h->xs[h->i].observer = observer;
	setObserver(&(h->xs[h->i]),h->i - 1);
	normalize_up(h,h->i,h->level_i,0);
      }
    }
  }
  HEAPTEST(h);
}

BOOLEAN NKDH_getMin(NKDHeapT h, void** res, unsigned long** observer, unsigned d)
{
  unsigned long min;
  unsigned min_level;
  BOOLEAN retVal = FALSE;

  HEAPTEST(h);
  if (!(1 <= h->i)){
    our_error("NKDH_getMin: getmin on empty heap!");
  }
  if (d >= h->k){
    our_error("NKDH_getMin: Given dimension to large!");
  }
  /* in den obersten d+1 Ebenen suchen */
  if (h->k > MAGIC_K){
    find_min2(h,0,0,d,d,&min,&min_level);
  }
  else {
    find_min(h,1,d,d,&min,&min_level);
  }
  /*U*//*printf("nkd702");*/
  if (!(h->fcts.isInfty(d,h->xs[min].data))) {
    *res = h->xs[min].data;
    /*printf("                   get min %u                the datapointer is set to %p\n", d, h->xs[min].data);*/
    if (observer != NULL) {
      /*printf("                   get min %u                the observerpointer is set to %p\n", d, (h->xs[min].observer));*/
      if (h->xs[min].observer != NULL) {
	/*printf("                   get min %u                the observervalue is %lu\n", d, *(h->xs[min].observer));*/
      }
      *observer = h->xs[min].observer;
    } else {
      /*      printf("                   get min %u                the observer is not set", d);*/
    }
    retVal = TRUE;
  }
  HEAPTEST(h);
  return retVal;
}

BOOLEAN NKDH_deleteMin(NKDHeapT h, void** res, unsigned long** observer, unsigned d)
{
  unsigned long min;
  unsigned min_level;
  BOOLEAN retVal = FALSE;

  HEAPTEST(h);
  if (!(1 <= h->i)){
    our_error("NKDH_deleteMin: Delete on empty heap!");
  }
  if (d >= h->k){
    our_error("NKDH_deleteMin: Given dimension to large!");
  }
  /* in den obersten d+1 Ebenen suchen */
  if (h->k > MAGIC_K){
    find_min2(h,0,0,d,d,&min,&min_level);
  }
  else {
    find_min(h,1,d,d,&min,&min_level);
  }
  /*U*//*printf("nkd741");*/
  if (!(h->fcts.isInfty(d,h->xs[min].data))) {
    deleteElement(h,res,observer,min,min_level,1,0);
    retVal = TRUE;
  }
  HEAPTEST(h);
  return retVal;
}

void NKDH_deleteElement(NKDHeapT h, unsigned long index, void** res, unsigned long** observer)
{
  HEAPTEST(h);
  if (!(index < h->i)){
    printf("NKDH_deleteElement: %lu cannot be deleted, as not in heap (size = %lu)\n", index, NKDH_size(h));
    our_error("NKDH_deleteElement: Delete element with index out of range!");
  }
  if (h->k > MAGIC_K){
    unsigned long bindex;
    bindex = block(h,index);
    deleteElement(h,res,observer,index, slot(h,index), bindex, level(bindex));
  }
  else {
    index++; /* Abbildung: Elementnummer -> Elementindex */
    deleteElement(h,res,observer,index,(h->k > 1) ? level(index) : DONT_CARE, 
		  DONT_CARE, DONT_CARE);
  }
  HEAPTEST(h);
}

void* NKDH_getElement(NKDHeapT h, unsigned long index)
/* Gibt einen Zeiger auf das durch index spezifizierte Element. Wird das 
 * Gewicht des Elementes von aussen veraendert, muss danach 
 * NKDH_heapifyElement oder NKDH_heapify aufgerufen werden.
 */
{
  HEAPTEST(h);
  if (!(index < h->i)){
    our_error("NKDH_getElement: Element with index out of range!");
  }
  if (h->k > MAGIC_K){
    /*if ((NULL != h->xs[index].observer) && (index != *(h->xs[index].observer))) {
      our_error("NKDH_getElement: index and observer differ");
    }*/
    return (h->xs[index].data);
  }
  else {
    /*if ((NULL != h->xs[index].observer) && (index != *(h->xs[index].observer))) {
      our_error("NKDH_getElement: index and observer differ");
      }*/
    return (h->xs[index+1].data); /* Abbildung: Elementnummer -> Elementindex */
  }
}

void NKDH_heapifyElement(NKDHeapT h, unsigned long index)
{
  if (!(index < h->i)){
    our_error("NKDH_heapifyElement: Heapify element with index out of range!");
  }
  if (h->k > MAGIC_K){           /* vieldimensionaler Heap */
    unsigned long bindex = block(h,index);
    normalize_up2(h,index,bindex,0,0,level(bindex));
    normalize_down2(h,index,slot(h,index));    
  }
  else {
    index++; /* Abbildung: Elementnummer -> Elementindex */
    if (h->k == 1){            /* eindimensionaler Heap */
      if ((index > 1) && (GREATER(h,0,&(h->xs[index/2]), &(h->xs[index])))){
	upheap(h,index,h->xs[index]);
      }
      else {
	downheap(h,index,h->xs[index]);
      }
    }
    else {                          /* wenigdimensionaler Heap */
      unsigned long level_index = level(index);
      normalize_up(h,index,level_index,0);
      normalize_down(h,index,level_index);    
    }
  }
  HEAPTEST(h);
}

void NKDH_heapify(NKDHeapT h)
{
  ReorgHeap(h);
  HEAPTEST(h);
}

void NKDH_Walkthrough(NKDHeapT h, NKDH_WalkthroughFct f)
{
  BOOLEAN reorg = FALSE;
  unsigned long index = firstIndex(h);
  
  HEAPTEST(h);
  while (index <= lastIndex(h)){
    switch (f(h->xs[index].data)){
    case NWTI_Delete:
      decreaseI(h);
      if (index <= lastIndex(h)){/* nicht letztes Element geloescht */
	h->xs[index] = h->xs[lastIndex(h) + 1]; 
	setObserver(&(h->xs[index]), (h->k > MAGIC_K) ? index : index - 1);
	/* letztes Element vorziehen. Wird als naechstes bearbeitet, 
	 * deswegen wird index nicht veraendert! */
	reorg = TRUE;
      }
      break;
    case NWTI_Reweight:
      reorg = TRUE;
      index++;
      break;
    case NWTI_Stop:
      reorg = TRUE;
      index = lastIndex(h)+1; /* Schleife sofort verlassen */
      break;
    case NWTI_Nothing:
    default:
      index++;
      break;
    }
  }
  if (reorg){
    ReorgHeap(h);
  }
  HEAPTEST(h);
}

/* ----------------------- SCREENDUMP FOR DEBUGGING PURPOSES --------------- */

void NKDH_ToScreen(NKDHeapT h, printElementFunc printE) {
  unsigned long index = firstIndex(h);
  unsigned long level = 1;
  unsigned long levelWidth = 1; /*invariant should be that levelWidth is 2^(level-1)*/
  unsigned long i;
  while (index <= lastIndex(h)) {
    printf("level %lu:\n", level);
    for (i = 0; (i < levelWidth) && (index <= lastIndex(h)); ++i) {
      if (NULL != h->xs[index].observer){
        printf("observer %lu :", *(h->xs[index].observer));
      }
      else {
        printf("observer <null> :");
      }
      printE(h->xs[index].data, TRUE);
      index++;
    }
    level++;
    levelWidth = levelWidth * 2;
  }
}
 
/* ------------------------------------------------------------------------- */
/* ------------------------------------------------------------------------- */

#if UNITTEST
static BOOLEAN greater(unsigned d, ElementT x, ElementT y)
{
  return EL_GREATER(d,x,y);
}

int main (int argn, char* argv[])
{
  NKDHeapT h;
  ElementT x;
  int i,j,k,n=0;
  unsigned dim;
  srand(0);
  dim = (argn == 2) ? atoi(argv[1]) : KAH;
  printf ("Dimension: %d\n", dim);
  h = NKDH_mkHeap(10000, dim, greater);
  printf("sizeof(ElementT) = %d\n", sizeof(ElementT));
  for(k = 0; k < 1000; k++){
    for (i = 0; i < 1000; i++){
      x.w1 = rand();
      x.w2 = n++;
      x.observer = our_alloc(sizeof(unsigned long));
    /*    printf("insert %d %d\n", i, x.w1);*/
      NKDH_insert(h, x);
    }
    for (i = 0; i < 30; i++){
      if (!NKDH_isEmpty(h)){
        j = rand() % NKDH_size(h) +1;
        NKDH_deleteElement(h,j,&x);
        if (PRINT){
          printf("delete at %d (%10ld,%10ld)\n", j, x.w1, x.w2);
        }
      }
    }
    for (i = 0; i < 30; i++){
      if (!NKDH_isEmpty(h)){
        /*U*//*printf("nkdh924 ");*/
	NKDH_deleteMin(h,&x, k % dim);
        if (PRINT){
          printf("Delete (%10ld,%10ld)\n",x.w1,x.w2);
        }
      }
    }
  }
#if 1
  while (!NKDH_isEmpty(h)){
    j = rand() % NKDH_size(h) +1;
    /*NKDH_deleteElement(h,j,&x);*/
    /*U*//*printf("nkdh936 ");*/
    NKDH_deleteMin(h,&x, k % KAH);
    /*      printf("delete at %d (%5d,%5d)\n", j, x.w1, x.w2);*/
  }
#endif
  NKDH_delHeap(h);
  printf("finis\n");
  return EXIT_SUCCESS;
}

#if 0
/opt/utils/shadekit/shade.v9.elf/bin/spixcounts -c NKDHeap
/opt/utils/shadekit/spixtools.v9.elf/bin/sprint -b NKDHeap.1.bb NKDHeap NKDHeap.c | p
#endif

#endif
