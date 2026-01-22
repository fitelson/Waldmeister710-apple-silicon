#include "compiler-extra.h"
#include "ConfluenceTrees_intern.h"
#include "Ausgaben.h"
#include "Constraints.h"
#include "NFBildung.h"
#include "MatchOperationen.h" 
#include "RUndEVerwaltung.h"
#include "Subsumption.h"

/* -------------------------------------------------------------------------- */
/* - file static variables -------------------------------------------------- */
/* -------------------------------------------------------------------------- */

static decModeT tree_mode       = Abbrechen; /* ?? */
static unsigned int level       = 0;


/* -------------------------------------------------------------------------- */
/* - forward declarations --------------------------------------------------- */
/* -------------------------------------------------------------------------- */

static BOOLEAN handleBranch (TermT s, TermT t, ContextT c);

/* -------------------------------------------------------------------------- */
/* - generische Hilfsroutingen ---------------------------------------------- */
/* -------------------------------------------------------------------------- */

#if VERBOSE

static void verbTreeIndent(void)
{
  int i = level;
  printf("|");
  while (i > 0){
    printf("> ");
    i--;
  }
}

#define VERB(x) {verbTreeIndent();IO_DruckeFlex x;}
#else
#define VERB(x) /* nix tun */
#endif

static UTermT anGleicherStelle (TermT s, TermT t, UTermT t1)
/* ist nur korrekt, weil t1 mittels Pointer-Gleichheit und nicht
 * TT-Gleichheit verglichen wird!
 */
{
  UTermT s1 = s;
  while (t != t1){
    t = TO_Schwanz(t);
    s1 = TO_Schwanz(s1);
  }
  return s1;
}

/* -------------------------------------------------------------------------- */
/* - Behandeln von Confluence-Trees ----------------------------------------- */
/* -------------------------------------------------------------------------- */

static BOOLEAN handleNode (TermT s, TermT t, ContextT c)
{
  BOOLEAN res = FALSE;
  OPENTRACE (("handleNode: %t =?= %t %C\n", s, t, c));
  /*  IO_DruckeFlex("handleNode: %t =?= %t %C\n", s, t, c);*/
  level++;
  if ((level > DEPTH_LIMIT) && LIMIT_DEPTH){
    our_error("nesting to deep");
  }
  VERB(("## %t =?= %t %C\n", s, t, c));
  {
    if (TO_TermeGleich(s,t)){
      RESTRACE(("handleNode: Terme sind direkt gleich: %t = %t\n",s,t));
      VERB(("handleNode: Terme sind direkt gleich: %t = %t\n",s,t));
      res = TRUE;
    }
    else {
      TermT s1 = CO_substitute(c->bindings,s);
      TermT t1 = CO_substitute(c->bindings,t);
      if(CO_somethingBound(c->bindings)){
        VERB(("handleNode: Terme nach subst: %t = %t\n",s1,t1));
      }
      if (TO_TermeGleich(s1,t1)){
	RESTRACE(("handleNode: Terme sind gleich nach subst: %t = %t\n",s1,t1));
        VERB(("handleNode: Terme sind gleich nach subst: %t = %t\n",s1,t1));
	res = TRUE;
      }
      else {
	ContextT c1 = CO_solveEQPart(c);
	ContextT cold = CO_getCurrentContext();
	CO_setCurrentContext(c1);
	NF_NormalformRE(s1);
	NF_NormalformRE(t1);
	if (TO_TermeGleich(s1,t1)){
	  RESTRACE(("handleNode: Terme sind gleich nach NFB:   %t = %t\n",s1,t1));
          VERB(("handleNode: Terme sind gleich nach NFB:   %t = %t\n",s1,t1));
	  res = TRUE;
	}
	else if (SS_TermpaarSubsummiertVonGM (s1,t1)){
	  RESTRACE(("handleNode: Terme werden subsumiert: %t = %t\n",s1,t1));
          VERB(("handleNode: Terme werden nach NFB subsumiert: %t = %t\n",s1,t1));
	  res = TRUE;
	}
	else {
          VERB(("handleNode: Terme nach NFB:   %t = %t\n",s1,t1));
	  res = handleBranch(s1,t1,c1);
	}
	CO_setCurrentContext(cold);
	CO_deepDelete(c1);
      }
      TO_TermeLoeschen(s1,t1);
    }
  }
  VERB(("%b\n", res));
  level--;
  CLOSETRACE (("handleNode: -> %b\n", res));
  return res;
}

