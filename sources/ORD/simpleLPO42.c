/* ------------------------------------------------------------------------- */
/* ----------- 42. variant of 40: using equality as pretest ----------------- */
/* ------------------------------------------------------------------------- */

#include "simpleLPO_private.h"

/* ------------------------------------------------------------------------- */
/* ------------- Internal forward declarations ----------------------------- */
/* ------------------------------------------------------------------------- */

static BOOLEAN s_LPO_Greater42(UTermT s, UTermT t, BOOLEAN pretest);

static BOOLEAN s_LPO_Alpha42(UTermT ss, UTermT s_NULL, UTermT t);
static BOOLEAN s_LPO_Delta42(UTermT s, UTermT t);

static BOOLEAN s_LPO_Majo42(UTermT s, UTermT ts, UTermT t_NULL);
static BOOLEAN s_LPO_LexMA42(UTermT s, UTermT t, 
                            UTermT ss, UTermT s_NULL, 
                            UTermT ts, UTermT t_NULL);

/* ------------------------------------------------------------------------- */
/* -------------- Interface to the outside --------------------------------- */
/* ------------------------------------------------------------------------- */

VergleichsT S_LPO42(UTermT s, UTermT t)
{
  VergleichsT res;

  OPENTRACE(("S_LPO42: s = %t,  t = %t\n", s, t));
  {
    if (sTO_TermeGleich(s,t)){
      res = Gleich;
    }
    else if (s_LPO_Greater42(t,s, FALSE)){
      res = Kleiner;
    }
    else if (s_LPO_Greater42(s,t, FALSE)){
      res = Groesser;
    }
    else {
      res = Unvergleichbar;
    }
  }
  CLOSETRACE(("S_LPO42: s = %t,  t = %t --> %v\n", s, t, res));
  return res;
}

BOOLEAN S_LPOGroesser42(UTermT s, UTermT t)
{
  BOOLEAN res;

  OPENTRACE(("S_LPOGroesser42: s = %t,  t = %t\n", s, t));
  {
    res = s_LPO_Greater42(s, t, TRUE);
  }
  CLOSETRACE(("S_LPOGroesser42: s = %t,  t = %t --> %b\n", s, t, res));
  return res;
}

/* ------------------------------------------------------------------------- */
/* -------------- Internal functionality ----------------------------------- */
/* ------------------------------------------------------------------------- */

static BOOLEAN s_LPO_Greater42(UTermT s, UTermT t, BOOLEAN pretest)
{
  BOOLEAN res;

  OPENTRACE(("s_LPO_Greater42: s = %t,  t = %t\n", s, t));
  {
#if S_doCounting
    S_counter++;
#endif
    if (pretest && sTO_TermeGleich(s,t)){
      res = FALSE;
    }
    else if (TO_TermIstNichtVar(s)){
      if (TO_TermIstNichtVar(t)){
        if SO_SymbGleich(TO_TopSymbol(s), TO_TopSymbol(t)){
          res = s_LPO_LexMA42(s, t, 
                             TO_ErsterTeilterm(s), TO_NULLTerm(s), 
                             TO_ErsterTeilterm(t), TO_NULLTerm(t) );
        }
        else if (PR_Praezedenz(TO_TopSymbol(s), TO_TopSymbol(t)) == Groesser){
          res = s_LPO_Majo42(s, TO_ErsterTeilterm(t), TO_NULLTerm(t));
        }
        else {
          res = s_LPO_Alpha42(TO_ErsterTeilterm(s), TO_NULLTerm(s), t);
        }
      }
      else {
        res = s_LPO_Delta42(s,t);
      }
    }
    else {
      res = FALSE;
    }
  }
  CLOSETRACE(("s_LPO_Greater42: s = %t,  t = %t --> %b\n", s, t, res));
  return res;
}

/* ------------------------------------------------------------------------- */

static BOOLEAN s_LPO_Alpha42(UTermT ss, UTermT s_NULL, UTermT t)
{
  BOOLEAN res = FALSE; /* default for empty list */

  OPENTRACE(("s_LPOAlpha42: ss = %A,  t = %t\n", ss, s_NULL, t));
  {
    UTermT s_i = ss;
    while(s_i != s_NULL) {    
      if (sTO_TermeGleich(s_i,t) || s_LPO_Greater42(s_i,t, FALSE)){
        res = TRUE;
        break;
      }
      else {
        s_i = TO_NaechsterTeilterm(s_i);
      }
    }
  }
  CLOSETRACE(("s_LPOAlpha42: ss = %A,  t = %t --> %b\n", ss, s_NULL, t, res));
  return res;
}

static BOOLEAN s_LPO_Delta42(UTermT s, UTermT t) 
{
  BOOLEAN res;

  OPENTRACE(("s_LPODelta42: s = %t,  t = %t\n", s, t));
  {
    res = sTO_SymbolEnthalten (TO_TopSymbol(t), s);
  }
  CLOSETRACE(("s_LPODelta42: s = %t,  t = %t --> %b\n", s, t, res));
  return res;
}

/* ------------------------------------------------------------------------- */

static BOOLEAN s_LPO_Majo42(UTermT s, UTermT ts, UTermT t_NULL)
{
  BOOLEAN res = TRUE; /* default value for empty list */

  OPENTRACE(("s_LPO_Majo42: s = %t, ts = %A\n", s, ts, t_NULL));
  {
    UTermT t_j = ts;
    while(t_j != t_NULL){
      if (s_LPO_Greater42(s, t_j, TRUE)) {
        t_j = TO_NaechsterTeilterm(t_j);
      }
      else {
        res = FALSE;
        break;
      }
    }
  }
  CLOSETRACE(("s_LPO_Majo42: s = %t, ts = %A --> %b\n", s, ts, t_NULL, res));
  return res;
}

static BOOLEAN s_LPO_LexMA42(UTermT s, UTermT t, 
                            UTermT ss, UTermT s_NULL, 
                            UTermT ts, UTermT t_NULL)
{
  BOOLEAN res = FALSE; /* default value for empty list */

  OPENTRACE(("s_LPO_LexMA42: s = %t,  t = %t, ss = %A, ts = %A\n", 
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
        if (s_LPO_Greater42(s_i, t_i, FALSE)){
          res = s_LPO_Majo42(s, TO_NaechsterTeilterm(t_i), t_NULL);
        }
        else {
          res = s_LPO_Alpha42(TO_NaechsterTeilterm(s_i), s_NULL, t);
        }
        break;
      }
    }
  }
  CLOSETRACE(("s_LPO_LexMA42: s = %t, t = %t, ss = %A, ts = %A --> %b\n", 
              s, t, ss, s_NULL, ts, t_NULL, res));
  return res;
}

/* ------------------------------------------------------------------------- */
/* -------- E O F ---------------------------------------------------------- */
/* ------------------------------------------------------------------------- */
