/* Constraints.c
 * 07.12.2000
 * Bernd Loechner
 */


#include "Constraints.h"
#include "Ausgaben.h"
#include "SpeicherVerwaltung.h"
#include "DSBaumOperationen.h"
#include "MatchOperationen.h"
#include "NFBildung.h"
#include "Ordnungen.h"
#include "Praezedenz.h"
#include "Subsumption.h"
#include <sys/time.h>
#include "compiler-extra.h"

#ifndef CCE_Source
#include "Parameter.h"
#else
#include "parse-util.h"
#endif

/* -------------------------------------------------------------------------- */

#define NewestVar ((-10 < 2 * BO_NeuesteBaumvariable) ? -10 : \
                   2 * BO_NeuesteBaumvariable)

/* -------------------------------------------------------------------------- */
/* - parameterization ------------------------------------------------------- */
/* -------------------------------------------------------------------------- */

#define DO_TRACE          0 /* 0, 1 */
#define TRACE_GUARD   CO_traceGuard
static BOOLEAN CO_traceGuard = FALSE;

#include "Trace.h"

#define DEC_TRACE   0 /* 0, 1*/ /* Traces nur fuer decompose aufrufe */

#define ZM_TIME_MEASURE   0 /* 0,1 */
#include "ZeitMessung.h"

#define DO_OVERLAP_CONSTRAINTS 1 /* 0, 1 */
#define DO_PURE_LPO_FIRST 0 /* 0, 1 */
#define MEHR_BRI          0 /* 0, 1 */
#define SORTIERE_E        0 /* 0, 1 */
#define IMPLIED_ORD       1 /* 0, 1, 2 */
#define ECONS_STAT        0 /* 0, 1 */
#define DO_HASHING        0 /* 0, 1 */
#define VERBOSE           0 /* 0, 1 */
#define VERBOSE_TREE      0 /* 0, 1 */
#define TIME_MEASURE      0 /* ... */
#define LENGTH_LIMITED    0 /* 0, 1 */
#define LIMIT_DEPTH       1 /* 0, 1 */
#define TRY_ALL_EQNS      0 /* 0, 1 */
#define GIVE_UP_BY_LENGTH_LIMIT 1 /* 0, 1 */

/* Einschraenkungen zwecks Vermeiden zu langer Wartereien... */
#define ALLOWED_DECOMPOSES      0 /* muss noch ueberlegt werden !!! */
#define ALLOWED_HANDLE_NODES    0
#define DEPTH_LIMIT            2000
#define MAX_LENGTH              5

/* Achtung: die HSIZEs muessen Zweierpotenzen sein! */
#define HSIZE0 (1 << 16)
#define HSIZE1 (1 << 13)

/* -------------------------------------------------------------------------- */
/* - file local types ------------------------------------------------------- */
/* -------------------------------------------------------------------------- */

typedef struct {
  UTermT s,t;
  int res;
  unsigned long gen;
} HashStruct, *Hashtable;

typedef enum {bottom,redundant,important} BRI;
/* Extension of VergleichsT : */
#define NOT_FOUND 5
#define NOT_GTEQ 6
#define NOT_LTEQ 7
#define UNKNOWN  8
#define IS_PROPER_RES(r) (r <= Unvergleichbar)

/* -------------------------------------------------------------------------- */
/* - file static variables -------------------------------------------------- */
/* -------------------------------------------------------------------------- */

static long gen0; 
static long rein, erfraus, nerfraus; 
static HashStruct H0 [HSIZE0], H1[HSIZE1];
static SV_NestedSpeicherManagerT EConsManager;
static SV_SpeicherManagerT ContextManager;
static unsigned long contextNumber = 0;
static SV_NestedSpeicherManagerT DConsManager;
static BOOLEAN *rel             = NULL;
static unsigned int varrelsize  = 0;
static DConsT lsgen             = NULL;
static unsigned int anzlsgn     = 0;
static unsigned int maxlsgn     = 0;
static unsigned int length_limit = 0;
static BOOLEAN abgebrochen      = FALSE;
static decModeT dec_mode        = Abbrechen;
static decModeT tree_mode       = Abbrechen;
static unsigned int number0     = 0;
static unsigned int number1     = 0;
static unsigned int number2     = 0;
static ContextT currentContext  = NULL;
static unsigned int performedDecomposes  = 0;
static unsigned int performedHandleNodes = 0;
static int blabla = 0;

static char *CNamen   [] = {":>:", ":=:", ":>="};
static char *BRINamen [] = {"bottom", "redundant", "important"};

/* -------------------------------------------------------------------------- */
/* - forward declarations --------------------------------------------------- */
/* -------------------------------------------------------------------------- */

static VergleichsT cLPOCMP (ContextT c, UTermT s, UTermT t);
static VergleichsT cLPOCMPGTEQ (ContextT c, UTermT s, UTermT t);
static BOOLEAN cVarEnthalten (ContextT c, SymbolT x, UTermT t);
static BOOLEAN decompose (EConsT e, ContextT c);


/* -------------------------------------------------------------------------- */
/* - hashing ---------------------------------------------------------------- */
/* -------------------------------------------------------------------------- */
/* Hier wird ein einfaches doppeltes Hashing realisiert. Es muss noch eine
 * genauere Analyse folgen, ggf. ist das hashing zu aendern.
 */

#define PRIME1 13
#define PRIME2 17
#define PRIME3 19
#define PRIME4 7
#define DIVI 4 /*GroesseTermzellenT*/ /* spart 2 Sekunden ! */
#define MAX_COLL 30

void CO_DruckeVergleichStrom(FILE *strom, VergleichsT vgl)
{
  switch (vgl){
  case Gleich:         fprintf(strom, "Gleich");         break;
  case Unvergleichbar: fprintf(strom, "Unvergleichbar"); break;
  case Groesser:       fprintf(strom, "Groesser");       break;
  case Kleiner:        fprintf(strom, "Kleiner");        break;
  case NOT_GTEQ:       fprintf(strom, "NOT_GTEQ");       break;
  case NOT_LTEQ:       fprintf(strom, "NOT_LTEQ");       break;
  case NOT_FOUND:      fprintf(strom, "NOT_FOUND");      break;
  case UNKNOWN:        fprintf(strom, "UNKNOWN");        break;
  default:             fprintf(strom, "??VGL?? %d", vgl);        break;
  }
}

static int flipVergleich (VergleichsT vgl)
{
  switch (vgl){
  case Groesser:       return Kleiner;
  case Kleiner:        return Groesser;
  case NOT_GTEQ:       return NOT_LTEQ;
  case NOT_LTEQ:       return NOT_GTEQ;
  case Gleich:
  case Unvergleichbar: 
  default:             return vgl;
  }
}

static int hash1(UTermT s, UTermT t)
{
  return (((unsigned long)s*PRIME1 + (unsigned long)t*PRIME2)/DIVI);
}

static int hash2(UTermT s, UTermT t)
{
  return 2099; /* drastisch besser! */
  return (((unsigned long)s * PRIME3 + (unsigned long)t * PRIME4)/DIVI);
}

static int getHash1(Hashtable H, int hsize, unsigned long gen, 
                    UTermT s, UTermT t)
{
  int i;
  int coll = 0;
  i = hash1(s,t) & (hsize - 1);
  if ((H[i].gen == gen)&&((H[i].s != s) || (H[i].t != t))){
    int i2;
    i2 = hash2(s,t);
    do {
      coll++;
      i = (i+i2) & (hsize - 1);
    } while ((coll < MAX_COLL) && (H[i].gen == gen) &&
	     ((H[i].s != s) || (H[i].t != t)));

    if (coll == MAX_COLL){
        IO_DruckeFlex("Hash Kollision[%d]R: (%X,%X)\n",gen,s,t);
        nerfraus++;
	return NOT_FOUND;
    }
  }
  if ((H[i].gen == gen) && (H[i].s == s) && (H[i].t == t)){
    erfraus++;
    return H[i].res;
  }
  nerfraus++;
  return NOT_FOUND;
}

static int getHash(ContextT c, UTermT s, UTermT t)
{
#if DO_HASHING
  int res;
  if ((unsigned int) s > (unsigned int) t){
    res = flipVergleich(getHash1(H0,HSIZE0,gen0,t,s));
    if ((c != NULL) && ((res == NOT_FOUND)||(res == Unvergleichbar))){
      res = flipVergleich(getHash1(H1,HSIZE1,c->id,t,s));
    }
  }
  else {
    res = getHash1(H0,HSIZE0,gen0,s,t);
    if ((c != NULL) && ((res == NOT_FOUND)||(res == Unvergleichbar))){
      res = getHash1(H1,HSIZE1,c->id,s,t);
    }
  }
  RESTRACE (("getHash(%t,%t) -> %v\n", s, t, res)); 
  return res;
#else
  return NOT_FOUND;
#endif
}

static void setHash1(Hashtable H, int hsize, unsigned long gen, 
                     UTermT s, UTermT t, int res)
{
  int i;
  int coll = 0;
  /*  IO_DruckeFlex("Eintragen: [%d] (%X,%X)->%d\n",gen0, s,t,res);*/
  
  rein++;
  i = hash1(s,t) & (hsize - 1);
  if (H[i].gen == gen){
    int i2;
    if ((H[i].s == s) && (H[i].t == t)){
      if (IS_PROPER_RES(H[i].res) && !((H == H0) && (res == Unvergleichbar))){
        IO_DruckeFlex("Doppeleintrag: (%X,%X) vs (%X,%X) == %d\n",
                      s,t,H[i].s,H[i].t,i);   
        return;
      }
      else {
        H[i].res = res;
      }
    }   

    i2 = hash2(s,t);
    do {
      coll++;
      i = (i+i2) & (hsize - 1);
    } while ((coll < MAX_COLL) && (H[i].gen == gen) &&
	     ((H[i].s != s) || (H[i].t != t)));

    if (coll == MAX_COLL){
        IO_DruckeFlex("Hash Kollision[%d]: (%X,%X) %d\n",gen,s,t,rein);
	return;
    }
    else if ((H[i].gen == gen) && (H[i].s == s) && (H[i].t == t)){
      if (IS_PROPER_RES(H[i].res) && !((H == H0) && (res == Unvergleichbar))){
        IO_DruckeFlex("Doppeleintrag: (%X,%X) vs (%X,%X) == %d\n",
                      s,t,H[i].s,H[i].t,i);   
        return;
      }
      else {
        H[i].res = res;
      }
    }
  }
  H[i].s = s;
  H[i].t = t;
  H[i].gen = gen;
  H[i].res = res;
}

static void setHash(ContextT c, UTermT s, UTermT t, int res)
{
#if DO_HASHING
  RESTRACE (("setHash(%t,%t) -> %v (%X,%X)\n", s, t, res,s,t)); 
  if ((unsigned int) s > (unsigned int) t){
    if ((c == NULL) || (res = Unvergleichbar)){
      setHash1(H0,HSIZE0,gen0,t,s,flipVergleich(res));
    }
    else {
      setHash1(H1,HSIZE1,c->id,t,s,flipVergleich(res));
    }
  }
  else {
    if ((c == NULL) || (res = Unvergleichbar)){
      setHash1(H0,HSIZE0,gen0,s,t,res);
    }
    else {
      setHash1(H1,HSIZE1,c->id,s,t,res);
    }
  }
#else
  return;
#endif
}

static void initHash(void){
  /*  printf("Generation: %d: %d %d %d\n", gen0, rein, erfraus, nerfraus);*/
  gen0++;
  rein = 0; 
  erfraus = 0; 
  nerfraus = 0;
  if (gen0 == 0){
    memset(H0, sizeof(H0), 0);
    gen0++;
  }
}

/* -------------------------------------------------------------------------- */
/* - elementare constraints ------------------------------------------------- */
/* -------------------------------------------------------------------------- */

/* EConsT werden ueber einen NestedManager verwaltet. */

static void EConsManagerInitialisieren (void)
{
  SV_NestedManagerInitialisieren(EConsManager,EConsZellenT,1024,
                                 "Elementare Constraints");
}

