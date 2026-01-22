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

static unsigned int  ct80_level = 0;
static unsigned int  ct80_tree_counter = 0;
static unsigned long ct80_node_counter = 0;

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

static HintT ct80_mkHint(unsigned long h, unsigned long pos, unsigned no)
{
  unsigned long pos2 = pos & 0xFFFF;
  unsigned no2 = no & 0xFF;
  if (pos2 != pos) pos2 = lambdaPos;
  if (no != no2) no2 = 0;
  return  h | (pos2 << 8) | (no2 << 24);
}

static BOOLEAN ct80_doLeaf(HintT h)
{
  //  return 1;
  return h & LeafHint;
}

static BOOLEAN ct80_doInst(HintT h)
{
  //  return 1;
  return h & InstHint;
}

static BOOLEAN ct80_doSplit(HintT h)
{
  //  return 1;
  return h & SplitHint;
}

static BOOLEAN ct80_doRewr(HintT h)
{
  //  return 1;
  return h & RewrHint;
}

static BOOLEAN ct80_doPath(HintT h)
{
  //  return 0;
  return h & PathHint;
}

static BOOLEAN ct80_doCRPath(HintT h)
{
  //  return 0;
  return h & CRPathHint;
}

static unsigned long ct80_getPos(HintT h)
{
  //  return 0;

  return (h >> 8) & 0xFFFF;
}

static unsigned long ct80_getNo(HintT h)
{
  //  return 0;

  return (h >> 24) & 0xFF;
}


/* ------------------------------------------------------------------------- */
/* - forward declarations --------------------------------------------------- */
/* -------------------------------------------------------------------------- */

static BOOLEAN ct80_handleNode (LiteralT l, ContextT c, EConsT e, HintT h);

/* -------------------------------------------------------------------------- */
/* - generische Hilfsroutingen ---------------------------------------------- */
/* -------------------------------------------------------------------------- */

static void ct80_print_DConsKetten(DConsT d,DConsT d1)
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

static BOOLEAN ct80_equalUptoPos(LiteralT l1, UTermT p, LiteralT l2)
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

static void ct80_Durchnumerieren(UTermT t)
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
 * BOOLEAN -- True:   Teilbaum abgeschlossen, d.h
 *                    alle ueberdeckten Instanzen gzfb.
 *            False:  weiterprobieren
 * ct80_f(
 *   LiteralT l, -- die zu untersuchende Gleichung
 *   ContextT c, -- der geloeste Teil des Constraints
 *   EConsT e,   -- der noch nicht geloeste Teil des Constraints (evtl. NULL)
 *   HintT h    -- fuer Wissenstransfer
 * )
 */

static BOOLEAN ct80_LeafTests(LiteralT l, 
                             ContextT c MAYBE_UNUSEDPARAM, 
                             EConsT e MAYBE_UNUSEDPARAM,
                             HintT h MAYBE_UNUSEDPARAM)
{
  BOOLEAN closed = FALSE;
  OPENTRACE (("ct80_LeafTests: %t %C %E\n", l, c, e));
  {
    TermT s = TO_LitLHS(l);
    TermT t = TO_LitRHS(l);
    if (TO_TermeGleich(s,t)){
      if (TEX_OUTPUT) IO_DruckeFlex("%*\\Equal%%\n", 2*ct80_level);
      closed = TRUE;
    }
    else if (PA_CTreeS() && SS_TermpaarSubsummiertVonGM (s,t)){
      if (TEX_OUTPUT) {
        IO_DruckeFlex("%*\\Subsumed{%d}%%\n", 
                      2*ct80_level, RE_AbsNr(MO_AngewendetesObjekt));
      }
      closed = TRUE;
    }
    else if (PA_CTreeP() && TO_ACGleich (s,t)){
      if (TEX_OUTPUT) {
        IO_DruckeFlex("%*\\PSubsumed{}%%\n", 2*ct80_level);
      }
      closed = TRUE;
    }
  }
  CLOSETRACE (("ct80_LeafTests: -> %b\n", closed));
  return closed;
}

