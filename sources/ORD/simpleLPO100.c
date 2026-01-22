/* ------------------------------------------------------------------------- */
/* ----------- 1. Direct (naive) implementation --- Bottom Up -------------- */
/* ------------------------------------------------------------------------- */

#include "simpleLPO_private.h"

/* - Parameter -------------------------------------------------------------- */

#define ausgaben 0 /* 0, 1 */
#define DEBUG 0 /* 0, 1 */

/* - Macros ----------------------------------------------------------------- */

#if DEBUG
#include <assert.h>
#else
#define assert(x) /* nix */
#endif

#if ausgaben
#define blah(x) { IO_DruckeFlex x ; }
#else
#define blah(x)
#endif

/* -------------------------------------------------------------------------- */

typedef enum {c_Groesser, c_Nichtgroesser, c_Kleiner, c_Nichtkleiner, c_Gleich,
              c_Unvergleichbar, c_Unbekannt} CompT;

/* Fuer schoene Ausgaben. */
static char *CompTChars[] = {" >","!>"," <","!<"," =","un","??"};

static CompT swap_compT(CompT rel) 
{
  static char swaptab[] = {
    c_Kleiner, c_Nichtkleiner, c_Groesser, c_Nichtgroesser, 
    c_Gleich, c_Unvergleichbar, c_Unbekannt
  };

  return swaptab[(rel)];
}

static VergleichsT convCTVT(CompT rel) 
{
  if (rel == c_Groesser){
    return Groesser;
  } if (rel == c_Kleiner){
    return Kleiner;
  } if (rel == c_Gleich){
    return Gleich;
  } else {
    return Unvergleichbar;
  }
}


/* -------------------------------------------------------------------------- */
/* - LPO_RelTab - Relations-Matrix ------------------------------------------ */
/* -------------------------------------------------------------------------- */

/* Speichert bekannte Ordnungsbeziehungen zwischen den Teiltermen von s und t.
   Enthaelt Rueckpointer: TermID --> Term
   Quadratisch: (|s|+|t|)x(|s|+|t|)
   Zellen: CompT */
typedef struct LPO_RelTabT {
  unsigned short maxWidth; /* Invariante: maxWidth >= aktWidth */
  unsigned short aktWidth; /* Invariante: entpricht |s|+|t| */
  UTermT *terms; /* Pointer auf die TT von s und t: TermID --> Term
                    Invariante: |Ids von s|+|Ids von t| = aktWidth */
  CompT *rels;   /* Ordnungsbeziehungen */
} LPO_RelTab;

static LPO_RelTab relations; /* Existiert genau _einmal_ pro Waldmeisterlauf. */

static void LPO_RelTab_reset(void)
{
  unsigned  i;
  unsigned  m = relations.maxWidth*relations.maxWidth;
  for (i = 0; i < m; i++){
    relations.rels[i] = c_Unbekannt;
  }
  m = relations.maxWidth;
  for (i = 0; i < m; i++){
    relations.terms[i] = NULL;
  }
}

static void LPO_RelTab_init(unsigned short aktWidth)
{
  blah(("LPO_RelTab_init:\n"));
  /* Wurde LPO_RelTab noch nicht initialisiert ? */
  if (relations.maxWidth == 0){
    blah(("LPO_RelTab wird zum ersten mal initialisiert.\n"));    
    relations.aktWidth = aktWidth;
    relations.maxWidth = relations.aktWidth;
    relations.rels = our_alloc((relations.maxWidth*relations.maxWidth)
                               * sizeof(CompT));
    relations.terms = our_alloc(relations.maxWidth * sizeof(UTermT));
  } else {
    /* LPO_RelTab wurde schon initialisiert, gross genug? */
    if (relations.maxWidth < aktWidth){
      blah(("LPO_RelTab war schon initialisiert, ist aber zu klein.\n"));
      relations.aktWidth = aktWidth;
      relations.maxWidth = relations.aktWidth;
      relations.rels = our_realloc(relations.rels,
                                   (relations.maxWidth*relations.maxWidth)
                                   * sizeof(CompT), "Extending LPO_RelTab");
      relations.terms = our_realloc(relations.terms,
                                    relations.maxWidth * sizeof(UTermT),
                                   "Extending LPO_RelTab");
      blah(("LPO_RelTab war schon initialisiert und ist jetzt gross genug\n"));
    }
    blah(("LPO_RelTab war schon initialisiert und ist gross genug\n"));
    relations.aktWidth = aktWidth;
  }
  LPO_RelTab_reset();
}