/* Speicherverwaltung fuer NestedManager  ist leider fuer macros "optimiert" */
/* Trotzdem wird das hier in static Functions verborgen */

EConsT CO_newEcons (UTermT s, UTermT t, EConsT next, Constraint tp)
{
  EConsT res;

  SV_NestedAlloc(res,EConsManager,EConsT);
  res->s = s;
  res->t = t;
  res->tp = tp;
  res->Nachf = next;
  return res;
}

void CO_deleteEcons (EConsT e)
{
  SV_Dealloc(EConsManager,e);
}

static EConsT newEconsList (void)
{
  return SV_ErstesNextFree(EConsManager,EConsT);
}

static EConsT nextEConsCell (EConsT e)
{
  SV_AufNextFree(EConsManager,e,EConsT)
  return e;
}

static void snipEconsList (EConsT e)
/* e ist hierbei die letzte verbrauchte Zelle! */
{
  SV_LetztesNextFreeMelden(EConsManager,e->Nachf)
}

void CO_deleteEConsList (EConsT e, EConsT next)
{
  if ((e != NULL) && (e != next)){
    EConsT run = e;
    while (run->Nachf != next){
      run = run->Nachf;
    }
    SV_NestedDealloc(EConsManager,e,run);
  }
}

static char *pnamen [] = {">", "=", ">="};
void printProlog(FILE *strom, EConsT e)
{
  if (e == NULL){
    fprintf(strom, "[]");
  }
  else {
    IO_DruckeFlexStrom(strom, ".(%s(%t,%t),", pnamen[e->tp],e->s,e->t);
    printProlog(strom,e->Nachf);
    fprintf(strom, ")");
  }
}

void CO_printECons(FILE *strom, EConsT e, BOOLEAN all)
{
#if 1
  if (e == NULL){
    IO_DruckeFlexStrom(strom, ";");
  }
  else if (all){
    IO_DruckeFlexStrom(strom, "%t %s %t;", e->s, CNamen[e->tp], e->t);
    e = e->Nachf;
    while (e != NULL) {
      IO_DruckeFlexStrom(strom, "%t %s %t;", e->s, CNamen[e->tp], e->t);
      e = e->Nachf;
    }
  }
  else {
    IO_DruckeFlexStrom(strom, "E [%t %s %t%s]", e->s, CNamen[e->tp], e->t,
                       (e->Nachf ? ",..." : ""));
  }
#elif 1
  if (e == NULL){
    fprintf(strom, "E(<null>)");
  }
  else if (all){
    IO_DruckeFlexStrom(strom, "E [%t %s %t", e->s, CNamen[e->tp], e->t);
    e = e->Nachf;
    while (e != NULL) {
      IO_DruckeFlexStrom(strom, ", %t %s %t", e->s, CNamen[e->tp], e->t);
      e = e->Nachf;
    }
    fprintf(strom, "]");
  }
  else {
    IO_DruckeFlexStrom(strom, "E [%t %s %t%s]", e->s, CNamen[e->tp], e->t,
                       (e->Nachf ? ",..." : ""));
  }
#else
  printProlog(strom,e);
#endif
}

static unsigned int econsLength(EConsT e)
{
  unsigned int res = 0;
  while (e != NULL){
    e = e->Nachf;
    res++;
  }
  return res;
}

void CO_deepDeleteECons(EConsT e)
{
  EConsT erun = e;
  while (erun != NULL){
    TO_TermeLoeschen(erun->s,erun->t);
    erun = erun->Nachf;
  }
  CO_deleteEConsList(e,NULL);
}

SymbolT CO_neuesteVariable(EConsT e)
/* neuestes (= als ganze Zahl kleinstes) Variablensymbol in e 
 * bzw. SO_VorErstemVarSymb(), falls e grund */
{
  SymbolT x = SO_VorErstemVarSymb;
  while (e != NULL) {
    SymbolT y = TO_neuesteVariable(e->s);
    if (SO_VariableIstNeuer(y,x)){
      x = y;
    }
    y = TO_neuesteVariable(e->t);
    if (SO_VariableIstNeuer(y,x)){
      x = y;
    }
    e = e->Nachf;
  } 
  return x;
}

/* -------------------------------------------------------------------------- */
/* - context ---------------------------------------------------------------- */
/* -------------------------------------------------------------------------- */

Bindings CO_allocBindings(void)
{
  SymbolT i;
  UTermT *b = negarray_alloc(SO_VarNummer(NewestVar),UTermT,
			     SO_VarNummer(NewestVar));
  SO_forSymboleAufwaerts(i,NewestVar,SO_ErstesVarSymb){
    b[i] = NULL;
  }
  /*  printf("allocBinding -> %p\n", b);*/
  return b;
}

static void deallocBindings(Bindings b)
{
  negarray_dealloc(b,SO_VarNummer(NewestVar));
}

BOOLEAN CO_somethingBound(Bindings bs)
{
  SymbolT i;
  SO_forSymboleAufwaerts(i,NewestVar,SO_ErstesVarSymb){
    if (bs[i] != NULL){
      return TRUE;
    }
  }
  return FALSE;
}

TermT CO_substitute(Bindings bs, UTermT s)
{
  TermT res;
  UTermT t,t0,vorg;
  if (CO_somethingBound(bs)){
    if (TO_TermIstVar(s) && (bs[TO_TopSymbol(s)] != NULL)){
      res = TO_Termkopie(bs[TO_TopSymbol(s)]);/* Hier geht ein, dass durchmultipl. */
      /*    IO_DruckeFlex("1 %t\n", res);*/

    }
    else {
      /*      IO_DruckeFlex("2a %t\n", s);*/
      res = TO_Termkopie(s);
      /*      IO_DruckeFlex("2 %t\n", res);*/
    }
    vorg = res;
    /*    IO_DruckeFlex("res %t\n", res);*/
    t = TO_Schwanz(res);
    t0 = TO_NULLTerm(res);
    while (t != t0){
      SymbolT x = TO_TopSymbol(t);
      /*      IO_DruckeFlex("%t\n", t);*/
      if (SO_SymbIstVar(x)){
	UTermT b = bs[x]; /* Hier geht ein, dass durchmultipl. */
	if (b != NULL){
	  if (TO_Nullstellig(b)){
	    TO_SymbolBesetzen(t,TO_TopSymbol(b));
	  }
	  else {
	    TermT u = TO_Termkopie(b);
	    UTermT ue = TO_TermEnde(u);
	    TO_NachfBesetzen(vorg,u);
	    TO_SymbolBesetzen(t,TO_TopSymbol(ue));
	    do {
	      if (TO_TermEnde(u) == ue){
		TO_EndeBesetzen(u,t);
	      }
	      if (TO_Schwanz(u) == ue){
		TO_NachfBesetzen(u,t);
	      }
	      u = TO_Schwanz(u);
	    } while (u != t);
	    TO_TermzelleLoeschen(ue);
	    t = TO_Schwanz(vorg);
	  }
	}
      }
      vorg = t;
      t = TO_Schwanz(t);
    }
  }
  else {
    res = TO_Termkopie(s);
  }
  return res;
}

EConsT CO_substituteECons(Bindings bs, EConsT e)
{
  if (e == NULL){
    return NULL;
  }
  else {
    return CO_newEcons(CO_substitute(bs,e->s),
                       CO_substitute(bs,e->s),
                       CO_substituteECons(bs,e->Nachf),
                       e->tp);
  }
}

static void ContextManagerInitialisieren (void)
{
  SV_ManagerInitialisieren(&ContextManager,ContextStruct,1024, "ContextStruct");
}

static void incId(ContextT c)
{
  contextNumber++;
  c->id = contextNumber;
}

ContextT CO_newContext (Bindings bs)
{
  ContextT c;
  SV_Alloc(c,ContextManager,ContextT);
  incId(c);
  c->bindings = bs;
  c->econses = NULL;
  /*  printf("new %d\n", c->id);*/
  return c;
}

static void deleteContext(ContextT c)
{
#if 0
  /* fuehrt zu Speicherleck mit -fstrict-aliasing */
  c->id = 0;
  c->bindings = NULL;
  c->econses = NULL;
#endif
  SV_Dealloc(ContextManager,c);
}

static ContextT flatCopy(ContextT c)
{
  Bindings b1 = CO_allocBindings();
  ContextT c1 = CO_newContext(b1);
  SymbolT i;
  SO_forSymboleAufwaerts(i,NewestVar,SO_ErstesVarSymb){
    b1[i] = c->bindings[i];
  }
  if (c->econses == NULL){
    c1->econses = NULL;
  }
  else {
    EConsT e = c->econses;
    EConsT e1 = newEconsList();
    c1->econses = e1;
    e1->s = e->s;
    e1->t = e->t;
    e1->tp = e->tp;
    e = e->Nachf;
    while (e != NULL){
      e1 = nextEConsCell(e1);
      e1->s = e->s;
      e1->t = e->t;
      e1->tp = e->tp;
      e = e->Nachf;
    }
    snipEconsList(e1);
    e1->Nachf = NULL;
  }
  return c1;
}

static ContextT deepCopy(ContextT c)
{
  Bindings b1 = CO_allocBindings();
  ContextT c1 = CO_newContext(b1);
  SymbolT i;
  SO_forSymboleAufwaerts(i,NewestVar,SO_ErstesVarSymb){
    b1[i] = TO_Termkopie(c->bindings[i]);
  }
  if (c->econses != NULL){
    EConsT e = c->econses;
    EConsT e1 = newEconsList();
    c1->econses = e1;
    e1->s = TO_Termkopie(e->s);
    e1->t = TO_Termkopie(e->t);
    e1->tp = e->tp;
    e = e->Nachf;
    while (e != NULL){
      e1 = nextEConsCell(e1);
      e1->s = TO_Termkopie(e->s);
      e1->t = TO_Termkopie(e->t);
      e1->tp = e->tp;
      e = e->Nachf;
    }
    snipEconsList(e1);
    e1->Nachf = NULL;
  }
  return c1;
}

void CO_flatDelete(ContextT c)
{
  deallocBindings(c->bindings);
  CO_deleteEConsList(c->econses,NULL);
  deleteContext(c);
}

void CO_deepDelete(ContextT c)
{
  EConsT e,e1;
  SymbolT i;
  SO_forSymboleAufwaerts(i,NewestVar,SO_ErstesVarSymb){
    if (c->bindings[i] != NULL){
      TO_TermLoeschen(c->bindings[i]);
    }
  }
  deallocBindings(c->bindings);
  if (c->econses != NULL){
    e = c->econses;
    do {
      e1 = e;
      TO_TermLoeschen(e->s);
      TO_TermLoeschen(e->t);
      e = e->Nachf;
    } while (e != NULL);
    SV_NestedDealloc(EConsManager,c->econses,e1);
  }
  deleteContext(c);
}

static BOOLEAN isBound (ContextT c, SymbolT x)
{
  return (c != NULL) && (c->bindings[x] != NULL);
}

static UTermT deref (ContextT c, UTermT t)
{
  SymbolT x = TO_TopSymbol(t);
  if (SO_SymbIstVar(x) && isBound(c,x)){
    return c->bindings[x]; /* Hier geht ein, dass durchmultipl. */
  }
  else {
    return t;
  }
}

ContextT CO_solveEQPart (ContextT c)
{
  Bindings b = c->bindings;
  ContextT c1 = CO_newContext(CO_allocBindings());
  if (c->econses != NULL){
    EConsT e = c->econses;
    EConsT e1 = newEconsList();
    c1->econses = e1;
    e1->s = CO_substitute(b,e->s);
    e1->t = CO_substitute(b,e->t);
    e1->tp = e->tp;
    e = e->Nachf;
    while (e != NULL){
      e1 = nextEConsCell(e1);
      e1->s = CO_substitute(b,e->s);
      e1->t = CO_substitute(b,e->t);
      e1->tp = e->tp;
      e = e->Nachf;
    }
    snipEconsList(e1);
    e1->Nachf = NULL;
  }
  return c1;
}

