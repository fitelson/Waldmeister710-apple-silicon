/* ------------------------------------------------------------------------- */
/* ----------- 50. four-valued functions through recursion ------------------ */
/* ------------------------------------------------------------------------- */

#include "simpleLPO_private.h"

/* ------------------------------------------------------------------------- */
/* ------------- Internal forward declarations ----------------------------- */
/* ------------------------------------------------------------------------- */

static VergleichsT s_cLPO50(UTermT s, UTermT t);
static BOOLEAN s_LPO_Greater50(UTermT s, UTermT t);

static BOOLEAN s_LPO_Alpha50(UTermT ss, UTermT s_NULL, UTermT t);
static BOOLEAN s_LPO_Delta50(UTermT s, UTermT t);

static BOOLEAN s_LPO_Majo50(UTermT s, UTermT ts, UTermT t_NULL);
static BOOLEAN s_LPO_LexMA50(UTermT s, UTermT t, 
                            UTermT ss, UTermT s_NULL, 
                            UTermT ts, UTermT t_NULL);

static VergleichsT s_cMA50(UTermT s, UTermT ts, UTermT t_NULL);
static VergleichsT s_cLMA50(UTermT s, UTermT t, 
                           UTermT ss, UTermT s_NULL, 
                           UTermT ts, UTermT t_NULL);
static VergleichsT s_cAA50(UTermT s, UTermT t, 
                          UTermT ss, UTermT s_NULL, 
                          UTermT ts, UTermT t_NULL);

static VergleichsT flip(VergleichsT v);

/* ------------------------------------------------------------------------- */
/* -------------- Interface to the outside --------------------------------- */
/* ------------------------------------------------------------------------- */

static unsigned counter;
VergleichsT S_LPO50(UTermT s, UTermT t)
{
  VergleichsT res;
  counter++;
  OPENTRACE(("S_LPO50: %d s = %t,  t = %t\n", counter, s, t));
  {
    res = s_cLPO50(s, t);
  }
  CLOSETRACE(("S_LPO50: s = %t,  t = %t --> %v\n", s, t, res));
  return res;
}

BOOLEAN S_LPOGroesser50(UTermT s, UTermT t)
{
  BOOLEAN res;

  OPENTRACE(("S_LPOGroesser50: s = %t,  t = %t\n", s, t));
  {
    res = s_LPO_Greater50(s, t);
  }
  CLOSETRACE(("S_LPOGroesser50: s = %t,  t = %t --> %b\n", s, t, res));
  return res;
}

/* ------------------------------------------------------------------------- */
/* -------------- Internal functionality ----------------------------------- */
/* ------------------------------------------------------------------------- */

static VergleichsT s_cLPO50(UTermT s, UTermT t)
{
  VergleichsT res = Unvergleichbar;

  OPENTRACE(("s_cLPO50: s = %t,  t = %t\n", s, t));
  {
#if S_doCounting
    S_counter++;
#endif
    if (TO_TermIstNichtVar(s)){
      if (TO_TermIstNichtVar(t)){               /* f(ss) vs g(ts) */
#if 0
        if (SO_SymbGleich(TO_TopSymbol(s), TO_TopSymbol(t))){
          goto Gleich;
        }
        switch (PR_Praezedenz(TO_TopSymbol(s), TO_TopSymbol(t))){
        case Gleich:
        Gleich:
          res = s_cLMA50(s, t, 
                        TO_ErsterTeilterm(s), TO_NULLTerm(s), 
                        TO_ErsterTeilterm(t), TO_NULLTerm(t) );
          break;
        case Groesser:
          res = s_cMA50(s, TO_ErsterTeilterm(t), TO_NULLTerm(t));
          break;
        case Kleiner:
          res = flip(s_cMA50(t, TO_ErsterTeilterm(s), TO_NULLTerm(s)));
          break;
        case Unvergleichbar:
          res = s_cAA50(s, t, 
                       TO_ErsterTeilterm(s), TO_NULLTerm(s), 
                       TO_ErsterTeilterm(t), TO_NULLTerm(t) );
          break;
        default:
          our_error("PR_Praezedenz delivers unexpected value (s_cLPO50)\n");
        }
#else
        if (SO_SymbGleich(TO_TopSymbol(s), TO_TopSymbol(t))){
          if (sTO_TermeGleich(s,t)){
            res = Gleich;
          }
          else {
            res = s_cLMA50(s, t, 
                          TO_ErsterTeilterm(s), TO_NULLTerm(s), 
                          TO_ErsterTeilterm(t), TO_NULLTerm(t) );
          }
        }
        else {
          VergleichsT prec = PR_Praezedenz(TO_TopSymbol(s), TO_TopSymbol(t));
          if (prec == Groesser){
            res = s_cMA50(s, TO_ErsterTeilterm(t), TO_NULLTerm(t));
          }
          else if (prec == Kleiner){
            res = flip(s_cMA50(t, TO_ErsterTeilterm(s), TO_NULLTerm(s)));
          }
          else {
            res = s_cAA50(s, t, 
                         TO_ErsterTeilterm(s), TO_NULLTerm(s), 
                         TO_ErsterTeilterm(t), TO_NULLTerm(t) );
          }
        }
#endif
      }
      else {                                    /* f(ss) vs y */
        if (sTO_SymbolEnthalten (TO_TopSymbol(t), s)){
          res = Groesser;
        }
      }
    }
    else {
      if (TO_TermIstNichtVar(t)){               /* x vs g(ts) */
        if (sTO_SymbolEnthalten (TO_TopSymbol(s), t)){
          res = Kleiner;
        }
      }
      else {                                    /* x vs y */
        if (SO_SymbGleich(TO_TopSymbol(s), TO_TopSymbol(t))){
          res = Gleich;
        }
      }
    }
  }
  CLOSETRACE(("s_cLPO50: s = %t,  t = %t --> %v\n", s, t, res));
  return res;
}

