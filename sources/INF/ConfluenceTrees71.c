#include "compiler-extra.h"
#include "ConfluenceTrees_intern.h"
#include "Ausgaben.h"
#include "Constraints.h"
#include "MatchOperationen.h" 
#include "NFBildung.h"
#include "RUndEVerwaltung.h"
#include "Subsumption.h"

#ifndef CCE_Source
#include "Parameter.h"
#else
#include "parse-util.h"
#endif

/* -------------------------------------------------------------------------- */
/* - file static variables -------------------------------------------------- */
/* -------------------------------------------------------------------------- */

static unsigned int  ct71_level = 0;
static unsigned int  ct71_tree_counter = 0;
static unsigned long ct71_node_counter = 0;

/* -------------------------------------------------------------------------- */
/* - hints ------------------------------------------------------------------ */
/* -------------------------------------------------------------------------- */

#define lambdaPos 1
typedef unsigned long HintT;

typedef enum {
  CrewHint   = 0x00, /* nothing means: only CRew sensible */
  LeafHint   = 0x01, /* try leaf tests */
  InstHint   = 0x02, /* try instantiation */
  SplitHint  = 0x04, /* try splitting */
  RewrHint   = 0x08, /* try unconditional rewriting */
  PathHint   = 0x10, /* 1 == backtrack path to pos, 0 == start from pos */
  CRPathHint = 0x20  /* 1 path from Crew step, 0 path from Rewr step */
} HintsT;

static HintT ct71_mkHint(unsigned long h, unsigned long pos, unsigned no)
{
  unsigned long pos2 = pos & 0xFFFF;
  unsigned no2 = no & 0xFF;
  if (pos2 != pos) pos2 = lambdaPos;
  if (no != no2) no2 = 0;
  return  h | (pos2 << 8) | (no2 << 24);
}

static BOOLEAN ct71_doLeaf(HintT h)
{
  //  return 1;
  return h & LeafHint;
}

static BOOLEAN ct71_doInst(HintT h)
{
  //  return 1;
  return h & InstHint;
}

static BOOLEAN ct71_doSplit(HintT h)
{
  //  return 1;
  return h & SplitHint;
}

static BOOLEAN ct71_doRewr(HintT h)
{
  //  return 1;
  return h & RewrHint;
}

static BOOLEAN ct71_doPath(HintT h)
{
  //  return 0;
  return h & PathHint;
}

static BOOLEAN ct71_doCRPath(HintT h)
{
  //  return 0;
  return h & CRPathHint;
}

static unsigned long ct71_getPos(HintT h)
{
  //  return 0;

  return (h >> 8) & 0xFFFF;
}

static unsigned long ct71_getNo(HintT h)
{
  //  return 0;

  return (h >> 24) & 0xFF;
}


/* ------------------------------------------------------------------------- */
/* - forward declarations --------------------------------------------------- */
/* -------------------------------------------------------------------------- */

static BOOLEAN ct71_handleNode (LiteralT l, ContextT c, EConsT e, HintT h);

/* -------------------------------------------------------------------------- */
/* - generische Hilfsroutingen ---------------------------------------------- */
/* -------------------------------------------------------------------------- */

