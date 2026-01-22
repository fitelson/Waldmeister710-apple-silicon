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

static unsigned int ct7_level = 0;
static int counter           = 0;
static unsigned long node_counter;

/* -------------------------------------------------------------------------- */
/* - hints ------------------------------------------------------------------ */
/* -------------------------------------------------------------------------- */

#define lambdaPos 1
typedef unsigned long HintT;

typedef enum {
  CrewHint   = 0x00, /* nothing means: only CRew sensible */
  JSHint     = 0x01, /* try joinability / subsumability test */
  InstHint   = 0x02, /* try instantiation */
  RewrHint   = 0x04, /* try unconditional rewriting */
  PathHint   = 0x08, /* 1 == backtrack path to pos, 0 == start from pos */
  CRPathHint = 0x10  /* 1 path from Crew step, 0 path from Rewr step */
} HintsT;

static HintT ct7_mkHint(unsigned long h, unsigned long pos, unsigned no)
{
  unsigned long pos2 = pos & 0xFFFF;
  unsigned no2 = no & 0xFF;
  if (pos2 != pos) pos2 = lambdaPos;
  if (no != no2) no2 = 0;
  return  h | (pos2 << 8) | (no2 << 24);
}

static BOOLEAN ct7_doJS(HintT h)
{
  return h & JSHint;
}

static BOOLEAN ct7_doInst(HintT h)
{
  return h & InstHint;
}

static BOOLEAN ct7_doRewr(HintT h)
{
  return h & RewrHint;
}

static BOOLEAN ct7_doPath(HintT h)
{
  return h & PathHint;
}

static BOOLEAN ct7_doCRPath(HintT h)
{
  return h & CRPathHint;
}

static unsigned long ct7_getPos(HintT h)
{
  return (h >> 8) & 0xFFFF;
}

static unsigned long ct7_getNo(HintT h)
{
  return (h >> 24) & 0xFF;
}


/* -------------------------------------------------------------------------- */
/* - forward declarations --------------------------------------------------- */
/* -------------------------------------------------------------------------- */

static BOOLEAN ct7_handleNode (LiteralT l, ContextT c, EConsT e, HintT h);

/* -------------------------------------------------------------------------- */
/* - generische Hilfsroutingen ---------------------------------------------- */
/* -------------------------------------------------------------------------- */

static void ct7_print_DConsKette(DConsT d)
{
  if (TEX_OUTPUT){
    DConsT drun = d;
    int i = 1;
    IO_DruckeFlex("%% ####################START\n%% %d: %C\n", i, drun->c);
    drun = drun->Nachf;
    while (drun != NULL){
      i++;
      IO_DruckeFlex("%% %d: %C\n", i, drun->c);
    drun = drun->Nachf;
    }
    IO_DruckeFlex("%% ####################STOP\n");
  }
}

static BOOLEAN ct7_equalUptoPos(LiteralT l1, UTermT p, LiteralT l2)
{
  UTermT ll1 = TO_Schwanz(l1);
  UTermT ll2 = TO_Schwanz(l2);
  while (ll1 != p){
    if (!SO_SymbGleich(TO_TopSymbol(ll1),TO_TopSymbol(ll2))){
      return FALSE;
    }
    ll1 = TO_Schwanz(ll1);
    ll2 = TO_Schwanz(ll2);
  }
  return SO_SymbGleich(TO_TopSymbol(ll1),TO_TopSymbol(ll2));
}

static void ct7_Durchnumerieren(UTermT t)
{
  UTermT t0 = TO_NULLTerm(t);
  UTermT t1 = t;
  unsigned int i = 0;
  while (t1 != t0){
    TO_SetId(t1,i);
    i++;
    t1 = TO_Schwanz(t1);
  }
}

/* -------------------------------------------------------------------------- */
/* - Behandeln von Confluence-Trees ----------------------------------------- */
/* -------------------------------------------------------------------------- */