void CO_printContext(FILE *strom, ContextT c)
{
  SymbolT i;
  if (c == NULL){
    fprintf(strom, "C[<null>]");
  }
  else {
#if 1
    /*    SO_forSymboleAbwaerts(i,SO_ErstesVarSymb,NewestVar){
      IO_DruckeFlexStrom(strom,"%d %X,", i, c->bindings[i]);
      }*/
    SO_forSymboleAbwaerts(i,SO_ErstesVarSymb,NewestVar){
      if (c->bindings[i] != NULL){
	IO_DruckeFlexStrom(strom,"x%d = %t;", -i, c->bindings[i]);
      }
    }
    CO_printECons(strom,c->econses,TRUE);
#else
    fprintf(strom,"C%ld[", c->id);
    /*    SO_forSymboleAbwaerts(i,SO_ErstesVarSymb,NewestVar){
      IO_DruckeFlexStrom(strom,"%d %X,", i, c->bindings[i]);
      }*/
    SO_forSymboleAbwaerts(i,SO_ErstesVarSymb,NewestVar){
      if (c->bindings[i] != NULL){
	IO_DruckeFlexStrom(strom,"x%d <- %t,", -i, c->bindings[i]);
      }
    }
    CO_printECons(strom,c->econses,TRUE);
    fprintf(strom,"]");
#endif
  }
}

/* -------------------------------------------------------------------------- */
/* - disjunktive constraints ------------------------------------------------ */
/* -------------------------------------------------------------------------- */

/* DConsT werden ueber einen NestedManager verwaltet. */

static void DConsManagerInitialisieren (void)
{
  SV_NestedManagerInitialisieren(DConsManager,DConsZellenT,1024,
                                 "Disjunktive Constraints");
}

/* Speicherverwaltung fuer NestedManager  ist leider fuer macros "optimiert" */
/* Trotzdem wird das hier in static Functions verborgen */

static DConsT newDcons (ContextT c, DConsT next)
{
  DConsT res;

  SV_NestedAlloc(res,DConsManager,DConsT);
  res->c = c;
  res->Nachf = next;
  return res;
}

static void deleteDcons (DConsT d)
{
  SV_Dealloc(DConsManager,d);
}

void CO_deleteDConsList (DConsT d)
{
  if (d != NULL){
    DConsT run = d;
    while (run->Nachf != NULL){
      CO_flatDelete(run->c);
      run = run->Nachf;
    }
    CO_flatDelete(run->c);
    SV_NestedDealloc(DConsManager,d,run);
  }
}

void CO_printDCons(FILE *strom, DConsT d)
{
  if (d == NULL){
    fprintf(strom, "D(<null>)");
  }
  else {
    IO_DruckeFlexStrom(strom, "D (%C", d->c);
    d = d->Nachf;
    while (d != NULL) {
      IO_DruckeFlexStrom(strom, ", %C", d->c);
      d = d->Nachf;
    }
    fprintf(strom, ")");
  }
}

unsigned int CO_DConsLength(DConsT d)
{
  unsigned int res = 0;
  while (d != NULL){
    d = d->Nachf;
    res++;
  }
  return res;
}

typedef struct {
  ContextT c;
  int n1, n2, n3;
} SortBufEntry;

SortBufEntry *co_sortBuf = NULL;
unsigned co_sortBufLen = 0;

void co_allocBuf(unsigned len)
{
  if (len > co_sortBufLen){
    co_sortBufLen = ((len + 15) / 16) * 16 ; /* aufrunden auf naechsten 16er */
    co_sortBuf = our_realloc(co_sortBuf, co_sortBufLen * sizeof(SortBufEntry),
                             "reallocating co_sortBuf");
  }
}

int co_NoOfBindings(Bindings bs)
{
  SymbolT i;
  int res = 0;
  SO_forSymboleAufwaerts(i,NewestVar,SO_ErstesVarSymb){
    if (bs[i] != NULL){
      res++;
    }
  }
  return res;
}

int co_NoOfXs(EConsT e, Constraint x)
/* wieviele Econse haben x als Constraint? */
{
  int res = 0;
  while (e != NULL){
    if (e->tp == x){
      res++;
    }
    e = e->Nachf;
  }
  return res;
}

void co_insertContext(ContextT c, unsigned i)
{
  co_sortBuf[i].c  = c;
  co_sortBuf[i].n1 = co_NoOfBindings(c->bindings);
  co_sortBuf[i].n2 = co_NoOfXs(c->econses, Gt_C);
  co_sortBuf[i].n3 = co_NoOfXs(c->econses, Ge_C);
}

void co_insertDConses(DConsT d)
{
  unsigned i = 0;
  while (d != NULL){
    co_insertContext(d->c,i);
    i++;
    d = d->Nachf;
  }
}

void co_rearrangeDCons(DConsT d)
{
  unsigned i = 0;
  while (d != NULL){
    SortBufEntry NullEntry = {NULL,0,0,0};
    d->c = co_sortBuf[i].c;
    co_sortBuf[i] = NullEntry;
    i++;
    d = d->Nachf;
  }
}

int co_SBcmp(const void *v1, const void *v2)
{
  SortBufEntry *e1 = (SortBufEntry *) v1;
  SortBufEntry *e2 = (SortBufEntry *) v2;       /* "Klein" sind Contexte mit */
  if (e1->n1 != e2->n1) return e1->n1 - e2->n1; /* - wenigen Bindungen       */
  if (e1->n2 != e2->n2) return e2->n2 - e1->n2; /* - vielen :>: Constraints  */
  return e2->n3 - e1->n3;                       /* - vielen :>= Constraints  */
}

void CO_sortDCons(DConsT d)
{
  unsigned len = CO_DConsLength(d);
  if (len > 1){
    co_allocBuf(len);
    co_insertDConses(d);
    qsort(co_sortBuf,len,sizeof(SortBufEntry),co_SBcmp);
    co_rearrangeDCons(d);
  }
}

/* -------------------------------------------------------------------------- */
/* - LPO-Variationen: 4 wertig ---------------------------------------------- */
/* -------------------------------------------------------------------------- */

static BOOLEAN cEQ (ContextT c, UTermT s, UTermT t)
/* Ist unter dem Gleichheitsanteil sigma_c von c: sigma_c(s) == sigma_c(t)? */
{
  BOOLEAN res = TRUE;
  OPENTRACE  (("cEQ %t =? %t (%C)\n", s, t, c));
  {
    if (s != t) {
      UTermT s0 = TO_NULLTerm(s);
      do {
        if (TO_TopSymbol(s) != TO_TopSymbol(t)){
          if ((TO_TermIstVar(s) && isBound(c, TO_TopSymbol(s))) ||
              (TO_TermIstVar(t) && isBound(c, TO_TopSymbol(t)))   ){
            if (cEQ(c, deref(c,s), deref(c,t))){
              s = TO_NaechsterTeilterm(s);
              t = TO_NaechsterTeilterm(t);
            }
            else {
              res = FALSE;
            }
          }
          else {
            res = FALSE;
          }
        }
        else {
          s = TO_Schwanz(s);
          t = TO_Schwanz(t);
        }
      } while (res && (s != s0));
    }
  }
  CLOSETRACE (("cEQ -> %b\n", res));
  return res;
}

BOOLEAN CO_cEQ (ContextT c, UTermT s, UTermT t)
/* Ist unter dem Gleichheitsanteil sigma_c von c: sigma_c(s) == sigma_c(t)? */
{
  return cEQ(c,s,t);
}

static BOOLEAN cSubterm (ContextT c, UTermT s, UTermT t, BOOLEAN proper)
/* ist s ein (echter) Teilterm von t unter sigma_c ? */
{
  BOOLEAN res = FALSE;
  UTermT t0;
  OPENTRACE  (("cSubterm(%s) %t <TT? %t (%C)\n", (proper?"proper":""),s,t,c));
  {
    if (s == t) {
      res = !proper;
    }
    else{
      s = deref(c,s);
      t = deref(c,t);
      t0 = TO_NULLTerm(t);
      if (proper){
        t = TO_Schwanz(t);
      }
      while (!res && (t != t0)){
        res = cEQ(c,s,t); /* Nachteil: Var(t) werden immer wieder durchsubst! */
        t = TO_Schwanz(t);
      }
    }
  }
  CLOSETRACE (("cSubterm -> %b\n", res));
  return res;
}

static BOOLEAN impliedORD(ContextT c, UTermT s, UTermT t, BOOLEAN gt, int sit)
{
#if (IMPLIED_ORD == 0)
  /* Vorschlag Roberto-Papier */
  if (sit == 1) {
    return cSubterm(c,s,t,gt);
  }
  else {
    return cEQ(c,s,t);
  }
#elif (IMPLIED_ORD == 1)
  /* Vernuenftiger (mehr Simplifkationskraft) */
  return cSubterm(c,s,t,gt);
#elif (IMPLIED_ORD == 2)
  /* ZU MUTIG: TERMINATION ! ! ! 
   * tut jetzt: mit Var-Abfrage gesichert
   * => viel kleinere Anzahl an Loesungen 
   * ABER: ZU MUTIG: KORREKTHEIT ! !  
   * Beispiel: fuer x1 :>: x2, i(i(x2)) :>: x1 findet er keine Loesung,
   * obwohl offensichtlich eine exisitiert (x1 <- i(x2))
   * Schade!
   | | is_important i(i(x2)) :>: x1 (C41[E (x1 :>: x2)])
   | | | cLPOCMP i(i(x2)) <>? x1 (C[<null>])
   | | | | getHash(i(i(x2)),x1) -> NOT_FOUND
   | | | | cLPOCMP1 i(i(x2)) <>? x1 (C[<null>])
   | | | | | impliedCMP i(i(x2)) =? x1 (C[<null>])
   | | | | | impliedCMP -> Unvergleichbar
   | | | | cLPOCMP1 -> Unvergleichbar
   | | | | setHash(i(i(x2)),x1) -> Unvergleichbar (0x1003c3c4,0x1003c3e8)
   | | | cLPOCMP -> Unvergleichbar
   | | | cLPOCMP i(i(x2)) <>? x1 (C41[E (x1 :>: x2)])
   | | | | getHash(i(i(x2)),x1) -> NOT_FOUND
   | | | | cLPOCMP1 i(i(x2)) <>? x1 (C41[E (x1 :>: x2)])
   | | | | | impliedCMP i(i(x2)) =? x1 (C41[E (x1 :>: x2)])
   | | | | | | cLPOCMPGTEQ i(i(x2)) <>? x2 (C41[E (x1 :>: x2)])
   | | | | | | | getHash(i(i(x2)),x2) -> NOT_FOUND
   | | | | | | | cLPOCMPGTEQ1 i(i(x2)) <>? x2 (C41[E (x1 :>: x2)])
   | | | | | | | cLPOCMPGTEQ1 -> Groesser
   | | | | | | | setHash(i(i(x2)),x2) -> Groesser (0x1003c3c4,0x1003c3ac)
   | | | | | | cLPOCMPGTEQ -> Groesser
   | | | | | impliedCMP -> Kleiner
   | | | | cLPOCMP1 -> Kleiner
   | | | | setHash(i(i(x2)),x1) -> Kleiner (0x1003c3c4,0x1003c3e8)
   | | | cLPOCMP -> Kleiner
   | | is_important -> bottom
   | integrate -> False
   DECOMPOSE: -> False
   
   -> Problem

MURKS, nochmals richtig analysieren, wer groesser wem sein muss!
   */

  isse noch nicht richtig;

  BOOLEAN res;
  OPENTRACE  (("impliedORD %t %s %t (%C)\n", s,(gt ? "<" : "<="), t, c));
  level++;
  /*  printf("%d##\n",level);*/
  /*  IO_DruckeFlex("%d: impliedORD %t %s %t (%C)\n", level,s,(gt ? "<" : "<="), t, c);*/
  if (TO_TermIstVar(s)){
    res = cSubterm(c,s,t,gt);
  }
  else {
    VergleichsT vres = cLPOCMPGTEQ(c,t,s);
    if(gt){
      res = (vres == Groesser);
    }
    else{
      res = ((vres == Groesser) || (vres == Gleich));
    }
  }
  level--;
  CLOSETRACE  (("impliedCMP -> %b\n", res));
  return res;
#else
  select IMPLIED_ORD
#endif
}