static BOOLEAN ct80_Split(LiteralT l, ContextT c, EConsT e,
                         HintT h)
{
  BOOLEAN closed = FALSE;
  OPENTRACE (("ct80_Split: %t %C %E\n", l, c, e));
  {
    if (e != NULL){
      DConsT d = CO_getAllSolutions(e,c);
      if (d == NULL){ /* OK, vorhin wurde andere Seite bei CRewrite */
        closed = TRUE;   /* als erfuellbar nachgewiesen. */
        if (TEX_OUTPUT) IO_DruckeFlex("%*\\Unsat%%\n", 2*ct80_level);
      }
      else {
        HintT h1 = 0;
        DConsT drun;
        DConsT d1 = d;
        BOOLEAN allclosed = TRUE;
        if (PA_optDCons()){
          d1 = CO_minimizeDCons(d);
          ct80_print_DConsKetten(d,d1);
        }
        if (PA_sortDCons()){
          CO_sortDCons(d1);
        }
        if (TEX_OUTPUT) IO_DruckeFlex("%*\\Split{%%\n", 2*ct80_level);
        ct80_level++;
        drun = d1; /* Hier: drun = d fuer Vergleichbarkeit DConsOpt herstellen*/
        do {
          allclosed = ct80_handleNode(l,drun->c,NULL,h1);
          drun = drun->Nachf;
        } while (allclosed && (drun != NULL));
        if (PA_optDCons()){
          CO_deleteDConsList(d1);
        }
        closed = allclosed;
        ct80_level--;
        if (TEX_OUTPUT) IO_DruckeFlex("%*}%%\n", 2*ct80_level);
      }
      CO_deleteDConsList(d);
    }
  }
  CLOSETRACE (("ct80_Split: -> %b\n", closed));
  return closed;
}

static BOOLEAN ct80_Inst(LiteralT l, ContextT c, EConsT e,
                        HintT h)
{
  BOOLEAN closed = FALSE;
  OPENTRACE (("ct80_Inst: %t %C %E\n", l, c, e));
  {
    if (CO_somethingBound(c->bindings)){
      HintT h1 = 0;
      LiteralT l1 = CO_substitute(c->bindings,l);
      ContextT c1 = CO_solveEQPart(c);
      EConsT e1 = CO_substituteECons(c->bindings,e);
      if (TEX_OUTPUT) IO_DruckeFlex("%*\\Inst{%%\n", 2*ct80_level);
      ct80_level++;
      closed = ct80_handleNode(l1,c1,NULL,h1);
      ct80_level--;
      if (TEX_OUTPUT) IO_DruckeFlex("%*}%%\n", 2*ct80_level);
      CO_deepDeleteECons(e1);
      CO_deepDelete(c1);
      TO_TermLoeschen(l1);
    }
  }
  CLOSETRACE (("ct80_Inst: -> %b\n", closed));
  return closed;
}

static BOOLEAN ct80_Rewrite(LiteralT l, ContextT c, EConsT e,
                           HintT h)
{
  BOOLEAN closed = FALSE;
  OPENTRACE (("ct80_Rewrite: %t %C %E\n", l, c, e));
  {
    UTermT l1           = l;
    UTermT l0           = TO_NULLTerm(l);
    HintT h1 = 0;
    ContextT cold       = CO_getCurrentContext();
    CO_setCurrentContext(c); /* Ordnung verstaerken fuer E-Schritte */
    do {
        if (TO_TermIstNichtVar(l1)){
          RegelOderGleichungsT re;
          LiteralT lneu;
          unsigned n, m;
          BOOLEAN leave = FALSE;
          /* Erstmal Regeln beackern */
          n = 0;
          while (!closed && !leave){
            /* Wichtig: Jedesmal neu Initialisieren, da ggf. im
               rekursiven Aufruf veraendert initialisiert! */
            MO_AllRegelMatchInit(l1);
            /* bereits behandelte Regeln uebergehen */
            for (m = 0; m < n; m++){ 
              if (!MO_AllRegelMatchNext(&re)){
                our_error("hach");
              }
            }
            if (MO_AllRegelMatchNext(&re)){
              n++;
              lneu = MO_AllTermErsetztNeu(l,l1); /* neue Kopie */
              if (TEX_OUTPUT) {
                IO_DruckeFlex("%*\\Rewrite{%s}{%d}{%P}{%%\n", 2*ct80_level,
                              TP_TermpaarIstRegel(re) ? "R" : "E",
                              RE_AbsNr(re), l, l1);
              }
              ct80_level++;
              closed = ct80_handleNode(lneu,c,e,h1);
              ct80_level--;
              if (TEX_OUTPUT) IO_DruckeFlex("%*}%%\n", 2*ct80_level);
              TO_TermLoeschen(lneu);
            }
            else {
              leave = TRUE;
            }
          }
          /* Jetzt mit gerichteten Gleichungen versuchen */
          leave = FALSE;
          n = 0;
          while (!closed && !leave){
            /* Wichtig: Jedesmal neu Initialisieren, da ggf. im
               rekursiven Aufruf veraendert initialisiert! */
            MO_AllGleichungMatchInit(l1);
            /* bereits behandelte Gleichungen uebergehen */
            for (m = 0; m < n; m++){ 
              if (!MO_AllGleichungMatchNext(TRUE,&re)){
                our_error("huch");
              }
            }
            if (MO_AllGleichungMatchNext(TRUE,&re)){
              n++;
              lneu = MO_AllTermErsetztNeu(l,l1); /* neue Kopie */
              if (TEX_OUTPUT) {
                IO_DruckeFlex("%*\\Rewrite{%s}{%d}{%P}{%%\n", 2*ct80_level,
                              TP_TermpaarIstRegel(re) ? "R" : "E",
                              RE_AbsNr(re), l, l1);
              }
              ct80_level++;
              closed = ct80_handleNode(lneu,c,e,h1);
              ct80_level--;
              if (TEX_OUTPUT) IO_DruckeFlex("%*}%%\n", 2*ct80_level);
              TO_TermLoeschen(lneu);
            }
            else {
              leave = TRUE;
            }
          }
        }
        l1 = TO_Schwanz(l1);
    } while (!closed && (l1 != l0));
    CO_setCurrentContext(cold); /* Ordnung zuruecksetzen */
  }
  CLOSETRACE (("ct80_Rewrite: -> %b\n", closed));
  return closed;
}

