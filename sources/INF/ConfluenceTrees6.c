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

static unsigned int ct6_level = 0;
static int counter           = 0;

/* -------------------------------------------------------------------------- */
/* - hints ------------------------------------------------------------------ */
/* -------------------------------------------------------------------------- */

#define lambdaPos 1
typedef unsigned long HintT;

typedef enum {
  CrewHint = 0x0, /* nothing means: only CRew sensible */
  JSHint   = 0x1, /* try joinability / subsumability test */
  InstHint = 0x2, /* try instantiation */
  RewrHint = 0x4, /* try unconditional rewriting */
  PathHint = 0x8  /* 1 == backtrack path to pos, 0 == start from pos */
} HintsT;

static HintT ct6_mkHint(unsigned long h, unsigned long pos, unsigned no)
{
  unsigned long pos2 = pos & 0xFFFF;
  unsigned no2 = no & 0xFF;
  if (pos2 != pos) pos2 = lambdaPos;
  if (no != no2) no2 = 0;
  return  h | (pos2 << 8) | (no2 << 24);
}

static BOOLEAN ct6_doJS(HintT h)
{
  return h & JSHint;
}

static BOOLEAN ct6_doInst(HintT h)
{
  return h & InstHint;
}

static BOOLEAN ct6_doRewr(HintT h)
{
  return h & RewrHint;
}

static BOOLEAN ct6_doPath(HintT h)
{
  return h & PathHint;
}

static unsigned long ct6_getPos(HintT h)
{
  return (h >> 8) & 0xFFFF;
}

static unsigned long ct6_getNo(HintT h)
{
  return (h >> 24) & 0xFF;
}


/* -------------------------------------------------------------------------- */
/* - forward declarations --------------------------------------------------- */
/* -------------------------------------------------------------------------- */

static BOOLEAN ct6_handleNode (LiteralT l, ContextT c, EConsT e, HintT h);

/* -------------------------------------------------------------------------- */
/* - generische Hilfsroutingen ---------------------------------------------- */
/* -------------------------------------------------------------------------- */

static void ct6_print_DConsKette(DConsT d)
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

static BOOLEAN ct6_Rewrite (LiteralT l, ContextT c, HintT h, BOOLEAN *resp)
{
  BOOLEAN expanded = FALSE;
  OPENTRACE (("ct6_RewriteOneSide: %t %C\n", l, c));
  {
    unsigned int pos = (ct6_doPath(h)) ? 1 : ct6_getPos(h);
    UTermT l1 = TO_TermAnStelle(l,pos);
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
          IO_DruckeFlex("%*\\Rewrite{%s}{%d}{%P}{%%\n", 2*ct6_level,
                        TP_TermpaarIstRegel(re) ? "R" : "E",
                        RE_AbsNr(re), l, l1);
        }
        ct6_level++;
        *resp = ct6_handleNode(lneu,c,NULL,
                              ct6_mkHint(JSHint|RewrHint|PathHint,pos,0));
        TO_TermLoeschen(lneu);
      }
      l1 = TO_Schwanz(l1);
      pos++;
    } while (!expanded && (l1 != l0));
    CO_setCurrentContext(cold); /* Ordnung zuruecksetzen */
  }
  CLOSETRACE (("ct6_RewriteOneSide: %s exapanded -> %b\n", 
               expanded ? "" : "not", *resp));
  return expanded;
}

static BOOLEAN ct6_CRew (LiteralT l, ContextT c, HintT h, BOOLEAN *resp)
{
  BOOLEAN expanded = FALSE;
  OPENTRACE (("ct6_CRewOneSide: %t  %C\n", l, c));
  {
    GleichungsT gleichung;
    unsigned int pos = (ct6_doPath(h)) ? 1 : ct6_getPos(h);
    unsigned int no = (ct6_doPath(h)) ? 0 : ct6_getNo(h);
    unsigned int no2;
    UTermT l1 = TO_TermAnStelle(l,pos);
    UTermT l0 = TO_NULLTerm(l);
    do {
      if (TO_TermIstNichtVar(l1)){
	MO_AllGleichungMatchInit(l1);
        no2 = 0;
	while (!expanded && 
	       MO_AllGleichungMatchNext(FALSE, &gleichung)){
          no2++;
          if (no2 <= no){
          }
          else {
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
                IO_DruckeFlex("%*\\CRewrite{%d%s}{%P}{%%\n", 2*ct6_level,
                              RE_AbsNr(gleichung), 
                              TP_RichtungAusgezeichnet(gleichung) ? "" :"'",
                              l, l1);
              }
              ct6_level++;
              *resp = PREFER_D1
                ? ct6_handleNode(lneu,c,e1,ct6_mkHint(JSHint|PathHint,pos,0)) 
                  && ct6_handleNode(l,c,e2,ct6_mkHint(CrewHint,pos,no2))
                : ct6_handleNode(l,c,e2,ct6_mkHint(CrewHint,pos,no2)) 
                  && ct6_handleNode(lneu,c,e1,ct6_mkHint(JSHint|PathHint,pos,0));
              CO_deleteEcons(e2);
            }
            else {
              if (TEX_OUTPUT_F){
                IO_DruckeFlex("%*\\FCRewrite{%d%s}{%P}{%E \\wedge %C}%%\n", 
                              2*ct6_level, RE_AbsNr(gleichung), 
                              TP_RichtungAusgezeichnet(gleichung) ? "" :"'",
                              l, l1, e1, c);
              }
            }
            CO_deleteEcons(e1);
            TO_TermLoeschen(lneu);
          }
        }
      }
      no = 0;
      l1 = TO_Schwanz(l1);
      pos++;
    } while (!expanded && (l1 != l0));
  }
  CLOSETRACE (("ct6_CRewOneSide: %s expanded -> %b\n", 
               expanded ? "" : "not", *resp));
  return expanded;
}