static BOOLEAN impliedCMP (ContextT c, UTermT s, UTermT t, BOOLEAN gteq)
/* Verbesserungspotential: statt cSubterm cLPOCMP (Achtung: Termination!!!) */
{
  VergleichsT res = Unvergleichbar;
  OPENTRACE  (("impliedCMP %t =? %t (%C)\n", s, t, c));
  {
    if (c != NULL){
      EConsT run = c->econses;
      BOOLEAN vars = TO_TermIstVar(s);
      SymbolT x = TO_TopSymbol(s);
      BOOLEAN vart = TO_TermIstVar(t);
      SymbolT y = TO_TopSymbol(t);
      while ((res == Unvergleichbar) && (run != NULL)){
        if (vars && (x == TO_TopSymbol(run->s))){
          if (impliedORD(c,t,run->t,run->tp == Ge_C,1)){
            res = Groesser;
          }
        }
        else if (vart && (y == TO_TopSymbol(run->s))){
          if (impliedORD(c,s,run->t,run->tp == Ge_C,1)){
            res = Kleiner;
          }
        }
        else if (vars && (x == TO_TopSymbol(run->t)) && (run->tp == Gt_C)){
          /*          if (cEQ(c,t,run->s))*/
          if (impliedORD(c,run->s,t,FALSE,2)){
            res = Kleiner;
          }
        }
        else if (vart && (y == TO_TopSymbol(run->t)) && (run->tp == Gt_C)){
          /*          if (cEQ(c,s,run->s))*/
          if (impliedORD(c,run->s,s,FALSE,2)){
            res = Groesser;
          }
        }
        run = run->Nachf;
      }
    }
  }
  CLOSETRACE (("impliedCMP -> %v\n", res));
  return res;
}

static BOOLEAN impliedGTEQ (ContextT c, UTermT s, UTermT t, BOOLEAN gteq)
/* Verbesserungspotential: statt cSubterm cLPOCMP (Achtung: Termination!!!) */
{
  VergleichsT res = NOT_GTEQ;
  OPENTRACE  (("impliedGTEQ %t =? %t (%C)\n", s, t, c));
  {
    if (c != NULL){
      EConsT run = c->econses;
      BOOLEAN vars = TO_TermIstVar(s);
      SymbolT x = TO_TopSymbol(s);
      BOOLEAN vart = TO_TermIstVar(t);
      SymbolT y = TO_TopSymbol(t);
      while ((res == NOT_GTEQ) && (run != NULL)){
        if (vars && (x == TO_TopSymbol(run->s))){
          if (impliedORD(c,t,run->t,run->tp == Ge_C,1)){
            res = Groesser;
          }
        }
        else if (vart && (y == TO_TopSymbol(run->t)) && (run->tp == Gt_C)){
          /*          if (cEQ(c,s,run->s))*/
          if (impliedORD(c,run->s,s,FALSE,2)){
            res = Groesser;
          }
        }
        run = run->Nachf;
      }
    }
  }
  CLOSETRACE (("impliedGTEQ -> %v\n", res));
  return res;
}

static BOOLEAN cVarEnthalten (ContextT c, SymbolT x, UTermT t)
{
  UTermT t0 = TO_NULLTerm(t);
  do {
    if (x == TO_TopSymbol(t)){
      return TRUE;
    }
    if (TO_TermIstVar(t) && isBound(c,TO_TopSymbol(t)) &&
        cVarEnthalten(c, x, deref(c,t))){
      return TRUE;
    }
    t = TO_Schwanz(t);
  } while (t != t0);
  return FALSE;
}

static BOOLEAN cVarEnthalten4 (ContextT c, UTermT x, UTermT t)
/* Es ist noch das Hashing einzubauen! */
{
  return cVarEnthalten(c,TO_TopSymbol(x),t);
}

static UTermT majo4 (ContextT c, UTermT s, UTermT ti, UTermT t0)
/* Sind alle Terme ti ..< t0 kleiner als s? Wenn nein, gibt
 * den ersten zurueck, bei dem es schief geht, ansonsten t0 */
{
  OPENTRACE  (("majo4 %t >>? %t (%C)\n", s, ti, c));
  while (ti != t0) {
    if (cLPOCMPGTEQ(c,s,ti) != Groesser){
      CLOSETRACE  (("majo4 -> FALSE\n"));
      return ti;
    }
    ti = TO_NaechsterTeilterm(ti);
  }
  CLOSETRACE  (("majo4 -> TRUE\n"));
  return t0;
}

static BOOLEAN alpha4 (ContextT c, UTermT si, UTermT s0, UTermT t)
/* Ist einer der Terme si ..< s0 groesser oder gleich t? */
{
  OPENTRACE  (("alpha4 %t >?? %t (%C)\n", si, t, c));
  {
    while (si != s0){
      int res = cLPOCMPGTEQ(c,si,t);
      if ((res == Gleich) || (res == Groesser)){
        CLOSETRACE  (("alpha4 -> TRUE\n"));
        return TRUE;
      }
      si = TO_NaechsterTeilterm(si);
    }
  }
  CLOSETRACE  (("alpha4 -> FALSE\n"));
  return FALSE;
}

static VergleichsT cLPOCMP1 (ContextT c, UTermT s, UTermT t)
/* Vergleicht die beiden Terme s und t modulo c. 
 * Ergebnisse: Groesser, Gleich, Kleiner, Unvergleichbar */
{
  UTermT si, s0, ti, t0;
  VergleichsT res = UNKNOWN;
  OPENTRACE  (("cLPOCMP1 %t <>? %t (%C)\n", s, t, c));
  {
    if (TO_TermIstVar(s)){
      if (TO_TopSymbol(s) == TO_TopSymbol(t)){
	res = Gleich;
      }
      else if (cVarEnthalten4(c,s,t)){
	res = Kleiner;
      }
      else {
	res = impliedCMP(c,s,t,FALSE);
      }
    }
    else if (TO_TermIstVar(t)) {
      if (cVarEnthalten4(c,t,s)) {
	res = Groesser;
      }
      else {
	res = impliedCMP(c,s,t,FALSE);
      }
    }
    else {
        switch (PR_Praezedenz(TO_TopSymbol(s), TO_TopSymbol(t))){
        case Groesser:
          t0 = TO_NULLTerm(t);
          ti = majo4(c,s,TO_ErsterTeilterm(t),t0);
          if (ti == t0){/* alles majorisiert */
	    res = Groesser;
	  }
	  else if (alpha4(c,ti,t0,s)){
	    res = Kleiner;
	  }
	  else {
	    res = Unvergleichbar;
	  }
          break;
        case Kleiner:
          s0 = TO_NULLTerm(s);
          si = majo4(c,t,TO_ErsterTeilterm(s),s0);
          if (si == s0){/* alles majorisiert */
	    res = Kleiner;
	  }
	  else if (alpha4(c,si,s0,t)){
	    res = Groesser;
	  }
	  else {
	    res = Unvergleichbar;
	  }
	  break;
        case Unvergleichbar:
          if (alpha4(c,TO_ErsterTeilterm(s),TO_NULLTerm(s),t)){
	    res = Groesser;
	  }
	  else if (alpha4(c,TO_ErsterTeilterm(t),TO_NULLTerm(t),s)){
	    res = Kleiner;
	  }
	  else {
	    res = Unvergleichbar;
	  }
          break;
        case Gleich: 
	  si = TO_ErsterTeilterm(s);
	  s0 = TO_NULLTerm(s);
	  ti = TO_ErsterTeilterm(t);
	  t0 = TO_NULLTerm(t);
	  res = Gleich;
	  while ((res == Gleich) && (ti != t0)){
	    switch (cLPOCMP(c,si,ti)){
	    case Unvergleichbar:
              if (alpha4(c,TO_NaechsterTeilterm(si),s0,t)){
                res = Groesser;
              }
              else if (alpha4(c,TO_NaechsterTeilterm(ti),t0,s)){
                res = Kleiner;
              }
              else {
                res = Unvergleichbar;
              }
	      break;
	    case Groesser:
              ti = majo4(c,s,TO_NaechsterTeilterm(ti),t0);
	      if (ti == t0){/* alles majorisiert */
		res = Groesser;
	      }
	      else if (alpha4(c,ti,t0,s)){
                res = Kleiner;
              }
              else {
                res = Unvergleichbar;
              }
	      break;
	    case Kleiner:
              si = majo4(c,t,TO_NaechsterTeilterm(si),s0);
	      if (si == s0){/* alles majorisiert */
		res = Kleiner;
	      }
	      else if (alpha4(c,si,s0,t)){
                res = Groesser;
              }
              else {
                res = Unvergleichbar;
              }
	      break;
	    case Gleich:
              si = TO_NaechsterTeilterm(si);
              ti = TO_NaechsterTeilterm(ti);
	      break;
	    default:
	      our_error("wrong compare 4");
	    }
	  }
	  break;
        default:
          our_error("wrong compare 3" );
        }
    }
  }
  CLOSETRACE (("cLPOCMP1 -> %v\n", res));
  /* res \in {Groesser, Gleich, Kleiner, Unvergleichbar} */
  return res;
}


static VergleichsT cLPOCMPGTEQ1 (ContextT c, UTermT s, UTermT t)
/* Vergleicht die beiden Terme s und t modulo c. 
 * Allerdings nur solange sie Groesser oder Gleich sein koennen.
 * Ergebnisse: Groesser, Gleich, NOT_GTEQ */
{
  UTermT si, s0, ti, t0;
  VergleichsT res = NOT_GTEQ;
  OPENTRACE  (("cLPOCMPGTEQ1 %t <>? %t (%C)\n", s, t, c));
  {
    if (TO_TermIstVar(s)){
      if (TO_TopSymbol(s) == TO_TopSymbol(t)){
	res = Gleich;
      }
      else {
	res = impliedGTEQ(c,s,t,FALSE);
      }
    }
    else if (TO_TermIstVar(t)) {
      if (cVarEnthalten4(c,t,s)) {
	res = Groesser;
      }
      else {
	res = impliedGTEQ(c,s,t,FALSE);
      }
    }
    else {
        switch (PR_Praezedenz(TO_TopSymbol(s), TO_TopSymbol(t))){
        case Groesser:
          t0 = TO_NULLTerm(t);
          ti = majo4(c,s,TO_ErsterTeilterm(t),t0);
          if (ti == t0){/* alles majorisiert */
	    res = Groesser;
	  }
	  else {
	    res = NOT_GTEQ;
	  }
          break;
        case Kleiner:
        case Unvergleichbar:
          if (alpha4(c,TO_ErsterTeilterm(s),TO_NULLTerm(s),t)){
	    res = Groesser;
	  }
	  else {
	    res = NOT_GTEQ;
	  }
          break;
        case Gleich: 
	  si = TO_ErsterTeilterm(s);
	  s0 = TO_NULLTerm(s);
	  ti = TO_ErsterTeilterm(t);
	  t0 = TO_NULLTerm(t);
	  res = Gleich;
	  while ((res == Gleich) && (ti != t0)){
	    switch (cLPOCMPGTEQ(c,si,ti)){
            case Unvergleichbar:
	    case NOT_GTEQ:
	    case Kleiner:
              if (alpha4(c,TO_NaechsterTeilterm(si),s0,t)){
                res = Groesser;
              }
              else {
                res = NOT_GTEQ;
              }
	      break;
	    case Groesser:
              ti = majo4(c,s,TO_NaechsterTeilterm(ti),t0);
	      if (ti == t0){/* alles majorisiert */
		res = Groesser;
	      }
              else {
                res = NOT_GTEQ;
              }
	      break;
	    case Gleich:
              si = TO_NaechsterTeilterm(si);
              ti = TO_NaechsterTeilterm(ti);
	      break;
	    default:
	      our_error("wrong compare 14");
	    }
	  }
	  break;
        default:
          our_error("wrong compare 13" );
        }
    }
  }
  CLOSETRACE (("cLPOCMPGTEQ1 -> %v\n", res));
  /* res \in {Groesser, Gleich, NOT_GTEQ} */
  return res;
}