static BOOLEAN ct80_CRew (LiteralT l, ContextT c, EConsT e,
                         HintT h)
{
  BOOLEAN closed = FALSE;
  OPENTRACE (("ct80_CRew: %t  %C %E\n", l, c, e));
  {
    GleichungsT gleichung;
    UTermT l1           = TO_Schwanz(l);
    UTermT l0           = TO_NULLTerm(l);
    unsigned int pos = 1;
    TermT lneu;
    UTermT lneu1;
    EConsT e1;
    unsigned n, m;
    BOOLEAN leave = FALSE;
    do {
        if (TO_TermIstNichtVar(l1)){
          n = 0;
          leave = FALSE;
          while (!closed && !leave){ 
            /* Wichtig: Jedesmal neu Initialisieren, da ggf. im
               rekursiven Aufruf veraendert initialisiert! */
            MO_AllGleichungMatchInit(l1);
            /* bereits behandelte Gleichungen uebergehen */
            for (m = 0; m < n; m++){
              if (!MO_AllGleichungMatchNext(FALSE, &gleichung)){
                our_error("hich");
              }
            }
            if (MO_AllGleichungMatchNext(FALSE, &gleichung)){
              n++;
              lneu  = MO_AllTermErsetztNeu(l,l1); /* neue Kopie */
              lneu1 = TO_TermAnStelle(lneu,pos);
              /* sigma(u) == l1, sigma(v) == lneu1 */
              e1 = CO_newEcons (l1,lneu1,e,Gt_C);
              if (CO_satisfiableUnder(e1,c)){
                /* Optimierungspotential: */
                /* CO_getAllSolutionsInBothDirections(l1,lneu1,c,&d1,&d2) */
                EConsT e2 = CO_newEcons (lneu1,l1,e,Ge_C);
                HintT h1 = 0;
                HintT h2 = 0;
                if (TEX_OUTPUT){
                  IO_DruckeFlex("%*\\CRewrite{%d%s}{%P}{%%\n", 2*ct80_level,
                                RE_AbsNr(gleichung), 
                                TP_RichtungAusgezeichnet(gleichung) ? "" :"'",
                                l, l1);
                }
                ct80_level++;
                closed = PA_preferD1()
                  ? ct80_handleNode(lneu,c,e1,h1) && ct80_handleNode(l,c,e2,h2)
                  : ct80_handleNode(l,c,e2,h2) && ct80_handleNode(lneu,c,e1,h1);
                ct80_level--;
                if (TEX_OUTPUT) IO_DruckeFlex("%*}%%\n", 2*ct80_level);
                CO_deleteEcons(e2);
              }
              else {
                if (TEX_OUTPUT_F){
                  IO_DruckeFlex("%*\\FCRewrite{%d%s}{%P}{%E \\wedge %C}%%\n", 
                                2*ct80_level, RE_AbsNr(gleichung), 
                                TP_RichtungAusgezeichnet(gleichung) ? "" :"'",
                                l, l1, e1, c);
                }
              }
              CO_deleteEcons(e1);
              TO_TermLoeschen(lneu);
            }
            else {
              leave = TRUE;
            }
          }
        }
        l1 = TO_Schwanz(l1);
        pos++;
    } while (!closed && (l1 != l0));
  }
  CLOSETRACE (("ct80_CRew -> %b\n", closed));
  return closed;
}

