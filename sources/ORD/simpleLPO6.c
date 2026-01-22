/* ------------------------------------------------------------------------- */
/* ----------- 6. s_LPO_Greater computes greater and equal ---------------- */
/* ------------------------------------------------------------------------- */

#include "simpleLPO_private.h"

/* ------------------------------------------------------------------------- */
/* ------------- Internal forward declarations ----------------------------- */
/* ------------------------------------------------------------------------- */

static VergleichsT s_LPO_Greater6(UTermT s, UTermT t);

static VergleichsT s_LPO_Alpha6(UTermT ss, UTermT s_NULL, UTermT t);
static VergleichsT s_LPO_Delta6(UTermT s, UTermT t);

static VergleichsT s_LPO_Majo6(UTermT s, UTermT ts, UTermT t_NULL);
static VergleichsT s_LPO_LexMA6(UTermT s, UTermT t, 
			    UTermT ss, UTermT s_NULL, 
			    UTermT ts, UTermT t_NULL);

/* ------------------------------------------------------------------------- */
/* -------------- Interface to the outside --------------------------------- */
/* ------------------------------------------------------------------------- */

VergleichsT S_LPO6(UTermT s, UTermT t)
{
  VergleichsT res = Unvergleichbar;

  OPENTRACE(("S_LPO6: s = %t,  t = %t\n", s, t));
  {
      switch(s_LPO_Greater6(s, t)) {
      case Groesser:
	res = Groesser;
	break;
      case Gleich:
	res = Gleich;
	break;
      case Unvergleichbar:
	res = (s_LPO_Greater6(t, s) == Groesser) ? Kleiner : Unvergleichbar;
	break;
      default:
	our_error("unexpected value from s_LPO_Greater6 (1)\n");
      }
  }
  CLOSETRACE(("S_LPO6: s = %t,  t = %t --> %v\n", s, t, res));
  return res;
}

BOOLEAN S_LPOGroesser6(UTermT s, UTermT t)
{
  BOOLEAN res;

  OPENTRACE(("S_LPOGroesser6: s = %t,  t = %t\n", s, t));
  {
    res = (s_LPO_Greater6(s, t) == Groesser);
  }
  CLOSETRACE(("S_LPOGroesser6: s = %t,  t = %t --> %b\n", s, t, res));
  return res;
}

/* ------------------------------------------------------------------------- */
/* -------------- Internal functionality ----------------------------------- */
/* ------------------------------------------------------------------------- */

static VergleichsT s_LPO_Greater6(UTermT s, UTermT t)
{
  VergleichsT res = Unvergleichbar;

  OPENTRACE(("s_LPO_Greater6: s = %t,  t = %t\n", s, t));
  {
#if S_doCounting
    S_counter++;
#endif
    if (TO_TermIstNichtVar(s)){
      if (TO_TermIstNichtVar(t)){
	if (SO_SymbGleich(TO_TopSymbol(s), TO_TopSymbol(t))){
	  res = s_LPO_LexMA6(s, t, 
			     TO_ErsterTeilterm(s), TO_NULLTerm(s), 
			     TO_ErsterTeilterm(t), TO_NULLTerm(t) );
	}
	else if (PR_Praezedenz(TO_TopSymbol(s), TO_TopSymbol(t)) == Groesser){
	  res = s_LPO_Majo6(s, TO_ErsterTeilterm(t), TO_NULLTerm(t));
	}
	else {
	  res = s_LPO_Alpha6(TO_ErsterTeilterm(s), TO_NULLTerm(s), t);
	}
      }
      else {
	res = s_LPO_Delta6(s,t);
      }
    }
    else {
      if (SO_SymbGleich(TO_TopSymbol(s), TO_TopSymbol(t))){
	res = Gleich;
      }
    }
  }
  CLOSETRACE(("s_LPO_Greater6: s = %t,  t = %t --> %v\n", s, t, res));
  return res;
}

/* ------------------------------------------------------------------------- */

static VergleichsT s_LPO_Alpha6(UTermT ss, UTermT s_NULL, UTermT t)
{
  VergleichsT res = Unvergleichbar; 
  UTermT s_i = ss;

  OPENTRACE(("s_LPOAlpha6: ss = %A,  t = %t\n", ss, s_NULL, t));
  {
    while(s_i != s_NULL) {    
      if (s_LPO_Greater6(s_i,t) != Unvergleichbar){
	res = Groesser;
	break;
      }
      s_i = TO_NaechsterTeilterm(s_i);
    }
    /* res was initalized with Unvergleichbar */ 
  }
  CLOSETRACE(("s_LPOAlpha6: ss = %A,  t = %t --> %v\n", ss, s_NULL, t, res));
  return res;
}

static VergleichsT s_LPO_Delta6(UTermT s, UTermT t) 
{
  VergleichsT res = Unvergleichbar;

  OPENTRACE(("s_LPODelta6: s = %t,  t = %t\n", s, t));
  {
    if (sTO_SymbolEnthalten (TO_TopSymbol(t), s)){
      res = Groesser;
    }
  }
  CLOSETRACE(("s_LPODelta6: s = %t,  t = %t --> %v\n", s, t, res));
  return res;
}

/* ------------------------------------------------------------------------- */

static VergleichsT s_LPO_Majo6(UTermT s, UTermT ts, UTermT t_NULL)
{
  VergleichsT res = Groesser;
  UTermT t_j = ts;

  OPENTRACE(("s_LPO_Majo6: s = %t, ts = %A\n", s, ts, t_NULL));
  {
    while(t_j != t_NULL) {    
      if (s_LPO_Greater6(s, t_j) != Groesser) {
	res = Unvergleichbar;
	break;
      }
      t_j = TO_NaechsterTeilterm(t_j);
    }
    /* res was initalized with Groesser */ 
  }
  CLOSETRACE(("s_LPO_Majo6: s = %t, ts = %A --> %v\n", s, ts, t_NULL, res));
  return res;
}

static VergleichsT s_LPO_LexMA6(UTermT s, UTermT t, 
			    UTermT ss, UTermT s_NULL, 
			    UTermT ts, UTermT t_NULL)
{
  VergleichsT res = Gleich;
  UTermT s_i  = ss;
  UTermT t_i  = ts;

  OPENTRACE(("s_LPO_LexMA6: s = %t,  t = %t, ss = %A, ts = %A\n", 
	     s, t, ss, s_NULL, ts, t_NULL));
  {
    while (s_i != s_NULL){       /* Implies t_i != t_NULL (fixed arity!) */ 
      switch(s_LPO_Greater6(s_i, t_i)) {
      case Groesser:
	res = s_LPO_Majo6(s, TO_NaechsterTeilterm(t_i), t_NULL);
	goto endwhile;
      case Gleich:
	s_i = TO_NaechsterTeilterm(s_i);
	t_i = TO_NaechsterTeilterm(t_i);
	break;
      case Unvergleichbar:
	res = s_LPO_Alpha6(TO_NaechsterTeilterm(s_i), s_NULL, t);
	goto endwhile;
      default:
	our_error("unexpected value from s_LPO_Greater6\n");
      }
    }
  endwhile:; 
  }
  CLOSETRACE(("s_LPO_LexMA6: s = %t, t = %t, ss = %A, ts = %A --> %v\n", 
	      s, t, ss, s_NULL, ts, t_NULL, res));
  return res;
}

/* ------------------------------------------------------------------------- */
/* -------- E O F ---------------------------------------------------------- */
/* ------------------------------------------------------------------------- */