static void ct71_print_DConsKetten(DConsT d,DConsT d1)
{
  if (0&&(1||TEX_OUTPUT)&&(d->Nachf != NULL)){
    IO_DruckeFlex("%% %d %d\n", CO_DConsLength(d), CO_DConsLength(d1));
    if (1||(CO_DConsLength(d) != CO_DConsLength(d1))){
      DConsT drun = d;
      int i = 1;
      IO_DruckeFlex("%% ####################START\n%% %d: %C\n", i, drun->c);
      drun = drun->Nachf;
      while (drun != NULL){
        i++;
        IO_DruckeFlex("%% %d: %C\n", i, drun->c);
        drun = drun->Nachf;
      }
      CO_sortDCons(d);
      drun = d;
      i = 1;
      IO_DruckeFlex("%% ####################Zwischen\n%% %d: %C\n", i, drun->c);
      drun = drun->Nachf;
      while (drun != NULL){
        i++;
        IO_DruckeFlex("%% %d: %C\n", i, drun->c);
        drun = drun->Nachf;
      }
      IO_DruckeFlex("%% ####################STOP\n");
    }
    else {
      DConsT drun = d;
      int i = 1;
      IO_DruckeFlex("%% ################FLOP\n%% %d: %C\n", i, drun->c);
      drun = drun->Nachf;
      while (drun != NULL){
        i++;
        IO_DruckeFlex("%% %d: %C\n", i, drun->c);
        drun = drun->Nachf;
      }
      drun = d1;
      i = 1;
      IO_DruckeFlex("%% #################FLUP\n%% %d: %C\n", i, drun->c);
      drun = drun->Nachf;
      while (drun != NULL){
        i++;
        IO_DruckeFlex("%% %d: %C\n", i, drun->c);
        drun = drun->Nachf;
      }
      IO_DruckeFlex("%% ################FLIP\n");
    }
  }
}

static BOOLEAN ct71_equalUptoPos(LiteralT l1, UTermT p, LiteralT l2)
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

static void ct71_Durchnumerieren(UTermT t)
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

/* Format der Unterroutinen:
 * static 
 * BOOLEAN -- True:   Teilbaum behandelt
 *            False:  weiterprobieren
 * ct71_f(
 *   LiteralT l, -- die zu untersuchende Gleichung
 *   ContextT c, -- der geloeste Teil des Constraints
 *   EConsT e,   -- der noch nicht geloeste Teil des Constraints (evtl. NULL)
 *   HintT h,    -- fuer Wissenstransfer
 * 
 *   BOOLEAN *closed  -- das Ergebnis: True gdw Teilbaum abgeschlossen, d.h
 *                       alle ueberdeckten Instanzen gzfb.
 * )
 */

static BOOLEAN ct71_LeafTests(LiteralT l, 
                             ContextT c MAYBE_UNUSEDPARAM, 
                             EConsT e MAYBE_UNUSEDPARAM,
                             HintT h MAYBE_UNUSEDPARAM, 
                             BOOLEAN *closed)
{
  BOOLEAN handled = FALSE;
  OPENTRACE (("ct71_LeafTests: %t %C %E\n", l, c, e));
  {
    TermT s = TO_LitLHS(l);
    TermT t = TO_LitRHS(l);
    if (TO_TermeGleich(s,t)){
      if (TEX_OUTPUT) IO_DruckeFlex("%*\\Equal%%\n", 2*ct71_level);
      handled = *closed = TRUE;
    }
    else if (PA_CTreeS() && SS_TermpaarSubsummiertVonGM (s,t)){
      if (TEX_OUTPUT) {
        IO_DruckeFlex("%*\\Subsumed{%d}%%\n", 
                      2*ct71_level, RE_AbsNr(MO_AngewendetesObjekt));
      }
      handled = *closed = TRUE;
    }
    else if (PA_CTreeP() && TO_ACGleich (s,t)){
      if (TEX_OUTPUT) {
        IO_DruckeFlex("%*\\PSubsumed{}%%\n", 2*ct71_level);
      }
      handled = *closed = TRUE;
    }
  }
  CLOSETRACE (("ct71_LeafTests: -> %b\n", handled));
  return handled;
}