/* Format von ct80_handleNode
 * static 
 * BOOLEAN -- True gdw Teilbaum abgeschlossen, 
 *            d.h alle ueberdeckten Instanzen gzfb.
 * ct80_handleNode(
 *   LiteralT l, -- die zu untersuchende Gleichung
 *   ContextT c, -- der geloeste Teil des Constraints
 *   EConsT e,   -- der noch nicht geloeste Teil des Constraints (evtl. NULL)
 *   HintT h     -- fuer Wissenstransfer
 * )
 */

static BOOLEAN ct80_handleNode (LiteralT l, ContextT c, EConsT e, HintT h)
{
  BOOLEAN closed = FALSE;
  OPENTRACE (("ct80_handleNode: %t %C %E\n", l, c, e));
  {
    unsigned int oldlevel = ct80_level;
    ct80_node_counter++;
    if (TEX_OUTPUT) {
      IO_DruckeFlex("%*\\Node{%t}{%t}{%E%C}{%%\n", 
                    2*ct80_level, TO_LitLHS(l), TO_LitRHS(l), e, c);
    }
    ct80_level++;
    if (LIMIT_DEPTH && (ct80_level > DEPTH_LIMIT)){
      /* to catch endless recursion */
      our_error("ct80_handleNode: nesting to deep");
    }
    else if ((PA_maxNodeCount() != 0) && 
             (ct80_node_counter >= PA_maxNodeCount())){
      if (TEX_OUTPUT) IO_DruckeFlex("%*\\ToManyNodes%%\n", 2*ct80_level);
      closed = FALSE;
    }
    else {
      closed =    ct80_LeafTests(l,c,e,h)
               || ct80_Inst(l,c,e,h)
               || ct80_Split(l,c,e,h)
               || ct80_Rewrite(l,c,e,h)
               || ct80_CRew(l,c,e,h);
    }
    while (ct80_level > oldlevel+1){
      ct80_level--;
      if (TEX_OUTPUT) IO_DruckeFlex("%*}%%\n", 2*ct80_level);
    }
    if (!closed) {
      if (TEX_OUTPUT) IO_DruckeFlex("%*\\GiveUp%%\n", 2*ct80_level);
    }
    while (ct80_level > oldlevel){
      ct80_level--;
      if (TEX_OUTPUT) IO_DruckeFlex("%*}%%\n", 2*ct80_level);
    }
  }
  CLOSETRACE (("ct80_handleNode: -> %b\n", closed));
  return closed;
}

/* -------------------------------------------------------------------------- */

BOOLEAN CO_GroundJoinable80(TermT s, TermT t)
{
  BOOLEAN res = FALSE;
  {
    TermT ss, tt;
    LiteralT l;
    ContextT c;
    HintT h = 0;
    ss = TO_Termkopie(s);
    tt = TO_Termkopie(t);
    IO_DruckeFlex("testing %t = %t...\n", ss, tt);
    TO_mkLit(l,ss,tt);
    ct80_Durchnumerieren(l);
    c = CO_newContext(CO_allocBindings());
    ct80_tree_counter++;
    ct80_node_counter = 0;
    if(TEX_OUTPUT) IO_DruckeFlex("\\ctree{%d}{%%\n", ct80_tree_counter);
    ct80_level++;
    res = ct80_handleNode(l,c,NULL,h);
    if(0)IO_DruckeFlex("#%l %b\n", ct80_node_counter, res);
    if(TEX_OUTPUT) IO_DruckeFlex("}{%s}%%\n", res ? "\\Success" : "\\Fail");
    ct80_level--;
    CO_deepDelete(c);
    TO_TermLoeschen(l);
  }
  return res;
}

/* ------------------------------------------------------------------------- */
/* -------- E O F ---------------------------------------------------------- */
/* ------------------------------------------------------------------------- */
