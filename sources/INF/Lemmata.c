/* ------------------------------------------------------------------------- */
/* -- Lemmata.c ------------------------------------------------------------ */
/* ------------------------------------------------------------------------- */

#include "Lemmata.h"

#include "Ausgaben.h"
#include "FehlerBehandlung.h"
#include "Parameter.h"
#include "SpeicherVerwaltung.h"
#include "SymbolOperationen.h"
#include "TermOperationen.h"

#if 1
#define NDEBUG 1
#endif
#include <assert.h>

/* ------------------------------------------------------------------------- */
/* -- local types ---------------------------------------------------------- */
/* ------------------------------------------------------------------------- */

typedef struct {
  TermT lhs;
  TermT rhs;
  VergleichsT vgl;
  BOOLEAN gzfb;
} le_BufT;

typedef struct {
  LemmaT typ;
  SymbolT f;
} le_InputT;

/* ------------------------------------------------------------------------- */
/* -- static variables ----------------------------------------------------- */
/* ------------------------------------------------------------------------- */

static le_InputT *le_InputQueue;
static unsigned le_QueuePending;
static unsigned le_QueueSize;

static le_BufT *le_Buf;
static unsigned le_current;
static unsigned le_occupied;
static unsigned le_size;

/* ------------------------------------------------------------------------- */
/* -- input queue handling ------------------------------------------------- */
/* ------------------------------------------------------------------------- */

static void le_pushInput(LemmaT typ, SymbolT f)
{
  if (le_QueueSize == le_QueuePending){
    le_QueueSize += 10;
    le_InputQueue = our_realloc(le_InputQueue, 
                                le_QueueSize * sizeof(*le_InputQueue), 
                                "le_InputQueue");
  }
  le_InputQueue[le_QueuePending].typ = typ;
  le_InputQueue[le_QueuePending].f   = f;
  le_QueuePending++;
}

static BOOLEAN le_InputQueueEmpty(void)
{
  return le_QueuePending == 0;
}

static void le_moveInputsOneDown(void)
{
  unsigned i;
  for (i = 0; i < le_QueuePending; i++){
    le_InputQueue[i] = le_InputQueue[i+1];
  }
}

static void le_popInput(LemmaT *typ, SymbolT *f)
{
  assert(!le_InputQueueEmpty());
  *typ = le_InputQueue[0].typ;
  *f   = le_InputQueue[0].f;
  le_QueuePending--;
  le_moveInputsOneDown();
}

static void le_clearInputQueue(void)
{
  le_QueuePending = 0;
}

static void le_freeInputQueue(void)
{
  le_clearInputQueue();
  le_QueueSize = 0;
  our_dealloc(le_InputQueue);
}

/* ------------------------------------------------------------------------- */
/* -- lemma buf handling --------------------------------------------------- */
/* ------------------------------------------------------------------------- */

static void le_pushLemma(TermT lhs, TermT rhs, VergleichsT vgl, BOOLEAN gzfb)
{
  if (le_occupied == le_size){
    le_size += 500;
    le_Buf = our_realloc(le_Buf, le_size * sizeof(*le_Buf), "le_Buf");
  }
  le_Buf[le_occupied].lhs  = lhs;
  le_Buf[le_occupied].rhs  = rhs;
  le_Buf[le_occupied].vgl  = vgl;
  le_Buf[le_occupied].gzfb = gzfb;
  le_occupied++;
}

static BOOLEAN le_BufEmpty(void)
{
  return le_occupied == 0;
}

static void le_popLemma(TermT *lhs, TermT *rhs, 
                        VergleichsT *vgl, BOOLEAN *gzfb)
{
  assert(!le_BufEmpty());
  *lhs  = le_Buf[le_current].lhs;
  *rhs  = le_Buf[le_current].rhs;
  *vgl  = le_Buf[le_current].vgl;
  *gzfb = le_Buf[le_current].gzfb;
  le_current++;
  if (le_current == le_occupied){
    le_current  = 0;
    le_occupied = 0; 
  }
}  

static void le_freeEntry(unsigned i)
{
  TO_TermeLoeschen(le_Buf[i].lhs, le_Buf[i].rhs);
}

static void le_clearBuf(void)
{
  unsigned i;
  for (i = le_current; i < le_occupied; i++){
    le_freeEntry(i);
  }
  le_current  = 0;
  le_occupied = 0;
}

static void le_freeBuf(void)
{
  le_clearBuf();
  le_size = 0;
  our_dealloc(le_Buf);
}

/* ------------------------------------------------------------------------- */
/* -- the lemmata we know -------------------------------------------------- */
/* ------------------------------------------------------------------------- */

static void le_fillIdempotenzLemmata(SymbolT f)
{
  if (SO_Stelligkeit(f) != 2){
    our_error("wrong symbol for lemma family");
  }
  {
    SymbolT x1 = SO_ErstesVarSymb;
    SymbolT x2 = SO_NaechstesVarSymb(x1);
    SymbolT x3 = SO_NaechstesVarSymb(x2);
    SymbolT x4 = SO_NaechstesVarSymb(x3);
    SymbolT x5 = SO_NaechstesVarSymb(x4);
    SymbolT x6 = SO_NaechstesVarSymb(x5);

    le_pushLemma(TO_(f,x1,f,x2,f,x3,x1),                TO_(f,x1,f,x2,x3),               Groesser,TRUE);
    le_pushLemma(TO_(f,x1,f,x2,f,x3,f,x4,x1),           TO_(f,x1,f,x2,f,x3,x4),          Groesser,TRUE);
    le_pushLemma(TO_(f,x1,f,x2,f,x3,f,x4,f,x5,x1),      TO_(f,x1,f,x2,f,x3,f,x4,x5),     Groesser,TRUE);
    le_pushLemma(TO_(f,x1,f,x2,f,x3,f,x4,f,x5,f,x6,x1), TO_(f,x1,f,x2,f,x3,f,x4,f,x5,x6),Groesser,TRUE);
  }
}

static void le_fillC_Lemma(SymbolT f)
{
  if (SO_Stelligkeit(f) != 2){
    our_error("wrong symbol for lemma family");
  }
  {
    SymbolT x1 = SO_ErstesVarSymb;
    SymbolT x2 = SO_NaechstesVarSymb(x1);
    SymbolT x3 = SO_NaechstesVarSymb(x2);

    le_pushLemma(TO_(f,x1,f,x2,x3),                TO_(f,x2,f,x1,x3),              Unvergleichbar,FALSE);
  }
}

static void le_fillPermLemmata(SymbolT f)
{
  if (SO_Stelligkeit(f) != 2){
    our_error("wrong symbol for lemma family");
  }
  {
    SymbolT x1 = SO_ErstesVarSymb;
    SymbolT x2 = SO_NaechstesVarSymb(x1);
    SymbolT x3 = SO_NaechstesVarSymb(x2);
    SymbolT x4 = SO_NaechstesVarSymb(x3);
    SymbolT x5 = SO_NaechstesVarSymb(x4);
    SymbolT x6 = SO_NaechstesVarSymb(x5);

    le_pushLemma(TO_(f,x1,f,x2,f,x3,x4),          TO_(f,x4,f,x2,f,x3,x1),          Unvergleichbar,TRUE);
    le_pushLemma(TO_(f,x1,f,x2,f,x3,f,x4,x5),     TO_(f,x5,f,x2,f,x3,f,x4,x1),     Unvergleichbar,TRUE);
    le_pushLemma(TO_(f,x1,f,x2,f,x3,f,x4,f,x5,x6),TO_(f,x6,f,x2,f,x3,f,x4,f,x5,x1),Unvergleichbar,TRUE);
    le_pushLemma(TO_(f,x1,f,x2,f,x3,x4),          TO_(f,x3,f,x2,f,x1,x4),          Unvergleichbar,TRUE);
    le_pushLemma(TO_(f,x1,f,x2,f,x3,f,x4,x5),     TO_(f,x4,f,x2,f,x3,f,x1,x5),     Unvergleichbar,TRUE);
    le_pushLemma(TO_(f,x1,f,x2,f,x3,f,x4,f,x5,x6),TO_(f,x5,f,x2,f,x3,f,x4,f,x1,x6),Unvergleichbar,TRUE);
  }
}

static void le_fillFullPermLemmata4(SymbolT f)
{
  if (SO_Stelligkeit(f) != 2){
    our_error("wrong symbol for lemma family");
  }
  {
    SymbolT x1 = SO_ErstesVarSymb;
    SymbolT x2 = SO_NaechstesVarSymb(x1);
    SymbolT x3 = SO_NaechstesVarSymb(x2);
    SymbolT x4 = SO_NaechstesVarSymb(x3);

    le_pushLemma(TO_(f,x1,f,x2,f,x3,x4),TO_(f,x2,f,x3,f,x4,x1),Unvergleichbar,TRUE);
    le_pushLemma(TO_(f,x1,f,x2,f,x3,x4),TO_(f,x3,f,x1,f,x2,x4),Unvergleichbar,TRUE);
    le_pushLemma(TO_(f,x1,f,x2,f,x3,x4),TO_(f,x4,f,x3,f,x1,x2),Unvergleichbar,TRUE);
    le_pushLemma(TO_(f,x1,f,x2,f,x3,x4),TO_(f,x3,f,x2,f,x4,x1),Unvergleichbar,TRUE);
    le_pushLemma(TO_(f,x1,f,x2,f,x3,x4),TO_(f,x3,f,x4,f,x1,x2),Unvergleichbar,TRUE);
    le_pushLemma(TO_(f,x1,f,x2,f,x3,x4),TO_(f,x4,f,x2,f,x3,x1),Unvergleichbar,TRUE);
    le_pushLemma(TO_(f,x1,f,x2,f,x3,x4),TO_(f,x2,f,x4,f,x1,x3),Unvergleichbar,TRUE);
    le_pushLemma(TO_(f,x1,f,x2,f,x3,x4),TO_(f,x4,f,x1,f,x3,x2),Unvergleichbar,TRUE);
    le_pushLemma(TO_(f,x1,f,x2,f,x3,x4),TO_(f,x3,f,x2,f,x1,x4),Unvergleichbar,TRUE);
    le_pushLemma(TO_(f,x1,f,x2,f,x3,x4),TO_(f,x4,f,x3,f,x2,x1),Unvergleichbar,TRUE);
    le_pushLemma(TO_(f,x1,f,x2,f,x3,x4),TO_(f,x2,f,x1,f,x4,x3),Unvergleichbar,TRUE);
  }
}