VergleichsT cLPOCMP (ContextT c, UTermT s, UTermT t)
{
  int res;
  OPENTRACE  (("cLPOCMP %t <>? %t (%C)\n", s, t, c));
  {
    if (s != t) {
      s = deref (c,s);
      t = deref (c,t);
      res = getHash(c,s,t);
      if (!IS_PROPER_RES(res)){
        if (res == NOT_GTEQ){
          res = flipVergleich(cLPOCMPGTEQ1 (c,t,s));
          if (res == NOT_LTEQ){
            res = Unvergleichbar;
          }
        }
        else if (res == NOT_LTEQ){
          res = cLPOCMPGTEQ1 (c,s,t);
          if (res == NOT_GTEQ){
            res = Unvergleichbar;
          }
        }
        else {
          res = cLPOCMP1 (c,s,t);
        }
	setHash(c,s,t,res);
      }
    }
    else {
      res = Gleich;
    }
  }
  CLOSETRACE (("cLPOCMP -> %v\n", res));
  return res;
}

VergleichsT cLPOCMPGTEQ (ContextT c, UTermT s, UTermT t)
/* wie cLPOCMP, aber nur soweit rechnen, solange Groesser oder Gleich
 * moeglich ist -> Werte sind Groesser, Gleich oder NOT_FOUND 
 * (wenn gehasht auch Kleiner bzw. Unvergleichbar!!!) */
{
  int res;
  OPENTRACE  (("cLPOCMPGTEQ %t <>? %t (%C)\n", s, t, c));
  {
    if (s != t) {
      s = deref (c,s);
      t = deref (c,t);
      res = getHash(c,s,t);
      if (res == NOT_FOUND){
        res = cLPOCMPGTEQ1 (c,s,t);
	setHash(c,s,t,res);
      }
      else if (res == NOT_LTEQ){
        res = cLPOCMPGTEQ1 (c,s,t);
        if (res == NOT_GTEQ){
          res = Unvergleichbar;
        }
	setHash(c,s,t,res);
      }
    }
    else {
      res = Gleich;
    }
  }
  CLOSETRACE (("cLPOCMPGTEQ -> %v\n", res));
  return res;
}

/* -------------------------------------------------------------------------- */

VergleichsT CO_cLPOCMP (ContextT c, UTermT s, UTermT t)
{
  int res;

  OPENTRACE  (("CO_cLPOCMP %t <>? %t (%C)\n", s, t, c));
  {
    initHash();
    res = cLPOCMP(c,s,t);
  }
  CLOSETRACE (("CO_cLPOCMP -> %v\n", res));
  return res;
}

VergleichsT CO_cLPOCMPGTEQ (ContextT c, UTermT s, UTermT t)
{
  int res;
  OPENTRACE  (("CO_cLPOCMPGTEQ %t <>? %t (%C)\n", s, t, c));
  {
    initHash();
    res = cLPOCMPGTEQ(c,s,t);
  }
  CLOSETRACE (("CO_cLPOCMPGTEQ -> %v\n", res));
  return res;
}

VergleichsT CO_LPOCMP (UTermT s, UTermT t)
{
  int res;
  OPENTRACE  (("CO_LPOCMP %t <>? %t (%C)\n", s, t, currentContext));
  {
    initHash();
    res = cLPOCMP(currentContext,s,t);
  }
  CLOSETRACE (("CO_LPOCMP -> %v\n", res));
  return res;
}

VergleichsT CO_LPOCMPGTEQ (UTermT s, UTermT t)
{
  int res;
  OPENTRACE  (("CO_LPOCMPGTEQ %t <>? %t (%C)\n", s, t, currentContext));
  {
    initHash();
    res = cLPOCMPGTEQ(currentContext,s,t);
  }
  CLOSETRACE (("CO_cLPOCMPGTEQ -> %v\n", res));
  return res;
}

void CO_setCurrentContext (ContextT c)
{
  currentContext = c;
  RESTRACE(("CO_setCurrentContext: %C\n", c));
}

ContextT CO_getCurrentContext (void)
{
  return currentContext;
}

/* -------------------------------------------------------------------------- */
/* - Zerlegen von Constraints ----------------------------------------------- */
/* -------------------------------------------------------------------------- */

static BOOLEAN schon_enthalten(ContextT c, UTermT s, UTermT t, Constraint tp)
/* WART WART WART WART WART WART WART WART WART WART WART WART WART WART */
{
  EConsT e = c->econses;
  while (e != NULL){
    if (TO_TermeGleich(s,e->s) &&
	TO_TermeGleich(t,e->t) &&
	(e->tp == tp)){
      return TRUE;
    }
    e = e->Nachf;
  }
  return FALSE;
}

static BRI is_important(ContextT c, UTermT s, UTermT t, Constraint tp)
{
  BRI res = important;
  VergleichsT vgl;
  OPENTRACE(("is_BRI %t %s %t (%C)\n", s, CNamen[tp], t, c));
  {
    if (DO_PURE_LPO_FIRST){
      vgl = cLPOCMP(NULL,s,t);
      if (vgl == Unvergleichbar){
        vgl = cLPOCMP(c,s,t);
      }
    }
    else {
      vgl = cLPOCMP(c,s,t);
    }
    if (vgl == Unvergleichbar){
      if (schon_enthalten(c,s,t,tp)){
	res = redundant;
      }
      else {
	res = important;
      }
    }
    else {
      switch (tp){
      case Gt_C:
	if (vgl == Groesser) {
	  res = redundant;
	}
	else {
	  res = bottom;
	}
	break;
      case Ge_C:
	if ((vgl == Gleich)||(vgl == Groesser)) {
	  res = redundant;
	}
	else {
	  res = bottom;
	}
	break;
      case Eq_C:
	if (vgl == Gleich) {
	  res = redundant;
	}
	else {
	  res = bottom;
	}
	break;
      default:
	our_error("falscher compare (5)");
      }
    }
  }
  CLOSETRACE (("is_BRI -> %s\n", BRINamen[res]));
  return res;
}

static BOOLEAN bri (ContextT c, EConsT e, EConsT *er)
{
  BOOLEAN res = TRUE;
  EConsT e1 = NULL;
  EConsT r1 = NULL;
  while (res && (e != NULL)){
    switch (is_important(c,e->s,e->t,e->tp)){
    case bottom:
      res = FALSE;
      CO_deleteEConsList(e1,NULL);
      break;
    case redundant:
      break;
    case important:
      if (e1 == NULL){
        e1 = CO_newEcons(e->s,e->t,NULL,e->tp);
        r1 = e1;
      }
      else {
        r1->Nachf = CO_newEcons(e->s,e->t,NULL,e->tp);
        r1 = r1->Nachf;
      }
      break;
    default:
      our_error("falscher BRI (1)");
    }
    e = e->Nachf;
  }
  if (res){
    *er = e1;
  }
  return res;
}

static void RelationInit (BOOLEAN *R, unsigned int n)
{
  unsigned int i,j;
  for (i=0; i<n; i++){
    for (j=0; j<n; j++){
      R[i*n+j] = FALSE;
    }
  }
}

static BOOLEAN *getRelation (void)
{
  if ((rel == NULL) || (varrelsize < (unsigned) -NewestVar)){
    if (rel != NULL){
      array_dealloc(rel);
    }
    varrelsize = -NewestVar;
    rel = array_alloc(varrelsize * varrelsize,BOOLEAN);
  }
  RelationInit(rel,varrelsize);
  return rel;
}

static void RoyWarshall (BOOLEAN *R, unsigned int n, 
			 unsigned int min, unsigned int max)
{
  unsigned int i,j,k;
  for (i=min; i<=max; i++)
    for (j=min; j<=max; j++)
      if (R[j*n+i])
	for (k=min; k<=max; k++)
	  R[j*n+k] = R[j*n+k] || R[i*n+k];
}

static BOOLEAN eineDiagonaleins (BOOLEAN *R, unsigned int n, 
				 unsigned int min, unsigned int max)
{
  unsigned int i;
  for (i=min; i<=max; i++)
    if (R[i*n+i])
      return TRUE;
  return FALSE;
}

static void cVarEintragen(BOOLEAN *varrel, ContextT c, SymbolT x,
                          UTermT t, BOOLEAN skip)
{
  UTermT t0 = TO_NULLTerm(t);
  int offset = (SO_VarNummer(x)-1)*varrelsize;
  /*  RESTRACE(("cVarEintragen(%C) %d -> %t\n", c, -x, t));*/
  if (skip) {
    t = TO_Schwanz(t);
  }
  while (t != t0){
    if (TO_TermIstVar(t)){
      if (isBound(c,TO_TopSymbol(t))){
        cVarEintragen(varrel, c, x, deref(c,t), FALSE);
      }
      else {
        varrel[offset+(SO_VarNummer(TO_TopSymbol(t))-1)] = TRUE;
      }
    }
    t = TO_Schwanz(t);
  }  
}

static BOOLEAN varCycle (ContextT c)
{
  BOOLEAN res = FALSE;
  OPENTRACE(("varCycle(%C)\n", c, res));
  if ((c != NULL) && (c->econses != NULL)){
    BOOLEAN *varrel = getRelation();
    EConsT e = c->econses;
    SymbolT new = SO_ErstesVarSymb;
    SymbolT old = NewestVar;
    do {
      if (TO_TermIstVar(e->s)){
	if (SO_VariableIstNeuer(TO_TopSymbol(e->s),new)){
	  new = TO_TopSymbol(e->s);
	}
	if (SO_VariableIstNeuer(old,TO_TopSymbol(e->s))){
	  old = TO_TopSymbol(e->s);
	}
	cVarEintragen(varrel,c,TO_TopSymbol(e->s),e->t,e->tp == Ge_C);
      }
      e = e->Nachf;
    } while (e != NULL);
    if (!(SO_VariableIstNeuer(old,new))){
      RoyWarshall(varrel,varrelsize, SO_VarNummer(old)-1, SO_VarNummer(new)-1);
      res = eineDiagonaleins(varrel,varrelsize,
			     SO_VarNummer(old)-1, SO_VarNummer(new)-1);
    }
    /*IO_DruckeFlex("#varCycle-> %b\n", res);*/
  }
  CLOSETRACE(("varCycle(%C) -> %b\n", c, res));
  return res;
}

static BOOLEAN integrateEq (SymbolT x, UTermT t, EConsT next, ContextT c)
{
  BOOLEAN res = FALSE;
  if (!cVarEnthalten(c,x,t)){/* occur check ! */
    Bindings b = CO_allocBindings();
    Bindings bold = c->bindings;
    ContextT c1 = CO_newContext(b);
    EConsT e1 = NULL;
    EConsT r1 = NULL;
    EConsT e2 = NULL;
    EConsT r2 = NULL;
    EConsT r  = c->econses;
    SymbolT i;
    while (r != NULL){
      if ((TO_TopSymbol(r->s) == x) || (TO_TopSymbol(r->t) == x)){
        if (e1 == NULL){
          e1 = CO_newEcons(r->s,r->t,NULL,r->tp);
          r1 = e1;
        }
        else {
          r1->Nachf = CO_newEcons(r->s,r->t,NULL,r->tp);
          r1 = r1->Nachf;
        }
      }
      else {
        if (e2 == NULL){
          e2 = CO_newEcons(r->s,r->t,NULL,r->tp);
          r2 = e2;
        }
        else {
          r2->Nachf = CO_newEcons(r->s,r->t,NULL,r->tp);
        r2 = r2->Nachf;
        }
      }
      r = r->Nachf;
    }
    SO_forSymboleAufwaerts(i,NewestVar,SO_ErstesVarSymb){
      if ((i == x) || ((bold[i] != NULL) && (x == TO_TopSymbol(bold[i])))){
        b[i] = t;
      }
      else {
        b[i] = bold[i];
      }
    }
    c1->bindings = b;
    c1->econses = e2;
    if (r1 != NULL){
      r1->Nachf = next;
      if (!varCycle(c1)){
#if MEHR_BRI
#endif
        res = decompose(e1,c1);
      }
      CO_deleteEConsList(e1,next);
    }
    else {
      if (!varCycle(c1)){
#if MEHR_BRI
#endif
        res = decompose(next,c1);
      }
    }
    deallocBindings(c1->bindings);
    CO_deleteEConsList(e2,NULL);
    deleteContext(c1);
  }
  return res;
}

