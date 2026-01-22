#include "ZwangsGrund.h"

#include "FehlerBehandlung.h"
#include "Praezedenz.h"
#include "SymbolOperationen.h"
#include "SymbolGewichte.h"
#include "TermOperationen.h"

static BOOLEAN zgo_containsExtraconst(UTermT t)
{
  UTermT t0 = TO_NULLTerm(t);
  UTermT t1 = t;
  do {
    if (SO_IstExtrakonstante(TO_TopSymbol(t1))){
      return TRUE;
    }
    t1 = TO_Schwanz(t1);
  } while (t1 != t0);
  return FALSE;
}

/* ------------------------------------------------------------------------- */
/* ------------------------------------------------------------------------- */
/* ----------- LPO stolen from simpleLPO4.c and then modified -------------- */
/* ------------------------------------------------------------------------- */
/* ------------------------------------------------------------------------- */

#define OPENTRACE(x)  /* nix */
#define CLOSETRACE(x) /* nix */
#define RESTRACE(x)   /* nix */

static BOOLEAN zgo_LPO_Greater(UTermT s, UTermT t);

static BOOLEAN zgo_LPO_Alpha(UTermT ss, UTermT s_NULL, UTermT t);

static BOOLEAN zgo_LPO_Majo(UTermT s, UTermT ts, UTermT t_NULL);
static BOOLEAN zgo_LPO_LexMA(UTermT s, UTermT t, 
                             UTermT ss, UTermT s_NULL, 
                             UTermT ts, UTermT t_NULL);

/* ------------------------------------------------------------------------- */
/* -------------- Interface to the outside --------------------------------- */
/* ------------------------------------------------------------------------- */

VergleichsT ZGO_LPO(UTermT s, UTermT t)
{
  VergleichsT res;
  OPENTRACE(("ZGO_LPO: s = %t,  t = %t\n", s, t));
  {
    if (zgo_containsExtraconst(s) || zgo_containsExtraconst(t)){
      our_error("ZGO_LPO called with terms containing extra-constants!");
    }
    if (TO_TermeGleich(s,t)){
      res = Gleich;
    }
    else if (zgo_LPO_Greater(s,t)){
      res = Groesser;
    }
    else if (zgo_LPO_Greater(t,s)){
      res = Kleiner;
    }
    else {
      our_error("ZGO_LPO should be total!");
      res = Unvergleichbar;
    }
  }
  CLOSETRACE(("ZGO_LPO: s = %t,  t = %t --> %v\n", s, t, res));
  return res;
}


BOOLEAN ZGO_LPOGroesser(UTermT s, UTermT t)
{
  BOOLEAN res;

  OPENTRACE(("ZGO_LPOGroesser: s = %t,  t = %t\n", s, t));
  {
    if (zgo_containsExtraconst(s) || zgo_containsExtraconst(t)){
      our_error("ZGO_LPOGroesser called with terms containing "
                "extra-constants!");
    }
    res = zgo_LPO_Greater(s, t);
  }
  CLOSETRACE(("ZGO_LPOGroesser: s = %t,  t = %t --> %b\n", s, t, res));
  return res;
}

/* ------------------------------------------------------------------------- */
/* -------------- Internal functionality ----------------------------------- */
/* ------------------------------------------------------------------------- */

static BOOLEAN zgo_LPO_Greater(UTermT s, UTermT t)
{
  BOOLEAN res;

  OPENTRACE(("zgo_LPO_Greater: s = %t,  t = %t\n", s, t));
  {
    if (TO_TermIstNichtVar(s)){
      if (TO_TermIstNichtVar(t)){
        /* f(s_1,...,s_n) > g(t_1,...,t_m) */
        if SO_SymbGleich(TO_TopSymbol(s), TO_TopSymbol(t)){
          res = zgo_LPO_LexMA(s, t, 
                             TO_ErsterTeilterm(s), TO_NULLTerm(s), 
                             TO_ErsterTeilterm(t), TO_NULLTerm(t) );
        }
        else if (PR_Praezedenz(TO_TopSymbol(s), TO_TopSymbol(t)) 
                 == Groesser){
          res = zgo_LPO_Majo(s, TO_ErsterTeilterm(t), TO_NULLTerm(t));
        }
        else {
          res = zgo_LPO_Alpha(TO_ErsterTeilterm(s), TO_NULLTerm(s), t);
        }
      }
      else {
        /* f(s_1,...,s_n) > y */
        res = TRUE;
      }
    }
    else {
      if (TO_TermIstNichtVar(t)){
        /* x > g(t_1,...,t_m) */
        res = FALSE;
      }
      else {
        /* x > y */
        res = (TO_TopSymbol(s) > TO_TopSymbol(t));
      }
    }
  }
  CLOSETRACE(("zgo_LPO_Greater: s = %t,  t = %t --> %b\n", s, t, res));
  return res;
}

/* ------------------------------------------------------------------------- */

static BOOLEAN zgo_LPO_Alpha(UTermT ss, UTermT s_NULL, UTermT t)
{
  BOOLEAN res = FALSE; /* default for empty list */

  OPENTRACE(("s_LPOAlpha4: ss = %A,  t = %t\n", ss, s_NULL, t));
  {
    UTermT s_i = ss;
    while (s_i != s_NULL) {    
      if (TO_TermeGleich(s_i,t) || zgo_LPO_Greater(s_i,t)){
        res = TRUE;
        break;
      }
      else {
        s_i = TO_NaechsterTeilterm(s_i);
      }
    }
  }
  CLOSETRACE(("s_LPOAlpha4: ss = %A,  t = %t --> %b\n", ss, s_NULL, t, res));
  return res;
}