static CompT LPO_RelTab_getRel(UTermT s, UTermT t)
{
  unsigned short sid, tid;
  sid = TO_GetId(s);
  tid = TO_GetId(t);
  assert(sid < relations.aktWidth);
  assert(tid < relations.aktWidth);
  assert(relations.rels[sid*relations.aktWidth + tid] != c_Unbekannt);

/*   blah(("LPO_RelTab_getRel(%t (%d), %t (%d)), rel= %s\n", s, sid, t, tid, */
/*         CompTChars[relations.rels[sid*relations.aktWidth + tid]])); */
  return relations.rels[sid*relations.aktWidth + tid];
}

/* traegt auch Symmetrie ein */
static void LPO_RelTab_setRel(UTermT s, UTermT t, CompT rel)
{
  unsigned short sid, tid;
  sid = TO_GetId(s);
  tid = TO_GetId(t);
  assert(sid < relations.aktWidth);
  assert(tid < relations.aktWidth);
  assert (relations.rels[sid*relations.aktWidth + tid] == c_Unbekannt);
  assert (relations.rels[tid*relations.aktWidth + sid] == c_Unbekannt);
  relations.rels[sid*relations.aktWidth + tid] = rel;
  relations.rels[tid*relations.aktWidth + sid] = swap_compT(rel); /*Symmetrie*/
}

static UTermT LPO_RelTab_getTerm(unsigned short id)
{
  assert (id < relations.aktWidth); 
  assert (relations.terms[id] != NULL);
  return relations.terms[id];
}

static void LPO_RelTab_setTerm(unsigned short id, UTermT term)
{
  assert (id < relations.aktWidth);
  assert (relations.terms[id] == NULL);
  relations.terms[id] = term;
}

static void LPO_RelTab_print(void)
{
  if (ausgaben){
    unsigned short idx, idy;
    blah(("### LPO_RelTab:\n"));
    blah(("----------------------------------------\n"));
    blah(("   "));
    for (idx = 0; idx < relations.aktWidth; idx++){
      printf("%2d ", idx);
    }
    blah(("\n"));
    for (idy = 0; idy < relations.aktWidth; idy++){
      printf("%2d ", idy);
      for (idx = 0; idx < relations.aktWidth; idx++){
        blah(("%s ", CompTChars[relations.rels[idy*relations.aktWidth + idx]]));
      }
      blah(("\n"));
    }
    blah(("----------------------------------------\n"));
    blah(("Terme:\n"));
    for (idy = 0; idy < relations.aktWidth; idy++){
      blah(("%d: %t\n", idy, LPO_RelTab_getTerm(idy)));
    }
    blah(("----------------------------------------\n"));
  }
}

/* LPO_RelTab anpassen, Terme numerieren, Rueckpointer in LPO_RelTab,
 * Gleichheit auf Diagonale der LPO_RelTab eintragen */
static void LPO_RelTab_prepare(UTermT s, UTermT t)
{
  unsigned short id;
  UTermT tmpTerm, NullTerm;

  LPO_RelTab_init(TO_Termlaenge(s)+TO_Termlaenge(t));

  NullTerm = TO_NULLTerm(s);
  id = 0;
  for (tmpTerm = s; tmpTerm != NullTerm; tmpTerm = TO_Schwanz(tmpTerm)){
    TO_SetId(tmpTerm, id);
    LPO_RelTab_setTerm(id, tmpTerm);
    LPO_RelTab_setRel(tmpTerm, tmpTerm, c_Gleich);
    id++;
  }

  NullTerm = TO_NULLTerm(t);
  /* id laueft weiter */
  for (tmpTerm = t; tmpTerm != NullTerm; tmpTerm = TO_Schwanz(tmpTerm)){
    TO_SetId(tmpTerm, id);
    LPO_RelTab_setTerm(id, tmpTerm);
    LPO_RelTab_setRel(tmpTerm, tmpTerm, c_Gleich);
    id++;
  }
}

