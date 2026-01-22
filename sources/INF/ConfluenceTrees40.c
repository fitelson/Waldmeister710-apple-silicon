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

static unsigned int  ct40_level = 0;
static unsigned int  ct40_tree_counter = 0;
static unsigned long ct40_node_counter = 0;

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

static HintT ct40_mkHint(unsigned long h, unsigned long pos, unsigned no)
{
  unsigned long pos2 = pos & 0xFFFF;
  unsigned no2 = no & 0xFF;
  if (pos2 != pos) pos2 = lambdaPos;
  if (no != no2) no2 = 0;
  return  h | (pos2 << 8) | (no2 << 24);
}

static BOOLEAN ct40_doLeaf(HintT h)
{
  //  return 1;
  return h & LeafHint;
}

static BOOLEAN ct40_doInst(HintT h)
{
  //  return 1;
  return h & InstHint;
}

static BOOLEAN ct40_doSplit(HintT h)
{
  //  return 1;
  return h & SplitHint;
}

static BOOLEAN ct40_doRewr(HintT h)
{
  //  return 1;
  return h & RewrHint;
}

static BOOLEAN ct40_doPath(HintT h)
{
  //  return 0;
  return h & PathHint;
}

static BOOLEAN ct40_doCRPath(HintT h)
{
  //  return 0;
  return h & CRPathHint;
}

static unsigned long ct40_getPos(HintT h)
{
  //  return 0;

  return (h >> 8) & 0xFFFF;
}

static unsigned long ct40_getNo(HintT h)
{
  //  return 0;

  return (h >> 24) & 0xFF;
}


/* ------------------------------------------------------------------------- */
/* - forward declarations --------------------------------------------------- */
/* -------------------------------------------------------------------------- */

static BOOLEAN ct40_handleNode (LiteralT l, ContextT c, EConsT e, HintT h);

/* -------------------------------------------------------------------------- */
/* - generische Hilfsroutingen ---------------------------------------------- */
/* -------------------------------------------------------------------------- */

static void ct40_print_DConsKetten(DConsT d,DConsT d1)
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

static BOOLEAN ct40_equalUptoPos(LiteralT l1, UTermT p, LiteralT l2)
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

static void ct40_Durchnumerieren(UTermT t)
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
 * ct40_f(
 *   LiteralT l, -- die zu untersuchende Gleichung
 *   ContextT c, -- der geloeste Teil des Constraints
 *   EConsT e,   -- der noch nicht geloeste Teil des Constraints (evtl. NULL)
 *   HintT h,    -- fuer Wissenstransfer
 * 
 *   BOOLEAN *closed  -- das Ergebnis: True gdw Teilbaum abgeschlossen, d.h
 *                       alle ueberdeckten Instanzen gzfb.
 * )
 */

static BOOLEAN ct40_LeafTests(LiteralT l, 
                             ContextT c MAYBE_UNUSEDPARAM, 
                             EConsT e MAYBE_UNUSEDPARAM,
                             HintT h MAYBE_UNUSEDPARAM, 
                             BOOLEAN *closed)
{
  BOOLEAN handled = FALSE;
  OPENTRACE (("ct40_LeafTests: %t %C %E\n", l, c, e));
  {
    TermT s = TO_LitLHS(l);
    TermT t = TO_LitRHS(l);
    if (TO_TermeGleich(s,t)){
      if (TEX_OUTPUT) IO_DruckeFlex("%*\\Equal%%\n", 2*ct40_level);
      handled = *closed = TRUE;
    }
    else if (PA_CTreeS() && SS_TermpaarSubsummiertVonGM (s,t)){
      if (TEX_OUTPUT) {
        IO_DruckeFlex("%*\\Subsumed{%d}%%\n", 
                      2*ct40_level, RE_AbsNr(MO_AngewendetesObjekt));
      }
      handled = *closed = TRUE;
    }
    else if (PA_CTreeP() && TO_ACGleich (s,t)){
      if (TEX_OUTPUT) {
        IO_DruckeFlex("%*\\PSubsumed{}%%\n", 2*ct40_level);
      }
      handled = *closed = TRUE;
    }
  }
  CLOSETRACE (("ct40_LeafTests: -> %b\n", handled));
  return handled;
}

