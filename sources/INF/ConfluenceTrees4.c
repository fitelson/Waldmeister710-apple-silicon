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

static unsigned int ct4_level = 0;
static int counter           = 0;

/* -------------------------------------------------------------------------- */
/* - forward declarations --------------------------------------------------- */
/* -------------------------------------------------------------------------- */

static BOOLEAN ct4_handleNode (LiteralT l, ContextT c, EConsT e);

/* -------------------------------------------------------------------------- */
/* - generische Hilfsroutingen ---------------------------------------------- */
/* -------------------------------------------------------------------------- */

static void ct4_print_DConsKette(DConsT d)
{
  DConsT drun = d;
  int i = 1;
  IO_DruckeFlex("####################START\n%d: %C\n", i, drun->c);
  drun = drun->Nachf;
  while (drun != NULL){
    i++;
    IO_DruckeFlex("%d: %C\n", i, drun->c);
    drun = drun->Nachf;
  }
  IO_DruckeFlex("####################STOP\n");
}

/* -------------------------------------------------------------------------- */
/* - Behandeln von Confluence-Trees ----------------------------------------- */
/* -------------------------------------------------------------------------- */

static BOOLEAN ct4_Rewrite (LiteralT l, ContextT c, BOOLEAN *resp)
{
  BOOLEAN expanded = FALSE;
  OPENTRACE (("ct4_RewriteOneSide: %t %C\n", l, c));
  {
    UTermT l1 = TO_Schwanz(l);
    UTermT l0 = TO_NULLTerm(l);
    ContextT cold = CO_getCurrentContext();
    CO_setCurrentContext(c); /* Ordnung verstaerken fuer E-Schritte */
    do {
      if (TO_TermIstNichtVar(l1) &&
          (MO_RegelGefunden(l1) || MO_GleichungGefunden(l1))) {
        RegelOderGleichungsT re = MO_AngewendetesObjekt;
        LiteralT lneu = MO_AllTermErsetztNeu(l,l1); /* neue Kopie */
        expanded = TRUE;
        if (TEX_OUTPUT) {
          IO_DruckeFlex("%*\\Rewrite{%s}{%d}{%P}{%%\n", 2*ct4_level,
                        TP_TermpaarIstRegel(re) ? "R" : "E",
                        RE_AbsNr(re), l, l1);
        }
        ct4_level++;
        *resp = ct4_handleNode(lneu,c,NULL);
        TO_TermLoeschen(lneu);
      }
      l1 = TO_Schwanz(l1);
    } while (!expanded && (l1 != l0));
    CO_setCurrentContext(cold); /* Ordnung zuruecksetzen */
  }
  CLOSETRACE (("ct4_RewriteOneSide: %s exapanded -> %b\n", 
               expanded ? "" : "not", *resp));
  return expanded;
}

static BOOLEAN ct4_CRew (LiteralT l, ContextT c, BOOLEAN *resp)
{
  BOOLEAN expanded = FALSE;
  OPENTRACE (("ct4_CRewOneSide: %t  %C\n", l, c));
  {
    GleichungsT gleichung;
    UTermT l1 = TO_Schwanz(l);
    UTermT l0 = TO_NULLTerm(l);
    unsigned int pos = 1;
    do {
      if (TO_TermIstNichtVar(l1)){
	MO_AllGleichungMatchInit(l1);
	while (!expanded && 
	       MO_AllGleichungMatchNext(FALSE, &gleichung)){
	  TermT lneu  = MO_AllTermErsetztNeu(l,l1); /* neue Kopie */
	  UTermT lneu1 = TO_TermAnStelle(lneu,pos);
	  /* sigma(u) == l1, sigma(v) == lneu1 */
          EConsT e1 = CO_newEcons (l1,lneu1,NULL,Gt_C);
          if (CO_satisfiableUnder(e1,c)){
            /* Optimierungspotential: */
            /* CO_getAllSolutionsInBothDirections(l1,lneu1,c,&d1,&d2) */
            EConsT e2 = CO_newEcons (lneu1,l1,NULL,Ge_C);
	    expanded = TRUE;
            if (TEX_OUTPUT){
              IO_DruckeFlex("%*\\CRewrite{%d%s}{%P}{%%\n", 2*ct4_level,
                            RE_AbsNr(gleichung), 
                            TP_RichtungAusgezeichnet(gleichung) ? "" :"'",
                            l, l1);
            }
            ct4_level++;
            *resp = PREFER_D1
                ? ct4_handleNode(lneu,c,e1) 
                  && ct4_handleNode(l,c,e2)
                : ct4_handleNode(l,c,e2) 
                  && ct4_handleNode(lneu,c,e1);
            CO_deleteEcons(e2);
          }
          else {
            if (TEX_OUTPUT_F){
              IO_DruckeFlex("%*\\FCRewrite{%d%s}{%P}{%E \\wedge %C}%%\n", 
                            2*ct4_level, RE_AbsNr(gleichung), 
                            TP_RichtungAusgezeichnet(gleichung) ? "" :"'",
                            l, l1, e1, c);
            }
          }
          CO_deleteEcons(e1);
	  TO_TermLoeschen(lneu);
	}
      }
      l1 = TO_Schwanz(l1);
      pos++;
    } while (!expanded && (l1 != l0));
  }
  CLOSETRACE (("ct4_CRewOneSide: %s expanded -> %b\n", 
               expanded ? "" : "not", *resp));
  return expanded;
}