static BOOLEAN ct71_Split(LiteralT l, ContextT c, EConsT e,
                         HintT h, BOOLEAN *closed)
{
  BOOLEAN handled = FALSE;
  OPENTRACE (("ct71_Split: %t %C %E\n", l, c, e));
  {
    if (e != NULL){
      DConsT d = CO_getAllSolutions(e,c);
      if (d == NULL){ /* OK, vorhin wurde andere Seite bei CRewrite */
        *closed = TRUE;   /* als erfuellbar nachgewiesen. */
        if (TEX_OUTPUT) IO_DruckeFlex("%*\\Unsat%%\n", 2*ct71_level);
      }
      else {
        HintT h1 = ct71_mkHint(InstHint|RewrHint,lambdaPos,0);
        DConsT drun;
        DConsT d1 = d;
        BOOLEAN allclosed = TRUE;
        if (PA_optDCons()){
          d1 = CO_minimizeDCons(d);
          ct71_print_DConsKetten(d,d1);
        }
        if (PA_sortDCons()){
          CO_sortDCons(d1);
        }
        if (TEX_OUTPUT) IO_DruckeFlex("%*\\Split{%%\n", 2*ct71_level);
        ct71_level++;
        drun = d1; /* Hier: drun = d fuer Vergleichbarkeit DConsOpt herstellen*/
        do {
          allclosed = ct71_handleNode(l,drun->c,NULL,h1);
          drun = drun->Nachf;
        } while (allclosed && (drun != NULL));
        if (PA_optDCons()){
          CO_deleteDConsList(d1);
        }
        *closed = allclosed;
      }
      CO_deleteDConsList(d);
      handled = TRUE;
    }
  }
  CLOSETRACE (("ct71_Split: -> %b\n", handled));
  return handled;
}

static BOOLEAN ct71_Inst(LiteralT l, ContextT c, EConsT e,
                        HintT h, BOOLEAN *closed)
{
  BOOLEAN handled = FALSE;
  OPENTRACE (("ct71_Inst: %t %C %E\n", l, c, e));
  {
    if (CO_somethingBound(c->bindings)){
      HintT h1;
      unsigned int pos = ct71_getPos(h);
      LiteralT l1 = CO_substitute(c->bindings,l);
      ContextT c1 = CO_solveEQPart(c);
      if (e != NULL){
        our_error("rethink for e != NULL!");
      }
      ct71_Durchnumerieren(l1);
      if (TEX_OUTPUT) IO_DruckeFlex("%*\\Inst{%%\n", 2*ct71_level);
      ct71_level++;
      h1 = ct71_mkHint(InstHint|RewrHint,lambdaPos,0);
      *closed = ct71_handleNode(l1,c1,NULL,h1);
      CO_deepDelete(c1);
      TO_TermLoeschen(l1);
      handled = TRUE;
    }
  }
  CLOSETRACE (("ct71_Inst: -> %b\n", handled));
  return handled;
}

static BOOLEAN ct71_Rewrite(LiteralT l, ContextT c, EConsT e,
                           HintT h, BOOLEAN *closed)
{
  BOOLEAN handled = FALSE;
  OPENTRACE (("ct71_Rewrite: %t %C %E\n", l, c, e));
  {
    unsigned int ppos   = ct71_getPos(h);
    unsigned int endNum = 0;
    unsigned int pos    = ct71_doPath(h) ? 1 : ppos;
    UTermT l1           = TO_TermAnStelle(l,pos);
    UTermT l0           = TO_NULLTerm(l);
    ContextT cold       = CO_getCurrentContext();
    CO_setCurrentContext(c); /* Ordnung verstaerken fuer E-Schritte */
    if (ct71_doPath(h) && (ppos != lambdaPos)){
      endNum = TO_GetId(TO_TermEnde(TO_TermAnStelle(l,ppos)));
    }
    do {
      if ((endNum != 0) /*&& (pos < ppos)*/ && (TO_GetId(TO_TermEnde(l1)) < endNum)){ 
        if(0)IO_DruckeFlex("Skip position %P (%t) because %d < %d last rewrite at %t (%d) = %P\n",
                      l,l1,l1,TO_GetId(TO_TermEnde(l1)), endNum,
                      TO_TermAnStelle(l,ct71_getPos(h)), TO_GetId(TO_TermAnStelle(l,ct71_getPos(h))),
                      l, TO_TermAnStelle(l,ct71_getPos(h)));
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
          HintT h1 = ct71_mkHint(LeafHint|RewrHint,lambdaPos,0);
          handled = TRUE;
          ct71_Durchnumerieren(lneu);
          if (TEX_OUTPUT) {
            IO_DruckeFlex("%*\\Rewrite{%s}{%d}{%P}{%%\n", 2*ct71_level,
                          TP_TermpaarIstRegel(re) ? "R" : "E",
                          RE_AbsNr(re), l, l1);
          }
          ct71_level++;
          *closed = ct71_handleNode(lneu,c,e,h1);
          TO_TermLoeschen(lneu);
        }
        l1 = TO_Schwanz(l1);
        pos++;
      }
      if (pos == ppos) endNum = 0;
    } while (!handled && (l1 != l0));
    CO_setCurrentContext(cold); /* Ordnung zuruecksetzen */
  }
  CLOSETRACE (("ct71_Rewrite: -> %b\n", handled));
  return handled;
}