static BOOLEAN ct40_Split(LiteralT l, ContextT c, EConsT e,
                         HintT h, BOOLEAN *closed)
{
  BOOLEAN handled = FALSE;
  OPENTRACE (("ct40_Split: %t %C %E\n", l, c, e));
  {
    if (e != NULL){
      DConsT d = CO_getAllSolutions(e,c);
      if (d == NULL){ /* OK, vorhin wurde andere Seite bei CRewrite */
        *closed = TRUE;   /* als erfuellbar nachgewiesen. */
        if (TEX_OUTPUT) IO_DruckeFlex("%*\\Unsat%%\n", 2*ct40_level);
      }
      else {
        HintT h1 = 0;
        DConsT drun;
        DConsT d1 = d;
        BOOLEAN allclosed = TRUE;
        if (PA_optDCons()){
          d1 = CO_minimizeDCons(d);
          ct40_print_DConsKetten(d,d1);
        }
        if (PA_sortDCons()){
          CO_sortDCons(d1);
        }
        if (TEX_OUTPUT) IO_DruckeFlex("%*\\Split{%%\n", 2*ct40_level);
        ct40_level++;
        drun = d1; /* Hier: drun = d fuer Vergleichbarkeit DConsOpt herstellen*/
        do {
          allclosed = ct40_handleNode(l,drun->c,NULL,h1);
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
  CLOSETRACE (("ct40_Split: -> %b\n", handled));
  return handled;
}

static BOOLEAN ct40_Inst(LiteralT l, ContextT c, EConsT e,
                        HintT h, BOOLEAN *closed)
{
  BOOLEAN handled = FALSE;
  OPENTRACE (("ct40_Inst: %t %C %E\n", l, c, e));
  {
    if (CO_somethingBound(c->bindings)){
      HintT h1 = 0;
      LiteralT l1 = CO_substitute(c->bindings,l);
      ContextT c1 = CO_solveEQPart(c);
      if (e != NULL){
        our_error("rethink for e != NULL!");
      }
      if (TEX_OUTPUT) IO_DruckeFlex("%*\\Inst{%%\n", 2*ct40_level);
      ct40_level++;
      *closed = ct40_handleNode(l1,c1,NULL,h1);
      CO_deepDelete(c1);
      TO_TermLoeschen(l1);
      handled = TRUE;
    }
  }
  CLOSETRACE (("ct40_Inst: -> %b\n", handled));
  return handled;
}

static BOOLEAN ct40_XRew (LiteralT l, ContextT c, EConsT e,
                         HintT h, BOOLEAN *closed)
{
  BOOLEAN handled = FALSE;
  OPENTRACE (("ct40_XRew: %t  %C %E\n", l, c, e));
  {
    GleichungsT gleichung;
    UTermT l1           = TO_Schwanz(l);
    UTermT l0           = TO_NULLTerm(l);
    unsigned int pos = 1;
    do {
        if (TO_TermIstNichtVar(l1) &&
            (MO_RegelGefunden(l1) || MO_GleichungGefunden(l1))) {
          RegelOderGleichungsT re = MO_AngewendetesObjekt;
          LiteralT lneu = MO_AllTermErsetztNeu(l,l1); /* neue Kopie */
          HintT h1 = 0;
          handled = TRUE;
          if (TEX_OUTPUT) {
            IO_DruckeFlex("%*\\Rewrite{%s}{%d}{%P}{%%\n", 2*ct40_level,
                          TP_TermpaarIstRegel(re) ? "R" : "E",
                          RE_AbsNr(re), l, l1);
          }
          ct40_level++;
          *closed = ct40_handleNode(lneu,c,e,h1);
          TO_TermLoeschen(lneu);
        }
        else if (TO_TermIstNichtVar(l1)){
          MO_AllGleichungMatchInit(l1);
          while (!handled && 
                 MO_AllGleichungMatchNext(FALSE, &gleichung)){
              TermT lneu  = MO_AllTermErsetztNeu(l,l1); /* neue Kopie */
              UTermT lneu1 = TO_TermAnStelle(lneu,pos);
              /* sigma(u) == l1, sigma(v) == lneu1 */
              EConsT e1 = CO_newEcons (l1,lneu1,e,Gt_C);
              if (CO_satisfiableUnder(e1,c)){
                /* Optimierungspotential: */
                /* CO_getAllSolutionsInBothDirections(l1,lneu1,c,&d1,&d2) */
                EConsT e2 = CO_newEcons (lneu1,l1,e,Ge_C);
                HintT h1 = 0;
                HintT h2 = 0;
                handled = TRUE;
                if (TEX_OUTPUT){
                  IO_DruckeFlex("%*\\CRewrite{%d%s}{%P}{%%\n", 2*ct40_level,
                                RE_AbsNr(gleichung), 
                                TP_RichtungAusgezeichnet(gleichung) ? "" :"'",
                                l, l1);
                }
                ct40_level++;
                *closed = PA_preferD1()
                  ? ct40_handleNode(lneu,c,e1,h1) && ct40_handleNode(l,c,e2,h2)
                  : ct40_handleNode(l,c,e2,h2) && ct40_handleNode(lneu,c,e1,h1);
                CO_deleteEcons(e2);
              }
              else {
                if (TEX_OUTPUT_F){
                  IO_DruckeFlex("%*\\FCRewrite{%d%s}{%P}{%E \\wedge %C}%%\n", 
                                2*ct40_level, RE_AbsNr(gleichung), 
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
    } while (!handled && (l1 != l0));
  }
  CLOSETRACE (("ct40_XRew -> %b\n", handled));
  return handled;
}

/* Format von ct40_handleNode
 * static 
 * BOOLEAN -- True gdw Teilbaum abgeschlossen, 
 *            d.h alle ueberdeckten Instanzen gzfb.
 * ct40_handleNode(
 *   LiteralT l, -- die zu untersuchende Gleichung
 *   ContextT c, -- der geloeste Teil des Constraints
 *   EConsT e,   -- der noch nicht geloeste Teil des Constraints (evtl. NULL)
 *   HintT h     -- fuer Wissenstransfer
 * )
 */

static BOOLEAN ct40_handleNode (LiteralT l, ContextT c, EConsT e, HintT h)
{
  BOOLEAN closed = FALSE;
  OPENTRACE (("ct40_handleNode: %t %C %E\n", l, c, e));
  {
    unsigned int oldlevel = ct40_level;
    ct40_node_counter++;
    if (TEX_OUTPUT) {
      IO_DruckeFlex("%*\\Node{%t}{%t}{%E%C}{%%\n", 
                    2*ct40_level, TO_LitLHS(l), TO_LitRHS(l), e, c);
    }
    ct40_level++;
    if (LIMIT_DEPTH && (ct40_level > DEPTH_LIMIT)){
      /* to catch endless recursion */
      our_error("ct40_handleNode: nesting to deep");
    }
    else if ((PA_maxNodeCount() != 0) && 
             (ct40_node_counter >= PA_maxNodeCount())){
      if (TEX_OUTPUT) IO_DruckeFlex("%*\\ToManyNodes%%\n", 2*ct40_level);
      closed = FALSE;
    }
    else if (ct40_LeafTests(l,c,e,h,&closed)){
    }
    else if (ct40_Inst(l,c,e,h,&closed)){
    }
    else if (ct40_Split(l,c,e,h,&closed)){
    }
    else if (ct40_XRew(l,c,e,h,&closed)){
    }
    else {
      if (TEX_OUTPUT) IO_DruckeFlex("%*\\GiveUp%%\n", 2*ct40_level);
      closed = FALSE; /* Machen wir's nochmals explizit */
    }
    while (ct40_level > oldlevel){
      ct40_level--;
      if (TEX_OUTPUT) IO_DruckeFlex("%*}%%\n", 2*ct40_level);
    }
  }
  CLOSETRACE (("ct40_handleNode: -> %b\n", closed));
  return closed;
}

/* -------------------------------------------------------------------------- */

BOOLEAN CO_GroundJoinable40(TermT s, TermT t)
{
  BOOLEAN res = FALSE;
  {
    TermT ss, tt;
    LiteralT l;
    ContextT c;
    HintT h = 0;
    ss = TO_Termkopie(s);
    tt = TO_Termkopie(t);
    TO_mkLit(l,ss,tt);
    ct40_Durchnumerieren(l);
    c = CO_newContext(CO_allocBindings());
    ct40_tree_counter++;
    ct40_node_counter = 0;
    if(TEX_OUTPUT) IO_DruckeFlex("\\ctree{%d}{%%\n", ct40_tree_counter);
    ct40_level++;
    res = ct40_handleNode(l,c,NULL,h);
    if(0)IO_DruckeFlex("#%l %b\n", ct40_node_counter, res);
    if(TEX_OUTPUT) IO_DruckeFlex("}{%s}%%\n", res ? "\\Success" : "\\Fail");
    ct40_level--;
    CO_deepDelete(c);
    TO_TermLoeschen(l);
  }
  return res;
}

/* ------------------------------------------------------------------------- */
/* -------- E O F ---------------------------------------------------------- */
/* ------------------------------------------------------------------------- */