static BOOLEAN s_LPO_Greater50(UTermT s, UTermT t)
{
  BOOLEAN res;

  OPENTRACE(("s_LPO_Greater50: s = %t,  t = %t\n", s, t));
  {
#if S_doCounting
    S_counter++;
#endif
    if (TO_TermIstNichtVar(s)){
      if (TO_TermIstNichtVar(t)){
        if SO_SymbGleich(TO_TopSymbol(s), TO_TopSymbol(t)){
          res = s_LPO_LexMA50(s, t, 
                             TO_ErsterTeilterm(s), TO_NULLTerm(s), 
                             TO_ErsterTeilterm(t), TO_NULLTerm(t) );
        }
        else if (PR_Praezedenz(TO_TopSymbol(s), TO_TopSymbol(t)) == Groesser){
          res = s_LPO_Majo50(s, TO_ErsterTeilterm(t), TO_NULLTerm(t));
        }
        else {
          res = s_LPO_Alpha50(TO_ErsterTeilterm(s), TO_NULLTerm(s), t);
        }
      }
      else {
        res = s_LPO_Delta50(s,t);
      }
    }
    else {
      res = FALSE;
    }
  }
  CLOSETRACE(("s_LPO_Greater50: s = %t,  t = %t --> %b\n", s, t, res));
  return res;
}

/* ------------------------------------------------------------------------- */

static BOOLEAN s_LPO_Alpha50(UTermT ss, UTermT s_NULL, UTermT t)
{
  BOOLEAN res = FALSE; /* default for empty list */

  OPENTRACE(("s_LPOAlpha50: ss = %A,  t = %t\n", ss, s_NULL, t));
  {
    UTermT s_i = ss;
    while(s_i != s_NULL) {    
      if (sTO_TermeGleich(s_i,t) || s_LPO_Greater50(s_i,t)){
        res = TRUE;
        break;
      }
      else {
        s_i = TO_NaechsterTeilterm(s_i);
      }
    }
  }
  CLOSETRACE(("s_LPOAlpha50: ss = %A,  t = %t --> %b\n", ss, s_NULL, t, res));
  return res;
}

static BOOLEAN s_LPO_Delta50(UTermT s, UTermT t) 
{
  BOOLEAN res;

  OPENTRACE(("s_LPODelta50: s = %t,  t = %t\n", s, t));
  {
    res = sTO_SymbolEnthalten (TO_TopSymbol(t), s);
  }
  CLOSETRACE(("s_LPODelta50: s = %t,  t = %t --> %b\n", s, t, res));
  return res;
}

/* ------------------------------------------------------------------------- */

static BOOLEAN s_LPO_Majo50(UTermT s, UTermT ts, UTermT t_NULL)
{
  BOOLEAN res = TRUE; /* default value for empty list */

  OPENTRACE(("s_LPO_Majo50: s = %t, ts = %A\n", s, ts, t_NULL));
  {
    UTermT t_j = ts;
    while(t_j != t_NULL){
      if (s_LPO_Greater50(s, t_j)) {
        t_j = TO_NaechsterTeilterm(t_j);
      }
      else {
        res = FALSE;
        break;
      }
    }
  }
  CLOSETRACE(("s_LPO_Majo50: s = %t, ts = %A --> %b\n", s, ts, t_NULL, res));
  return res;
}

static BOOLEAN s_LPO_LexMA50(UTermT s, UTermT t, 
                            UTermT ss, UTermT s_NULL, 
                            UTermT ts, UTermT t_NULL)
{
  BOOLEAN res = FALSE; /* default value for empty list */

  OPENTRACE(("s_LPO_LexM50: s = %t,  t = %t, ss = %A, ts = %A\n", 
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
        if (s_LPO_Greater50(s_i, t_i)){
          res = s_LPO_Majo50(s, TO_NaechsterTeilterm(t_i), t_NULL);
        }
        else {
          res = s_LPO_Alpha50(TO_NaechsterTeilterm(s_i), s_NULL, t);
        }
        break;
      }
    }
  }
  CLOSETRACE(("s_LPO_LexM50: s = %t, t = %t, ss = %A, ts = %A --> %b\n", 
              s, t, ss, s_NULL, ts, t_NULL, res));
  return res;
}