static BOOLEAN ct7_Rewrite (LiteralT l, ContextT c, HintT h, BOOLEAN *resp)
{
  BOOLEAN expanded = FALSE;
  OPENTRACE (("ct7_RewriteOneSide: %t %C\n", l, c));
  {
    unsigned int ppos   = ct7_getPos(h);
    unsigned int endNum = 0;
    unsigned int pos    = ct7_doPath(h) ? 1 : ppos;
    UTermT l1           = TO_TermAnStelle(l,pos);
    UTermT l0           = TO_NULLTerm(l);
    ContextT cold       = CO_getCurrentContext();
    CO_setCurrentContext(c); /* Ordnung verstaerken fuer E-Schritte */
    if (ct7_doPath(h) && (ppos != lambdaPos)){
      endNum = TO_GetId(TO_TermEnde(TO_TermAnStelle(l,ppos)));
    }
    do {
      if ((endNum != 0) /*&& (pos < ppos)*/ && (TO_GetId(TO_TermEnde(l1)) < endNum)){ 
        if(0)IO_DruckeFlex("Skip position %P (%t) because %d < %d last rewrite at %t (%d) = %P\n",
                      l,l1,l1,TO_GetId(TO_TermEnde(l1)), endNum,
                      TO_TermAnStelle(l,ct7_getPos(h)), TO_GetId(TO_TermAnStelle(l,ct7_getPos(h))),
                      l, TO_TermAnStelle(l,ct7_getPos(h)));
        if (0){
          l1 = TO_Schwanz(l1);
          pos++;
        }
        else {
          l1 = TO_NaechsterTeilterm(l1);
          pos = TO_GetId(l1);
        }
      }
      else {
        if (TO_TermIstNichtVar(l1) &&
            (MO_RegelGefunden(l1) || MO_GleichungGefunden(l1))) {
          RegelOderGleichungsT re = MO_AngewendetesObjekt;
          LiteralT lneu = MO_AllTermErsetztNeu(l,l1); /* neue Kopie */
          expanded = TRUE;
          ct7_Durchnumerieren(lneu);
          if (TEX_OUTPUT) {
            IO_DruckeFlex("%*\\Rewrite{%s}{%d}{%P}{%%\n", 2*ct7_level,
                          TP_TermpaarIstRegel(re) ? "R" : "E",
                          RE_AbsNr(re), l, l1);
          }
          ct7_level++;
          *resp = ct7_handleNode(lneu,c,NULL,
                                ct7_mkHint(JSHint|RewrHint|PathHint,pos,0));
          TO_TermLoeschen(lneu);
        }
        l1 = TO_Schwanz(l1);
        pos++;
      }
      if (pos == ppos) endNum = 0;
    } while (!expanded && (l1 != l0));
    CO_setCurrentContext(cold); /* Ordnung zuruecksetzen */
  }
  CLOSETRACE (("ct7_RewriteOneSide: %s exapanded -> %b\n", 
               expanded ? "" : "not", *resp));
  return expanded;
}