static Constraint mkTp (Constraint tp1, Constraint tp2)
{
  return ((tp1 == Ge_C) && (tp2 == Ge_C)) ? Ge_C : Gt_C;
}

static BOOLEAN mkTrans (ContextT c, EConsT next, EConsT *eres)
/* c->econses enthaelt den neuen Constraint als ersten */
{
  BOOLEAN res = TRUE;
  EConsT e = c->econses;
  EConsT run = e->Nachf;
  EConsT e1 = NULL;
  EConsT r1 = NULL;
  if (run != NULL){
    UTermT  s = e->s;
    BOOLEAN vars = TO_TermIstVar(s);
    SymbolT x = TO_TopSymbol(s);
    UTermT  t = e->t;
    BOOLEAN vart = TO_TermIstVar(t);
    SymbolT y = TO_TopSymbol(t);
    Constraint tp = e->tp;
    do {
      if (vart && (y == TO_TopSymbol(run->s))){
        Constraint newTp = mkTp(tp,run->tp);
        switch (is_important(c,s,run->t,newTp)){
        case bottom:
          res = FALSE;
          CO_deleteEConsList(e1,NULL);
          break;
        case redundant:
          /* nix tun */
          break;
        case important:
          if (e1 == NULL){
            e1 = CO_newEcons(s,run->t,NULL,newTp);
            r1 = e1;
          }
          else {
            r1->Nachf = CO_newEcons(s,run->t,NULL,newTp);
            r1 = r1->Nachf;
          }
          break;
        default:
          our_error("wrong BRI(3)");
        }
      }
      if (vars && (x == TO_TopSymbol(run->t))){
        Constraint newTp = mkTp(tp,run->tp);
        switch (is_important(c,run->s,t,newTp)){
        case bottom:
          res = FALSE;
          CO_deleteEConsList(e1,NULL);
          break;
        case redundant:
          /* nix tun */
          break;
        case important:
          if (e1 == NULL){
            e1 = CO_newEcons(run->s,t,NULL,newTp);
            r1 = e1;
          }
          else {
            r1->Nachf = CO_newEcons(run->s,t,NULL,newTp);
            r1 = r1->Nachf;
          }
          break;
        default:
          our_error("wrong BRI(4)");
        }
      }
      run = run->Nachf;
    } while (res && (run != NULL));
    if (res){
      /*      IO_DruckeFlex("#mkTrans: %E  %C\n", e1, c);*/
      if (e1 == NULL){
        *eres = next;
      }
      else {
        r1->Nachf = next;
        *eres = e1;
      }
    }
  }
  else {
    *eres = next;
  }
  /*  IO_DruckeFlex("#mkTr: %b\n", res);*/
  return res;
}

static BOOLEAN integrateGteq(Constraint tp, UTermT s, UTermT t, EConsT next,
                             ContextT c)
{
  BOOLEAN res = FALSE;
  ContextT c1 = CO_newContext(c->bindings);
  EConsT e1;
#if MEHR_BRI
  EConsT e2;
#endif
  c1->econses = CO_newEcons(s,t,NULL,tp);
  if (bri(c1,c->econses,&e1)){
    incId(c1);
    c1->econses->Nachf = e1;
    if (!varCycle(c1)){
#if MEHR_BRI
      if(bri(c1,next,&e2) && mkTrans(c1,e2,&e1)){
        res = decompose(e1,c1);
        CO_deleteEConsList(e1,NULL);
      }
#else
      if (mkTrans(c1,next,&e1)){
        res = decompose(e1,c1);
	CO_deleteEConsList(e1,next);
      }
#endif
    }
  }
  CO_deleteEConsList(c1->econses,NULL);
  deleteContext(c1);
  
  return res;
}

static BOOLEAN integrate(Constraint tp, UTermT s, UTermT t, EConsT next,
                         ContextT c)
{
  BOOLEAN res = FALSE;
  OPENTRACE(("integrate %t %s %t (%C)\n", s, CNamen[tp], t, c));
  {
    if (tp == Eq_C){
      if (TO_TermIstVar(s)){
	if (TO_TermIstVar(t)){
	  if (TO_TopSymbol(s) < TO_TopSymbol(t)){
	    res = integrateEq(TO_TopSymbol(s),t,next,c);
	  }
	  else if (TO_TopSymbol(t) < TO_TopSymbol(s)){
	    res = integrateEq(TO_TopSymbol(t),s,next,c);
	  }
	  else {
	    our_error("versuch x an x zu binden");
	  }
	}
	else {
	  res = integrateEq(TO_TopSymbol(s),t,next,c);
	}
      }
      else if (TO_TermIstVar(t)){
	res = integrateEq(TO_TopSymbol(t),s,next,c);
      }
      else {
	our_error("versuch nichtvar an nichtvar zu binden");
      }
    }
    else {
      res = integrateGteq(tp,s,t,next,c);
    }
  }
  CLOSETRACE(("integrate -> %b\n", res));
  return res;
}

static BOOLEAN alphaC (ContextT c, UTermT s, UTermT t, EConsT next,
		       BOOLEAN skip)
{
  BOOLEAN res = FALSE;
  OPENTRACE (("ALPHA C(%s): %t :>? %t %e %C\n", skip ? "SKIP" : "nonskip",
	      s,t,next,c));
  {
    UTermT s0 = TO_NULLTerm(s);
    UTermT si = TO_ErsterTeilterm(s);
    EConsT e  = CO_newEcons(s,t,next,Ge_C);
    if (skip && (si != s0)){
      si = TO_NaechsterTeilterm(si);
    }
    while (!res && (si != s0)){
      e->s = si;
      res = decompose(e,c);
      si = TO_NaechsterTeilterm(si);
    }
    CO_deleteEcons(e);
  }
  CLOSETRACE (("ALPHA C: -> %b\n", res));
  return res;
}

static EConsT mkBeta (UTermT s, UTermT t, EConsT next)
/* arity > 0 !!! */
{
  EConsT res = newEconsList();
  UTermT ti = TO_ErsterTeilterm(t);
  UTermT t0 = TO_NULLTerm(t);
  EConsT run = res;

  run->s  = s;
  run->tp = Gt_C;
  run->t  = ti;
  ti = TO_NaechsterTeilterm(ti);
  while (ti != t0){
    run = nextEConsCell(run);
    run->s  = s;
    run->tp = Gt_C;
    run->t  = ti;
    ti = TO_NaechsterTeilterm(ti);    
  }
  snipEconsList(run);
  run->Nachf = next;
  return res;
}

static BOOLEAN betaC (ContextT c, UTermT s, UTermT t, EConsT next)
{
  BOOLEAN res = FALSE;
  OPENTRACE (("BETA C: %t :>? %t %e %C\n", s,t,next,c));
  {
    int arity = TO_AnzahlTeilterme(t);
    if (arity > 0){
      EConsT e = mkBeta (s,t,next);
      res = decompose(e,c);
      CO_deleteEConsList(e,next);
    }
    else {
      res = decompose(next,c); /* leerer Allquantor ist immer erfuellbar */ 
    }
  }
  CLOSETRACE (("BETA C: -> %b\n", res));
  return res;
}


static EConsT mkGamma (UTermT s, UTermT s1, UTermT si, UTermT s0, 
                       UTermT t1, UTermT ti, UTermT t0, 
                       EConsT next, BOOLEAN gteq)
/* arity > 0 !!! si ist maximal das letzte Element */
/* Furchtbarer Code, verbesserungsfaehig */
{
  EConsT res = newEconsList();
  EConsT run = res;
  UTermT srun = s1;
  UTermT trun = t1;

  while (srun != si){
    run->s  = srun;
    run->tp = Eq_C;
    run->t  = trun;
    srun = TO_NaechsterTeilterm(srun);
    trun = TO_NaechsterTeilterm(trun);
    run = nextEConsCell(run);
  }
  run->s  = srun;
  run->tp = Gt_C;
  run->t  = trun;
  trun = TO_NaechsterTeilterm(trun);
  while (trun != t0){
    run = nextEConsCell(run);
    run->s  = s;
    run->tp = Gt_C;
    run->t  = trun;
    trun = TO_NaechsterTeilterm(trun);    
  }
  snipEconsList(run);
  run->Nachf = next;
  if (gteq && (run->s == si)) {
    /* Patchen fuer letzten Fall bei >= */
    run->tp = Ge_C;
  }
  return res;
}

static BOOLEAN gammaC (ContextT c, UTermT s, UTermT t, EConsT next,
		       BOOLEAN gteq)
{
  BOOLEAN res = FALSE;
  OPENTRACE (("GAMMA C(%s): %t :>? %t %e %C\n", gteq ? "gteq" : "gt",
	      s,t,next,c));
  {
    int arity = TO_AnzahlTeilterme(t);
    if (arity == 0){
      if (gteq){
        res = decompose(next,c); /* a :>= a ist erfuellbar */
      }
      else {
        res = FALSE; /* a :>:a ist unerfuellbar */
      }
    }
    else {
      UTermT s0 = TO_NULLTerm(s);
      UTermT s1 = TO_ErsterTeilterm(s);
      UTermT si = s1;
      UTermT t0 = TO_NULLTerm(t);
      UTermT t1 = TO_ErsterTeilterm(t);
      UTermT ti = t1;
      do {
	EConsT e = mkGamma (s,s1,si,s0,t1,ti,t0,next,gteq);
	res = decompose(e,c);
	CO_deleteEConsList(e,next);
	si = TO_NaechsterTeilterm(si);
	ti = TO_NaechsterTeilterm(ti);
      } while (!res && (si != s0));
    } 
  }
  CLOSETRACE (("GAMMA C: -> %b\n", res));
  return res;
}

static BOOLEAN decompose_gt (ContextT c, UTermT s, UTermT t, EConsT next,
			     BOOLEAN gteq)
{
  BOOLEAN res = FALSE;
  OPENTRACE (("DECOMPOSE_GT(%s): %t :>? %t %e %C\n", gteq ? "gteq" : "gt",
	      s,t,next,c));
  {
    switch (PR_Praezedenz(TO_TopSymbol(s), TO_TopSymbol(t))){
    case Groesser:
      res = betaC(c,s,t,next);
      break;
    case Kleiner:
      res = alphaC(c,s,t,next,FALSE);
      break;
    case Unvergleichbar:
      res = alphaC(c,s,t,next,FALSE); /* ??? */
      break;
    case Gleich: 
      res = gammaC(c,s,t,next,gteq) || alphaC(c,s,t,next,TRUE);
      break;
    default:
      our_error("wrong compare 8" );
    }
  }
  CLOSETRACE (("DECOMPOSE_GT: -> %b\n", res));
  return res;
}

static EConsT mkEqs (UTermT s, UTermT t, EConsT next)
/* arity > 0 !!! */
{
  EConsT res = newEconsList();
  UTermT si = TO_ErsterTeilterm(s);
  UTermT ti = TO_ErsterTeilterm(t);
  UTermT t0 = TO_NULLTerm(t);
  EConsT run = res;

  run->s  = si;
  run->tp = Eq_C;
  run->t  = ti;
  si = TO_NaechsterTeilterm(si);
  ti = TO_NaechsterTeilterm(ti);
  while (ti != t0){
    run = nextEConsCell(run);
    run->s  = si;
    run->tp = Eq_C;
    run->t  = ti;
    si = TO_NaechsterTeilterm(si);    
    ti = TO_NaechsterTeilterm(ti);    
  }
  snipEconsList(run);
  run->Nachf = next;
  return res;
}

