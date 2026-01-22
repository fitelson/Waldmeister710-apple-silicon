/* ------------------------------------------------------------------------- */
/* ----------- 37. optimize Majo-calls when in Lex -------------------------- */
/* ------------------------------------------------------------------------- */

#include "simpleLPO_private.h"

/* ------------------------------------------------------------------------- */
/* ------------- Internal forward declarations ----------------------------- */
/* ------------------------------------------------------------------------- */

static BOOLEAN s_LPO_Greater37(UTermT s, UTermT t);

static BOOLEAN s_LPO_Alpha37(UTermT ss, UTermT s_NULL, UTermT t);
static BOOLEAN s_LPO_Gamma37(UTermT s, UTermT t);
static BOOLEAN s_LPO_Beta37(UTermT s, UTermT t);
static BOOLEAN s_LPO_Delta37(UTermT s, UTermT t);

static BOOLEAN s_LPO_Majo37(UTermT s, UTermT ts, UTermT t_NULL);
static BOOLEAN s_LPO_LexM37(UTermT s, UTermT ss, UTermT s_NULL, 
                           UTermT ts, UTermT t_NULL);

/* ------------------------------------------------------------------------- */
/* -------------- Interface to the outside --------------------------------- */
/* ------------------------------------------------------------------------- */

VergleichsT S_LPO37(UTermT s, UTermT t)
{
  VergleichsT res;

  OPENTRACE(("S_LPO37: s = %t,  t = %t\n", s, t));
  {
    if (sTO_TermeGleich(s,t)){
      res = Gleich;
    }
    else if (s_LPO_Greater37(s,t)){
      res = Groesser;
    }
    else if (s_LPO_Greater37(t,s)){
      res = Kleiner;
    }
    else {
      res = Unvergleichbar;
    }
  }
  CLOSETRACE(("S_LPO37: s = %t,  t = %t --> %v\n", s, t, res));
  return res;
}

BOOLEAN S_LPOGroesser37(UTermT s, UTermT t)
{
  BOOLEAN res;

  OPENTRACE(("S_LPOGroesser37: s = %t,  t = %t\n", s, t));
  {
    res = s_LPO_Greater37(s, t);
  }
  CLOSETRACE(("S_LPOGroesser37: s = %t,  t = %t --> %b\n", s, t, res));
  return res;
}

/* ------------------------------------------------------------------------- */
/* -------------- Internal functionality ----------------------------------- */
/* ------------------------------------------------------------------------- */

static BOOLEAN s_LPO_Greater37(UTermT s, UTermT t)
{
  BOOLEAN res;

  OPENTRACE(("s_LPO_Greater37: s = %t,  t = %t\n", s, t));
  {
#if S_doCounting
    S_counter++;
#endif
  packman:
    if (TO_TermIstNichtVar(s)){
      if (TO_TermIstNichtVar(t)){
        if ((TO_TopSymbol(s) == TO_TopSymbol(t) ) && 
            (SO_Stelligkeit(TO_TopSymbol(s)) == 1)){
          s = TO_ErsterTeilterm(s);
          t = TO_ErsterTeilterm(t);
          goto packman;
        }
        res =    s_LPO_Beta37(s,t)
              || s_LPO_Gamma37(s,t)
              || s_LPO_Alpha37(TO_ErsterTeilterm(s), TO_NULLTerm(s), t);
      }
      else {
        res = s_LPO_Delta37(s,t);
      }
    }
    else {
      res = FALSE;
    }
  }
  CLOSETRACE(("s_LPO_Greater37: s = %t,  t = %t --> %b\n", s, t, res));
  return res;
}

/* ------------------------------------------------------------------------- */

static BOOLEAN s_LPO_Alpha37(UTermT ss, UTermT s_NULL, UTermT t)
{
  BOOLEAN res = FALSE; /* default for empty list */

  OPENTRACE(("s_LPOAlpha37: ss = %A,  t = %t\n", ss, s_NULL, t));
  {
    UTermT s_i = ss;
    while(s_i != s_NULL) {    
      if (sTO_TermeGleich(s_i,t) || s_LPO_Greater37(s_i,t)){
        res = TRUE;
        break;
      }
      else {
        s_i = TO_NaechsterTeilterm(s_i);
      }
    }
  }
  CLOSETRACE(("s_LPOAlpha37: ss = %A,  t = %t --> %b\n", ss, s_NULL, t, res));
  return res;
}

