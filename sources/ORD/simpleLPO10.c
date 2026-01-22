/* ------------------------------------------------------------------------- */
/* ----------- 10. try to detect subterms first ---------------------------- */
/* ------------------------------------------------------------------------- */

#include "simpleLPO_private.h"

/* ------------------------------------------------------------------------- */
/* ------------- Internal forward declarations ----------------------------- */
/* ------------------------------------------------------------------------- */

static BOOLEAN s_LPO_Greater10(UTermT s, UTermT t);

static BOOLEAN s_LPO_Alpha10(UTermT ss, UTermT s_NULL, UTermT t);
static BOOLEAN s_LPO_Delta10(UTermT s, UTermT t);

static BOOLEAN s_LPO_Majo10(UTermT s, UTermT ts, UTermT t_NULL);
static BOOLEAN s_LPO_LexMA10(UTermT s, UTermT t, 
                            UTermT ss, UTermT s_NULL, 
                            UTermT ts, UTermT t_NULL);

static BOOLEAN s_IstEchterTeilterm(UTermT s, UTermT t);

/* ------------------------------------------------------------------------- */
/* -------------- Interface to the outside --------------------------------- */
/* ------------------------------------------------------------------------- */

VergleichsT S_LPO10(UTermT s, UTermT t)
{
  VergleichsT res;

  OPENTRACE(("S_LPO10: s = %t,  t = %t\n", s, t));
  {
    if (sTO_TermeGleich(s,t)){
      res = Gleich;
    }
    else if (s_LPO_Greater10(s,t)){
      res = Groesser;
    }
    else if (s_LPO_Greater10(t,s)){
      res = Kleiner;
    }
    else {
      res = Unvergleichbar;
    }
  }
  CLOSETRACE(("S_LPO10: s = %t,  t = %t --> %v\n", s, t, res));
  return res;
}

BOOLEAN S_LPOGroesser10(UTermT s, UTermT t)
{
  BOOLEAN res;

  OPENTRACE(("S_LPOGroesser10: s = %t,  t = %t\n", s, t));
  {
    res = s_LPO_Greater10(s, t);
  }
  CLOSETRACE(("S_LPOGroesser10: s = %t,  t = %t --> %b\n", s, t, res));
  return res;
}

/* ------------------------------------------------------------------------- */
/* -------------- Internal functionality ----------------------------------- */
/* ------------------------------------------------------------------------- */

static BOOLEAN s_LPO_Greater10(UTermT s, UTermT t)
{
  BOOLEAN res;

  OPENTRACE(("s_LPO_Greater10: s = %t,  t = %t\n", s, t));
  {
#if S_doCounting
    S_counter++;
#endif
    if (TO_TermIstNichtVar(s)){
      if (TO_TermIstNichtVar(t)){
        if (s_IstEchterTeilterm(t,s)){
          res = TRUE;
        }
        else if SO_SymbGleich(TO_TopSymbol(s), TO_TopSymbol(t)){
          res = s_LPO_LexMA10(s, t, 
                             TO_ErsterTeilterm(s), TO_NULLTerm(s), 
                             TO_ErsterTeilterm(t), TO_NULLTerm(t) );
        }
        else if (PR_Praezedenz(TO_TopSymbol(s), TO_TopSymbol(t)) == Groesser){
          res = s_LPO_Majo10(s, TO_ErsterTeilterm(t), TO_NULLTerm(t));
        }
        else {
          res = s_LPO_Alpha10(TO_ErsterTeilterm(s), TO_NULLTerm(s), t);
        }
      }
      else {
        res = s_LPO_Delta10(s,t);
      }
    }
    else {
      res = FALSE;
    }
  }
  CLOSETRACE(("s_LPO_Greater10: s = %t,  t = %t --> %b\n", s, t, res));
  return res;
}

/* ------------------------------------------------------------------------- */