static BOOLEAN ct7_CRew (LiteralT l, ContextT c, HintT h, BOOLEAN *resp)
{
  BOOLEAN expanded = FALSE;
  OPENTRACE (("ct7_CRewOneSide: %t  %C\n", l, c));
  {
    GleichungsT gleichung;
    unsigned int ppos   = ct7_getPos(h);
    unsigned int endNum = 0;
    unsigned int pos    = ct7_doPath(h) ? 1 : ppos;
    unsigned int no     = ct7_doPath(h) ? 0 : ct7_getNo(h);
    unsigned int no2;
    UTermT l1           = TO_TermAnStelle(l,pos);
    UTermT l0           = TO_NULLTerm(l);
    if (ct7_doCRPath(h) && ct7_doPath(h) && (ppos != lambdaPos)){
      endNum = TO_GetId(TO_TermEnde(TO_TermAnStelle(l,ppos)));
    }
    do {
      if ((endNum != 0) /*&& (pos < ppos)*/ && (TO_GetId(TO_TermEnde(l1)) < endNum)){ 
        if(0)IO_DruckeFlex("Skip position %P (%t) because %d < %d last rewrite at %t (%d) = %P\n",
                      l,l1,l1,TO_GetId(TO_TermEnde(l1)), endNum,
                      TO_TermAnStelle(l,ct7_getPos(h)), TO_GetId(TO_TermAnStelle(l,ct7_getPos(h))),
                      l, TO_TermAnStelle(l,ct7_getPos(h)));
        if (0){
          l1 = TO_Schwanz(l1);
          pos++;
        }
        else {
          l1 = TO_NaechsterTeilterm(l1);
          pos = TO_GetId(l1);
        }
      }
      else {
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
                  IO_DruckeFlex("%*\\CRewrite{%d%s}{%P}{%%\n", 2*ct7_level,
                                RE_AbsNr(gleichung), 
                                TP_RichtungAusgezeichnet(gleichung) ? "" :"'",
                                l, l1);
                }
                ct7_Durchnumerieren(lneu);
                ct7_level++;
                *resp = PREFER_D1
                  ? ct7_handleNode(lneu,c,e1,ct7_mkHint(JSHint|PathHint|CRPathHint,pos,0)) 
                    && ct7_handleNode(l,c,e2,ct7_mkHint(CrewHint,pos,no2))
                  : ct7_handleNode(l,c,e2,ct7_mkHint(CrewHint,pos,no2)) 
                    && ct7_handleNode(lneu,c,e1,ct7_mkHint(JSHint|PathHint|CRPathHint,pos,0));
                CO_deleteEcons(e2);
              }
              else {
                if (TEX_OUTPUT_F){
                  IO_DruckeFlex("%*\\FCRewrite{%d%s}{%P}{%E \\wedge %C}%%\n", 
                                2*ct7_level, RE_AbsNr(gleichung), 
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
      }
      if (pos == ppos) endNum = 0;
    } while (!expanded && (l1 != l0));
  }
  CLOSETRACE (("ct7_CRewOneSide: %s expanded -> %b\n", 
               expanded ? "" : "not", *resp));
  return expanded;
}

static BOOLEAN ct7_handleNode (LiteralT l, ContextT c, EConsT e, HintT h)
     /* falls e == NULL, dann ist c satisfiable! */
{
  BOOLEAN res = FALSE;
  unsigned int oldlevel = ct7_level;
  OPENTRACE (("ct7_handleNode: %t %C %E \n", l, c, e));
  node_counter++;
  if (TEX_OUTPUT) {
    IO_DruckeFlex("%*\\Node{%t}{%t}{%E%C}{%%\n", 
                  2*ct7_level, TO_LitLHS(l), TO_LitRHS(l), e, c);
  }
  ct7_level++;
  if ((ct7_level > DEPTH_LIMIT) && LIMIT_DEPTH){
    our_error("nesting to deep");
  }
  {
    if (DO_NODE_COUNTS && (node_counter > MAX_NODE_COUNT)){
      if (TEX_OUTPUT) IO_DruckeFlex("%*\\ToManyNodes%%\n", 2*ct7_level);
      RESTRACE(("ct7_handleNode: Zahl der Knoten ueberschritten\n"));
      res = FALSE;
    }
    else if (ct7_doJS(h) && TO_TermeGleich(TO_LitLHS(l),TO_LitRHS(l))){
      if (TEX_OUTPUT) IO_DruckeFlex("%*\\Equal%%\n", 2*ct7_level);
      RESTRACE(("ct7_handleNode: Terme sind direkt gleich\n"));
      res = TRUE;
    }
    else if (ct7_doJS(h) && 
             SS_TermpaarSubsummiertVonGM (TO_LitLHS(l),TO_LitRHS(l))){
      if (TEX_OUTPUT) {
        IO_DruckeFlex("%*\\Subsumed{%d}%%\n", 
                      2*ct7_level, RE_AbsNr(MO_AngewendetesObjekt));
      }
      RESTRACE(("ct7_handleNode: Terme werden subsumiert\n"));
      res = TRUE;
    }
    else if ((e != NULL)){ /* Erfuellbarkeit unklar, also erstmal splitten */
      DConsT d = CO_getAllSolutions(e,c);
      if (d == NULL){ /* OK, vorhin wurde andere Seite bei CRewrite */
        res = TRUE;   /* als erfuellbar nachgewiesen. */
        if (TEX_OUTPUT) IO_DruckeFlex("%*\\Unsat%%\n", 2*ct7_level);
      }
      else {
        DConsT drun;
        HintT h1 = ct7_mkHint(InstHint|RewrHint|ct7_doPath(h)|ct7_doCRPath(h),
                             ct7_getPos(h),ct7_getNo(h));
        if (1) ct7_print_DConsKette(d);
        if (TEX_OUTPUT) IO_DruckeFlex("%*\\Split{%%\n", 2*ct7_level);
        ct7_level++;
        drun = d;
        do {
          res = ct7_handleNode(l,drun->c,NULL,h1);
          drun = drun->Nachf;
        } while (res && (drun != NULL));
      }
      CO_deleteDConsList(d);
    }
    else if (ct7_doInst(h) && CO_somethingBound(c->bindings)){
      HintT h1;
      unsigned int pos = ct7_getPos(h);
      LiteralT l1 = CO_substitute(c->bindings,l);
      ContextT c1 = CO_solveEQPart(c);
      ct7_Durchnumerieren(l1);
      RESTRACE(("ct7_handleNode: Literal nach subst: %t = %t\n",l1));
      if (TEX_OUTPUT) IO_DruckeFlex("%*\\Inst{%%\n", 2*ct7_level);
      ct7_level++;
      if (ct7_equalUptoPos(l,TO_TermAnStelle(l,pos),l1)){
        if(0)IO_DruckeFlex("EQUAL UP TO POS: %t %t (%P == %d == %t)\n",
                      l, l1, l, TO_TermAnStelle(l,ct7_getPos(h)), 
                      ct7_getPos(h), TO_TermAnStelle(l,ct7_getPos(h)));
        h1 = ct7_mkHint(JSHint|RewrHint|PathHint,
                        ct7_getPos(h),ct7_getNo(h));
      }
      else {
        h1 = ct7_mkHint(JSHint|RewrHint,lambdaPos,0);
      }
      res = ct7_handleNode(l1,c1,NULL,h1);
      CO_deepDelete(c1);
      TO_TermLoeschen(l1);
    }
    else if (ct7_doRewr(h) && ct7_Rewrite(l,c,h,&res)){
    }
    else if (ct7_CRew(l,c,h,&res)){
    }
    else {
      res = FALSE; /* Machen wir's nochmals explizit */
      if (TEX_OUTPUT) IO_DruckeFlex("%*\\GiveUp%%\n", 2*ct7_level);
    }
  }
  while (ct7_level > oldlevel){
    ct7_level--;
    if (TEX_OUTPUT) IO_DruckeFlex("%*}%%\n", 2*ct7_level);
  }
  CLOSETRACE (("ct7_handleNode: -> %b\n", res));
  return res;
}

/* -------------------------------------------------------------------------- */

BOOLEAN CO_GroundJoinable7(TermT s, TermT t)
{
  BOOLEAN res = FALSE;
  {
    TermT ss, tt;
    LiteralT l;
    ContextT c;
    ss = TO_Termkopie(s);
    tt = TO_Termkopie(t);
    TO_mkLit(l,ss,tt);
    ct7_Durchnumerieren(l);
    c = CO_newContext(CO_allocBindings());
    counter++;
    node_counter = 0;
    if(TEX_OUTPUT) IO_DruckeFlex("\\ctree{%d}{%%\n", counter);
    ct7_level++;
    res = ct7_handleNode(l,c,NULL,ct7_mkHint(CrewHint,lambdaPos,0));
    if(0)IO_DruckeFlex("#%l %b\n", node_counter, res);
    if(TEX_OUTPUT) IO_DruckeFlex("}{%s}%%\n", res ? "\\Success" : "\\Fail");
    ct7_level--;
    CO_deepDelete(c);
    TO_TermLoeschen(l);
  }
  return res;
}

/* ------------------------------------------------------------------------- */
/* -------- E O F ---------------------------------------------------------- */
/* ------------------------------------------------------------------------- */
