/* ------------------------------------------------------------------------- */
/* ----------- 8. determining varsets only at interface points ------------- */
/* ------------------------------------------------------------------------- */

#include "simpleLPO_private.h"

#if VARSET_FOR_LPO
/* ------------------------------------------------------------------------- */
/* ------------- Internal forward declarations ----------------------------- */
/* ------------------------------------------------------------------------- */

static BOOLEAN s_LPO_Greater8(UTermT s, UTermT t);

static BOOLEAN s_LPO_Alpha8(UTermT ss, UTermT s_NULL, UTermT t);
static BOOLEAN s_LPO_Delta8(UTermT s, UTermT t);

static BOOLEAN s_LPO_Majo8(UTermT s, UTermT ts, UTermT t_NULL);
static BOOLEAN s_LPO_LexMA8(UTermT s, UTermT t, 
                            UTermT ss, UTermT s_NULL, 
                            UTermT ts, UTermT t_NULL);

static VarsetT s_determineVS(UTermT s);

/* ------------------------------------------------------------------------- */
/* -------------- Interface to the outside --------------------------------- */
/* ------------------------------------------------------------------------- */

VergleichsT S_LPO8(UTermT s, UTermT t)
{
  VergleichsT res;

  OPENTRACE(("S_LPO8: s = %t,  t = %t\n", s, t));
  {
    VarsetT vs = s_determineVS(s);
    VarsetT vt = s_determineVS(t);
    if (VS_EQUAL(vs,vt) && sTO_TermeGleich(s,t)){
      res = Gleich;
    }
    else if (VS_IS_SUBSET(vt,vs) && s_LPO_Greater8(s,t)){
      res = Groesser;
    }
    else if (VS_IS_SUBSET(vs,vt) && s_LPO_Greater8(t,s)){
      res = Kleiner;
    }
    else {
      res = Unvergleichbar;
    }
  }
  CLOSETRACE(("S_LPO8: s = %t,  t = %t --> %v\n", s, t, res));
  return res;
}

BOOLEAN S_LPOGroesser8(UTermT s, UTermT t)
{
  BOOLEAN res;

  OPENTRACE(("S_LPOGroesser8: s = %t,  t = %t\n", s, t));
  {
    VarsetT vs = s_determineVS(s);
    VarsetT vt = s_determineVS(t);
    res = VS_IS_SUBSET(vt,vs) && s_LPO_Greater8(s, t);
  }
  CLOSETRACE(("S_LPOGroesser8: s = %t,  t = %t --> %b\n", s, t, res));
  return res;
}

/* ------------------------------------------------------------------------- */
/* -------------- Internal functionality ----------------------------------- */
/* ------------------------------------------------------------------------- */

static BOOLEAN s_LPO_Greater8(UTermT s, UTermT t)
{
  BOOLEAN res;

  OPENTRACE(("s_LPO_Greater8: s = %t,  t = %t\n", s, t));
  {
#if S_doCounting
    S_counter++;
#endif
    if (TO_TermIstNichtVar(s)){
      if (TO_TermIstNichtVar(t)){
        if (PR_Praezedenz(TO_TopSymbol(s), TO_TopSymbol(t)) == Groesser){
          res = s_LPO_Majo8(s, TO_ErsterTeilterm(t), TO_NULLTerm(t));
        }
        else if SO_SymbGleich(TO_TopSymbol(s), TO_TopSymbol(t)){
          res = s_LPO_LexMA8(s, t, 
                             TO_ErsterTeilterm(s), TO_NULLTerm(s), 
                             TO_ErsterTeilterm(t), TO_NULLTerm(t) );
        }
        else {
          res = s_LPO_Alpha8(TO_ErsterTeilterm(s), TO_NULLTerm(s), t);
        }
      }
      else {
        res = s_LPO_Delta8(s,t);
      }
    }
    else {
      res = FALSE;
    }
  }
  CLOSETRACE(("s_LPO_Greater8: s = %t,  t = %t --> %b\n", s, t, res));
  return res;
}

