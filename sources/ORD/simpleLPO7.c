/* ------------------------------------------------------------------------- */
/* ----------- 7. decorating with varsets and pretesting with them --------- */
/* ------------------------------------------------------------------------- */

#include "simpleLPO_private.h"

#if VARSET_FOR_LPO
/* ------------------------------------------------------------------------- */
/* ------------- Internal forward declarations ----------------------------- */
/* ------------------------------------------------------------------------- */

static BOOLEAN s_LPO_Greater7(UTermT s, UTermT t);

static BOOLEAN s_LPO_Alpha7(UTermT ss, UTermT s_NULL, UTermT t);

static BOOLEAN s_LPO_Majo7(UTermT s, UTermT ts, UTermT t_NULL);
static BOOLEAN s_LPO_LexMA7(UTermT s, UTermT t, 
                            UTermT ss, UTermT s_NULL, 
                            UTermT ts, UTermT t_NULL);
static VarsetT s_decorate(UTermT s);

/* ------------------------------------------------------------------------- */
/* -------------- Interface to the outside --------------------------------- */
/* ------------------------------------------------------------------------- */

VergleichsT S_LPO7(UTermT s, UTermT t)
{
  VergleichsT res;

  OPENTRACE(("S_LPO7: s = %t,  t = %t\n", s, t));
  {
    s_decorate(s);
    s_decorate(t);
    if (VS_EQUAL(TO_GetVarset(s), TO_GetVarset(t)) && sTO_TermeGleich(s,t)){
      res = Gleich;
    }
    else if (s_LPO_Greater7(s,t)){
      res = Groesser;
    }
    else if (s_LPO_Greater7(t,s)){
      res = Kleiner;
    }
    else {
      res = Unvergleichbar;
    }
  }
  CLOSETRACE(("S_LPO7: s = %t,  t = %t --> %v\n", s, t, res));
  return res;
}

BOOLEAN S_LPOGroesser7(UTermT s, UTermT t)
{
  BOOLEAN res;

  OPENTRACE(("S_LPOGroesser7: s = %t,  t = %t\n", s, t));
  {
    s_decorate(s);
    s_decorate(t);
    res = s_LPO_Greater7(s, t);
  }
  CLOSETRACE(("S_LPOGroesser7: s = %t,  t = %t --> %b\n", s, t, res));
  return res;
}

/* ------------------------------------------------------------------------- */
/* -------------- Internal functionality ----------------------------------- */
/* ------------------------------------------------------------------------- */

static BOOLEAN s_LPO_Greater7(UTermT s, UTermT t)
{
  BOOLEAN res;

  OPENTRACE(("s_LPO_Greater7: s = %t,  t = %t\n", s, t));
  {
#if S_doCounting
    S_counter++;
#endif
    if (!VS_IS_SUBSET(TO_GetVarset(t), TO_GetVarset(s))){
      res = FALSE;
    }
    else {
      if (TO_TermIstNichtVar(s)){
        if (TO_TermIstNichtVar(t)){
          if (PR_Praezedenz(TO_TopSymbol(s), TO_TopSymbol(t)) == Groesser){
            res = s_LPO_Majo7(s, TO_ErsterTeilterm(t), TO_NULLTerm(t));
          }
          else if SO_SymbGleich(TO_TopSymbol(s), TO_TopSymbol(t)){
            res = s_LPO_LexMA7(s, t, 
                               TO_ErsterTeilterm(s), TO_NULLTerm(s), 
                               TO_ErsterTeilterm(t), TO_NULLTerm(t) );
          }
          else {
            res = s_LPO_Alpha7(TO_ErsterTeilterm(s), TO_NULLTerm(s), t);
          }
        }
        else {
          res = TRUE;
        }
      }
      else {
        res = FALSE;
      }
    }
  }
  CLOSETRACE(("s_LPO_Greater7: s = %t,  t = %t --> %b\n", s, t, res));
  return res;
}

/* ------------------------------------------------------------------------- */

static BOOLEAN s_LPO_Alpha7(UTermT ss, UTermT s_NULL, UTermT t)
{
  BOOLEAN res = FALSE; /* default for empty list */

  OPENTRACE(("s_LPOAlpha7: ss = %A,  t = %t\n", ss, s_NULL, t));
  {
    UTermT s_i = ss;
    while(s_i != s_NULL) {    
      if (sTO_TermeGleich(s_i,t) || s_LPO_Greater7(s_i,t)){
        res = TRUE;
        break;
      }
      else {
        s_i = TO_NaechsterTeilterm(s_i);
      }
    }
  }
  CLOSETRACE(("s_LPOAlpha7: ss = %A,  t = %t --> %b\n", ss, s_NULL, t, res));
  return res;
}

/* ------------------------------------------------------------------------- */

static BOOLEAN s_LPO_Majo7(UTermT s, UTermT ts, UTermT t_NULL)
{
  BOOLEAN res = TRUE; /* default value for empty list */

  OPENTRACE(("s_LPO_Majo7: s = %t, ts = %A\n", s, ts, t_NULL));
  {
    UTermT t_j = ts;
    while(t_j != t_NULL){
      if (s_LPO_Greater7(s, t_j)) {
        t_j = TO_NaechsterTeilterm(t_j);
      }
      else {
        res = FALSE;
        break;
      }
    }
  }
  CLOSETRACE(("s_LPO_Majo7: s = %t, ts = %A --> %b\n", s, ts, t_NULL, res));
  return res;
}

static BOOLEAN s_LPO_LexMA7(UTermT s, UTermT t, 
                            UTermT ss, UTermT s_NULL, 
                            UTermT ts, UTermT t_NULL)
{
  BOOLEAN res = FALSE; /* default value for empty list */

  OPENTRACE(("s_LPO_LexMA7: s = %t,  t = %t, ss = %A, ts = %A\n", 
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
        if (s_LPO_Greater7(s_i, t_i)){
          res = s_LPO_Majo7(s, TO_NaechsterTeilterm(t_i), t_NULL);
        }
        else {
          res = s_LPO_Alpha7(TO_NaechsterTeilterm(s_i), s_NULL, t);
        }
        break;
      }
    }
  }
  CLOSETRACE(("s_LPO_LexMA7: s = %t, t = %t, ss = %A, ts = %A --> %b\n", 
              s, t, ss, s_NULL, ts, t_NULL, res));
  return res;
}

static VarsetT s_decorate(UTermT t)
{
  VarsetT v;
  if (TO_TermIstNichtVar(t)){
    UTermT t_NULL = TO_NULLTerm(t);
    UTermT t_i = TO_ErsterTeilterm(t);
    v = VS_EMPTY();
    while (t_i != t_NULL){
      v |= s_decorate(t_i);
      t_i = TO_NaechsterTeilterm(t_i);
    }
  }
  else {
    v = VS_TIP(SO_VarNummer(TO_TopSymbol(t)));
  }
  TO_SetVarset(t,v);
  return v;
}

#else

VergleichsT S_LPO7(UTermT s, UTermT t)
{
  our_error("VARSET_FOR_LPO not set");
  return Unvergleichbar;
}

BOOLEAN S_LPOGroesser7(UTermT s, UTermT t)
{
  our_error("VARSET_FOR_LPO not set");
  return FALSE;
}

#endif

/* ------------------------------------------------------------------------- */
/* -------- E O F ---------------------------------------------------------- */
/* ------------------------------------------------------------------------- */