static VergleichsT s_cMA50(UTermT s, UTermT ts, UTermT t_NULL)
{
  VergleichsT res = Groesser; /* default value for empty list */

  OPENTRACE(("s_cMA50: s = %t, ts = %A\n", s, ts, t_NULL));
  {
    UTermT t_j = ts;
#if 0
    while ((t_j != t_NULL) && (res == Groesser)){
      switch (s_cLPO50(s,t_j)){
        case Groesser:
          t_j = TO_NaechsterTeilterm(t_j);
          break;
        case Gleich:
        case Kleiner:
          res = Kleiner;
          break;
        case Unvergleichbar:
          if (s_LPO_Alpha50(TO_NaechsterTeilterm(t_j), t_NULL, s)){
            res = Kleiner;
          }
          else {
            res = Unvergleichbar;
          }
          break;
        default:
          our_error("s_cLPO50 delivers unexpected value (s_cMA50)\n");
      }
    }
#else
    while ((t_j != t_NULL)){
      VergleichsT v = s_cLPO50(s,t_j);
      if (v == Groesser){
        t_j = TO_NaechsterTeilterm(t_j);
      }
      else if (v == Unvergleichbar){
        if (s_LPO_Alpha50(TO_NaechsterTeilterm(t_j), t_NULL, s)){
          res = Kleiner;
        }
        else {
          res = Unvergleichbar;
        }
        break;
      }
      else {
        res = Kleiner;
        break;
      }
    }
#endif
  }
  CLOSETRACE(("s_cMA50: s = %t, ts = %A --> %v\n", s, ts, t_NULL, res));
  return res;
}
static VergleichsT s_cLMA50(UTermT s, UTermT t, 
                           UTermT ss, UTermT s_NULL, 
                           UTermT ts, UTermT t_NULL)
{
  VergleichsT res = Gleich; /* default value for empty list */

  OPENTRACE(("s_cLMA50: s = %t,  t = %t, ss = %A, ts = %A\n", 
             s, t, ss, s_NULL, ts, t_NULL));
  {
    UTermT s_i  = ss;
    UTermT t_i  = ts;
#if 0
    while ((s_i != s_NULL) && (res == Gleich)){
      switch (s_cLPO50(s_i, t_i)){
        case Gleich:
          s_i = TO_NaechsterTeilterm(s_i);
          t_i = TO_NaechsterTeilterm(t_i);
          break;
        case Groesser:
          res = s_cMA50(s, TO_NaechsterTeilterm(t_i), t_NULL);
          break; /* res != Gleich */
        case Kleiner:
          res = flip(s_cMA50(t, TO_NaechsterTeilterm(s_i), s_NULL));
          break; /* res != Gleich */
        case Unvergleichbar:
          res = s_cAA50(s,t,
                       TO_NaechsterTeilterm(s_i), s_NULL, 
                       TO_NaechsterTeilterm(t_i), t_NULL);
          break; /* res != Gleich */
        default:
          our_error("s_cLPO50 delivers unexpected value (s_cLMA50)\n");
      }
    }
#else
    while ((s_i != s_NULL)){
      VergleichsT v = s_cLPO50(s_i, t_i);
      if (v == Gleich){
        s_i = TO_NaechsterTeilterm(s_i);
        t_i = TO_NaechsterTeilterm(t_i);
      }
      else if (v == Groesser) {
        res = s_cMA50(s, TO_NaechsterTeilterm(t_i), t_NULL);
        break;
      }
      else if (v == Kleiner){
        res = flip(s_cMA50(t, TO_NaechsterTeilterm(s_i), s_NULL));
        break;
      }
      else {
          res = s_cAA50(s,t,
                       TO_NaechsterTeilterm(s_i), s_NULL, 
                       TO_NaechsterTeilterm(t_i), t_NULL);
          break;
      }
    }
#endif
  }
  CLOSETRACE(("s_cLMA50: s = %t, t = %t, ss = %A, ts = %A --> %v\n", 
              s, t, ss, s_NULL, ts, t_NULL, res));
  return res;
}

static VergleichsT s_cAA50(UTermT s, UTermT t, 
                          UTermT ss, UTermT s_NULL, 
                          UTermT ts, UTermT t_NULL)
{
  VergleichsT res;

  OPENTRACE(("s_cAA50: s = %t,  t = %t, ss = %A, ts = %A\n", 
             s, t, ss, s_NULL, ts, t_NULL));
  {
    if (s_LPO_Alpha50(ss, s_NULL, t)){
      res = Groesser;
    }
    else if (s_LPO_Alpha50(ts, t_NULL, s)){
      res = Kleiner;
    }
    else {
      res = Unvergleichbar;
    }
  }
  CLOSETRACE(("s_cAA50: s = %t, t = %t, ss = %A, ts = %A --> %v\n", 
              s, t, ss, s_NULL, ts, t_NULL, res));
  return res;
}

static VergleichsT flip(VergleichsT v)
{
  if (v == Groesser) return Kleiner;
  if (v == Kleiner)  return Groesser;
  return v;
}

/* ------------------------------------------------------------------------- */
/* -------- E O F ---------------------------------------------------------- */
/* ------------------------------------------------------------------------- */