static BOOLEAN s_LPO_Alpha10(UTermT ss, UTermT s_NULL, UTermT t)
{
  BOOLEAN res = FALSE; /* default for empty list */

  OPENTRACE(("s_LPOAlpha10: ss = %A,  t = %t\n", ss, s_NULL, t));
  {
    UTermT s_i = ss;
    while(s_i != s_NULL) {    
      if (sTO_TermeGleich(s_i,t) || s_LPO_Greater10(s_i,t)){
        res = TRUE;
        break;
      }
      else {
        s_i = TO_NaechsterTeilterm(s_i);
      }
    }
  }
  CLOSETRACE(("s_LPOAlpha10: ss = %A,  t = %t --> %b\n", ss, s_NULL, t, res));
  return res;
}

static BOOLEAN s_LPO_Delta10(UTermT s, UTermT t) 
{
  BOOLEAN res;

  OPENTRACE(("s_LPODelta10: s = %t,  t = %t\n", s, t));
  {
    res = sTO_SymbolEnthalten (TO_TopSymbol(t), s);
  }
  CLOSETRACE(("s_LPODelta10: s = %t,  t = %t --> %b\n", s, t, res));
  return res;
}

/* ------------------------------------------------------------------------- */

static BOOLEAN s_LPO_Majo10(UTermT s, UTermT ts, UTermT t_NULL)
{
  BOOLEAN res = TRUE; /* default value for empty list */

  OPENTRACE(("s_LPO_Majo10: s = %t, ts = %A\n", s, ts, t_NULL));
  {
    UTermT t_j = ts;
    while(t_j != t_NULL){
      if (s_LPO_Greater10(s, t_j)) {
        t_j = TO_NaechsterTeilterm(t_j);
      }
      else {
        res = FALSE;
        break;
      }
    }
  }
  CLOSETRACE(("s_LPO_Majo10: s = %t, ts = %A --> %b\n", s, ts, t_NULL, res));
  return res;
}

static BOOLEAN s_LPO_LexMA10(UTermT s, UTermT t, 
                            UTermT ss, UTermT s_NULL, 
                            UTermT ts, UTermT t_NULL)
{
  BOOLEAN res = FALSE; /* default value for empty list */

  OPENTRACE(("s_LPO_LexMA10: s = %t,  t = %t, ss = %A, ts = %A\n", 
             s, t, ss, s_NULL, ts, t_NULL));
  {
    UTermT s_i  = ss;
    UTermT t_i  = ts;
    while (s_i != s_NULL){       /* Implies t_i != t_NULL (fixed arity!) */ 
      if (sTO_TermeGleich(s_i,t_i)){ 
        s_i = TO_NaechsterTeilterm(s_i);
        t_i = TO_NaechsterTeilterm(t_i);
      }
      else {
        if (s_LPO_Greater10(s_i, t_i)){
          res = s_LPO_Majo10(s, TO_NaechsterTeilterm(t_i), t_NULL);
        }
        else {
          res = s_LPO_Alpha10(TO_NaechsterTeilterm(s_i), s_NULL, t);
        }
        break;
      }
    }
  }
  CLOSETRACE(("s_LPO_LexMA10: s = %t, t = %t, ss = %A, ts = %A --> %b\n", 
              s, t, ss, s_NULL, ts, t_NULL, res));
  return res;
}

static BOOLEAN s_IstEchterTeilterm(UTermT s, UTermT t)
/* simple quadratic test. On could do something along KMP string searching */
{
  UTermT t0 = TO_NULLTerm(t);
  UTermT ti = TO_Schwanz(t);
  while (ti != t0){
    if(sTO_TermeGleich(s,ti)){
      /*      printf("##T\n");*/
      return TRUE;
    }
    ti = TO_Schwanz(ti);
  }
  /*  printf("##F\n");*/
  return FALSE;
}

/* ------------------------------------------------------------------------- */
/* -------- E O F ---------------------------------------------------------- */
/* ------------------------------------------------------------------------- */