/* ------------------------------------------------------------------------- */

static BOOLEAN zgo_LPO_Majo(UTermT s, UTermT ts, UTermT t_NULL)
{
  BOOLEAN res = TRUE; /* default value for empty list */

  OPENTRACE(("zgo_LPO_Majo: s = %t, ts = %A\n", s, ts, t_NULL));
  {
    UTermT t_j = ts;
    while (t_j != t_NULL){
      if (zgo_LPO_Greater(s, t_j)) {
        t_j = TO_NaechsterTeilterm(t_j);
      }
      else {
        res = FALSE;
        break;
      }
    }
  }
  CLOSETRACE(("zgo_LPO_Majo: s = %t, ts = %A --> %b\n", s, ts, t_NULL, res));
  return res;
}

static BOOLEAN zgo_LPO_LexMA(UTermT s, UTermT t, 
                            UTermT ss, UTermT s_NULL, 
                            UTermT ts, UTermT t_NULL)
{
  BOOLEAN res = FALSE; /* default value for empty list */

  OPENTRACE(("zgo_LPO_LexMA: s = %t,  t = %t, ss = %A, ts = %A\n", 
             s, t, ss, s_NULL, ts, t_NULL));
  {
    UTermT s_i  = ss;
    UTermT t_i  = ts;
    while (s_i != s_NULL){       /* Implies t_i != t_NULL (fixed arity!) */ 
      if (TO_TermeGleich(s_i,t_i)){ 
        s_i = TO_NaechsterTeilterm(s_i);
        t_i = TO_NaechsterTeilterm(t_i);
      }
      else {
        if (zgo_LPO_Greater(s_i, t_i)){
          res = zgo_LPO_Majo(s, TO_NaechsterTeilterm(t_i), t_NULL);
        }
        else {
          res = zgo_LPO_Alpha(TO_NaechsterTeilterm(s_i), s_NULL, t);
        }
        break;
      }
    }
  }
  CLOSETRACE(("zgo_LPO_LexMA: s = %t, t = %t, ss = %A, ts = %A --> %b\n", 
              s, t, ss, s_NULL, ts, t_NULL, res));
  return res;
}

/* ------------------------------------------------------------------------- */
/* ------------------------------------------------------------------------- */
/* ----------- KBO stolen from KBO.c (Neufassung) and then modified -------- */
/* ------------------------------------------------------------------------- */
/* ------------------------------------------------------------------------- */

static long zgo_phi(UTermT s)
{
  long res  = 0;
  UTermT s0 = TO_NULLTerm(s);
  UTermT s1 = s;
  do {
    res = res + SG_SymbolGewichtKBO(TO_TopSymbol(s1));
    s1  = TO_Schwanz(s1);
  } while (s0 != s1);
  return res;
}

static BOOLEAN zgo_KBOGroesser(UTermT s, UTermT t)
{
  SymbolT tops = TO_TopSymbol(s);
  SymbolT topt = TO_TopSymbol(t);

  if (SO_SymbIstVar(tops))
    return SO_SymbIstVar(topt) && (tops > topt);
  if (SO_SymbIstVar(topt))
    return TRUE;

  {
    long phidiff = zgo_phi(s) - zgo_phi(t);
    if (phidiff != 0)
      return phidiff > 0;
  }
  {
    VergleichsT precVgl = PR_Praezedenz(tops,topt);
    if (precVgl != Gleich) 
      return precVgl == Groesser;
  }
  {
    UTermT s0 = TO_NULLTerm(s);
    UTermT s1 = TO_ErsterTeilterm(s);
    UTermT t1 = TO_ErsterTeilterm(t);
    while (s0 != s1){
      if (!TO_TermeGleich(s1,t1)) 
        return zgo_KBOGroesser(s1,t1);
      s1 = TO_NaechsterTeilterm(s1);
      t1 = TO_NaechsterTeilterm(t1);
    } 
    return FALSE; /* Nur wenn TO_TermeGleich(s, t) */
  }
}

/* ------------------------------------------------------------------------- */
/* --------------- exportierte Funktionen ---------------------------------- */
/* ------------------------------------------------------------------------- */

BOOLEAN ZGO_KBOGroesser(UTermT s, UTermT t)
{
  if (zgo_containsExtraconst(s) || zgo_containsExtraconst(t)){
    our_error("ZGO_KBOGroesser called with terms containing extra-constants!");
  }
  return zgo_KBOGroesser(s,t);
}

VergleichsT ZGO_KBOVergleich(UTermT s, UTermT t)
{
  if (zgo_containsExtraconst(s) || zgo_containsExtraconst(t)){
    our_error("ZGO_KBOVergleich called with terms containing "
              "extra-constants!");
  }
  if (TO_TermeGleich(s,t))  return Gleich;
  if (zgo_KBOGroesser(s,t)) return Groesser;
  if (zgo_KBOGroesser(s,t)) return Kleiner;
  our_error("ZGO_KBOVergleich should be total!");
  return Unvergleichbar;
}

/* ------------------------------------------------------------------------- */
/* -------- E O F ---------------------------------------------------------- */
/* ------------------------------------------------------------------------- */
