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

static unsigned int level2      = 0;

/* -------------------------------------------------------------------------- */
/* - forward declarations --------------------------------------------------- */
/* -------------------------------------------------------------------------- */

static BOOLEAN handleBranch2 (TermT s, TermT t, EConsT e, ContextT c);

/* -------------------------------------------------------------------------- */
/* - generische Hilfsroutingen ---------------------------------------------- */
/* -------------------------------------------------------------------------- */

#if VERBOSE

static void verbTreeIndent(void)
{
  int i = level2;
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


static BOOLEAN handleNode2 (TermT s, TermT t, EConsT e, ContextT c)
{
  BOOLEAN res = FALSE;
  OPENTRACE (("handleNode2: %t =?= %t %E %C\n", s, t, e,c));
  VERB (("handleNode2: %t =?= %t %E\n", s, t, e));
  /*  IO_DruckeFlex("handleNode: %t =?= %t %C\n", s, t, c);*/
  {
    if (TO_TermeGleich(s,t)){
      RESTRACE(("handleNode2: Terme sind direkt gleich: %t = %t\n",s,t));
      VERB(("handleNode2: Terme sind direkt gleich: %t = %t\n",s,t));
      res = TRUE;
    }
    else if (SS_TermpaarSubsummiertVonGM (s,t)){
      RESTRACE(("handleNode: Terme werden subsumiert: %t = %t\n",s,t));
      VERB(("handleNode2: Terme werden subsumiert: %t = %t\n",s,t));
      res = TRUE;
    }
    else {
      VERB(("handleNode2: Terme noch nciht gleich: %t = %t\n",s,t));
      res = handleBranch2(s,t,e,c);
    }
  }
  CLOSETRACE (("handleNode2: -> %b\n", res));
  return res;
}

static BOOLEAN expandOneSide2(TermT s, TermT tl, TermT tr, EConsT e, ContextT c, 
                              BOOLEAN *resexp)
{
  BOOLEAN res = FALSE;
  BOOLEAN expanded = FALSE;
  OPENTRACE (("expandOneSide2(%s): %t =?= %t %E\n", (s == tl) ? "left" : "right",
	      tl, tr, e));
  /*  IO_DruckeFlex("expandOneSide(%s): %t =?= %t %C\n", (s == tl) ? "left" : "right",
      tl, tr, c);*/
  {
    GleichungsT gleichung;
    UTermT s1 = s;
    UTermT s0 = TO_NULLTerm(s);
    VERB(("expandOneSide2(%s): %t =?= %t %E %C\n", (s == tl) ? "left" : "right",
	      tl, tr, e, c));
    do {
      if (TO_TermIstNichtVar(s1)){
#if TRY_ALL_EQNS
	unsigned int try = 0;
#endif 
	MO_AllGleichungMatchInit(s1);
	while (!expanded && 
	       MO_AllGleichungMatchNext(FALSE,&gleichung)){
	  TermT sneu = MO_AllTermErsetztNeu(s,s1); /* neue Kopie */
	  UTermT sneu1 = anGleicherStelle(sneu,s,s1);
	  /* sigma(u) == s1, sigma(v) == sneu1 */
          EConsT e1,e2;
#if TRY_ALL_EQNS
	  try++;
#endif 
          /*IO_DruckeFlex("@@%t und %t\n", sneu, sneu1);*/

          e1 = CO_newEcons(s1,sneu1,e,Gt_C);
          e2 = CO_newEcons(sneu1,s1,e,Ge_C);
          VERB((" investigate %E and %E\n", e2, e1));
	  if (CO_satisfiableUnder(e1,c)){
	    expanded = TRUE;
	    VERB((" split with %E and %E\n", e2, e1));
	    /*IO_DruckeFlex(" split with %D and %D\n", d2, d1);*/
            if (CO_satisfiableUnder(e2,c)){
              res = handleBranch2(tl,tr,e2,c);
            }
            else {
              res = TRUE;
            }
            if (res){
              TermT sneu2 = TO_Termkopie(sneu);
              NF_NormalformR(sneu2);
              if (s == tl){
		res = handleNode2(sneu2,tr,e1,c);
	      }
	      else {
		res = handleNode2(tl,sneu2,e1,c);
	      }
              TO_TermLoeschen(sneu2);
            }
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
	  }
	  CO_deleteEcons(e1);
          CO_deleteEcons(e2);
	  TO_TermLoeschen(sneu);
	}
      }
      s1 = TO_Schwanz(s1);
    } while (!expanded && (s1 != s0));
  }
  CLOSETRACE (("expandOneSide2: -> %b\n", res));
  *resexp = expanded;
  return res;
}

static BOOLEAN expandConstraint(TermT s, TermT t, EConsT e, ContextT c)
{
  BOOLEAN res = TRUE;
  OPENTRACE (("expandConstraint: %t =?= %t %E\n", s, t, e));
  {
    DConsT d = CO_getAllSolutions(e,c);
    DConsT drun = d;
    VERB (("expandConstraint: %t =?= %t %D\n", s, t, d));
    while (res && (drun != NULL)){
      TermT s1 = CO_substitute(drun->c->bindings,s);
      TermT t1 = CO_substitute(drun->c->bindings,t);
      ContextT c1 = CO_solveEQPart(drun->c);
      ContextT cold = CO_getCurrentContext();
      CO_setCurrentContext(c1);
      /*IO_DruckeFlex("@@%t und %t\n", s1, t1);*/
      NF_NormalformRE(s1);
      NF_NormalformRE(t1);
      /*IO_DruckeFlex("@@@%t und %t\n", s1, t1);*/
      res = handleNode2(s1,t1,NULL,c1);
      CO_setCurrentContext(cold);
      CO_deepDelete(c1);
      TO_TermeLoeschen(s1,t1);   
      drun = drun->Nachf;
    }
    CO_deleteDConsList(d);
  }
  CLOSETRACE (("expandConstraint: %b\n", res));
  return res;
}
static BOOLEAN handleBranch2 (TermT s, TermT t, EConsT e, ContextT c)
/* s und t konnten nicht unter e zusammengefuehrt werden, e ist erfuellbar.
 * e ist nicht aufgeloest.
 * Also suchen wir u=v \in E und machen einen Rewrite-Schritt unter einem
 * erweiterten Constraint (sowie das Gegenstueck).
 * klappt das nicht, droeseln wir den Constraint auf, und untersuchen jeden
 * einzelnen Knoten.
 */
{
  BOOLEAN res = FALSE;
  OPENTRACE (("handleBranch2: %t =?= %t %E %C\n", s, t, e, c));
  /*  IO_DruckeFlex("handleBranch: %t =?= %t %C\n", s, t, c);*/
  level2++;
  if ((depth_limit > 0) && (level2 > depth_limit)){
    level2=0;
    return FALSE;
  }
  {
    BOOLEAN expanded = FALSE;
    VERB(("## %t =?= %t %E %C\n", s, t, e, c));
    res = expandOneSide2(s,s,t,e,c,&expanded);
    if (!expanded){
      res = expandOneSide2(t,s,t,e,c,&expanded);
    }
    if (!expanded && (e != NULL)){
      res = expandConstraint(s,t,e,c);
    }
    if (!res && do_witnesses && (witness_l == NULL)){
      witness_l = TO_Termkopie(s);
      witness_r = TO_Termkopie(t);
      /*      IO_DruckeFlex("WITNESSES: %t und %t\n", witness_l, witness_r);*/
    }
  }
  level2--;
  CLOSETRACE (("handleBranch2: -> %b\n", res));
  return res;
  /* INVARIANTE:     RES = FALSE -> BACKTRACKING ABBRECHEN
   *                 RES = TRUE  -> BACKTRACKING WEITERMACHEN
   * GESAMTERGEBNIS: SEITENEFFEKT !!!
   */
}

/* -------------------------------------------------------------------------- */

BOOLEAN CO_GroundJoinable2(TermT s, TermT t)
{
  BOOLEAN res = FALSE;
  {
    ContextT c;
    level2 = 0;
    c = CO_newContext(CO_allocBindings());
    if (VERBOSE) {
      IO_DruckeFlex("| %t =?= %t %C\n", s, t, c);
    }
    res = handleBranch2(s,t,NULL,c);
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
