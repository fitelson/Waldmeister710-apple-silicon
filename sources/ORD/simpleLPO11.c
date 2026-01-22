/* ------------------------------------------------------------------------- */
/* ----------- 11. special handling when s or t are constants -------------- */
/* ------------------------------------------------------------------------- */

#include "simpleLPO_private.h"

/* ------------------------------------------------------------------------- */
/* ------------- Internal forward declarations ----------------------------- */
/* ------------------------------------------------------------------------- */

static BOOLEAN s_LPO_Greater11(UTermT s, UTermT t);

static BOOLEAN s_LPO_Alpha11(UTermT ss, UTermT s_NULL, UTermT t);
static BOOLEAN s_LPO_Delta11(UTermT s, UTermT t);

static BOOLEAN s_LPO_Majo11(UTermT s, UTermT ts, UTermT t_NULL);
static BOOLEAN s_LPO_LexMA11(UTermT s, UTermT t, 
                            UTermT ss, UTermT s_NULL, 
                            UTermT ts, UTermT t_NULL);

static BOOLEAN s_s_is_const(SymbolT c, UTermT t);
static BOOLEAN s_t_is_const(SymbolT c, UTermT s);

/* ------------------------------------------------------------------------- */
/* -------------- Interface to the outside --------------------------------- */
/* ------------------------------------------------------------------------- */

VergleichsT S_LPO11(UTermT s, UTermT t)
{
  VergleichsT res;

  OPENTRACE(("S_LPO11: s = %t,  t = %t\n", s, t));
  {
    if (sTO_TermeGleich(s,t)){
      res = Gleich;
    }
    else if (s_LPO_Greater11(s,t)){
      res = Groesser;
    }
    else if (s_LPO_Greater11(t,s)){
      res = Kleiner;
    }
    else {
      res = Unvergleichbar;
    }
  }
  CLOSETRACE(("S_LPO11: s = %t,  t = %t --> %v\n", s, t, res));
  return res;
}

BOOLEAN S_LPOGroesser11(UTermT s, UTermT t)
{
  BOOLEAN res;

  OPENTRACE(("S_LPOGroesser11: s = %t,  t = %t\n", s, t));
  {
    res = s_LPO_Greater11(s, t);
  }
  CLOSETRACE(("S_LPOGroesser11: s = %t,  t = %t --> %b\n", s, t, res));
  return res;
}

/* ------------------------------------------------------------------------- */
/* -------------- Internal functionality ----------------------------------- */
/* ------------------------------------------------------------------------- */

static BOOLEAN s_LPO_Greater11(UTermT s, UTermT t)
{
  BOOLEAN res;

  OPENTRACE(("s_LPO_Greater11: s = %t,  t = %t\n", s, t));
  {
#if S_doCounting
    /*#error "klaerungsbedarf... a la delta"*/
    S_counter++;
#endif
    if (TO_TermIstNichtVar(s)){
      if (TO_Nullstellig(s)){
        res = s_s_is_const(TO_TopSymbol(s),t);
      }
      else if (TO_TermIstNichtVar(t)){
        if (TO_Nullstellig(t)){
          res = s_t_is_const(TO_TopSymbol(t),s);
        }
        else if SO_SymbGleich(TO_TopSymbol(s), TO_TopSymbol(t)){
          res = s_LPO_LexMA11(s, t, 
                             TO_ErsterTeilterm(s), TO_NULLTerm(s), 
                             TO_ErsterTeilterm(t), TO_NULLTerm(t) );
        }
        else if (PR_Praezedenz(TO_TopSymbol(s), TO_TopSymbol(t)) == Groesser){
          res = s_LPO_Majo11(s, TO_ErsterTeilterm(t), TO_NULLTerm(t));
        }
        else {
          res = s_LPO_Alpha11(TO_ErsterTeilterm(s), TO_NULLTerm(s), t);
        }
      }
      else {
        res = s_LPO_Delta11(s,t);
      }
    }
    else {
      res = FALSE;
    }
  }
  CLOSETRACE(("s_LPO_Greater11: s = %t,  t = %t --> %b\n", s, t, res));
  return res;
}