static void le_fillFullPermLemmata5(SymbolT f)
{
  if (SO_Stelligkeit(f) != 2){
    our_error("wrong symbol for lemma family");
  }
  {
    SymbolT x1 = SO_ErstesVarSymb;
    SymbolT x2 = SO_NaechstesVarSymb(x1);
    SymbolT x3 = SO_NaechstesVarSymb(x2);
    SymbolT x4 = SO_NaechstesVarSymb(x3);
    SymbolT x5 = SO_NaechstesVarSymb(x4);

    le_pushLemma(TO_(f,x1,f,x2,f,x3,f,x4,x5),TO_(f,x4,f,x5,f,x1,f,x2,x3),Unvergleichbar,TRUE);
    le_pushLemma(TO_(f,x1,f,x2,f,x3,f,x4,x5),TO_(f,x5,f,x1,f,x2,f,x3,x4),Unvergleichbar,TRUE);
    le_pushLemma(TO_(f,x1,f,x2,f,x3,f,x4,x5),TO_(f,x5,f,x1,f,x4,f,x3,x2),Unvergleichbar,TRUE);
    le_pushLemma(TO_(f,x1,f,x2,f,x3,f,x4,x5),TO_(f,x5,f,x1,f,x3,f,x2,x4),Unvergleichbar,TRUE);
    le_pushLemma(TO_(f,x1,f,x2,f,x3,f,x4,x5),TO_(f,x5,f,x1,f,x3,f,x4,x2),Unvergleichbar,TRUE);
    le_pushLemma(TO_(f,x1,f,x2,f,x3,f,x4,x5),TO_(f,x5,f,x1,f,x4,f,x2,x3),Unvergleichbar,TRUE);
    le_pushLemma(TO_(f,x1,f,x2,f,x3,f,x4,x5),TO_(f,x2,f,x3,f,x4,f,x1,x5),Unvergleichbar,TRUE);
    le_pushLemma(TO_(f,x1,f,x2,f,x3,f,x4,x5),TO_(f,x5,f,x2,f,x3,f,x4,x1),Unvergleichbar,TRUE);
    le_pushLemma(TO_(f,x1,f,x2,f,x3,f,x4,x5),TO_(f,x3,f,x5,f,x1,f,x2,x4),Unvergleichbar,TRUE);
    le_pushLemma(TO_(f,x1,f,x2,f,x3,f,x4,x5),TO_(f,x3,f,x4,f,x2,f,x1,x5),Unvergleichbar,TRUE);
    le_pushLemma(TO_(f,x1,f,x2,f,x3,f,x4,x5),TO_(f,x3,f,x4,f,x1,f,x2,x5),Unvergleichbar,TRUE);
    le_pushLemma(TO_(f,x1,f,x2,f,x3,f,x4,x5),TO_(f,x3,f,x4,f,x2,f,x5,x1),Unvergleichbar,TRUE);
    le_pushLemma(TO_(f,x1,f,x2,f,x3,f,x4,x5),TO_(f,x5,f,x3,f,x4,f,x1,x2),Unvergleichbar,TRUE);
    le_pushLemma(TO_(f,x1,f,x2,f,x3,f,x4,x5),TO_(f,x2,f,x5,f,x1,f,x3,x4),Unvergleichbar,TRUE);
    le_pushLemma(TO_(f,x1,f,x2,f,x3,f,x4,x5),TO_(f,x5,f,x4,f,x1,f,x2,x3),Unvergleichbar,TRUE);
    le_pushLemma(TO_(f,x1,f,x2,f,x3,f,x4,x5),TO_(f,x4,f,x3,f,x5,f,x1,x2),Unvergleichbar,TRUE);
    le_pushLemma(TO_(f,x1,f,x2,f,x3,f,x4,x5),TO_(f,x3,f,x2,f,x4,f,x5,x1),Unvergleichbar,TRUE);
    le_pushLemma(TO_(f,x1,f,x2,f,x3,f,x4,x5),TO_(f,x2,f,x5,f,x3,f,x1,x4),Unvergleichbar,TRUE);
    le_pushLemma(TO_(f,x1,f,x2,f,x3,f,x4,x5),TO_(f,x2,f,x1,f,x4,f,x5,x3),Unvergleichbar,TRUE);
    le_pushLemma(TO_(f,x1,f,x2,f,x3,f,x4,x5),TO_(f,x2,f,x3,f,x5,f,x4,x1),Unvergleichbar,TRUE);
    le_pushLemma(TO_(f,x1,f,x2,f,x3,f,x4,x5),TO_(f,x2,f,x3,f,x5,f,x1,x4),Unvergleichbar,TRUE);
    le_pushLemma(TO_(f,x1,f,x2,f,x3,f,x4,x5),TO_(f,x5,f,x2,f,x4,f,x1,x3),Unvergleichbar,TRUE);
    le_pushLemma(TO_(f,x1,f,x2,f,x3,f,x4,x5),TO_(f,x2,f,x4,f,x1,f,x3,x5),Unvergleichbar,TRUE);
    le_pushLemma(TO_(f,x1,f,x2,f,x3,f,x4,x5),TO_(f,x4,f,x2,f,x5,f,x1,x3),Unvergleichbar,TRUE);
    le_pushLemma(TO_(f,x1,f,x2,f,x3,f,x4,x5),TO_(f,x2,f,x4,f,x3,f,x1,x5),Unvergleichbar,TRUE);
    le_pushLemma(TO_(f,x1,f,x2,f,x3,f,x4,x5),TO_(f,x3,f,x1,f,x5,f,x4,x2),Unvergleichbar,TRUE);
    le_pushLemma(TO_(f,x1,f,x2,f,x3,f,x4,x5),TO_(f,x3,f,x1,f,x5,f,x2,x4),Unvergleichbar,TRUE);
    le_pushLemma(TO_(f,x1,f,x2,f,x3,f,x4,x5),TO_(f,x5,f,x2,f,x3,f,x1,x4),Unvergleichbar,TRUE);
    le_pushLemma(TO_(f,x1,f,x2,f,x3,f,x4,x5),TO_(f,x3,f,x2,f,x4,f,x1,x5),Unvergleichbar,TRUE);
    le_pushLemma(TO_(f,x1,f,x2,f,x3,f,x4,x5),TO_(f,x2,f,x1,f,x4,f,x3,x5),Unvergleichbar,TRUE);
    le_pushLemma(TO_(f,x1,f,x2,f,x3,f,x4,x5),TO_(f,x3,f,x5,f,x1,f,x4,x2),Unvergleichbar,TRUE);
    le_pushLemma(TO_(f,x1,f,x2,f,x3,f,x4,x5),TO_(f,x4,f,x3,f,x1,f,x5,x2),Unvergleichbar,TRUE);
    le_pushLemma(TO_(f,x1,f,x2,f,x3,f,x4,x5),TO_(f,x4,f,x1,f,x5,f,x3,x2),Unvergleichbar,TRUE);
    le_pushLemma(TO_(f,x1,f,x2,f,x3,f,x4,x5),TO_(f,x4,f,x5,f,x1,f,x3,x2),Unvergleichbar,TRUE);
    le_pushLemma(TO_(f,x1,f,x2,f,x3,f,x4,x5),TO_(f,x5,f,x3,f,x2,f,x4,x1),Unvergleichbar,TRUE);
    le_pushLemma(TO_(f,x1,f,x2,f,x3,f,x4,x5),TO_(f,x5,f,x4,f,x3,f,x1,x2),Unvergleichbar,TRUE);
    le_pushLemma(TO_(f,x1,f,x2,f,x3,f,x4,x5),TO_(f,x4,f,x3,f,x2,f,x5,x1),Unvergleichbar,TRUE);
    le_pushLemma(TO_(f,x1,f,x2,f,x3,f,x4,x5),TO_(f,x4,f,x2,f,x1,f,x5,x3),Unvergleichbar,TRUE);
    le_pushLemma(TO_(f,x1,f,x2,f,x3,f,x4,x5),TO_(f,x4,f,x5,f,x3,f,x1,x2),Unvergleichbar,TRUE);
    le_pushLemma(TO_(f,x1,f,x2,f,x3,f,x4,x5),TO_(f,x5,f,x4,f,x3,f,x2,x1),Unvergleichbar,TRUE);
    le_pushLemma(TO_(f,x1,f,x2,f,x3,f,x4,x5),TO_(f,x5,f,x4,f,x2,f,x1,x3),Unvergleichbar,TRUE);
    le_pushLemma(TO_(f,x1,f,x2,f,x3,f,x4,x5),TO_(f,x5,f,x4,f,x2,f,x3,x1),Unvergleichbar,TRUE);
    le_pushLemma(TO_(f,x1,f,x2,f,x3,f,x4,x5),TO_(f,x4,f,x2,f,x3,f,x1,x5),Unvergleichbar,TRUE);
    le_pushLemma(TO_(f,x1,f,x2,f,x3,f,x4,x5),TO_(f,x2,f,x4,f,x5,f,x1,x3),Unvergleichbar,TRUE);
    le_pushLemma(TO_(f,x1,f,x2,f,x3,f,x4,x5),TO_(f,x3,f,x1,f,x2,f,x5,x4),Unvergleichbar,TRUE);
    le_pushLemma(TO_(f,x1,f,x2,f,x3,f,x4,x5),TO_(f,x5,f,x2,f,x4,f,x3,x1),Unvergleichbar,TRUE);
    le_pushLemma(TO_(f,x1,f,x2,f,x3,f,x4,x5),TO_(f,x5,f,x3,f,x1,f,x4,x2),Unvergleichbar,TRUE);
    le_pushLemma(TO_(f,x1,f,x2,f,x3,f,x4,x5),TO_(f,x4,f,x3,f,x2,f,x1,x5),Unvergleichbar,TRUE);
    le_pushLemma(TO_(f,x1,f,x2,f,x3,f,x4,x5),TO_(f,x3,f,x2,f,x5,f,x4,x1),Unvergleichbar,TRUE);
    le_pushLemma(TO_(f,x1,f,x2,f,x3,f,x4,x5),TO_(f,x5,f,x4,f,x1,f,x3,x2),Unvergleichbar,TRUE);
    le_pushLemma(TO_(f,x1,f,x2,f,x3,f,x4,x5),TO_(f,x2,f,x1,f,x3,f,x5,x4),Unvergleichbar,TRUE);
    le_pushLemma(TO_(f,x1,f,x2,f,x3,f,x4,x5),TO_(f,x3,f,x2,f,x1,f,x5,x4),Unvergleichbar,TRUE);
    le_pushLemma(TO_(f,x1,f,x2,f,x3,f,x4,x5),TO_(f,x2,f,x1,f,x5,f,x4,x3),Unvergleichbar,TRUE);
  }
}