static BOOLEAN ct4_handleNode (LiteralT l, ContextT c, EConsT e)
     /* falls e == NULL, dann ist c satisfiable! */
{
  BOOLEAN res = FALSE;
  unsigned int oldlevel = ct4_level;
  OPENTRACE (("ct4_handleNode: %t %C %E \n", l, c, e));
  if (TEX_OUTPUT) {
    IO_DruckeFlex("%*\\Node{%t}{%t}{%E%C}{%%\n", 
                  2*ct4_level, TO_LitLHS(l), TO_LitRHS(l), e, c);
  }
  ct4_level++;
  if ((ct4_level > DEPTH_LIMIT) && LIMIT_DEPTH){
    our_error("nesting to deep");
  }
  {
    if (TO_TermeGleich(TO_LitLHS(l),TO_LitRHS(l))){
      if (TEX_OUTPUT) IO_DruckeFlex("%*\\Equal%%\n", 2*ct4_level);
      RESTRACE(("ct4_handleNode: Terme sind direkt gleich\n"));
      res = TRUE;
    }
    else if (0&&SS_TermpaarSubsummiertVonGM (TO_LitLHS(l),TO_LitRHS(l))){
      if (TEX_OUTPUT) {
        IO_DruckeFlex("%*\\Subsumed{%d}%%\n", 
                      2*ct4_level, RE_AbsNr(MO_AngewendetesObjekt));
      }
      RESTRACE(("ct4_handleNode: Terme werden subsumiert\n"));
      res = TRUE;
    }
    else if ((e != NULL)){ /* Erfuellbarkeit unklar, also erstmal splitten */
      DConsT d = CO_getAllSolutions(e,c);
      if (d == NULL){ /* OK, vorhin wurde andere Seite bei CRewrite */
        res = TRUE;   /* als erfuellbar nachgewiesen. */
        if (TEX_OUTPUT) IO_DruckeFlex("%*\\Unsat%%\n", 2*ct4_level);
      }
      else if (1){
        DConsT drun;
        if (0) ct4_print_DConsKette(d);
        if (TEX_OUTPUT) IO_DruckeFlex("%*\\Split{%%\n", 2*ct4_level);
        ct4_level++;
        drun = d;
        do {
          res = ct4_handleNode(l,drun->c,NULL);
          drun = drun->Nachf;
        } while (res && (drun != NULL));
      }
      CO_deleteDConsList(d);
    }
    else if (0 && CO_somethingBound(c->bindings)){
      LiteralT l1 = CO_substitute(c->bindings,l);
      ContextT c1 = CO_solveEQPart(c);
      RESTRACE(("ct4_handleNode: Literal nach subst: %t = %t\n",l1));
      if (TEX_OUTPUT) IO_DruckeFlex("%*\\Inst{%%\n", 2*ct4_level);
      ct4_level++;
      res = ct4_handleNode(l1,c1,NULL);
      CO_deepDelete(c1);
      TO_TermLoeschen(l1);
    }
    else if (0&& ct4_Rewrite(l,c,&res)){
    }
    else if (ct4_CRew(l,c,&res)){
    }
    else {
      res = FALSE; /* Machen wir's nochmals explizit */
      if (TEX_OUTPUT) IO_DruckeFlex("%*\\GiveUp%%\n", 2*ct4_level);
    }
  }
  while (ct4_level > oldlevel){
    ct4_level--;
    if (TEX_OUTPUT) IO_DruckeFlex("%*}%%\n", 2*ct4_level);
  }
  CLOSETRACE (("ct4_handleNode: -> %b\n", res));
  return res;
}

/* -------------------------------------------------------------------------- */

BOOLEAN CO_GroundJoinable4(TermT s, TermT t)
{
  BOOLEAN res = FALSE;
  {
    TermT ss, tt;
    LiteralT l;
    ContextT c;
    ss = TO_Termkopie(s);
    tt = TO_Termkopie(t);
    TO_mkLit(l,ss,tt);
    c = CO_newContext(CO_allocBindings());
    counter++;
    if(TEX_OUTPUT) IO_DruckeFlex("\\ctree{%d}{%%\n", counter);
    ct4_level++;
    res = ct4_handleNode(l,c,NULL);
    if(TEX_OUTPUT) IO_DruckeFlex("}{%s}%%\n", res ? "\\Success" : "\\Fail");
    ct4_level--;
    CO_deepDelete(c);
    TO_TermLoeschen(l);
  }
  return res;
}

/* ------------------------------------------------------------------------- */
/* -------- E O F ---------------------------------------------------------- */
/* ------------------------------------------------------------------------- */