static BOOLEAN s_LPO_Equal1BU(UTermT s, UTermT t)
{
  BOOLEAN res;
  UTermT s_i, t_i, s_0;

  OPENTRACE(("S_LPO_Equal1BU: s = %t,  t = %t\n", s, t));
  {
    s_0 = TO_NULLTerm(s);
    s_i = TO_ErsterTeilterm(s);
    t_i = TO_ErsterTeilterm(t);

    res = TO_TopSymboleGleich(s,t);
    while (res && s_i != s_0){ /* impliziert t_i != t_NULL (feste Stelligk.) */
      res = (LPO_RelTab_getRel(s_i, t_i) == c_Gleich);
      s_i = TO_NaechsterTeilterm(s_i);
      t_i = TO_NaechsterTeilterm(t_i);
    }
  }
  CLOSETRACE(("s_LPO_Equal1BU: s = %t,  t = %t --> %b\n", s, t, res));
  return res;
}

/* ------------------------------------------------------------------------- */
/* ----------- 1. Direct (naive) implementation ---------------------------- */
/* ------------------------------------------------------------------------- */


/* ------------------------------------------------------------------------- */
/* ------------- Internal forward declarations ----------------------------- */
/* ------------------------------------------------------------------------- */

static BOOLEAN s_LPO_Greater1BU(UTermT s, UTermT t);

static BOOLEAN s_LPO_Alpha1BU(UTermT ss, UTermT s_NULL, UTermT t);
static BOOLEAN s_LPO_Gamma1BU(UTermT s, UTermT t);
static BOOLEAN s_LPO_Beta1BU(UTermT s, UTermT t);
static BOOLEAN s_LPO_Delta1BU(UTermT s, UTermT t);

static BOOLEAN s_LPO_Majo1BU(UTermT s, UTermT ts, UTermT t_NULL);
static BOOLEAN s_LPO_Lex1BU(UTermT ss, UTermT s_NULL, UTermT ts, UTermT t_NULL);

/* ------------------------------------------------------------------------- */
/* -------------- Interface to the outside --------------------------------- */
/* ------------------------------------------------------------------------- */

/* realisiert Dynamic Programming, benutzt dabei LPO_RelTab */
VergleichsT S_LPO1BU(UTermT s, UTermT t)
{
  int id_t, id_s, lastid_t, lastid_s, firstid_t, firstid_s;
  UTermT tterm_s, tterm_t;
  VergleichsT res;

  OPENTRACE(("S_LPO1BU: s = %t,  t = %t\n", s, t));
  {
    LPO_RelTab_prepare(s,t);
    LPO_RelTab_print();
    lastid_s = TO_GetId(s);
    firstid_s = TO_GetId(TO_TermEnde(s));
    lastid_t = TO_GetId(t);
    firstid_t = TO_GetId(TO_TermEnde(t));
    for (id_s = firstid_s; id_s >= lastid_s; id_s--){
      tterm_s = LPO_RelTab_getTerm(id_s);
      for (id_t = firstid_t; id_t >= lastid_t; id_t--){
        if (id_s == id_t) break; /* nur oberes Dreieck */
        tterm_t = LPO_RelTab_getTerm(id_t);
        blah(("%d %t, %d %t\n",id_s,tterm_s,id_t,tterm_t));

        if (s_LPO_Equal1BU(tterm_s, tterm_t)){
          LPO_RelTab_setRel(tterm_s, tterm_t, c_Gleich);
        } else if (s_LPO_Greater1BU(tterm_s, tterm_t)){
          LPO_RelTab_setRel(tterm_s, tterm_t, c_Groesser);
        } else if (s_LPO_Greater1BU(tterm_t, tterm_s)){
          LPO_RelTab_setRel(tterm_s, tterm_t, c_Kleiner);
        } else {
          LPO_RelTab_setRel(tterm_s, tterm_t, c_Unvergleichbar);
        }
        LPO_RelTab_print();      
      }
    }
    res = convCTVT(LPO_RelTab_getRel(s, t));
  }
  CLOSETRACE(("S_LPO1BU: s = %t,  t = %t --> %v\n\n", s, t, res));
  return res;
}