static BOOLEAN ct6_handleNode (LiteralT l, ContextT c, EConsT e, HintT h)
     /* falls e == NULL, dann ist c satisfiable! */
{
  BOOLEAN res = FALSE;
  unsigned int oldlevel = ct6_level;
  OPENTRACE (("ct6_handleNode: %t %C %E \n", l, c, e));
  if (TEX_OUTPUT) {
    IO_DruckeFlex("%*\\Node{%t}{%t}{%E%C}{%%\n", 
                  2*ct6_level, TO_LitLHS(l), TO_LitRHS(l), e, c);
  }
  ct6_level++;
  if ((ct6_level > DEPTH_LIMIT) && LIMIT_DEPTH){
    our_error("nesting to deep");
  }
  {
    if (ct6_doJS(h) && TO_TermeGleich(TO_LitLHS(l),TO_LitRHS(l))){
      if (TEX_OUTPUT) IO_DruckeFlex("%*\\Equal%%\n", 2*ct6_level);
      RESTRACE(("ct6_handleNode: Terme sind direkt gleich\n"));
      res = TRUE;
    }
    else if (ct6_doJS(h) && 
             SS_TermpaarSubsummiertVonGM (TO_LitLHS(l),TO_LitRHS(l))){
      if (TEX_OUTPUT) {
        IO_DruckeFlex("%*\\Subsumed{%d}%%\n", 
                      2*ct6_level, RE_AbsNr(MO_AngewendetesObjekt));
      }
      RESTRACE(("ct6_handleNode: Terme werden subsumiert\n"));
      res = TRUE;
    }
    else if ((e != NULL)){ /* Erfuellbarkeit unklar, also erstmal splitten */
      DConsT d = CO_getAllSolutions(e,c);
      if (d == NULL){ /* OK, vorhin wurde andere Seite bei CRewrite */
        res = TRUE;   /* als erfuellbar nachgewiesen. */
        if (TEX_OUTPUT) IO_DruckeFlex("%*\\Unsat%%\n", 2*ct6_level);
      }
      else {
        DConsT drun;
        HintT h1 = ct6_mkHint(InstHint|RewrHint|ct6_doPath(h),
                             ct6_getPos(h),ct6_getNo(h));
        if (0) ct6_print_DConsKette(d);
        if (TEX_OUTPUT) IO_DruckeFlex("%*\\Split{%%\n", 2*ct6_level);
        ct6_level++;
        drun = d;
        do {
          res = ct6_handleNode(l,drun->c,NULL,h1);
          drun = drun->Nachf;
        } while (res && (drun != NULL));
      }
      CO_deleteDConsList(d);
    }
    else if (ct6_doInst(h) && CO_somethingBound(c->bindings)){
      LiteralT l1 = CO_substitute(c->bindings,l);
      ContextT c1 = CO_solveEQPart(c);
      RESTRACE(("ct6_handleNode: Literal nach subst: %t = %t\n",l1));
      if (TEX_OUTPUT) IO_DruckeFlex("%*\\Inst{%%\n", 2*ct6_level);
      ct6_level++;
      res = ct6_handleNode(l1,c1,NULL,ct6_mkHint(JSHint|RewrHint,lambdaPos,0));
      CO_deepDelete(c1);
      TO_TermLoeschen(l1);
    }
    else if (ct6_doRewr(h) && ct6_Rewrite(l,c,h,&res)){
    }
    else if (ct6_CRew(l,c,h,&res)){
    }
    else {
      res = FALSE; /* Machen wir's nochmals explizit */
      if (TEX_OUTPUT) IO_DruckeFlex("%*\\GiveUp%%\n", 2*ct6_level);
    }
  }
  while (ct6_level > oldlevel){
    ct6_level--;
    if (TEX_OUTPUT) IO_DruckeFlex("%*}%%\n", 2*ct6_level);
  }
  CLOSETRACE (("ct6_handleNode: -> %b\n", res));
  return res;
}

/* -------------------------------------------------------------------------- */

BOOLEAN CO_GroundJoinable6(TermT s, TermT t)
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
    ct6_level++;
    res = ct6_handleNode(l,c,NULL,ct6_mkHint(CrewHint,lambdaPos,0));
    if(TEX_OUTPUT) IO_DruckeFlex("}{%s}%%\n", res ? "\\Success" : "\\Fail");
    ct6_level--;
    CO_deepDelete(c);
    TO_TermLoeschen(l);
  }
  return res;
}

/* ------------------------------------------------------------------------- */
/* -------- E O F ---------------------------------------------------------- */
/* ------------------------------------------------------------------------- */