static BOOLEAN s_LPO_Beta37(UTermT s, UTermT t) 
{
  BOOLEAN res;

  OPENTRACE(("s_LPO_Beta37: s = %t,  t = %t\n", s, t));
  {
    res = (PR_Praezedenz(TO_TopSymbol(s), TO_TopSymbol(t)) == Groesser) &&
          s_LPO_Majo37(s, TO_ErsterTeilterm(t), TO_NULLTerm(t));
  }
  CLOSETRACE(("s_LPO_Beta37: s = %t,  t = %t --> %b\n", s, t, res));
  return res;
}


static BOOLEAN s_LPO_Gamma37(UTermT s, UTermT t) 
{
  BOOLEAN res;

  OPENTRACE(("s_LPO_Gamma37: s = %t,  t = %t\n", s, t));
  {
    res = SO_SymbGleich(TO_TopSymbol(s), TO_TopSymbol(t))     &&
          s_LPO_LexM37(s,TO_ErsterTeilterm(s), TO_NULLTerm(s), 
                      TO_ErsterTeilterm(t), TO_NULLTerm(t));
  }
  CLOSETRACE(("s_LPO_Gamma37: s = %t,  t = %t --> %b\n", s, t, res));
  return res;
}

static BOOLEAN s_LPO_Delta37(UTermT s, UTermT t) 
{
  BOOLEAN res;

  OPENTRACE(("s_LPODelta37: s = %t,  t = %t\n", s, t));
  {
    res = sTO_SymbolEnthalten (TO_TopSymbol(t), s);
  }
  CLOSETRACE(("s_LPODelta37: s = %t,  t = %t --> %b\n", s, t, res));
  return res;
}

/* ------------------------------------------------------------------------- */

static BOOLEAN s_LPO_Majo37(UTermT s, UTermT ts, UTermT t_NULL)
{
  BOOLEAN res = TRUE; /* default value for empty list */

  OPENTRACE(("s_LPO_Majo37: s = %t, ts = %A\n", s, ts, t_NULL));
  {
    UTermT t_j = ts;
    while(t_j != t_NULL){
      if (s_LPO_Greater37(s, t_j)) {
        t_j = TO_NaechsterTeilterm(t_j);
      }
      else {
        res = FALSE;
        break;
      }
    }
  }
  CLOSETRACE(("s_LPO_Majo37: s = %t, ts = %A --> %b\n", s, ts, t_NULL, res));
  return res;
}

static BOOLEAN s_LPO_LexM37(UTermT s, UTermT ss, UTermT s_NULL, 
                           UTermT ts, UTermT t_NULL)
{
  BOOLEAN res = FALSE; /* default value for empty list */

  OPENTRACE(("s_LPO_LexM37: s = %t,  ss = %A, ts = %A\n", 
             s, ss, s_NULL, ts, t_NULL));
  {
    UTermT s_i  = ss;
    UTermT t_i  = ts;
    while (s_i != s_NULL){       /* Implies t_i != t_NULL (fixed arity!) */ 
      if (sTO_TermeGleich(s_i,t_i)){ 
        s_i = TO_NaechsterTeilterm(s_i);
        t_i = TO_NaechsterTeilterm(t_i);
      }
      else {
        res = s_LPO_Greater37(s_i, t_i) &&
              s_LPO_Majo37(s, TO_NaechsterTeilterm(t_i), t_NULL);
        break;
      }
    }
  }
  CLOSETRACE(("s_LPO_LexM37: s = %t, ss = %A, ts = %A --> %b\n", 
              s, ss, s_NULL, ts, t_NULL, res));
  return res;
}

/* ------------------------------------------------------------------------- */
/* -------- E O F ---------------------------------------------------------- */
/* ------------------------------------------------------------------------- */