/* realisiert Dynamic Programming, benutzt dabei LPO_RelTab */
BOOLEAN S_LPOGroesser1BU(UTermT s, UTermT t)
{
  int id_t, id_s, lastid_t, lastid_s, firstid_t, firstid_s;
  UTermT tterm_s, tterm_t;
  BOOLEAN res;

  OPENTRACE(("S_LPOGroesser1BU: s = %t,  t = %t\n", s, t));
  {
    LPO_RelTab_prepare(s,t);
    LPO_RelTab_print();
    lastid_s = TO_GetId(s);
    firstid_s = TO_GetId(TO_TermEnde(s));
    lastid_t = TO_GetId(t);
    firstid_t = TO_GetId(TO_TermEnde(t));
    for (id_s = firstid_s; id_s >= lastid_s; id_s--){
      tterm_s = LPO_RelTab_getTerm(id_s);
      for (id_t = firstid_t; id_t >= lastid_t; id_t--){
        if (id_s == id_t) break; /* nur oberes Dreieck */
        tterm_t = LPO_RelTab_getTerm(id_t);
        blah(("%d %t, %d %t\n",id_s,tterm_s,id_t,tterm_t));

        if (s_LPO_Equal1BU(tterm_s, tterm_t)){
          LPO_RelTab_setRel(tterm_s, tterm_t, c_Gleich);
        } else if (s_LPO_Greater1BU(tterm_s, tterm_t)){
          LPO_RelTab_setRel(tterm_s, tterm_t, c_Groesser);
        } else {
          LPO_RelTab_setRel(tterm_s, tterm_t, c_Nichtgroesser);
        }
        LPO_RelTab_print();      
      }
    }
    res = (LPO_RelTab_getRel(s, t) == c_Groesser);
  }
  CLOSETRACE(("S_LPOGroesser1BU: s = %t,  t = %t --> %b\n", s, t, res));
  return res;
}

/* ------------------------------------------------------------------------- */
/* -------------- Internal functionality ----------------------------------- */
/* ------------------------------------------------------------------------- */

static BOOLEAN s_LPO_Greater1BU(UTermT s, UTermT t)
{
  BOOLEAN res;

  OPENTRACE(("s_LPO_Greater1BU: s = %t,  t = %t\n", s, t));
  {
#if S_doCounting
    S_counter++;
#endif
    if (TO_TermIstNichtVar(s)){
      if (TO_TermIstNichtVar(t)){
        res = s_LPO_Alpha1BU(TO_ErsterTeilterm(s), TO_NULLTerm(s), t)
              || s_LPO_Beta1BU(s,t)
              || s_LPO_Gamma1BU(s,t);
      }
      else {
        res = s_LPO_Delta1BU(s,t);
      }
    }
    else {
      res = FALSE;
    }
  }
  CLOSETRACE(("s_LPO_Greater1BU: s = %t,  t = %t --> %b\n", s, t, res));
  return res;
}

/* ------------------------------------------------------------------------- */

static BOOLEAN s_LPO_Alpha1BU(UTermT ss, UTermT s_NULL, UTermT t)
{
  BOOLEAN res = FALSE; /* default for empty list */
  
  OPENTRACE(("s_LPOAlpha1BU: ss = %A,  t = %t\n", ss, s_NULL, t));
  {
    UTermT s_i = ss;
    while(s_i != s_NULL) {    
      blah(("s_i= %t id= %d, t= %t id= %d\n",s_i,TO_GetId(s_i),t,TO_GetId(t)));
      if (LPO_RelTab_getRel(s_i, t) == c_Gleich
          || LPO_RelTab_getRel(s_i, t) == c_Groesser){
        res = TRUE;
        break;
      }
      else {
        s_i = TO_NaechsterTeilterm(s_i);
      }
    }
  }
  CLOSETRACE(("s_LPOAlpha1BU: ss = %A,  t = %t --> %b\n", ss, s_NULL, t, res));
  blah(("res %b\n",res));
  return res;
}