/* ------------------------------------------------------------------------- */

static BOOLEAN s_LPO_Alpha11(UTermT ss, UTermT s_NULL, UTermT t)
{
  BOOLEAN res = FALSE; /* default for empty list */

  OPENTRACE(("s_LPOAlpha11: ss = %A,  t = %t\n", ss, s_NULL, t));
  {
    UTermT s_i = ss;
    while(s_i != s_NULL) {    
      if (sTO_TermeGleich(s_i,t) || s_LPO_Greater11(s_i,t)){
        res = TRUE;
        break;
      }
      else {
        s_i = TO_NaechsterTeilterm(s_i);
      }
    }
  }
  CLOSETRACE(("s_LPOAlpha11: ss = %A,  t = %t --> %b\n", ss, s_NULL, t, res));
  return res;
}

static BOOLEAN s_LPO_Delta11(UTermT s, UTermT t) 
{
  BOOLEAN res;

  OPENTRACE(("s_LPODelta11: s = %t,  t = %t\n", s, t));
  {
    res = sTO_SymbolEnthalten (TO_TopSymbol(t), s);
  }
  CLOSETRACE(("s_LPODelta11: s = %t,  t = %t --> %b\n", s, t, res));
  return res;
}

/* ------------------------------------------------------------------------- */

static BOOLEAN s_LPO_Majo11(UTermT s, UTermT ts, UTermT t_NULL)
{
  BOOLEAN res = TRUE; /* default value for empty list */

  OPENTRACE(("s_LPO_Majo11: s = %t, ts = %A\n", s, ts, t_NULL));
  {
    UTermT t_j = ts;
    while(t_j != t_NULL){
      if (s_LPO_Greater11(s, t_j)) {
        t_j = TO_NaechsterTeilterm(t_j);
      }
      else {
        res = FALSE;
        break;
      }
    }
  }
  CLOSETRACE(("s_LPO_Majo11: s = %t, ts = %A --> %b\n", s, ts, t_NULL, res));
  return res;
}

static BOOLEAN s_LPO_LexMA11(UTermT s, UTermT t, 
                            UTermT ss, UTermT s_NULL, 
                            UTermT ts, UTermT t_NULL)
{
  BOOLEAN res = FALSE; /* default value for empty list */

  OPENTRACE(("s_LPO_LexMA11: s = %t,  t = %t, ss = %A, ts = %A\n", 
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
        if (s_LPO_Greater11(s_i, t_i)){
          res = s_LPO_Majo11(s, TO_NaechsterTeilterm(t_i), t_NULL);
        }
        else {
          res = s_LPO_Alpha11(TO_NaechsterTeilterm(s_i), s_NULL, t);
        }
        break;
      }
    }
  }
  CLOSETRACE(("s_LPO_LexMA11: s = %t, t = %t, ss = %A, ts = %A --> %b\n", 
              s, t, ss, s_NULL, ts, t_NULL, res));
  return res;
}

static BOOLEAN s_s_is_const(SymbolT c, UTermT t)
/* then t must be ground and c > f for each Symbol in t */
{
  UTermT t0 = TO_NULLTerm(t);
  UTermT ti = t;
  while (ti != t0){
    if (TO_TermIstVar(t) ||
        (PR_Praezedenz(c, TO_TopSymbol(ti)) != Groesser)){
      return FALSE;
    }
    ti = TO_Schwanz(ti);
  }
  return TRUE;
}
static BOOLEAN s_t_is_const(SymbolT c, UTermT s)
/* then s is greater if any of its function symbols is */
{
  UTermT s0 = TO_NULLTerm(s);
  UTermT si = s;
  while (si != s0){
    if (TO_TermIstNichtVar(si) &&
        ((TO_TopSymbol(si) == c) ||
         (PR_Praezedenz(TO_TopSymbol(si),c) == Groesser))){
      return TRUE;
    }
    si = TO_Schwanz(si);
  }
  return FALSE;
}

/* ------------------------------------------------------------------------- */
/* -------- E O F ---------------------------------------------------------- */
/* ------------------------------------------------------------------------- */