static BOOLEAN decompose_eq (ContextT c, UTermT s, UTermT t, EConsT next)
{
  BOOLEAN res = FALSE;
  OPENTRACE (("DECOMPOSE_EQ: %t :=: %t %e %C\n", s, t, next, c));
  /*IO_DruckeFlex("#DECOMPOSE_EQ: %t :=: %t %E %C\n", s, t, next, c);*/
  {
    if (TO_TopSymbol(s) == TO_TopSymbol(t)){
      int arity = TO_AnzahlTeilterme(s);
      if (arity == 0){
        res = decompose(next,c); /* a :=: a ist erfuellbar */
      }
      else {
        EConsT e = mkEqs(s,t,next);
        res = decompose(e,c);
        CO_deleteEConsList(e,next);
      }
    }
  }
  CLOSETRACE (("DECOMPOSE_EQ: -> %b\n", res));
  return res;
}

static BOOLEAN fertig (ContextT c)
{
  if (DO_TRACE){
    IO_DruckeFlex("\n Loesung: %C\n\n",c);
  }
  else if (VERBOSE){
    IO_DruckeFlex(" Loesung: %C\n",c);
  }
  if (dec_mode == Weitersuchen){
    lsgen = newDcons(flatCopy(c),lsgen);
    anzlsgn++;
    if ((maxlsgn > 0) && (anzlsgn > maxlsgn)){
      if (VERBOSE || DO_TRACE){
	printf("Abbrechen! Schon %d Loesungen (max %d erlaubt)\n",anzlsgn,maxlsgn);
      }
      abgebrochen = TRUE;
      return TRUE;
    }
    else {
      return FALSE;
    }
  }
  else {
    return TRUE;
  }
}

#if DEC_TRACE
static int dec_level=-1;
#endif
static BOOLEAN decompose (EConsT e, ContextT c)
{
  BOOLEAN res = FALSE;
  OPENTRACE (("DECOMPOSE: %E %C\n", e, c));
#if DEC_TRACE
  dec_level++;
  TR_Traceindent(TRUE, dec_level);
  IO_DruckeFlex("DEC %E %C\n", e, c);
#endif
#if ECONS_STAT
  printf("###%d\n", econsLength(e));
#endif
#if ALLOWED_DECOMPOSES
  performedDecomposes++;
  if (performedDecomposes > ALLOWED_DECOMPOSES){
    if (VERBOSE){
      IO_DruckeFlex("Zuviele Decomposes!\n");
    }
    return TRUE;
  }
#endif
  {
    if (e == NULL){
      res = fertig(c);
    }
    else {
      Constraint tp = e->tp;
      EConsT next = e->Nachf;
      UTermT s = deref(c,e->s);
      UTermT t = deref(c,e->t);
      BRI bri = is_important(c,s,t,tp);
      if (bri == bottom){
	res = FALSE;
      }
      else if (bri == redundant){
	res = decompose (next,c);
      }
      else {
        if (TO_TermIstVar(s) || TO_TermIstVar(t)) {
          res = integrate(tp,s,t,next,c);
        }
        else {
          switch (tp){
          case Gt_C:
            res = decompose_gt(c,s,t,next,FALSE);
            break;
          case Ge_C:
            res = decompose_gt(c,s,t,next,TRUE);
            break;
          case Eq_C:
            res = decompose_eq(c,s,t,next);
            break;
          default:
            our_error("falscher compare (7)");
          }	
        }
      }
    }
  }
#if DEC_TRACE
  TR_Traceindent(TRUE, dec_level);
  IO_DruckeFlex("DEC --> %b\n", res);
  dec_level--;
#endif
  CLOSETRACE (("DECOMPOSE: -> %b\n", res));
  return res;
  /* INVARIANTE:     RES = TRUE  -> BACKTRACKING ABBRECHEN
   *                 RES = FALSE -> BACKTRACKING WEITERMACHEN
   * GESAMTERGEBNIS: SEITENEFFEKT !!!
   */
}

/* -------------------------------------------------------------------------- */

DConsT CO_getAllSolutions(EConsT e, ContextT c)
{
  DConsT res;
  ZM_START(1);
  {
    dec_mode = Weitersuchen;
    initHash();
    /*  IO_DruckeFlex("solve %E under %C\n", e,c);*/
    decompose(e,c);
    res = lsgen;
    lsgen = NULL;
    anzlsgn = 0;
  }
  ZM_STOP(1,res);
  return res;
}

#if 0
static BOOLEAN getAllSolutionsInBothDirections(UTermT s, UTermT t, ContextT c,
					       DConsT *d1, DConsT *d2)
/* Spezialisierte Fassung fuer Confluence Trees. Caching zwischen s :>: t und
 * t :>= s. True, fals s:>: t erfuellbar. Bei False wird t :>= nicht untersucht.
 * Bei False sind *d1 == *d2 == NULL. Bei True auf jeden Fall *d1 != NULL.
 * Bei LENGTH_LIMITED wird entsprechend verfahren (verunstaltet Code!).
 */
{
  BOOLEAN res = FALSE;
  {
    EConsT e = CO_newEcons(s,t,NULL,Gt_C);
#if LENGTH_LIMITED
    unsigned int d1length,d2length;
    maxlsgn = MAX_LENGTH;
#endif
    dec_mode = Weitersuchen;
    
    initHash(); /* Nur einmal ! */

    ZM_START(1);
    decompose(e,c);
    *d1 = lsgen;
    d1length = anzlsgn;
    lsgen = NULL;
    anzlsgn = 0;
    /*    ZM_STOP(1);*/
    gettimeofday(&after,NULL);
    if (before.tv_usec > after.tv_usec){
      after.tv_sec  = after.tv_sec - 1 - before.tv_sec;
      after.tv_usec = after.tv_usec + 1000000 - before.tv_usec;
    }
    else {
      after.tv_sec  = after.tv_sec - before.tv_sec;
      after.tv_usec = after.tv_usec - before.tv_usec;
    }
    printf("#%d %ld %6ld %6d\n", 2, after.tv_sec, after.tv_usec, number2);  
    if (after.tv_usec > 1000){
      blabla = 1;
      decompose(e,c);
      CO_deleteDConsList(lsgen);
      lsgen = NULL;
      anzlsgn = 0;
      blabla = 0;
    }
  }

    if (*d1 != NULL){   /* der neue Constraint muss erfuellbar sein */
#if LENGTH_LIMITED
      if (d1length > MAX_LENGTH){
	CO_deleteDConsList(*d1);
	res = FALSE;
	*d1 = NULL;
	*d2 = NULL;
      }
      else
#endif
      {
	res = TRUE;
	e->s = t;
	e->t = s;
	e->tp = Ge_C;
	ZM_START(1);
	initHash(); /* Nur einmal ! */
	maxlsgn = MAX_LENGTH - d1length;
	decompose(e,c);
	*d2 = lsgen;
	d2length = anzlsgn;
	lsgen = NULL;
	anzlsgn = 0;
	/*	ZM_STOP(1);*/
    gettimeofday(&after,NULL);
    if (before.tv_usec > after.tv_usec){
      after.tv_sec  = after.tv_sec - 1 - before.tv_sec;
      after.tv_usec = after.tv_usec + 1000000 - before.tv_usec;
    }
    else {
      after.tv_sec  = after.tv_sec - before.tv_sec;
      after.tv_usec = after.tv_usec - before.tv_usec;
    }
    printf("#%d %ld %6ld %6d\n", 2, after.tv_sec, after.tv_usec, number2);  
    if (after.tv_usec > 1000){
      blabla = 1;
      decompose(e,c);
      CO_deleteDConsList(lsgen);
      lsgen = NULL;
      anzlsgn = 0;
      blabla = 0;
    }
  }

#if LENGTH_LIMITED
	if (d1length + d2length > MAX_LENGTH){
	  CO_deleteDConsList(*d1);
	  CO_deleteDConsList(*d2);
	  res = FALSE;
	  *d1 = NULL;
	  *d2 = NULL;
	}
#endif
      }
    }
    CO_deleteEcons(e);
  }
  return res;
}
#else
int CO_getAllSolutionsInBothDirections(UTermT s, UTermT t, ContextT c,
					       DConsT *d1, DConsT *d2)
/* Spezialisierte Fassung fuer Confluence Trees. Caching zwischen s :>: t und
 * t :>= s. True, fals s:>: t erfuellbar. Bei False wird t :>= nicht untersucht.
 * Bei False sind *d1 == *d2 == NULL. Bei True auf jeden Fall *d1 != NULL.
 * Bei LENGTH_LIMITED wird entsprechend verfahren (verunstaltet Code!).
 */
/* res = 0: unerfuellbar, res = 1: OK, res = 2: zuviele Lsg. */
{
  int res = 0;
  {
    EConsT e = CO_newEcons(s,t,NULL,Gt_C);
    unsigned int d1length,d2length;
    if (length_limit > 0){
      maxlsgn = MAX_LENGTH;
    }
    dec_mode = Weitersuchen;
    
    initHash(); /* Nur einmal ! */

    ZM_START(1);
    decompose(e,c);
    *d1 = lsgen;
    d1length = anzlsgn;
    lsgen = NULL;
    anzlsgn = 0;
    ZM_STOP(1,d1length);

    if (*d1 != NULL){   /* der neue Constraint muss erfuellbar sein */
      if ((length_limit > 0) && (d1length > length_limit)){
	CO_deleteDConsList(*d1);
	res = 2;
	*d1 = NULL;
	*d2 = NULL;
      }
      else {
	res = 1;
	e->s = t;
	e->t = s;
	e->tp = Ge_C;
	ZM_START(1);
	initHash(); /* Nur einmal ! */
	maxlsgn = MAX_LENGTH - d1length;
	decompose(e,c);
	*d2 = lsgen;
	d2length = anzlsgn;
	lsgen = NULL;
	anzlsgn = 0;
        ZM_STOP(1,d2length);
	if ((length_limit > 0)&& (d1length + d2length > length_limit)){
	  CO_deleteDConsList(*d1);
	  CO_deleteDConsList(*d2);
	  res = 2;
	  *d1 = NULL;
	  *d2 = NULL;
	}
      }
    }
    CO_deleteEcons(e);
  }
  return res;
}
#endif
BOOLEAN CO_satisfiable(EConsT e)
{
  BOOLEAN res, ressuff;
  ZM_START(0);
  {
    ContextT c = CO_newContext(CO_allocBindings());
    dec_mode = Abbrechen;
    initHash();
    res = decompose(e,c);
    CO_flatDelete(c);
  }
  ZM_STOP(0,res);
  return res;
}

BOOLEAN CO_satisfiableUnder(EConsT e, ContextT c)
{
  BOOLEAN res;
  ZM_START(0);
  /*  IO_DruckeFlex("@@%E %C.\n", e,c);*/
  {
    dec_mode = Abbrechen;
    initHash();
    res = decompose(e,c);
  }
  ZM_STOP(0,res);
  return res;
}

/* -------------------------------------------------------------------------- */

BOOLEAN CO_TermGroesserUnif(TermT s, TermT t, TermT u, TermT v)
/* Fuer den Ordnungscheck beim Ueberlappen 
 * Ggf. sind u und v gleich NULL und dann nicht zu betrachten.
 * TRUE, wenn die Ueberlappung VERWORFEN werden kann, also bisher
 * wenn s > t oder u > v
 * Mit Constraints:
 * FALSE, wenn (t :>: s) & (v :>: u) erfuellbar
 */
{
  our_error("merger problem ORD_OrdIstcLPO");
  if (DO_OVERLAP_CONSTRAINTS /*&& ORD_OrdIstcLPO()*/){
    EConsT e1,e2=NULL;
    BOOLEAN res;
    if (u != NULL) {
      e2 = CO_newEcons(v,u,NULL,Gt_C);
    }
    e1 = CO_newEcons(t,s,e2,Gt_C);
    res = !CO_satisfiable(e1);
    CO_deleteEcons(e1);
    if (u != NULL){
      CO_deleteEcons(e2);
    }
    return res;
  }
  else {
    return ORD_TermGroesserUnif(s,t) || 
           ((u != NULL) && ORD_TermGroesserUnif(u,v));
  }
}

/* -------------------------------------------------------------------------- */
/* - Minimizing disjunctive constraints ------------------------------------- */
/* -------------------------------------------------------------------------- */