static BOOLEAN s_LPO_Beta1BU(UTermT s, UTermT t) 
{
  BOOLEAN res;

  OPENTRACE(("s_LPO_Beta1BU: s = %t,  t = %t\n", s, t));
  {
    res = (PR_Praezedenz(TO_TopSymbol(s), TO_TopSymbol(t)) == Groesser) &&
          s_LPO_Majo1BU(s, TO_ErsterTeilterm(t), TO_NULLTerm(t));
  }
  CLOSETRACE(("s_LPO_Beta1BU: s = %t,  t = %t --> %b\n", s, t, res));
  return res;
}


static BOOLEAN s_LPO_Gamma1BU(UTermT s, UTermT t) 
{
  BOOLEAN res;

  OPENTRACE(("s_LPO_Gamma1BU: s = %t,  t = %t\n", s, t));
  {
    res = SO_SymbGleich(TO_TopSymbol(s), TO_TopSymbol(t))     &&
          s_LPO_Lex1BU(TO_ErsterTeilterm(s), TO_NULLTerm(s), 
                     TO_ErsterTeilterm(t), TO_NULLTerm(t) )   &&
          s_LPO_Majo1BU(s, TO_ErsterTeilterm(t), TO_NULLTerm(t));
  }
  CLOSETRACE(("s_LPO_Gamma1BU: s = %t,  t = %t --> %b\n", s, t, res));
  return res;
}

static BOOLEAN s_LPO_Delta1BU(UTermT s, UTermT t) 
{
  BOOLEAN res;

  OPENTRACE(("s_LPODelta1BU: s = %t,  t = %t\n", s, t));
  {
    res = s_LPO_Alpha1BU(TO_ErsterTeilterm(s), TO_NULLTerm(s), t);
  }
  CLOSETRACE(("s_LPODelta1BU: s = %t,  t = %t --> %b\n", s, t, res));
  return res;
}

/* ------------------------------------------------------------------------- */

static BOOLEAN s_LPO_Majo1BU(UTermT s, UTermT ts, UTermT t_NULL)
{
  BOOLEAN res = TRUE; /* default value for empty list */

  OPENTRACE(("s_LPO_Majo1BU: s = %t, ts = %A\n", s, ts, t_NULL));
  {
    UTermT t_j = ts;
    while(t_j != t_NULL){
      if (LPO_RelTab_getRel(s, t_j) == c_Groesser) {
        t_j = TO_NaechsterTeilterm(t_j);
      }
      else {
        res = FALSE;
        break;
      }
    }
  }
  CLOSETRACE(("s_LPO_Majo1BU: s = %t, ts = %A --> %b\n", s, ts, t_NULL, res));
  return res;
}

static BOOLEAN s_LPO_Lex1BU(UTermT ss, UTermT s_NULL, UTermT ts, UTermT t_NULL)
{
  BOOLEAN res = FALSE; /* default value for empty list */

  OPENTRACE(("s_LPO_Lex1BU: ss = %A, ts = %A\n", ss, s_NULL, ts, t_NULL));
  {
    UTermT s_i  = ss;
    UTermT t_i  = ts;
    while (s_i != s_NULL){ /* Implies t_i != t_NULL (fixed arity!) */ 
      if (LPO_RelTab_getRel(s_i, t_i) == c_Gleich){ 
        s_i = TO_NaechsterTeilterm(s_i);
        t_i = TO_NaechsterTeilterm(t_i);
      }
      else {
        res = LPO_RelTab_getRel(s_i, t_i) == c_Groesser;
        break;
      }
    }
  }
  CLOSETRACE(("s_LPO_Lex1: ss = %A, ts = %A --> %b\n", 
              ss, s_NULL, ts, t_NULL, res));
  return res;
}

/* ------------------------------------------------------------------------- */
/* -------- E O F ---------------------------------------------------------- */
/* ------------------------------------------------------------------------- */
