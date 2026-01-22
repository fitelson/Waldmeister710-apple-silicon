/* ------------------------------------------------------------------------- */
/* ----------- 41. variation of 40: lexMA slightly changed ------------------ */
/* ------------------------------------------------------------------------- */

#include "simpleLPO_private.h"

/* ------------------------------------------------------------------------- */
/* ------------- Internal forward declarations ----------------------------- */
/* ------------------------------------------------------------------------- */

static BOOLEAN s_LPO_Greater41(UTermT s, UTermT t);

static BOOLEAN s_LPO_Alpha41(UTermT ss, UTermT s_NULL, UTermT t);
static BOOLEAN s_LPO_Delta41(UTermT s, UTermT t);

static BOOLEAN s_LPO_Majo41(UTermT s, UTermT ts, UTermT t_NULL);
static BOOLEAN s_LPO_LexMA41(UTermT s, UTermT t, 
                            UTermT ss, UTermT s_NULL, 
                            UTermT ts, UTermT t_NULL);

/* ------------------------------------------------------------------------- */
/* -------------- Interface to the outside --------------------------------- */
/* ------------------------------------------------------------------------- */

VergleichsT S_LPO41(UTermT s, UTermT t)
{
  VergleichsT res;

  OPENTRACE(("S_LPO41: s = %t,  t = %t\n", s, t));
  {
    if (sTO_TermeGleich(s,t)){
      res = Gleich;
    }
    else if (s_LPO_Greater41(t,s)){
      res = Kleiner;
    }
    else if (s_LPO_Greater41(s,t)){
      res = Groesser;
    }
    else {
      res = Unvergleichbar;
    }
  }
  CLOSETRACE(("S_LPO41: s = %t,  t = %t --> %v\n", s, t, res));
  return res;
}

BOOLEAN S_LPOGroesser41(UTermT s, UTermT t)
{
  BOOLEAN res;

  OPENTRACE(("S_LPOGroesser41: s = %t,  t = %t\n", s, t));
  {
    res = s_LPO_Greater41(s, t);
  }
  CLOSETRACE(("S_LPOGroesser41: s = %t,  t = %t --> %b\n", s, t, res));
  return res;
}

/* ------------------------------------------------------------------------- */
/* -------------- Internal functionality ----------------------------------- */
/* ------------------------------------------------------------------------- */

static BOOLEAN s_LPO_Greater41(UTermT s, UTermT t)
{
  BOOLEAN res;

  OPENTRACE(("s_LPO_Greater41: s = %t,  t = %t\n", s, t));
  {
#if S_doCounting
    S_counter++;
#endif
    if (TO_TermIstNichtVar(s)){
      if (TO_TermIstNichtVar(t)){
        if SO_SymbGleich(TO_TopSymbol(s), TO_TopSymbol(t)){
          res = s_LPO_LexMA41(s, t, 
                             TO_ErsterTeilterm(s), TO_NULLTerm(s), 
                             TO_ErsterTeilterm(t), TO_NULLTerm(t) );
        }
        else if (PR_Praezedenz(TO_TopSymbol(s), TO_TopSymbol(t)) == Groesser){
          res = s_LPO_Majo41(s, TO_ErsterTeilterm(t), TO_NULLTerm(t));
        }
        else {
          res = s_LPO_Alpha41(TO_ErsterTeilterm(s), TO_NULLTerm(s), t);
        }
      }
      else {
        res = s_LPO_Delta41(s,t);
      }
    }
    else {
      res = FALSE;
    }
  }
  CLOSETRACE(("s_LPO_Greater41: s = %t,  t = %t --> %b\n", s, t, res));
  return res;
}

/* ------------------------------------------------------------------------- */

static BOOLEAN s_LPO_Alpha41(UTermT ss, UTermT s_NULL, UTermT t)
{
  BOOLEAN res = FALSE; /* default for empty list */

  OPENTRACE(("s_LPOAlpha41: ss = %A,  t = %t\n", ss, s_NULL, t));
  {
    UTermT s_i = ss;
    while(s_i != s_NULL) {    
      if (sTO_TermeGleich(s_i,t) || s_LPO_Greater41(s_i,t)){
        res = TRUE;
        break;
      }
      else {
        s_i = TO_NaechsterTeilterm(s_i);
      }
    }
  }
  CLOSETRACE(("s_LPOAlpha41: ss = %A,  t = %t --> %b\n", ss, s_NULL, t, res));
  return res;
}

static BOOLEAN s_LPO_Delta41(UTermT s, UTermT t) 
{
  BOOLEAN res;

  OPENTRACE(("s_LPODelta41: s = %t,  t = %t\n", s, t));
  {
    res = sTO_SymbolEnthalten (TO_TopSymbol(t), s);
  }
  CLOSETRACE(("s_LPODelta41: s = %t,  t = %t --> %b\n", s, t, res));
  return res;
}

/* ------------------------------------------------------------------------- */

static BOOLEAN s_LPO_Majo41(UTermT s, UTermT ts, UTermT t_NULL)
{
  BOOLEAN res = TRUE; /* default value for empty list */

  OPENTRACE(("s_LPO_Majo41: s = %t, ts = %A\n", s, ts, t_NULL));
  {
    UTermT t_j = ts;
    while(t_j != t_NULL){
      if (s_LPO_Greater41(s, t_j)) {
        t_j = TO_NaechsterTeilterm(t_j);
      }
      else {
        res = FALSE;
        break;
      }
    }
  }
  CLOSETRACE(("s_LPO_Majo41: s = %t, ts = %A --> %b\n", s, ts, t_NULL, res));
  return res;
}

static BOOLEAN s_LPO_LexMA41(UTermT s, UTermT t, 
                            UTermT ss, UTermT s_NULL, 
                            UTermT ts, UTermT t_NULL)
{
  BOOLEAN res = FALSE; /* default value for empty list */

  OPENTRACE(("s_LPO_LexMA41: s = %t,  t = %t, ss = %A, ts = %A\n", 
             s, t, ss, s_NULL, ts, t_NULL));
  {
    UTermT s_i  = ss;
    UTermT t_i  = ts;
    while (s_i != s_NULL){       /* Implies t_i != t_NULL (fixed arity!) */ 
      if (TO_NaechsterTeilterm(s_i) == s_NULL){ /* last subterm */
        res = s_LPO_Greater41(s_i, t_i);
        break;
      }
      else if (sTO_TermeGleich(s_i,t_i)){ 
        s_i = TO_NaechsterTeilterm(s_i);
        t_i = TO_NaechsterTeilterm(t_i);
      }
      else {
        if (s_LPO_Greater41(s_i, t_i)){
          res = s_LPO_Majo41(s, TO_NaechsterTeilterm(t_i), t_NULL);
        }
        else {
          res = s_LPO_Alpha41(TO_NaechsterTeilterm(s_i), s_NULL, t);
        }
        break;
      }
    }
  }
  CLOSETRACE(("s_LPO_LexMA41: s = %t, t = %t, ss = %A, ts = %A --> %b\n", 
              s, t, ss, s_NULL, ts, t_NULL, res));
  return res;
}

/* ------------------------------------------------------------------------- */
/* -------- E O F ---------------------------------------------------------- */
/* ------------------------------------------------------------------------- */