static BOOLEAN ct71_CRew (LiteralT l, ContextT c, EConsT e,
                         HintT h, BOOLEAN *closed)
{
  BOOLEAN handled = FALSE;
  OPENTRACE (("ct71_CRew: %t  %C %E\n", l, c, e));
  {
    GleichungsT gleichung;
    unsigned int ppos   = ct71_getPos(h);
    unsigned int endNum = 0;
    unsigned int pos    = ct71_doPath(h) ? 1 : ppos;
    unsigned int no     = ct71_doPath(h) ? 0 : ct71_getNo(h);
    unsigned int no2;
    UTermT l1           = TO_TermAnStelle(l,pos);
    UTermT l0           = TO_NULLTerm(l);
    if (ct71_doCRPath(h) && ct71_doPath(h) && (ppos != lambdaPos)){
      endNum = TO_GetId(TO_TermEnde(TO_TermAnStelle(l,ppos)));
    }
    do {
      if ((endNum != 0) /*&& (pos < ppos)*/ && (TO_GetId(TO_TermEnde(l1)) < endNum)){ 
        if(0)IO_DruckeFlex("Skip position %P (%t) because %d < %d last rewrite at %t (%d) = %P\n",
                      l,l1,l1,TO_GetId(TO_TermEnde(l1)), endNum,
                      TO_TermAnStelle(l,ct71_getPos(h)), TO_GetId(TO_TermAnStelle(l,ct71_getPos(h))),
                      l, TO_TermAnStelle(l,ct71_getPos(h)));
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
          while (!handled && 
                 MO_AllGleichungMatchNext(FALSE, &gleichung)){
            no2++;
            if (no2 <= no){
            }
            else {
              TermT lneu  = MO_AllTermErsetztNeu(l,l1); /* neue Kopie */
              UTermT lneu1 = TO_TermAnStelle(lneu,pos);
              /* sigma(u) == l1, sigma(v) == lneu1 */
              EConsT e1 = CO_newEcons (l1,lneu1,e,Gt_C);
              if (CO_satisfiableUnder(e1,c)){
                /* Optimierungspotential: */
                /* CO_getAllSolutionsInBothDirections(l1,lneu1,c,&d1,&d2) */
                EConsT e2 = CO_newEcons (lneu1,l1,e,Ge_C);
                HintT h1 = ct71_mkHint(LeafHint|RewrHint,lambdaPos,0);
                HintT h2 = ct71_mkHint(CrewHint,pos,no2);
                handled = TRUE;
                if (TEX_OUTPUT){
                  IO_DruckeFlex("%*\\CRewrite{%d%s}{%P}{%%\n", 2*ct71_level,
                                RE_AbsNr(gleichung), 
                                TP_RichtungAusgezeichnet(gleichung) ? "" :"'",
                                l, l1);
                }
                ct71_Durchnumerieren(lneu);
                ct71_level++;
                *closed = PA_preferD1()
                  ? ct71_handleNode(lneu,c,e1,h1) && ct71_handleNode(l,c,e2,h2)
                  : ct71_handleNode(l,c,e2,h2) && ct71_handleNode(lneu,c,e1,h1);
                CO_deleteEcons(e2);
              }
              else {
                if (TEX_OUTPUT_F){
                  IO_DruckeFlex("%*\\FCRewrite{%d%s}{%P}{%E \\wedge %C}%%\n", 
                                2*ct71_level, RE_AbsNr(gleichung), 
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
    } while (!handled && (l1 != l0));
  }
  CLOSETRACE (("ct71_CRew -> %b\n", handled));
  return handled;
}

/* Format von ct71_handleNode
 * static 
 * BOOLEAN -- True gdw Teilbaum abgeschlossen, 
 *            d.h alle ueberdeckten Instanzen gzfb.
 * ct71_handleNode(
 *   LiteralT l, -- die zu untersuchende Gleichung
 *   ContextT c, -- der geloeste Teil des Constraints
 *   EConsT e,   -- der noch nicht geloeste Teil des Constraints (evtl. NULL)
 *   HintT h     -- fuer Wissenstransfer
 * )
 */

static BOOLEAN ct71_handleNode (LiteralT l, ContextT c, EConsT e, HintT h)
{
  BOOLEAN closed = FALSE;
  OPENTRACE (("ct71_handleNode: %t %C %E\n", l, c, e));
  {
    unsigned int oldlevel = ct71_level;
    ct71_node_counter++;
    if (TEX_OUTPUT) {
      IO_DruckeFlex("%*\\Node{%t}{%t}{%E%C}{%%\n", 
                    2*ct71_level, TO_LitLHS(l), TO_LitRHS(l), e, c);
    }
    ct71_level++;
    if (LIMIT_DEPTH && (ct71_level > DEPTH_LIMIT)){
      /* to catch endless recursion */
      our_error("ct71_handleNode: nesting to deep");
    }
    else if ((PA_maxNodeCount() != 0) && 
             (ct71_node_counter >= PA_maxNodeCount())){
      if (TEX_OUTPUT) IO_DruckeFlex("%*\\ToManyNodes%%\n", 2*ct71_level);
      closed = FALSE;
    }
    else if (ct71_doLeaf(h) && ct71_LeafTests(l,c,e,h,&closed)){
    }
    else if (ct71_doInst(h)  && ct71_Inst(l,c,e,h,&closed)){
    }
    else if (ct71_doRewr(h)  && ct71_Rewrite(l,c,e,h,&closed)){
    }
    else if (ct71_Split(l,c,e,h,&closed)){
    }
    else if (ct71_CRew(l,c,e,h,&closed)){
    }
    else {
      if (TEX_OUTPUT) IO_DruckeFlex("%*\\GiveUp%%\n", 2*ct71_level);
      closed = FALSE; /* Machen wir's nochmals explizit */
    }
    while (ct71_level > oldlevel){
      ct71_level--;
      if (TEX_OUTPUT) IO_DruckeFlex("%*}%%\n", 2*ct71_level);
    }
  }
  CLOSETRACE (("ct71_handleNode: -> %b\n", closed));
  return closed;
}

/* -------------------------------------------------------------------------- */

BOOLEAN CO_GroundJoinable71(TermT s, TermT t)
{
  BOOLEAN res = FALSE;
  {
    TermT ss, tt;
    LiteralT l;
    ContextT c;
    HintT h = ct71_mkHint(CrewHint,lambdaPos,0);
    ss = TO_Termkopie(s);
    tt = TO_Termkopie(t);
    TO_mkLit(l,ss,tt);
    ct71_Durchnumerieren(l);
    c = CO_newContext(CO_allocBindings());
    ct71_tree_counter++;
    ct71_node_counter = 0;
    if(TEX_OUTPUT) IO_DruckeFlex("\\ctree{%d}{%%\n", ct71_tree_counter);
    ct71_level++;
    res = ct71_handleNode(l,c,NULL,h);
    if(0)IO_DruckeFlex("#%l %b\n", ct71_node_counter, res);
    if(TEX_OUTPUT) IO_DruckeFlex("}{%s}%%\n", res ? "\\Success" : "\\Fail");
    ct71_level--;
    CO_deepDelete(c);
    TO_TermLoeschen(l);
  }
  return res;
}

/* ------------------------------------------------------------------------- */
/* -------- E O F ---------------------------------------------------------- */
/* ------------------------------------------------------------------------- */