static void le_fillFullPermLemmata6(SymbolT f)
{
  if (SO_Stelligkeit(f) != 2){
    our_error("wrong symbol for lemma family");
  }
  {
    SymbolT x1 = SO_ErstesVarSymb;
    SymbolT x2 = SO_NaechstesVarSymb(x1);
    SymbolT x3 = SO_NaechstesVarSymb(x2);
    SymbolT x4 = SO_NaechstesVarSymb(x3);
    SymbolT x5 = SO_NaechstesVarSymb(x4);
    SymbolT x6 = SO_NaechstesVarSymb(x5);

    le_pushLemma(TO_(f,x1,f,x2,f,x3,f,x4,f,x5,x6),TO_(f,x4,f,x5,f,x1,f,x2,f,x3,x6),Unvergleichbar,TRUE);
    le_pushLemma(TO_(f,x1,f,x2,f,x3,f,x4,f,x5,x6),TO_(f,x6,f,x1,f,x3,f,x4,f,x5,x2),Unvergleichbar,TRUE);
    le_pushLemma(TO_(f,x1,f,x2,f,x3,f,x4,f,x5,x6),TO_(f,x4,f,x5,f,x3,f,x6,f,x1,x2),Unvergleichbar,TRUE);
    le_pushLemma(TO_(f,x1,f,x2,f,x3,f,x4,f,x5,x6),TO_(f,x6,f,x1,f,x5,f,x2,f,x3,x4),Unvergleichbar,TRUE);
    le_pushLemma(TO_(f,x1,f,x2,f,x3,f,x4,f,x5,x6),TO_(f,x2,f,x3,f,x6,f,x1,f,x4,x5),Unvergleichbar,TRUE);
    le_pushLemma(TO_(f,x1,f,x2,f,x3,f,x4,f,x5,x6),TO_(f,x2,f,x3,f,x4,f,x5,f,x6,x1),Unvergleichbar,TRUE);
    le_pushLemma(TO_(f,x1,f,x2,f,x3,f,x4,f,x5,x6),TO_(f,x3,f,x1,f,x5,f,x2,f,x4,x6),Unvergleichbar,TRUE);
    le_pushLemma(TO_(f,x1,f,x2,f,x3,f,x4,f,x5,x6),TO_(f,x2,f,x3,f,x5,f,x1,f,x4,x6),Unvergleichbar,TRUE);
    le_pushLemma(TO_(f,x1,f,x2,f,x3,f,x4,f,x5,x6),TO_(f,x4,f,x5,f,x2,f,x6,f,x1,x3),Unvergleichbar,TRUE);
    le_pushLemma(TO_(f,x1,f,x2,f,x3,f,x4,f,x5,x6),TO_(f,x6,f,x1,f,x4,f,x2,f,x3,x5),Unvergleichbar,TRUE);
    le_pushLemma(TO_(f,x1,f,x2,f,x3,f,x4,f,x5,x6),TO_(f,x3,f,x1,f,x6,f,x2,f,x4,x5),Unvergleichbar,TRUE);
    le_pushLemma(TO_(f,x1,f,x2,f,x3,f,x4,f,x5,x6),TO_(f,x3,f,x1,f,x4,f,x5,f,x6,x2),Unvergleichbar,TRUE);
    le_pushLemma(TO_(f,x1,f,x2,f,x3,f,x4,f,x5,x6),TO_(f,x3,f,x1,f,x4,f,x5,f,x2,x6),Unvergleichbar,TRUE);
    le_pushLemma(TO_(f,x1,f,x2,f,x3,f,x4,f,x5,x6),TO_(f,x2,f,x3,f,x4,f,x5,f,x1,x6),Unvergleichbar,TRUE);
    le_pushLemma(TO_(f,x1,f,x2,f,x3,f,x4,f,x5,x6),TO_(f,x4,f,x5,f,x1,f,x2,f,x6,x3),Unvergleichbar,TRUE);
    le_pushLemma(TO_(f,x1,f,x2,f,x3,f,x4,f,x5,x6),TO_(f,x6,f,x1,f,x3,f,x4,f,x2,x5),Unvergleichbar,TRUE);
    le_pushLemma(TO_(f,x1,f,x2,f,x3,f,x4,f,x5,x6),TO_(f,x5,f,x6,f,x1,f,x2,f,x4,x3),Unvergleichbar,TRUE);
    le_pushLemma(TO_(f,x1,f,x2,f,x3,f,x4,f,x5,x6),TO_(f,x5,f,x6,f,x2,f,x4,f,x1,x3),Unvergleichbar,TRUE);
    le_pushLemma(TO_(f,x1,f,x2,f,x3,f,x4,f,x5,x6),TO_(f,x5,f,x6,f,x3,f,x4,f,x1,x2),Unvergleichbar,TRUE);
    le_pushLemma(TO_(f,x1,f,x2,f,x3,f,x4,f,x5,x6),TO_(f,x5,f,x6,f,x1,f,x2,f,x3,x4),Unvergleichbar,TRUE);
    le_pushLemma(TO_(f,x1,f,x2,f,x3,f,x4,f,x5,x6),TO_(f,x6,f,x5,f,x4,f,x3,f,x1,x2),Unvergleichbar,TRUE);
    le_pushLemma(TO_(f,x1,f,x2,f,x3,f,x4,f,x5,x6),TO_(f,x5,f,x6,f,x3,f,x2,f,x4,x1),Unvergleichbar,TRUE);
    le_pushLemma(TO_(f,x1,f,x2,f,x3,f,x4,f,x5,x6),TO_(f,x3,f,x1,f,x6,f,x5,f,x2,x4),Unvergleichbar,TRUE);
    le_pushLemma(TO_(f,x1,f,x2,f,x3,f,x4,f,x5,x6),TO_(f,x2,f,x3,f,x6,f,x5,f,x1,x4),Unvergleichbar,TRUE);
    le_pushLemma(TO_(f,x1,f,x2,f,x3,f,x4,f,x5,x6),TO_(f,x4,f,x5,f,x3,f,x2,f,x6,x1),Unvergleichbar,TRUE);
    le_pushLemma(TO_(f,x1,f,x2,f,x3,f,x4,f,x5,x6),TO_(f,x6,f,x1,f,x5,f,x4,f,x2,x3),Unvergleichbar,TRUE);
    le_pushLemma(TO_(f,x1,f,x2,f,x3,f,x4,f,x5,x6),TO_(f,x6,f,x5,f,x2,f,x3,f,x1,x4),Unvergleichbar,TRUE);
    le_pushLemma(TO_(f,x1,f,x2,f,x3,f,x4,f,x5,x6),TO_(f,x6,f,x5,f,x3,f,x1,f,x2,x4),Unvergleichbar,TRUE);
    le_pushLemma(TO_(f,x1,f,x2,f,x3,f,x4,f,x5,x6),TO_(f,x6,f,x5,f,x4,f,x1,f,x2,x3),Unvergleichbar,TRUE);
    le_pushLemma(TO_(f,x1,f,x2,f,x3,f,x4,f,x5,x6),TO_(f,x6,f,x5,f,x2,f,x3,f,x4,x1),Unvergleichbar,TRUE);
    le_pushLemma(TO_(f,x1,f,x2,f,x3,f,x4,f,x5,x6),TO_(f,x6,f,x5,f,x3,f,x4,f,x2,x1),Unvergleichbar,TRUE);
    le_pushLemma(TO_(f,x1,f,x2,f,x3,f,x4,f,x5,x6),TO_(f,x5,f,x6,f,x2,f,x3,f,x1,x4),Unvergleichbar,TRUE);
    le_pushLemma(TO_(f,x1,f,x2,f,x3,f,x4,f,x5,x6),TO_(f,x3,f,x1,f,x5,f,x6,f,x4,x2),Unvergleichbar,TRUE);
    le_pushLemma(TO_(f,x1,f,x2,f,x3,f,x4,f,x5,x6),TO_(f,x2,f,x3,f,x5,f,x6,f,x4,x1),Unvergleichbar,TRUE);
    le_pushLemma(TO_(f,x1,f,x2,f,x3,f,x4,f,x5,x6),TO_(f,x4,f,x5,f,x2,f,x3,f,x1,x6),Unvergleichbar,TRUE);
    le_pushLemma(TO_(f,x1,f,x2,f,x3,f,x4,f,x5,x6),TO_(f,x6,f,x1,f,x4,f,x5,f,x3,x2),Unvergleichbar,TRUE);
    le_pushLemma(TO_(f,x1,f,x2,f,x3,f,x4,f,x5,x6),TO_(f,x6,f,x2,f,x4,f,x5,f,x3,x1),Unvergleichbar,TRUE);
    le_pushLemma(TO_(f,x1,f,x2,f,x3,f,x4,f,x5,x6),TO_(f,x6,f,x2,f,x5,f,x4,f,x1,x3),Unvergleichbar,TRUE);
    le_pushLemma(TO_(f,x1,f,x2,f,x3,f,x4,f,x5,x6),TO_(f,x6,f,x2,f,x3,f,x4,f,x1,x5),Unvergleichbar,TRUE);
    le_pushLemma(TO_(f,x1,f,x2,f,x3,f,x4,f,x5,x6),TO_(f,x6,f,x2,f,x4,f,x1,f,x3,x5),Unvergleichbar,TRUE);
    le_pushLemma(TO_(f,x1,f,x2,f,x3,f,x4,f,x5,x6),TO_(f,x6,f,x2,f,x5,f,x1,f,x3,x4),Unvergleichbar,TRUE);
    le_pushLemma(TO_(f,x1,f,x2,f,x3,f,x4,f,x5,x6),TO_(f,x6,f,x2,f,x3,f,x4,f,x5,x1),Unvergleichbar,TRUE);
    le_pushLemma(TO_(f,x1,f,x2,f,x3,f,x4,f,x5,x6),TO_(f,x5,f,x4,f,x2,f,x1,f,x3,x6),Unvergleichbar,TRUE);
    le_pushLemma(TO_(f,x1,f,x2,f,x3,f,x4,f,x5,x6),TO_(f,x6,f,x2,f,x4,f,x3,f,x5,x1),Unvergleichbar,TRUE);
    le_pushLemma(TO_(f,x1,f,x2,f,x3,f,x4,f,x5,x6),TO_(f,x6,f,x5,f,x3,f,x2,f,x4,x1),Unvergleichbar,TRUE);
    le_pushLemma(TO_(f,x1,f,x2,f,x3,f,x4,f,x5,x6),TO_(f,x5,f,x6,f,x2,f,x1,f,x3,x4),Unvergleichbar,TRUE);
    le_pushLemma(TO_(f,x1,f,x2,f,x3,f,x4,f,x5,x6),TO_(f,x3,f,x1,f,x5,f,x4,f,x6,x2),Unvergleichbar,TRUE);
    le_pushLemma(TO_(f,x1,f,x2,f,x3,f,x4,f,x5,x6),TO_(f,x2,f,x3,f,x5,f,x4,f,x6,x1),Unvergleichbar,TRUE);
    le_pushLemma(TO_(f,x1,f,x2,f,x3,f,x4,f,x5,x6),TO_(f,x4,f,x5,f,x2,f,x1,f,x3,x6),Unvergleichbar,TRUE);
    le_pushLemma(TO_(f,x1,f,x2,f,x3,f,x4,f,x5,x6),TO_(f,x6,f,x1,f,x4,f,x3,f,x5,x2),Unvergleichbar,TRUE);
    le_pushLemma(TO_(f,x1,f,x2,f,x3,f,x4,f,x5,x6),TO_(f,x5,f,x4,f,x2,f,x3,f,x1,x6),Unvergleichbar,TRUE);
    le_pushLemma(TO_(f,x1,f,x2,f,x3,f,x4,f,x5,x6),TO_(f,x5,f,x4,f,x3,f,x2,f,x6,x1),Unvergleichbar,TRUE);
    le_pushLemma(TO_(f,x1,f,x2,f,x3,f,x4,f,x5,x6),TO_(f,x5,f,x4,f,x1,f,x2,f,x6,x3),Unvergleichbar,TRUE);
    le_pushLemma(TO_(f,x1,f,x2,f,x3,f,x4,f,x5,x6),TO_(f,x5,f,x4,f,x2,f,x6,f,x1,x3),Unvergleichbar,TRUE);
    le_pushLemma(TO_(f,x1,f,x2,f,x3,f,x4,f,x5,x6),TO_(f,x5,f,x4,f,x3,f,x6,f,x1,x2),Unvergleichbar,TRUE);
    le_pushLemma(TO_(f,x1,f,x2,f,x3,f,x4,f,x5,x6),TO_(f,x5,f,x4,f,x1,f,x2,f,x3,x6),Unvergleichbar,TRUE);
    le_pushLemma(TO_(f,x1,f,x2,f,x3,f,x4,f,x5,x6),TO_(f,x5,f,x4,f,x3,f,x1,f,x6,x2),Unvergleichbar,TRUE);
    le_pushLemma(TO_(f,x1,f,x2,f,x3,f,x4,f,x5,x6),TO_(f,x6,f,x2,f,x5,f,x3,f,x1,x4),Unvergleichbar,TRUE);
    le_pushLemma(TO_(f,x1,f,x2,f,x3,f,x4,f,x5,x6),TO_(f,x6,f,x5,f,x4,f,x2,f,x1,x3),Unvergleichbar,TRUE);
    le_pushLemma(TO_(f,x1,f,x2,f,x3,f,x4,f,x5,x6),TO_(f,x5,f,x6,f,x3,f,x1,f,x4,x2),Unvergleichbar,TRUE);
    le_pushLemma(TO_(f,x1,f,x2,f,x3,f,x4,f,x5,x6),TO_(f,x3,f,x1,f,x6,f,x4,f,x2,x5),Unvergleichbar,TRUE);
    le_pushLemma(TO_(f,x1,f,x2,f,x3,f,x4,f,x5,x6),TO_(f,x2,f,x3,f,x6,f,x4,f,x1,x5),Unvergleichbar,TRUE);
    le_pushLemma(TO_(f,x1,f,x2,f,x3,f,x4,f,x5,x6),TO_(f,x4,f,x5,f,x3,f,x1,f,x6,x2),Unvergleichbar,TRUE);
    le_pushLemma(TO_(f,x1,f,x2,f,x3,f,x4,f,x5,x6),TO_(f,x6,f,x1,f,x5,f,x3,f,x2,x4),Unvergleichbar,TRUE);
    le_pushLemma(TO_(f,x1,f,x2,f,x3,f,x4,f,x5,x6),TO_(f,x5,f,x6,f,x4,f,x2,f,x1,x3),Unvergleichbar,TRUE);
    le_pushLemma(TO_(f,x1,f,x2,f,x3,f,x4,f,x5,x6),TO_(f,x5,f,x6,f,x3,f,x4,f,x2,x1),Unvergleichbar,TRUE);
    le_pushLemma(TO_(f,x1,f,x2,f,x3,f,x4,f,x5,x6),TO_(f,x5,f,x6,f,x4,f,x3,f,x1,x2),Unvergleichbar,TRUE);
    le_pushLemma(TO_(f,x1,f,x2,f,x3,f,x4,f,x5,x6),TO_(f,x5,f,x6,f,x4,f,x1,f,x2,x3),Unvergleichbar,TRUE);
    le_pushLemma(TO_(f,x1,f,x2,f,x3,f,x4,f,x5,x6),TO_(f,x5,f,x6,f,x2,f,x3,f,x4,x1),Unvergleichbar,TRUE);
    le_pushLemma(TO_(f,x1,f,x2,f,x3,f,x4,f,x5,x6),TO_(f,x5,f,x4,f,x2,f,x3,f,x6,x1),Unvergleichbar,TRUE);
    le_pushLemma(TO_(f,x1,f,x2,f,x3,f,x4,f,x5,x6),TO_(f,x6,f,x2,f,x4,f,x5,f,x1,x3),Unvergleichbar,TRUE);
    le_pushLemma(TO_(f,x1,f,x2,f,x3,f,x4,f,x5,x6),TO_(f,x3,f,x1,f,x5,f,x6,f,x2,x4),Unvergleichbar,TRUE);
    le_pushLemma(TO_(f,x1,f,x2,f,x3,f,x4,f,x5,x6),TO_(f,x2,f,x3,f,x5,f,x6,f,x1,x4),Unvergleichbar,TRUE);
    le_pushLemma(TO_(f,x1,f,x2,f,x3,f,x4,f,x5,x6),TO_(f,x4,f,x5,f,x2,f,x3,f,x6,x1),Unvergleichbar,TRUE);
    le_pushLemma(TO_(f,x1,f,x2,f,x3,f,x4,f,x5,x6),TO_(f,x6,f,x1,f,x4,f,x5,f,x2,x3),Unvergleichbar,TRUE);
    le_pushLemma(TO_(f,x1,f,x2,f,x3,f,x4,f,x5,x6),TO_(f,x6,f,x4,f,x2,f,x3,f,x5,x1),Unvergleichbar,TRUE);
    le_pushLemma(TO_(f,x1,f,x2,f,x3,f,x4,f,x5,x6),TO_(f,x6,f,x4,f,x3,f,x1,f,x5,x2),Unvergleichbar,TRUE);
    le_pushLemma(TO_(f,x1,f,x2,f,x3,f,x4,f,x5,x6),TO_(f,x6,f,x4,f,x2,f,x1,f,x3,x5),Unvergleichbar,TRUE);
    le_pushLemma(TO_(f,x1,f,x2,f,x3,f,x4,f,x5,x6),TO_(f,x6,f,x4,f,x2,f,x3,f,x1,x5),Unvergleichbar,TRUE);
    le_pushLemma(TO_(f,x1,f,x2,f,x3,f,x4,f,x5,x6),TO_(f,x6,f,x4,f,x3,f,x2,f,x5,x1),Unvergleichbar,TRUE);
    le_pushLemma(TO_(f,x1,f,x2,f,x3,f,x4,f,x5,x6),TO_(f,x6,f,x4,f,x1,f,x2,f,x5,x3),Unvergleichbar,TRUE);
    le_pushLemma(TO_(f,x1,f,x2,f,x3,f,x4,f,x5,x6),TO_(f,x6,f,x4,f,x2,f,x5,f,x1,x3),Unvergleichbar,TRUE);
    le_pushLemma(TO_(f,x1,f,x2,f,x3,f,x4,f,x5,x6),TO_(f,x6,f,x4,f,x1,f,x2,f,x3,x5),Unvergleichbar,TRUE);
    le_pushLemma(TO_(f,x1,f,x2,f,x3,f,x4,f,x5,x6),TO_(f,x5,f,x6,f,x4,f,x2,f,x3,x1),Unvergleichbar,TRUE);
    le_pushLemma(TO_(f,x1,f,x2,f,x3,f,x4,f,x5,x6),TO_(f,x5,f,x4,f,x3,f,x1,f,x2,x6),Unvergleichbar,TRUE);
    le_pushLemma(TO_(f,x1,f,x2,f,x3,f,x4,f,x5,x6),TO_(f,x6,f,x5,f,x4,f,x2,f,x3,x1),Unvergleichbar,TRUE);
    le_pushLemma(TO_(f,x1,f,x2,f,x3,f,x4,f,x5,x6),TO_(f,x3,f,x1,f,x6,f,x4,f,x5,x2),Unvergleichbar,TRUE);
    le_pushLemma(TO_(f,x1,f,x2,f,x3,f,x4,f,x5,x6),TO_(f,x2,f,x3,f,x6,f,x4,f,x5,x1),Unvergleichbar,TRUE);
    le_pushLemma(TO_(f,x1,f,x2,f,x3,f,x4,f,x5,x6),TO_(f,x4,f,x5,f,x3,f,x1,f,x2,x6),Unvergleichbar,TRUE);
    le_pushLemma(TO_(f,x1,f,x2,f,x3,f,x4,f,x5,x6),TO_(f,x6,f,x1,f,x5,f,x3,f,x4,x2),Unvergleichbar,TRUE);
    le_pushLemma(TO_(f,x1,f,x2,f,x3,f,x4,f,x5,x6),TO_(f,x5,f,x1,f,x4,f,x2,f,x3,x6),Unvergleichbar,TRUE);
    le_pushLemma(TO_(f,x1,f,x2,f,x3,f,x4,f,x5,x6),TO_(f,x5,f,x1,f,x3,f,x4,f,x6,x2),Unvergleichbar,TRUE);
    le_pushLemma(TO_(f,x1,f,x2,f,x3,f,x4,f,x5,x6),TO_(f,x5,f,x1,f,x4,f,x2,f,x6,x3),Unvergleichbar,TRUE);
    le_pushLemma(TO_(f,x1,f,x2,f,x3,f,x4,f,x5,x6),TO_(f,x5,f,x1,f,x3,f,x2,f,x4,x6),Unvergleichbar,TRUE);
    le_pushLemma(TO_(f,x1,f,x2,f,x3,f,x4,f,x5,x6),TO_(f,x5,f,x1,f,x3,f,x4,f,x2,x6),Unvergleichbar,TRUE);
    le_pushLemma(TO_(f,x1,f,x2,f,x3,f,x4,f,x5,x6),TO_(f,x5,f,x1,f,x4,f,x3,f,x6,x2),Unvergleichbar,TRUE);
    le_pushLemma(TO_(f,x1,f,x2,f,x3,f,x4,f,x5,x6),TO_(f,x5,f,x1,f,x2,f,x3,f,x6,x4),Unvergleichbar,TRUE);
    le_pushLemma(TO_(f,x1,f,x2,f,x3,f,x4,f,x5,x6),TO_(f,x5,f,x1,f,x3,f,x6,f,x2,x4),Unvergleichbar,TRUE);
    le_pushLemma(TO_(f,x1,f,x2,f,x3,f,x4,f,x5,x6),TO_(f,x5,f,x1,f,x4,f,x6,f,x2,x3),Unvergleichbar,TRUE);
    le_pushLemma(TO_(f,x1,f,x2,f,x3,f,x4,f,x5,x6),TO_(f,x2,f,x6,f,x3,f,x5,f,x1,x4),Unvergleichbar,TRUE);
    le_pushLemma(TO_(f,x1,f,x2,f,x3,f,x4,f,x5,x6),TO_(f,x6,f,x4,f,x1,f,x3,f,x5,x2),Unvergleichbar,TRUE);
    le_pushLemma(TO_(f,x1,f,x2,f,x3,f,x4,f,x5,x6),TO_(f,x5,f,x4,f,x1,f,x3,f,x6,x2),Unvergleichbar,TRUE);
    le_pushLemma(TO_(f,x1,f,x2,f,x3,f,x4,f,x5,x6),TO_(f,x6,f,x2,f,x3,f,x5,f,x1,x4),Unvergleichbar,TRUE);
    le_pushLemma(TO_(f,x1,f,x2,f,x3,f,x4,f,x5,x6),TO_(f,x6,f,x5,f,x2,f,x4,f,x1,x3),Unvergleichbar,TRUE);
    le_pushLemma(TO_(f,x1,f,x2,f,x3,f,x4,f,x5,x6),TO_(f,x5,f,x6,f,x1,f,x3,f,x4,x2),Unvergleichbar,TRUE);
    le_pushLemma(TO_(f,x1,f,x2,f,x3,f,x4,f,x5,x6),TO_(f,x3,f,x1,f,x4,f,x6,f,x2,x5),Unvergleichbar,TRUE);
    le_pushLemma(TO_(f,x1,f,x2,f,x3,f,x4,f,x5,x6),TO_(f,x4,f,x5,f,x1,f,x3,f,x6,x2),Unvergleichbar,TRUE);
    le_pushLemma(TO_(f,x1,f,x2,f,x3,f,x4,f,x5,x6),TO_(f,x6,f,x1,f,x3,f,x5,f,x2,x4),Unvergleichbar,TRUE);
    le_pushLemma(TO_(f,x1,f,x2,f,x3,f,x4,f,x5,x6),TO_(f,x2,f,x6,f,x4,f,x5,f,x1,x3),Unvergleichbar,TRUE);
    le_pushLemma(TO_(f,x1,f,x2,f,x3,f,x4,f,x5,x6),TO_(f,x2,f,x6,f,x5,f,x3,f,x1,x4),Unvergleichbar,TRUE);
    le_pushLemma(TO_(f,x1,f,x2,f,x3,f,x4,f,x5,x6),TO_(f,x2,f,x6,f,x5,f,x4,f,x1,x3),Unvergleichbar,TRUE);
    le_pushLemma(TO_(f,x1,f,x2,f,x3,f,x4,f,x5,x6),TO_(f,x2,f,x6,f,x4,f,x1,f,x3,x5),Unvergleichbar,TRUE);
    le_pushLemma(TO_(f,x1,f,x2,f,x3,f,x4,f,x5,x6),TO_(f,x2,f,x6,f,x5,f,x1,f,x3,x4),Unvergleichbar,TRUE);
    le_pushLemma(TO_(f,x1,f,x2,f,x3,f,x4,f,x5,x6),TO_(f,x2,f,x6,f,x4,f,x1,f,x5,x3),Unvergleichbar,TRUE);
    le_pushLemma(TO_(f,x1,f,x2,f,x3,f,x4,f,x5,x6),TO_(f,x6,f,x4,f,x2,f,x5,f,x3,x1),Unvergleichbar,TRUE);
    le_pushLemma(TO_(f,x1,f,x2,f,x3,f,x4,f,x5,x6),TO_(f,x5,f,x4,f,x2,f,x6,f,x3,x1),Unvergleichbar,TRUE);
    le_pushLemma(TO_(f,x1,f,x2,f,x3,f,x4,f,x5,x6),TO_(f,x6,f,x2,f,x4,f,x1,f,x5,x3),Unvergleichbar,TRUE);
    le_pushLemma(TO_(f,x1,f,x2,f,x3,f,x4,f,x5,x6),TO_(f,x6,f,x5,f,x3,f,x1,f,x4,x2),Unvergleichbar,TRUE);
    le_pushLemma(TO_(f,x1,f,x2,f,x3,f,x4,f,x5,x6),TO_(f,x5,f,x6,f,x2,f,x4,f,x3,x1),Unvergleichbar,TRUE);
    le_pushLemma(TO_(f,x1,f,x2,f,x3,f,x4,f,x5,x6),TO_(f,x3,f,x1,f,x5,f,x2,f,x6,x4),Unvergleichbar,TRUE);
    le_pushLemma(TO_(f,x1,f,x2,f,x3,f,x4,f,x5,x6),TO_(f,x2,f,x3,f,x5,f,x1,f,x6,x4),Unvergleichbar,TRUE);
    le_pushLemma(TO_(f,x1,f,x2,f,x3,f,x4,f,x5,x6),TO_(f,x4,f,x5,f,x2,f,x6,f,x3,x1),Unvergleichbar,TRUE);
    le_pushLemma(TO_(f,x1,f,x2,f,x3,f,x4,f,x5,x6),TO_(f,x6,f,x1,f,x4,f,x2,f,x5,x3),Unvergleichbar,TRUE);
    le_pushLemma(TO_(f,x1,f,x2,f,x3,f,x4,f,x5,x6),TO_(f,x4,f,x6,f,x2,f,x5,f,x3,x1),Unvergleichbar,TRUE);
    le_pushLemma(TO_(f,x1,f,x2,f,x3,f,x4,f,x5,x6),TO_(f,x4,f,x6,f,x1,f,x3,f,x5,x2),Unvergleichbar,TRUE);
    le_pushLemma(TO_(f,x1,f,x2,f,x3,f,x4,f,x5,x6),TO_(f,x4,f,x6,f,x2,f,x3,f,x5,x1),Unvergleichbar,TRUE);
    le_pushLemma(TO_(f,x1,f,x2,f,x3,f,x4,f,x5,x6),TO_(f,x4,f,x6,f,x3,f,x1,f,x5,x2),Unvergleichbar,TRUE);
    le_pushLemma(TO_(f,x1,f,x2,f,x3,f,x4,f,x5,x6),TO_(f,x4,f,x6,f,x2,f,x1,f,x3,x5),Unvergleichbar,TRUE);
    le_pushLemma(TO_(f,x1,f,x2,f,x3,f,x4,f,x5,x6),TO_(f,x4,f,x6,f,x2,f,x3,f,x1,x5),Unvergleichbar,TRUE);
    le_pushLemma(TO_(f,x1,f,x2,f,x3,f,x4,f,x5,x6),TO_(f,x4,f,x6,f,x1,f,x2,f,x5,x3),Unvergleichbar,TRUE);
    le_pushLemma(TO_(f,x1,f,x2,f,x3,f,x4,f,x5,x6),TO_(f,x4,f,x6,f,x2,f,x5,f,x1,x3),Unvergleichbar,TRUE);
    le_pushLemma(TO_(f,x1,f,x2,f,x3,f,x4,f,x5,x6),TO_(f,x4,f,x6,f,x1,f,x2,f,x3,x5),Unvergleichbar,TRUE);
    le_pushLemma(TO_(f,x1,f,x2,f,x3,f,x4,f,x5,x6),TO_(f,x2,f,x6,f,x5,f,x1,f,x4,x3),Unvergleichbar,TRUE);
    le_pushLemma(TO_(f,x1,f,x2,f,x3,f,x4,f,x5,x6),TO_(f,x5,f,x6,f,x4,f,x1,f,x3,x2),Unvergleichbar,TRUE);
    le_pushLemma(TO_(f,x1,f,x2,f,x3,f,x4,f,x5,x6),TO_(f,x5,f,x4,f,x3,f,x6,f,x2,x1),Unvergleichbar,TRUE);
    le_pushLemma(TO_(f,x1,f,x2,f,x3,f,x4,f,x5,x6),TO_(f,x6,f,x2,f,x5,f,x1,f,x4,x3),Unvergleichbar,TRUE);
    le_pushLemma(TO_(f,x1,f,x2,f,x3,f,x4,f,x5,x6),TO_(f,x6,f,x5,f,x4,f,x1,f,x3,x2),Unvergleichbar,TRUE);
    le_pushLemma(TO_(f,x1,f,x2,f,x3,f,x4,f,x5,x6),TO_(f,x3,f,x1,f,x6,f,x2,f,x5,x4),Unvergleichbar,TRUE);
    le_pushLemma(TO_(f,x1,f,x2,f,x3,f,x4,f,x5,x6),TO_(f,x2,f,x3,f,x6,f,x1,f,x5,x4),Unvergleichbar,TRUE);
    le_pushLemma(TO_(f,x1,f,x2,f,x3,f,x4,f,x5,x6),TO_(f,x6,f,x1,f,x5,f,x2,f,x4,x3),Unvergleichbar,TRUE);
    le_pushLemma(TO_(f,x1,f,x2,f,x3,f,x4,f,x5,x6),TO_(f,x6,f,x1,f,x3,f,x5,f,x4,x2),Unvergleichbar,TRUE);
    le_pushLemma(TO_(f,x1,f,x2,f,x3,f,x4,f,x5,x6),TO_(f,x6,f,x1,f,x3,f,x2,f,x4,x5),Unvergleichbar,TRUE);
    le_pushLemma(TO_(f,x1,f,x2,f,x3,f,x4,f,x5,x6),TO_(f,x6,f,x1,f,x2,f,x3,f,x5,x4),Unvergleichbar,TRUE);
    le_pushLemma(TO_(f,x1,f,x2,f,x3,f,x4,f,x5,x6),TO_(f,x4,f,x6,f,x1,f,x3,f,x2,x5),Unvergleichbar,TRUE);
    le_pushLemma(TO_(f,x1,f,x2,f,x3,f,x4,f,x5,x6),TO_(f,x5,f,x1,f,x2,f,x4,f,x3,x6),Unvergleichbar,TRUE);
    le_pushLemma(TO_(f,x1,f,x2,f,x3,f,x4,f,x5,x6),TO_(f,x6,f,x4,f,x1,f,x3,f,x2,x5),Unvergleichbar,TRUE);
    le_pushLemma(TO_(f,x1,f,x2,f,x3,f,x4,f,x5,x6),TO_(f,x5,f,x4,f,x1,f,x3,f,x2,x6),Unvergleichbar,TRUE);
    le_pushLemma(TO_(f,x1,f,x2,f,x3,f,x4,f,x5,x6),TO_(f,x6,f,x2,f,x3,f,x5,f,x4,x1),Unvergleichbar,TRUE);
    le_pushLemma(TO_(f,x1,f,x2,f,x3,f,x4,f,x5,x6),TO_(f,x6,f,x5,f,x2,f,x4,f,x3,x1),Unvergleichbar,TRUE);
    le_pushLemma(TO_(f,x1,f,x2,f,x3,f,x4,f,x5,x6),TO_(f,x5,f,x6,f,x1,f,x3,f,x2,x4),Unvergleichbar,TRUE);
    le_pushLemma(TO_(f,x1,f,x2,f,x3,f,x4,f,x5,x6),TO_(f,x3,f,x1,f,x4,f,x6,f,x5,x2),Unvergleichbar,TRUE);
    le_pushLemma(TO_(f,x1,f,x2,f,x3,f,x4,f,x5,x6),TO_(f,x4,f,x5,f,x1,f,x3,f,x2,x6),Unvergleichbar,TRUE);
    le_pushLemma(TO_(f,x1,f,x2,f,x3,f,x4,f,x5,x6),TO_(f,x3,f,x2,f,x4,f,x6,f,x5,x1),Unvergleichbar,TRUE);
    le_pushLemma(TO_(f,x1,f,x2,f,x3,f,x4,f,x5,x6),TO_(f,x3,f,x2,f,x6,f,x1,f,x5,x4),Unvergleichbar,TRUE);
    le_pushLemma(TO_(f,x1,f,x2,f,x3,f,x4,f,x5,x6),TO_(f,x3,f,x2,f,x5,f,x1,f,x6,x4),Unvergleichbar,TRUE);
    le_pushLemma(TO_(f,x1,f,x2,f,x3,f,x4,f,x5,x6),TO_(f,x3,f,x2,f,x4,f,x6,f,x1,x5),Unvergleichbar,TRUE);
    le_pushLemma(TO_(f,x1,f,x2,f,x3,f,x4,f,x5,x6),TO_(f,x3,f,x2,f,x6,f,x4,f,x5,x1),Unvergleichbar,TRUE);
    le_pushLemma(TO_(f,x1,f,x2,f,x3,f,x4,f,x5,x6),TO_(f,x3,f,x2,f,x5,f,x6,f,x1,x4),Unvergleichbar,TRUE);
    le_pushLemma(TO_(f,x1,f,x2,f,x3,f,x4,f,x5,x6),TO_(f,x3,f,x2,f,x6,f,x4,f,x1,x5),Unvergleichbar,TRUE);
    le_pushLemma(TO_(f,x1,f,x2,f,x3,f,x4,f,x5,x6),TO_(f,x3,f,x2,f,x5,f,x4,f,x6,x1),Unvergleichbar,TRUE);
    le_pushLemma(TO_(f,x1,f,x2,f,x3,f,x4,f,x5,x6),TO_(f,x3,f,x2,f,x5,f,x6,f,x4,x1),Unvergleichbar,TRUE);
    le_pushLemma(TO_(f,x1,f,x2,f,x3,f,x4,f,x5,x6),TO_(f,x3,f,x2,f,x6,f,x5,f,x1,x4),Unvergleichbar,TRUE);
    le_pushLemma(TO_(f,x1,f,x2,f,x3,f,x4,f,x5,x6),TO_(f,x3,f,x2,f,x4,f,x5,f,x1,x6),Unvergleichbar,TRUE);
    le_pushLemma(TO_(f,x1,f,x2,f,x3,f,x4,f,x5,x6),TO_(f,x3,f,x2,f,x5,f,x1,f,x4,x6),Unvergleichbar,TRUE);
    le_pushLemma(TO_(f,x1,f,x2,f,x3,f,x4,f,x5,x6),TO_(f,x3,f,x2,f,x6,f,x1,f,x4,x5),Unvergleichbar,TRUE);
    le_pushLemma(TO_(f,x1,f,x2,f,x3,f,x4,f,x5,x6),TO_(f,x3,f,x2,f,x4,f,x5,f,x6,x1),Unvergleichbar,TRUE);
    le_pushLemma(TO_(f,x1,f,x2,f,x3,f,x4,f,x5,x6),TO_(f,x3,f,x2,f,x5,f,x4,f,x1,x6),Unvergleichbar,TRUE);
    le_pushLemma(TO_(f,x1,f,x2,f,x3,f,x4,f,x5,x6),TO_(f,x6,f,x1,f,x3,f,x2,f,x5,x4),Unvergleichbar,TRUE);
    le_pushLemma(TO_(f,x1,f,x2,f,x3,f,x4,f,x5,x6),TO_(f,x4,f,x6,f,x2,f,x1,f,x5,x3),Unvergleichbar,TRUE);
    le_pushLemma(TO_(f,x1,f,x2,f,x3,f,x4,f,x5,x6),TO_(f,x5,f,x1,f,x3,f,x2,f,x6,x4),Unvergleichbar,TRUE);
    le_pushLemma(TO_(f,x1,f,x2,f,x3,f,x4,f,x5,x6),TO_(f,x6,f,x4,f,x2,f,x1,f,x5,x3),Unvergleichbar,TRUE);
    le_pushLemma(TO_(f,x1,f,x2,f,x3,f,x4,f,x5,x6),TO_(f,x5,f,x4,f,x2,f,x1,f,x6,x3),Unvergleichbar,TRUE);
    le_pushLemma(TO_(f,x1,f,x2,f,x3,f,x4,f,x5,x6),TO_(f,x6,f,x2,f,x4,f,x3,f,x1,x5),Unvergleichbar,TRUE);
    le_pushLemma(TO_(f,x1,f,x2,f,x3,f,x4,f,x5,x6),TO_(f,x5,f,x6,f,x2,f,x1,f,x4,x3),Unvergleichbar,TRUE);
    le_pushLemma(TO_(f,x1,f,x2,f,x3,f,x4,f,x5,x6),TO_(f,x3,f,x1,f,x5,f,x4,f,x2,x6),Unvergleichbar,TRUE);
    le_pushLemma(TO_(f,x1,f,x2,f,x3,f,x4,f,x5,x6),TO_(f,x4,f,x5,f,x2,f,x1,f,x6,x3),Unvergleichbar,TRUE);
    le_pushLemma(TO_(f,x1,f,x2,f,x3,f,x4,f,x5,x6),TO_(f,x6,f,x1,f,x4,f,x3,f,x2,x5),Unvergleichbar,TRUE);
    le_pushLemma(TO_(f,x1,f,x2,f,x3,f,x4,f,x5,x6),TO_(f,x6,f,x5,f,x2,f,x1,f,x4,x3),Unvergleichbar,TRUE);
    le_pushLemma(TO_(f,x1,f,x2,f,x3,f,x4,f,x5,x6),TO_(f,x6,f,x5,f,x1,f,x3,f,x2,x4),Unvergleichbar,TRUE);
    le_pushLemma(TO_(f,x1,f,x2,f,x3,f,x4,f,x5,x6),TO_(f,x6,f,x5,f,x1,f,x3,f,x4,x2),Unvergleichbar,TRUE);
    le_pushLemma(TO_(f,x1,f,x2,f,x3,f,x4,f,x5,x6),TO_(f,x6,f,x5,f,x2,f,x1,f,x3,x4),Unvergleichbar,TRUE);
    le_pushLemma(TO_(f,x1,f,x2,f,x3,f,x4,f,x5,x6),TO_(f,x6,f,x5,f,x1,f,x2,f,x4,x3),Unvergleichbar,TRUE);
    le_pushLemma(TO_(f,x1,f,x2,f,x3,f,x4,f,x5,x6),TO_(f,x6,f,x5,f,x1,f,x2,f,x3,x4),Unvergleichbar,TRUE);
    le_pushLemma(TO_(f,x1,f,x2,f,x3,f,x4,f,x5,x6),TO_(f,x3,f,x2,f,x6,f,x5,f,x4,x1),Unvergleichbar,TRUE);
    le_pushLemma(TO_(f,x1,f,x2,f,x3,f,x4,f,x5,x6),TO_(f,x2,f,x6,f,x5,f,x4,f,x3,x1),Unvergleichbar,TRUE);
    le_pushLemma(TO_(f,x1,f,x2,f,x3,f,x4,f,x5,x6),TO_(f,x5,f,x1,f,x4,f,x3,f,x2,x6),Unvergleichbar,TRUE);
    le_pushLemma(TO_(f,x1,f,x2,f,x3,f,x4,f,x5,x6),TO_(f,x5,f,x4,f,x3,f,x2,f,x1,x6),Unvergleichbar,TRUE);
    le_pushLemma(TO_(f,x1,f,x2,f,x3,f,x4,f,x5,x6),TO_(f,x6,f,x2,f,x5,f,x4,f,x3,x1),Unvergleichbar,TRUE);
    le_pushLemma(TO_(f,x1,f,x2,f,x3,f,x4,f,x5,x6),TO_(f,x6,f,x5,f,x4,f,x3,f,x2,x1),Unvergleichbar,TRUE);
    le_pushLemma(TO_(f,x1,f,x2,f,x3,f,x4,f,x5,x6),TO_(f,x3,f,x1,f,x6,f,x5,f,x4,x2),Unvergleichbar,TRUE);
    le_pushLemma(TO_(f,x1,f,x2,f,x3,f,x4,f,x5,x6),TO_(f,x2,f,x3,f,x6,f,x5,f,x4,x1),Unvergleichbar,TRUE);
    le_pushLemma(TO_(f,x1,f,x2,f,x3,f,x4,f,x5,x6),TO_(f,x2,f,x1,f,x5,f,x4,f,x3,x6),Unvergleichbar,TRUE);
    le_pushLemma(TO_(f,x1,f,x2,f,x3,f,x4,f,x5,x6),TO_(f,x2,f,x1,f,x4,f,x3,f,x6,x5),Unvergleichbar,TRUE);
    le_pushLemma(TO_(f,x1,f,x2,f,x3,f,x4,f,x5,x6),TO_(f,x2,f,x1,f,x3,f,x5,f,x4,x6),Unvergleichbar,TRUE);
    le_pushLemma(TO_(f,x1,f,x2,f,x3,f,x4,f,x5,x6),TO_(f,x2,f,x1,f,x5,f,x6,f,x4,x3),Unvergleichbar,TRUE);
    le_pushLemma(TO_(f,x1,f,x2,f,x3,f,x4,f,x5,x6),TO_(f,x2,f,x1,f,x4,f,x6,f,x5,x3),Unvergleichbar,TRUE);
    le_pushLemma(TO_(f,x1,f,x2,f,x3,f,x4,f,x5,x6),TO_(f,x2,f,x1,f,x3,f,x5,f,x6,x4),Unvergleichbar,TRUE);
    le_pushLemma(TO_(f,x1,f,x2,f,x3,f,x4,f,x5,x6),TO_(f,x2,f,x1,f,x5,f,x3,f,x4,x6),Unvergleichbar,TRUE);
    le_pushLemma(TO_(f,x1,f,x2,f,x3,f,x4,f,x5,x6),TO_(f,x2,f,x1,f,x4,f,x5,f,x6,x3),Unvergleichbar,TRUE);
    le_pushLemma(TO_(f,x1,f,x2,f,x3,f,x4,f,x5,x6),TO_(f,x2,f,x1,f,x5,f,x3,f,x6,x4),Unvergleichbar,TRUE);
    le_pushLemma(TO_(f,x1,f,x2,f,x3,f,x4,f,x5,x6),TO_(f,x2,f,x1,f,x5,f,x4,f,x6,x3),Unvergleichbar,TRUE);
    le_pushLemma(TO_(f,x1,f,x2,f,x3,f,x4,f,x5,x6),TO_(f,x2,f,x1,f,x3,f,x4,f,x6,x5),Unvergleichbar,TRUE);
    le_pushLemma(TO_(f,x1,f,x2,f,x3,f,x4,f,x5,x6),TO_(f,x2,f,x1,f,x5,f,x6,f,x3,x4),Unvergleichbar,TRUE);
    le_pushLemma(TO_(f,x1,f,x2,f,x3,f,x4,f,x5,x6),TO_(f,x2,f,x1,f,x3,f,x6,f,x5,x4),Unvergleichbar,TRUE);
    le_pushLemma(TO_(f,x1,f,x2,f,x3,f,x4,f,x5,x6),TO_(f,x6,f,x5,f,x1,f,x4,f,x3,x2),Unvergleichbar,TRUE);
    le_pushLemma(TO_(f,x1,f,x2,f,x3,f,x4,f,x5,x6),TO_(f,x3,f,x2,f,x4,f,x1,f,x6,x5),Unvergleichbar,TRUE);
    le_pushLemma(TO_(f,x1,f,x2,f,x3,f,x4,f,x5,x6),TO_(f,x4,f,x6,f,x1,f,x5,f,x3,x2),Unvergleichbar,TRUE);
    le_pushLemma(TO_(f,x1,f,x2,f,x3,f,x4,f,x5,x6),TO_(f,x2,f,x6,f,x3,f,x1,f,x5,x4),Unvergleichbar,TRUE);
    le_pushLemma(TO_(f,x1,f,x2,f,x3,f,x4,f,x5,x6),TO_(f,x6,f,x4,f,x1,f,x5,f,x3,x2),Unvergleichbar,TRUE);
    le_pushLemma(TO_(f,x1,f,x2,f,x3,f,x4,f,x5,x6),TO_(f,x5,f,x4,f,x1,f,x6,f,x3,x2),Unvergleichbar,TRUE);
    le_pushLemma(TO_(f,x1,f,x2,f,x3,f,x4,f,x5,x6),TO_(f,x6,f,x2,f,x3,f,x1,f,x5,x4),Unvergleichbar,TRUE);
    le_pushLemma(TO_(f,x1,f,x2,f,x3,f,x4,f,x5,x6),TO_(f,x5,f,x6,f,x1,f,x4,f,x3,x2),Unvergleichbar,TRUE);
    le_pushLemma(TO_(f,x1,f,x2,f,x3,f,x4,f,x5,x6),TO_(f,x3,f,x1,f,x4,f,x2,f,x6,x5),Unvergleichbar,TRUE);
    le_pushLemma(TO_(f,x1,f,x2,f,x3,f,x4,f,x5,x6),TO_(f,x2,f,x3,f,x4,f,x1,f,x6,x5),Unvergleichbar,TRUE);
    le_pushLemma(TO_(f,x1,f,x2,f,x3,f,x4,f,x5,x6),TO_(f,x4,f,x5,f,x1,f,x6,f,x3,x2),Unvergleichbar,TRUE);
    le_pushLemma(TO_(f,x1,f,x2,f,x3,f,x4,f,x5,x6),TO_(f,x4,f,x5,f,x6,f,x1,f,x2,x3),Unvergleichbar,TRUE);
    le_pushLemma(TO_(f,x1,f,x2,f,x3,f,x4,f,x5,x6),TO_(f,x4,f,x5,f,x6,f,x2,f,x1,x3),Unvergleichbar,TRUE);
    le_pushLemma(TO_(f,x1,f,x2,f,x3,f,x4,f,x5,x6),TO_(f,x4,f,x5,f,x6,f,x2,f,x3,x1),Unvergleichbar,TRUE);
    le_pushLemma(TO_(f,x1,f,x2,f,x3,f,x4,f,x5,x6),TO_(f,x2,f,x5,f,x6,f,x1,f,x3,x4),Unvergleichbar,TRUE);
    le_pushLemma(TO_(f,x1,f,x2,f,x3,f,x4,f,x5,x6),TO_(f,x2,f,x4,f,x5,f,x1,f,x6,x3),Unvergleichbar,TRUE);
    le_pushLemma(TO_(f,x1,f,x2,f,x3,f,x4,f,x5,x6),TO_(f,x3,f,x5,f,x6,f,x2,f,x1,x4),Unvergleichbar,TRUE);
    le_pushLemma(TO_(f,x1,f,x2,f,x3,f,x4,f,x5,x6),TO_(f,x2,f,x4,f,x5,f,x6,f,x1,x3),Unvergleichbar,TRUE);
    le_pushLemma(TO_(f,x1,f,x2,f,x3,f,x4,f,x5,x6),TO_(f,x6,f,x4,f,x5,f,x2,f,x1,x3),Unvergleichbar,TRUE);
    le_pushLemma(TO_(f,x1,f,x2,f,x3,f,x4,f,x5,x6),TO_(f,x3,f,x5,f,x6,f,x1,f,x2,x4),Unvergleichbar,TRUE);
    le_pushLemma(TO_(f,x1,f,x2,f,x3,f,x4,f,x5,x6),TO_(f,x4,f,x2,f,x3,f,x5,f,x6,x1),Unvergleichbar,TRUE);
    le_pushLemma(TO_(f,x1,f,x2,f,x3,f,x4,f,x5,x6),TO_(f,x5,f,x6,f,x1,f,x4,f,x2,x3),Unvergleichbar,TRUE);
    le_pushLemma(TO_(f,x1,f,x2,f,x3,f,x4,f,x5,x6),TO_(f,x5,f,x2,f,x3,f,x4,f,x1,x6),Unvergleichbar,TRUE);
    le_pushLemma(TO_(f,x1,f,x2,f,x3,f,x4,f,x5,x6),TO_(f,x3,f,x6,f,x1,f,x2,f,x4,x5),Unvergleichbar,TRUE);
    le_pushLemma(TO_(f,x1,f,x2,f,x3,f,x4,f,x5,x6),TO_(f,x3,f,x4,f,x2,f,x1,f,x6,x5),Unvergleichbar,TRUE);
    le_pushLemma(TO_(f,x1,f,x2,f,x3,f,x4,f,x5,x6),TO_(f,x3,f,x4,f,x1,f,x6,f,x5,x2),Unvergleichbar,TRUE);
    le_pushLemma(TO_(f,x1,f,x2,f,x3,f,x4,f,x5,x6),TO_(f,x3,f,x4,f,x2,f,x5,f,x1,x6),Unvergleichbar,TRUE);
    le_pushLemma(TO_(f,x1,f,x2,f,x3,f,x4,f,x5,x6),TO_(f,x3,f,x4,f,x1,f,x5,f,x2,x6),Unvergleichbar,TRUE);
    le_pushLemma(TO_(f,x1,f,x2,f,x3,f,x4,f,x5,x6),TO_(f,x3,f,x4,f,x2,f,x6,f,x1,x5),Unvergleichbar,TRUE);
    le_pushLemma(TO_(f,x1,f,x2,f,x3,f,x4,f,x5,x6),TO_(f,x3,f,x4,f,x2,f,x6,f,x5,x1),Unvergleichbar,TRUE);
    le_pushLemma(TO_(f,x1,f,x2,f,x3,f,x4,f,x5,x6),TO_(f,x3,f,x4,f,x1,f,x6,f,x2,x5),Unvergleichbar,TRUE);
    le_pushLemma(TO_(f,x1,f,x2,f,x3,f,x4,f,x5,x6),TO_(f,x3,f,x4,f,x1,f,x2,f,x6,x5),Unvergleichbar,TRUE);
    le_pushLemma(TO_(f,x1,f,x2,f,x3,f,x4,f,x5,x6),TO_(f,x3,f,x4,f,x2,f,x5,f,x6,x1),Unvergleichbar,TRUE);
    le_pushLemma(TO_(f,x1,f,x2,f,x3,f,x4,f,x5,x6),TO_(f,x3,f,x6,f,x2,f,x1,f,x4,x5),Unvergleichbar,TRUE);
    le_pushLemma(TO_(f,x1,f,x2,f,x3,f,x4,f,x5,x6),TO_(f,x6,f,x4,f,x1,f,x5,f,x2,x3),Unvergleichbar,TRUE);
    le_pushLemma(TO_(f,x1,f,x2,f,x3,f,x4,f,x5,x6),TO_(f,x2,f,x6,f,x3,f,x1,f,x4,x5),Unvergleichbar,TRUE);
    le_pushLemma(TO_(f,x1,f,x2,f,x3,f,x4,f,x5,x6),TO_(f,x3,f,x1,f,x2,f,x6,f,x5,x4),Unvergleichbar,TRUE);
    le_pushLemma(TO_(f,x1,f,x2,f,x3,f,x4,f,x5,x6),TO_(f,x3,f,x6,f,x1,f,x4,f,x5,x2),Unvergleichbar,TRUE);
    le_pushLemma(TO_(f,x1,f,x2,f,x3,f,x4,f,x5,x6),TO_(f,x3,f,x6,f,x2,f,x5,f,x1,x4),Unvergleichbar,TRUE);
    le_pushLemma(TO_(f,x1,f,x2,f,x3,f,x4,f,x5,x6),TO_(f,x3,f,x6,f,x1,f,x5,f,x2,x4),Unvergleichbar,TRUE);
    le_pushLemma(TO_(f,x1,f,x2,f,x3,f,x4,f,x5,x6),TO_(f,x3,f,x6,f,x2,f,x4,f,x1,x5),Unvergleichbar,TRUE);
    le_pushLemma(TO_(f,x1,f,x2,f,x3,f,x4,f,x5,x6),TO_(f,x3,f,x6,f,x2,f,x4,f,x5,x1),Unvergleichbar,TRUE);
    le_pushLemma(TO_(f,x1,f,x2,f,x3,f,x4,f,x5,x6),TO_(f,x3,f,x6,f,x1,f,x4,f,x2,x5),Unvergleichbar,TRUE);
    le_pushLemma(TO_(f,x1,f,x2,f,x3,f,x4,f,x5,x6),TO_(f,x3,f,x6,f,x2,f,x1,f,x5,x4),Unvergleichbar,TRUE);
    le_pushLemma(TO_(f,x1,f,x2,f,x3,f,x4,f,x5,x6),TO_(f,x3,f,x6,f,x1,f,x5,f,x4,x2),Unvergleichbar,TRUE);
    le_pushLemma(TO_(f,x1,f,x2,f,x3,f,x4,f,x5,x6),TO_(f,x3,f,x6,f,x2,f,x5,f,x4,x1),Unvergleichbar,TRUE);
    le_pushLemma(TO_(f,x1,f,x2,f,x3,f,x4,f,x5,x6),TO_(f,x2,f,x1,f,x6,f,x5,f,x4,x3),Unvergleichbar,TRUE);
    le_pushLemma(TO_(f,x1,f,x2,f,x3,f,x4,f,x5,x6),TO_(f,x3,f,x5,f,x2,f,x1,f,x4,x6),Unvergleichbar,TRUE);
    le_pushLemma(TO_(f,x1,f,x2,f,x3,f,x4,f,x5,x6),TO_(f,x3,f,x5,f,x2,f,x1,f,x6,x4),Unvergleichbar,TRUE);
    le_pushLemma(TO_(f,x1,f,x2,f,x3,f,x4,f,x5,x6),TO_(f,x3,f,x1,f,x2,f,x5,f,x4,x6),Unvergleichbar,TRUE);
    le_pushLemma(TO_(f,x1,f,x2,f,x3,f,x4,f,x5,x6),TO_(f,x3,f,x5,f,x6,f,x4,f,x2,x1),Unvergleichbar,TRUE);
    le_pushLemma(TO_(f,x1,f,x2,f,x3,f,x4,f,x5,x6),TO_(f,x3,f,x5,f,x2,f,x4,f,x1,x6),Unvergleichbar,TRUE);
    le_pushLemma(TO_(f,x1,f,x2,f,x3,f,x4,f,x5,x6),TO_(f,x3,f,x5,f,x1,f,x4,f,x2,x6),Unvergleichbar,TRUE);
    le_pushLemma(TO_(f,x1,f,x2,f,x3,f,x4,f,x5,x6),TO_(f,x3,f,x5,f,x2,f,x6,f,x1,x4),Unvergleichbar,TRUE);
    le_pushLemma(TO_(f,x1,f,x2,f,x3,f,x4,f,x5,x6),TO_(f,x3,f,x5,f,x2,f,x6,f,x4,x1),Unvergleichbar,TRUE);
    le_pushLemma(TO_(f,x1,f,x2,f,x3,f,x4,f,x5,x6),TO_(f,x3,f,x5,f,x1,f,x6,f,x2,x4),Unvergleichbar,TRUE);
    le_pushLemma(TO_(f,x1,f,x2,f,x3,f,x4,f,x5,x6),TO_(f,x3,f,x5,f,x6,f,x1,f,x4,x2),Unvergleichbar,TRUE);
    le_pushLemma(TO_(f,x1,f,x2,f,x3,f,x4,f,x5,x6),TO_(f,x3,f,x5,f,x2,f,x4,f,x6,x1),Unvergleichbar,TRUE);
    le_pushLemma(TO_(f,x1,f,x2,f,x3,f,x4,f,x5,x6),TO_(f,x5,f,x4,f,x6,f,x2,f,x1,x3),Unvergleichbar,TRUE);
    le_pushLemma(TO_(f,x1,f,x2,f,x3,f,x4,f,x5,x6),TO_(f,x6,f,x3,f,x2,f,x4,f,x5,x1),Unvergleichbar,TRUE);
    le_pushLemma(TO_(f,x1,f,x2,f,x3,f,x4,f,x5,x6),TO_(f,x6,f,x4,f,x5,f,x1,f,x3,x2),Unvergleichbar,TRUE);
    le_pushLemma(TO_(f,x1,f,x2,f,x3,f,x4,f,x5,x6),TO_(f,x3,f,x1,f,x2,f,x6,f,x4,x5),Unvergleichbar,TRUE);
    le_pushLemma(TO_(f,x1,f,x2,f,x3,f,x4,f,x5,x6),TO_(f,x6,f,x4,f,x5,f,x2,f,x3,x1),Unvergleichbar,TRUE);
    le_pushLemma(TO_(f,x1,f,x2,f,x3,f,x4,f,x5,x6),TO_(f,x2,f,x1,f,x6,f,x4,f,x5,x3),Unvergleichbar,TRUE);
    le_pushLemma(TO_(f,x1,f,x2,f,x3,f,x4,f,x5,x6),TO_(f,x3,f,x1,f,x2,f,x5,f,x6,x4),Unvergleichbar,TRUE);
    le_pushLemma(TO_(f,x1,f,x2,f,x3,f,x4,f,x5,x6),TO_(f,x2,f,x5,f,x6,f,x4,f,x1,x3),Unvergleichbar,TRUE);
    le_pushLemma(TO_(f,x1,f,x2,f,x3,f,x4,f,x5,x6),TO_(f,x4,f,x1,f,x3,f,x2,f,x6,x5),Unvergleichbar,TRUE);
    le_pushLemma(TO_(f,x1,f,x2,f,x3,f,x4,f,x5,x6),TO_(f,x4,f,x1,f,x5,f,x3,f,x2,x6),Unvergleichbar,TRUE);
    le_pushLemma(TO_(f,x1,f,x2,f,x3,f,x4,f,x5,x6),TO_(f,x4,f,x1,f,x6,f,x2,f,x5,x3),Unvergleichbar,TRUE);
    le_pushLemma(TO_(f,x1,f,x2,f,x3,f,x4,f,x5,x6),TO_(f,x4,f,x1,f,x5,f,x2,f,x6,x3),Unvergleichbar,TRUE);
    le_pushLemma(TO_(f,x1,f,x2,f,x3,f,x4,f,x5,x6),TO_(f,x4,f,x1,f,x3,f,x6,f,x2,x5),Unvergleichbar,TRUE);
    le_pushLemma(TO_(f,x1,f,x2,f,x3,f,x4,f,x5,x6),TO_(f,x4,f,x1,f,x6,f,x3,f,x2,x5),Unvergleichbar,TRUE);
    le_pushLemma(TO_(f,x1,f,x2,f,x3,f,x4,f,x5,x6),TO_(f,x4,f,x1,f,x6,f,x5,f,x2,x3),Unvergleichbar,TRUE);
    le_pushLemma(TO_(f,x1,f,x2,f,x3,f,x4,f,x5,x6),TO_(f,x4,f,x1,f,x3,f,x5,f,x2,x6),Unvergleichbar,TRUE);
    le_pushLemma(TO_(f,x1,f,x2,f,x3,f,x4,f,x5,x6),TO_(f,x4,f,x1,f,x5,f,x2,f,x3,x6),Unvergleichbar,TRUE);
    le_pushLemma(TO_(f,x1,f,x2,f,x3,f,x4,f,x5,x6),TO_(f,x4,f,x2,f,x3,f,x5,f,x1,x6),Unvergleichbar,TRUE);
    le_pushLemma(TO_(f,x1,f,x2,f,x3,f,x4,f,x5,x6),TO_(f,x6,f,x3,f,x2,f,x4,f,x1,x5),Unvergleichbar,TRUE);
    le_pushLemma(TO_(f,x1,f,x2,f,x3,f,x4,f,x5,x6),TO_(f,x2,f,x4,f,x6,f,x5,f,x1,x3),Unvergleichbar,TRUE);
    le_pushLemma(TO_(f,x1,f,x2,f,x3,f,x4,f,x5,x6),TO_(f,x2,f,x3,f,x1,f,x4,f,x6,x5),Unvergleichbar,TRUE);
    le_pushLemma(TO_(f,x1,f,x2,f,x3,f,x4,f,x5,x6),TO_(f,x4,f,x2,f,x6,f,x5,f,x1,x3),Unvergleichbar,TRUE);
    le_pushLemma(TO_(f,x1,f,x2,f,x3,f,x4,f,x5,x6),TO_(f,x5,f,x2,f,x4,f,x6,f,x1,x3),Unvergleichbar,TRUE);
    le_pushLemma(TO_(f,x1,f,x2,f,x3,f,x4,f,x5,x6),TO_(f,x5,f,x2,f,x6,f,x4,f,x1,x3),Unvergleichbar,TRUE);
    le_pushLemma(TO_(f,x1,f,x2,f,x3,f,x4,f,x5,x6),TO_(f,x4,f,x2,f,x5,f,x6,f,x1,x3),Unvergleichbar,TRUE);
    le_pushLemma(TO_(f,x1,f,x2,f,x3,f,x4,f,x5,x6),TO_(f,x6,f,x3,f,x2,f,x1,f,x5,x4),Unvergleichbar,TRUE);
    le_pushLemma(TO_(f,x1,f,x2,f,x3,f,x4,f,x5,x6),TO_(f,x6,f,x3,f,x2,f,x5,f,x1,x4),Unvergleichbar,TRUE);
    le_pushLemma(TO_(f,x1,f,x2,f,x3,f,x4,f,x5,x6),TO_(f,x6,f,x3,f,x2,f,x5,f,x4,x1),Unvergleichbar,TRUE);
    le_pushLemma(TO_(f,x1,f,x2,f,x3,f,x4,f,x5,x6),TO_(f,x6,f,x3,f,x2,f,x1,f,x4,x5),Unvergleichbar,TRUE);
    le_pushLemma(TO_(f,x1,f,x2,f,x3,f,x4,f,x5,x6),TO_(f,x4,f,x2,f,x5,f,x1,f,x6,x3),Unvergleichbar,TRUE);
    le_pushLemma(TO_(f,x1,f,x2,f,x3,f,x4,f,x5,x6),TO_(f,x4,f,x5,f,x6,f,x1,f,x3,x2),Unvergleichbar,TRUE);
    le_pushLemma(TO_(f,x1,f,x2,f,x3,f,x4,f,x5,x6),TO_(f,x5,f,x2,f,x4,f,x1,f,x6,x3),Unvergleichbar,TRUE);
    le_pushLemma(TO_(f,x1,f,x2,f,x3,f,x4,f,x5,x6),TO_(f,x4,f,x3,f,x2,f,x1,f,x6,x5),Unvergleichbar,TRUE);
    le_pushLemma(TO_(f,x1,f,x2,f,x3,f,x4,f,x5,x6),TO_(f,x4,f,x3,f,x2,f,x6,f,x1,x5),Unvergleichbar,TRUE);
    le_pushLemma(TO_(f,x1,f,x2,f,x3,f,x4,f,x5,x6),TO_(f,x4,f,x3,f,x2,f,x5,f,x1,x6),Unvergleichbar,TRUE);
    le_pushLemma(TO_(f,x1,f,x2,f,x3,f,x4,f,x5,x6),TO_(f,x5,f,x2,f,x3,f,x1,f,x6,x4),Unvergleichbar,TRUE);
    le_pushLemma(TO_(f,x1,f,x2,f,x3,f,x4,f,x5,x6),TO_(f,x5,f,x2,f,x3,f,x6,f,x1,x4),Unvergleichbar,TRUE);
    le_pushLemma(TO_(f,x1,f,x2,f,x3,f,x4,f,x5,x6),TO_(f,x4,f,x2,f,x5,f,x1,f,x3,x6),Unvergleichbar,TRUE);
    le_pushLemma(TO_(f,x1,f,x2,f,x3,f,x4,f,x5,x6),TO_(f,x3,f,x2,f,x1,f,x5,f,x4,x6),Unvergleichbar,TRUE);
    le_pushLemma(TO_(f,x1,f,x2,f,x3,f,x4,f,x5,x6),TO_(f,x3,f,x2,f,x1,f,x5,f,x6,x4),Unvergleichbar,TRUE);
    le_pushLemma(TO_(f,x1,f,x2,f,x3,f,x4,f,x5,x6),TO_(f,x5,f,x3,f,x2,f,x6,f,x1,x4),Unvergleichbar,TRUE);
    le_pushLemma(TO_(f,x1,f,x2,f,x3,f,x4,f,x5,x6),TO_(f,x4,f,x2,f,x5,f,x3,f,x1,x6),Unvergleichbar,TRUE);
    le_pushLemma(TO_(f,x1,f,x2,f,x3,f,x4,f,x5,x6),TO_(f,x5,f,x3,f,x2,f,x4,f,x1,x6),Unvergleichbar,TRUE);
    le_pushLemma(TO_(f,x1,f,x2,f,x3,f,x4,f,x5,x6),TO_(f,x4,f,x2,f,x6,f,x1,f,x5,x3),Unvergleichbar,TRUE);
    le_pushLemma(TO_(f,x1,f,x2,f,x3,f,x4,f,x5,x6),TO_(f,x4,f,x2,f,x3,f,x1,f,x6,x5),Unvergleichbar,TRUE);
    le_pushLemma(TO_(f,x1,f,x2,f,x3,f,x4,f,x5,x6),TO_(f,x2,f,x5,f,x4,f,x6,f,x1,x3),Unvergleichbar,TRUE);
    le_pushLemma(TO_(f,x1,f,x2,f,x3,f,x4,f,x5,x6),TO_(f,x3,f,x2,f,x1,f,x6,f,x5,x4),Unvergleichbar,TRUE);
    le_pushLemma(TO_(f,x1,f,x2,f,x3,f,x4,f,x5,x6),TO_(f,x5,f,x4,f,x6,f,x1,f,x3,x2),Unvergleichbar,TRUE);
    le_pushLemma(TO_(f,x1,f,x2,f,x3,f,x4,f,x5,x6),TO_(f,x5,f,x2,f,x4,f,x3,f,x1,x6),Unvergleichbar,TRUE);
    le_pushLemma(TO_(f,x1,f,x2,f,x3,f,x4,f,x5,x6),TO_(f,x3,f,x2,f,x1,f,x4,f,x6,x5),Unvergleichbar,TRUE);
    le_pushLemma(TO_(f,x1,f,x2,f,x3,f,x4,f,x5,x6),TO_(f,x4,f,x6,f,x5,f,x1,f,x3,x2),Unvergleichbar,TRUE);
  }
}