/* ------------------------------------------------------------------------- */

static BOOLEAN s_LPO_Alpha8(UTermT ss, UTermT s_NULL, UTermT t)
{
  BOOLEAN res = FALSE; /* default for empty list */

  OPENTRACE(("s_LPOAlpha8: ss = %A,  t = %t\n", ss, s_NULL, t));
  {
    UTermT s_i = ss;
    while(s_i != s_NULL) {    
      if (sTO_TermeGleich(s_i,t) || s_LPO_Greater8(s_i,t)){
        res = TRUE;
        break;
      }
      else {
        s_i = TO_NaechsterTeilterm(s_i);
      }
    }
  }
  CLOSETRACE(("s_LPOAlpha8: ss = %A,  t = %t --> %b\n", ss, s_NULL, t, res));
  return res;
}

static BOOLEAN s_LPO_Delta8(UTermT s, UTermT t) 
{
  BOOLEAN res;

  OPENTRACE(("s_LPODelta8: s = %t,  t = %t\n", s, t));
  {
    res = sTO_SymbolEnthalten (TO_TopSymbol(t), s);
  }
  CLOSETRACE(("s_LPODelta8: s = %t,  t = %t --> %b\n", s, t, res));
  return res;
}

/* ------------------------------------------------------------------------- */

static BOOLEAN s_LPO_Majo8(UTermT s, UTermT ts, UTermT t_NULL)
{
  BOOLEAN res = TRUE; /* default value for empty list */

  OPENTRACE(("s_LPO_Majo8: s = %t, ts = %A\n", s, ts, t_NULL));
  {
    UTermT t_j = ts;
    while(t_j != t_NULL){
      if (s_LPO_Greater8(s, t_j)) {
        t_j = TO_NaechsterTeilterm(t_j);
      }
      else {
        res = FALSE;
        break;
      }
    }
  }
  CLOSETRACE(("s_LPO_Majo8: s = %t, ts = %A --> %b\n", s, ts, t_NULL, res));
  return res;
}

static BOOLEAN s_LPO_LexMA8(UTermT s, UTermT t, 
                            UTermT ss, UTermT s_NULL, 
                            UTermT ts, UTermT t_NULL)
{
  BOOLEAN res = FALSE; /* default value for empty list */

  OPENTRACE(("s_LPO_LexMA8: s = %t,  t = %t, ss = %A, ts = %A\n", 
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
        if (s_LPO_Greater8(s_i, t_i)){
          res = s_LPO_Majo8(s, TO_NaechsterTeilterm(t_i), t_NULL);
        }
        else {
          res = s_LPO_Alpha8(TO_NaechsterTeilterm(s_i), s_NULL, t);
        }
        break;
      }
    }
  }
  CLOSETRACE(("s_LPO_LexMA8: s = %t, t = %t, ss = %A, ts = %A --> %b\n", 
              s, t, ss, s_NULL, ts, t_NULL, res));
  return res;
}

static VarsetT s_determineVS(UTermT t)
{
  VarsetT v = VS_EMPTY();
  UTermT t_NULL = TO_NULLTerm(t);
  UTermT t_i = t;
  while (t_i != t_NULL){
    if (TO_TermIstVar(t_i)){
      v |= VS_TIP(SO_VarNummer(TO_TopSymbol(t_i)));
    }
    t_i = TO_Schwanz(t_i);
  }
  return v;
}

#else

VergleichsT S_LPO8(UTermT s, UTermT t)
{
  our_error("VARSET_FOR_LPO not set");
  return Unvergleichbar;
}

BOOLEAN S_LPOGroesser8(UTermT s, UTermT t)
{
  our_error("VARSET_FOR_LPO not set");
  return FALSE;
}

#endif

/* ------------------------------------------------------------------------- */
/* -------- E O F ---------------------------------------------------------- */
/* ------------------------------------------------------------------------- */