static BOOLEAN co_constraintImpliesBindings(ContextT c, Bindings bs)
/* return Sol(c) \subseteq Sol(bs) */
{
  SymbolT xi;
  SO_forSymboleAbwaerts(xi,SO_ErstesVarSymb,NewestVar){
    if (bs[xi] != NULL){
      TermT x = TO_(xi);
      if (!cEQ(c,x,bs[xi])){
        return FALSE;
      }
      TO_TermLoeschen(x);
    }
  }
  return TRUE;
}

static BOOLEAN co_constraintImpliesEConses(ContextT c, EConsT e)
/* return Sol(c) \subseteq Sol(e) */
{
  EConsT e1 = e;
  while(e1 != NULL){
    if (is_important(c,e1->s,e1->t,e1->tp) != redundant){
      return FALSE;
    }
    e1 = e1->Nachf;
  }
  return TRUE;
}

static BOOLEAN co_constraintImpliesConstraint(ContextT c, ContextT c1)
/* return Sol(c) \subseteq Sol(c1) */
{
  return co_constraintImpliesBindings(c,c1->bindings) &&
         co_constraintImpliesEConses(c,c1->econses);
}

static BOOLEAN co_constraintImpliesSomeOther(ContextT c, DConsT d)
/* return Sol(c) \subseteq Sol(d) */
{
  DConsT d1 = d;
  while (d1 != NULL){
    ContextT c1 = d1->c;
    if (c != c1){
      if (co_constraintImpliesConstraint(c, c1)){
        return TRUE;
      }
    }
    d1 = d1->Nachf;
  }
  return FALSE;
}

static DConsT co_deleteImplyingOthers(DConsT d, ContextT c)
/* d := d - {c' | c' in d, Sol(c') \subseteq c}
 * recursive formulation (because it's simpler ;-)
 */
{
  if (d == NULL){
    return NULL;
  }
  else {
    DConsT rest = co_deleteImplyingOthers(d->Nachf,c);
    ContextT c1 = d->c;
    if (co_constraintImpliesConstraint(c1,c)){
      CO_flatDelete(c1);
      d->Nachf = NULL;
      d->c = NULL;
      deleteDcons(d);
      return rest;
    }
    else {
      d->Nachf = rest;
      return d;
    }
  }
}

static EConsT co_correspondingECons(SymbolT xi, TermT t, EConsT e)
/* search xi :>:t or t :>: xi in e */
{
  EConsT e1 = e;
  while (e1 != NULL){
    if (((TO_TopSymbol(e1->s) == xi) && TO_TermeGleich(t, e1->t)) ||
        ((TO_TopSymbol(e1->t) == xi) && TO_TermeGleich(t, e1->s))   ){
      return e1;
    }
    e1 = e1->Nachf;
  }
  return NULL;
}

static BOOLEAN co_canMergeTwo(ContextT c1, ContextT c2, DConsT d)
/* if c1 == s :=: t & c' and c2 == s :>: t & c''  and some other conditions then
 *    if d == NULL then
 *       c2 := s :>= t & c''  // i.e., modify s :>: t of c2
 *    else 
 *       flatDelete (d->c) // identical to c1
 *       d->c := flatCopy(c2)
 *       d->c := s :>= t & c'' // i.e., modify s :>: t of d->c
 */
{
  SymbolT xi;
  SO_forSymboleAbwaerts(xi,SO_ErstesVarSymb,NewestVar){
    TermT t = c1->bindings[xi];
    if (t != NULL){
      EConsT e = co_correspondingECons(xi,t,c2->econses);
      if (e != NULL) { /* check for additional conditions */
        Constraint tp = e->tp;
        e->tp = Ge_C; 
        if (co_constraintImpliesConstraint(c1,c2)){
          BOOLEAN memo;
          if (c2->bindings[xi] != NULL){
            our_error("Cannot happen ;-(");
          }
          c2->bindings[xi] = t;
          memo = co_constraintImpliesConstraint(c2,c1);
          c2->bindings[xi] = NULL;
          if (memo){
            if (d == NULL){
              /* c2 is already modified */
              return TRUE;
            }
            else {
              CO_flatDelete(d->c);
              d->c = flatCopy(c2);
              /* restore c2 */
              e->tp = tp; 
              return TRUE;
            }
          }
        }
        /* Additional conditions failed -> undo change */
        e->tp = tp; 
      }
    }
  }  
  return FALSE;
}

static BOOLEAN co_canMerge(ContextT c, DConsT d)
/* if c can be merged with some c' in d then modify d */
{
  while (d != NULL){
    if (co_canMergeTwo(c,d->c,NULL) || co_canMergeTwo(d->c,c,d)){
      return TRUE;
    }
    d = d->Nachf;
  }
  return FALSE;
}

static DConsT co_handleContext(ContextT c, DConsT d)
/* perform the following optimizations:
 * - if c implies some c' in d then c is not necessary
 * - all c' in d that imply c are deleted
 * - if c can be merged with some c' in d then modify c'
 *   (c is then superfluous)
 * otherwise c is added to d
 * the new d is returned
 */
{
  DConsT d1 = d;
  if (co_constraintImpliesSomeOther(c,d1)){
    return d1;
  }
  d1 = co_deleteImplyingOthers(d1,c);
  if (PA_mergeDCons() && co_canMerge(c,d1)){
    return d1;
  }
  else {
    return newDcons(flatCopy(c),d1);
  }
}

DConsT CO_minimizeDCons(DConsT d)
{
  DConsT res = NULL;
  DConsT drun = d;
  while (drun != NULL){
    res = co_handleContext(drun->c,res);
    drun = drun->Nachf;
  }
  return res;
}

/* -------------------------------------------------------------------------- */
/* - Stuff for the CONSTRAINT MODE ------------------------------------------ */
/* -------------------------------------------------------------------------- */

static BOOLEAN setupDecompose (UTermT s, UTermT t, Constraint tp)
{
  int res;
  OPENTRACE  (("setupDecompose %t %s %t\n", s, CNamen[tp],t));
  {
    EConsT e   = CO_newEcons(s,t,NULL,tp);
    Bindings b;
    ContextT c;
    b = CO_allocBindings();
    c = CO_newContext(b);
    initHash();
    if (!DO_TRACE){
      IO_DruckeFlex("checking %t %s %t\n", s, CNamen[tp],t);
    }
    res = decompose(e,c);
    deleteContext(c);
    deallocBindings(b);
    CO_deleteEcons(e);
    /*    printf("##%d\n",CO_DConsLength(lsgen));*/
    CO_deleteDConsList(lsgen);
    lsgen = NULL;
    anzlsgn = 0;
  }
  CLOSETRACE (("setupDecompose -> %b\n", res));
  return res;
}

static EConsT mkECons(UTermT s)
{
  EConsT res = NULL;
  OPENTRACE  (("mkECons %t\n", s));
  if (TO_TermIstVar(s)){
    our_error("Formaterror 11");
  }
  if (strcmp(IO_FktSymbName(TO_TopSymbol(s)),"cgt") == 0){
    res = CO_newEcons(TO_ErsterTeilterm(s), TO_ZweiterTeilterm(s), NULL, Gt_C);
  }
  else if (strcmp(IO_FktSymbName(TO_TopSymbol(s)),"cge") == 0){
    res = CO_newEcons(TO_ErsterTeilterm(s), TO_ZweiterTeilterm(s), NULL, Ge_C);
  }
  else if (strcmp(IO_FktSymbName(TO_TopSymbol(s)),"ceq") == 0){
    res = CO_newEcons(TO_ErsterTeilterm(s), TO_ZweiterTeilterm(s), NULL, Eq_C);
  }
  else {
    our_error("Formaterror 12");
  }
  CLOSETRACE (("mkECons -> %e\n", res));
  return res;
}

EConsT CO_constructECons(UTermT s)
{
  EConsT res = NULL;
  EConsT e = NULL;
  OPENTRACE  (("CO_constructECons %t\n", s));
  while (!TO_TermIstVar(s) && 
	 strcmp(IO_FktSymbName(TO_TopSymbol(s)),"and") == 0){
    if (res == NULL){
      res =  mkECons(TO_ErsterTeilterm(s));
      e = res;
    }
    else {
      e->Nachf = mkECons(TO_ErsterTeilterm(s));
      e = e->Nachf;
    }
    s = TO_ZweiterTeilterm(s);
  }
  if (res == NULL){
    res =  mkECons(s);
    e = res;
  }
  else {
    e->Nachf = mkECons(s);
  }
  CLOSETRACE (("CO_constructECons -> %E\n", res));
  return res;
}

static BOOLEAN constructBool(UTermT s)
{
  BOOLEAN res = FALSE;
  OPENTRACE  (("constructBool %t\n", s));
  if (TO_TermIstVar(s)){
    our_error("Formaterror 1");
  }
  if (strcmp(IO_FktSymbName(TO_TopSymbol(s)),"True") == 0){
    res = TRUE;
  }
  else if (strcmp(IO_FktSymbName(TO_TopSymbol(s)),"False") == 0){
    res = FALSE;
  }
  else {
    our_error("Formaterror 2");
  }
  CLOSETRACE (("constructBool -> %b\n", res));
  return res;
}

static BOOLEAN checkConstraint (EConsT e, BOOLEAN soll)
{
  BOOLEAN res = FALSE;
  OPENTRACE  (("checkConstraint %E ->? %b\n", e, soll));
  {
    BOOLEAN haben;
    Bindings b;
    ContextT c;
#if 0
    b = CO_allocBindings();
    c = CO_newContext(b);
#endif
#if VERBOSE
    IO_DruckeFlex("%E -->? %b\n", e,soll);
#endif
#if 0
    initHash();
    haben = decompose(e,c);
    deleteContext(c);
    deallocBindings(b);
#endif
    haben = CO_satisfiable(e);
#if VERBOSE
    IO_DruckeFlex("Loesungen: %D\n", lsgen);
#endif
#if 0
    if ((!soll && (lsgen != NULL)) || (soll && (lsgen == NULL)))
#else
    if (soll != haben)
#endif
      {
      /*    our_error("sch***");*/
      IO_DruckeFlex("##Problem##\n");
      res = FALSE;
    }
    else {
      res = TRUE;
    }
    /*    printf("##%d\n",CO_DConsLength(lsgen));*/
#if 0
    CO_deleteDConsList(lsgen);
    lsgen = NULL;
    anzlsgn = 0;
#endif
  }
  CLOSETRACE (("checkConstraint -> %b\n", res));
  return res;
}

BOOLEAN CO_decompose (UTermT s, UTermT t)
{
  BOOLEAN res = FALSE;
  EConsT e;
  BOOLEAN soll;
  dec_mode = Weitersuchen; /*Abbrechen;*/
  if (!TO_TermIstVar(t) &&
      ((strcmp(IO_FktSymbName(TO_TopSymbol(t)),"False") == 0) ||
       (strcmp(IO_FktSymbName(TO_TopSymbol(t)),"True")  == 0)    )){
    e = CO_constructECons(s);
    soll = constructBool(t);
    res = checkConstraint(e,soll);
    CO_deleteEConsList(e,NULL);
  }
  else if (!TO_TermIstVar(s) &&
	   ((strcmp(IO_FktSymbName(TO_TopSymbol(s)),"False") == 0) ||
	    (strcmp(IO_FktSymbName(TO_TopSymbol(s)),"True")  == 0)    )){
    e = CO_constructECons(t);
    soll = constructBool(s);
    res = checkConstraint(e,soll);
    CO_deleteEConsList(e,NULL);
  }
  else {
    setupDecompose(s,t,Gt_C);
    setupDecompose(s,t,Ge_C);
    setupDecompose(s,t,Eq_C);
    setupDecompose(t,s,Gt_C);
    setupDecompose(t,s,Ge_C);
    setupDecompose(t,s,Eq_C);
    
    res = TRUE;
  }
  return res;
}

/* -------------------------------------------------------------------------- */
/* - Initialization --------------------------------------------------------- */
/* -------------------------------------------------------------------------- */

void CO_ConstraintsInitialisieren(void)
{
  EConsManagerInitialisieren();
  ContextManagerInitialisieren();
  DConsManagerInitialisieren();
}