static void le_fillLemata(LemmaT typ, SymbolT f)
{
  assert(le_BufEmpty());
  switch (typ){
  case C_Lemma:
    le_fillC_Lemma(f);
    break;
  case IdempotenzLemmata:
    le_fillIdempotenzLemmata(f);
    break;
  case PermLemmata:
    le_fillPermLemmata(f);
    break;
  case FullPermLemmata:
    le_fillFullPermLemmata4(f);
    if (PA_FullPermLemmata() > 4){
      le_fillFullPermLemmata5(f);
    }
    if (PA_FullPermLemmata() > 5){
      le_fillFullPermLemmata6(f);
    }
    break;
  default:
    our_error("Unknown lemma family!");
  }
}

/* ------------------------------------------------------------------------- */
/* -- interface to outside ------------------------------------------------- */
/* ------------------------------------------------------------------------- */

void LE_LemmataAktivieren(LemmaT typ, SymbolT f)
/* aktiviert fuer f die entsprechenden Lemmata. 
 * Es koenenn mehrere Lemmatafamilien aktiviert werden, 
 * diese werden chronologisch abgearbeitet.
 */
{
  if (le_BufEmpty()){
    le_fillLemata(typ,f);
  }
  else {
    le_pushInput(typ,f);
  }
}

BOOLEAN LE_getLemma(SelectRecT *selRecP)
/* Falls noch ein Lemma einer aktivierten Familie ansteht, 
 * liefert das naechste in selRec zurueck, Rueckgabewert TRUE.
 * Ansonsten FALSE.
 */
{
  if (le_BufEmpty()){
    return FALSE;
  }
  selRecP->actualParent = 0;
  selRecP->otherParent  = 0;
  selRecP->tp           = NULL;
  selRecP->wasFIFO      = FALSE;
  { 
    WId wid = {0,{0,1,0}};
    selRecP->wid = wid;
  }
  le_popLemma(&(selRecP->lhs),&(selRecP->rhs),
              &(selRecP->result),&(selRecP->istGZFB));
  if (le_BufEmpty() && !le_InputQueueEmpty()){
    LemmaT typ; 
    SymbolT f;
    le_popInput(&typ,&f);
    le_fillLemata(typ,f);
  }
  return TRUE;
}

void LE_SoftCleanup(void)
/* Raeumt die internen Strukturen ab, fuer Strategiewechsel */
{
  le_clearInputQueue();
  le_clearBuf();
}

void LE_Cleanup(void)
/* Raeumt die internen Strukturen ab und gibt sie wieder frei, 
 * fuer Programmende */
{
  le_freeInputQueue();
  le_freeBuf();
}

/* ------------------------------------------------------------------------- */
/* -- E O F ---------------------------------------------------------------- */
/* ------------------------------------------------------------------------- */