static BOOLEAN expandOneSide(TermT s, TermT tl, TermT tr, ContextT c, 
			     BOOLEAN *resexp)
{
  BOOLEAN res = FALSE;
  BOOLEAN expanded = FALSE;
  OPENTRACE (("expandOneSide(%s): %t =?= %t %C\n", (s == tl) ? "left" : "right",
	      tl, tr, c));
  /*  IO_DruckeFlex("expandOneSide(%s): %t =?= %t %C\n", (s == tl) ? "left" : "right",
      tl, tr, c);*/
  {
    GleichungsT gleichung;
    UTermT s1 = s;
    UTermT s0 = TO_NULLTerm(s);
    do {
      if (TO_TermIstNichtVar(s1)){
#if TRY_ALL_EQNS
	unsigned int try = 0;
#endif 
	MO_AllGleichungMatchInit(s1);
	while (!expanded && 
	       MO_AllGleichungMatchNext(FALSE, &gleichung)){
	  TermT sneu = MO_AllTermErsetztNeu(s,s1); /* neue Kopie */
	  UTermT sneu1 = anGleicherStelle(sneu,s,s1);
	  /* sigma(u) == s1, sigma(v) == sneu1 */
	  DConsT d1,d2,drun;
#if TRY_ALL_EQNS
	  try++;
#endif 
	  switch (CO_getAllSolutionsInBothDirections(s1,sneu1,c,&d1,&d2)){
          case 1:
	    expanded = TRUE;
	    res = TRUE;

#if 0
	    VERB((" split with %D and %D\n", d1, d2));
	    drun = d1;
	    /*	      printf("go down\n");*/
	    do {
	      if (s == tl){
		res = handleNode(sneu,tr,drun->c);
	      }
	      else {
		res = handleNode(tl,sneu,drun->c);
	      }
	      drun = drun->Nachf;
	    } while (res && (drun != NULL));
	    drun = d2;
	    while (res && (drun != NULL)) {
	      res = handleNode(tl,tr,drun->c);
	      drun = drun->Nachf;
	    } 
#else
	    VERB((" split with %D and %D\n", d2, d1));
	    /*IO_DruckeFlex(" split with %D and %D\n", d2, d1);*/
	    drun = d2;
	    while ((res || (tree_mode == Weitersuchen)) && (drun != NULL)) {
	      res = handleNode(tl,tr,drun->c);
	      drun = drun->Nachf;
	    } 
	    drun = d1;
	    while ((res || (tree_mode == Weitersuchen)) && (drun != NULL)) {
	      if (s == tl){
		res = handleNode(sneu,tr,drun->c);
	      }
	      else {
		res = handleNode(tl,sneu,drun->c);
	      }
	      drun = drun->Nachf;
	    } 
#endif
#if TRY_ALL_EQNS
	    if (!res){
	      unsigned int counter = 0;
	      expanded = res;
	      /* Problem: matcher nicht reentrant -> rekonstruieren */
	      MO_AllGleichungMatchInit(s1); 
	      while (counter < try){
		MO_AllGleichungMatchNext(FALSE, &gleichung);
		/*		  printf("reconstruct\n");*/
		counter++;
	      }
	    }
#endif 
	    CO_deleteDConsList(d1);
	    CO_deleteDConsList(d2);
            break;
          case 2:
#if GIVE_UP_BY_LENGTH_LIMIT
            res = FALSE;
            expanded = TRUE;
            break;
#endif
          default:
            /* nix */
            break;
          }
	  TO_TermLoeschen(sneu);
	}
      }
      s1 = TO_Schwanz(s1);
    } while (!expanded && (s1 != s0));
  }
  CLOSETRACE (("expandOneSide: -> %b\n", res));
  *resexp = expanded;
  return res;
}

static BOOLEAN handleBranch (TermT s, TermT t, ContextT c)
/* s und t konnten nicht unter c zusammengefuehrt werden, c ist erfuellbar.
 * Der Gleichheitsanteil von c ist schon durchsubstituiert.
 * Also suchen wir u=v \in E und machen einen Rewrite-Schritt unter einem
 * erweiterten Constraint (sowie das Gegenstueck).
 */
{
  BOOLEAN res = FALSE;
  OPENTRACE (("handleBranch: %t =?= %t %C\n", s, t, c));
  /*  IO_DruckeFlex("handleBranch: %t =?= %t %C\n", s, t, c);*/
  {
    BOOLEAN expanded = FALSE;
    res = expandOneSide(s,s,t,c,&expanded);
    if (!expanded){
      res = expandOneSide(t,s,t,c,&expanded);
    }
    if (!res && do_witnesses && (witness_l == NULL)){
      witness_l = TO_Termkopie(s);
      witness_r = TO_Termkopie(t);
    }
  }
  CLOSETRACE (("handleBranch: -> %b\n", res));
  return res;
  /* INVARIANTE:     RES = FALSE -> BACKTRACKING ABBRECHEN
   *                 RES = TRUE  -> BACKTRACKING WEITERMACHEN
   * GESAMTERGEBNIS: SEITENEFFEKT !!!
   */
}

/* -------------------------------------------------------------------------- */

BOOLEAN CO_GroundJoinable1(TermT s, TermT t)
{
  BOOLEAN res = FALSE;
  {
    ContextT c;
    level = 0;
    c = CO_newContext(CO_allocBindings());
    if (VERBOSE) {
      IO_DruckeFlex("| %t =?= %t %C\n", s, t, c);
    }
    res = handleBranch(s,t,c);
    if (VERBOSE) {
      IO_DruckeFlex("| %b\n", res);
    }
    CO_deepDelete(c);
  }
  return res;
}

/* ------------------------------------------------------------------------- */
/* -------- E O F ---------------------------------------------------------- */
/* ------------------------------------------------------------------------- */
