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

static unsigned int ct3_level = 0;
static int counter           = 0;

/* -------------------------------------------------------------------------- */
/* - hints ------------------------------------------------------------------ */
/* -------------------------------------------------------------------------- */

#define DO_HINTS 0 /* 0, 1 */

typedef enum {
  do_sj = 0x1, do_split = 0x2, do_inst = 0x4, do_rewr = 0x8
} ct3_Hints;

#define NO_HINT 0xFF

#if DO_HINTS
#define DO_SJ(h)    ((h) & do_sj)
#define DO_SPLIT(h) ((h) & do_split)
#define DO_INST(h)  ((h) & do_inst)
#define DO_REWR(h)  ((h) & do_rewr)
#else
#define DO_SJ(h)    TRUE
#define DO_SPLIT(h) TRUE
#define DO_INST(h)  TRUE
#define DO_REWR(h)  TRUE
#endif

/* -------------------------------------------------------------------------- */
/* - forward declarations --------------------------------------------------- */
/* -------------------------------------------------------------------------- */

static BOOLEAN ct3_handleNode (TermT s, TermT t, ContextT c, EConsT e, 
                              unsigned hint);

/* -------------------------------------------------------------------------- */
/* - generische Hilfsroutingen ---------------------------------------------- */
/* -------------------------------------------------------------------------- */

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

static void ct3_print_DConsKette(DConsT d)
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

static void ct3_einruecken(void)
{
  if (TEX_OUTPUT)printf("%*s", 2*ct3_level, "");
}


static BOOLEAN ct3_RewriteOneSide (TermT s, TermT tl, TermT tr, ContextT c, 
                                  BOOLEAN *resp)
{
  BOOLEAN expanded = FALSE;
  OPENTRACE (("ct3_RewriteOneSide(%s): %t =?= %t %C\n", 
              (s == tl) ? "left" : "right", tl, tr, c));
  {
    UTermT s1 = s;
    UTermT s0 = TO_NULLTerm(s);
    UTermT Vorg = NULL;
    ContextT cold = CO_getCurrentContext();
    CO_setCurrentContext(c); /* Ordnung verstaerken fuer E-Schritte */
    do {
      if (TO_TermIstNichtVar(s1) &&
          (MO_RegelGefunden(s1) || MO_GleichungGefunden(s1))) {
        RegelOderGleichungsT re = MO_AngewendetesObjekt;
        expanded = TRUE;
        if (TEX_OUTPUT) {
          ct3_einruecken();
          IO_DruckeFlex("\\Rewrite{%s}{%d}{%p}{%%\n", 
                        TP_TermpaarIstRegel(re) ? "R" : "E",
                        TP_TermpaarIstRegel(re) ? TP_ElternNr(re) : -TP_ElternNr(re), 
                        (s == tl) ? linkeSeite : rechteSeite, s, s1);
        }
        if (Vorg == NULL){
          MO_SigmaRInEZ();
        }
        else {
          TO_NachfBesetzen(Vorg, MO_SigmaRInLZ());
        }
        ct3_level++;
        *resp = ct3_handleNode(tl,tr,c,NULL, NO_HINT);
      }
      Vorg = s1;
      s1 = TO_Schwanz(s1);
    } while (!expanded && (s1 != s0));
    CO_setCurrentContext(cold); /* Ordnung zuruecksetzen */
  }
  CLOSETRACE (("ct3_RewriteOneSide(%s): %s exapanded -> %b\n", 
               (s == tl) ? "left" : "right", expanded ? "" : "not", *resp));
  return expanded;
}

static BOOLEAN ct3_CRewOneSide (TermT s, TermT tl, TermT tr, ContextT c, 
                               BOOLEAN *resp)
{
  BOOLEAN expanded = FALSE;
  OPENTRACE (("ct3_CRewOneSide(%s): %t =?= %t %C\n", 
              (s == tl) ? "left" : "right", tl, tr, c));
  {
    GleichungsT gleichung;
    UTermT s1 = s;
    UTermT s0 = TO_NULLTerm(s);
    do {
      if (TO_TermIstNichtVar(s1)){
	MO_AllGleichungMatchInit(s1);
	while (!expanded && 
	       MO_AllGleichungMatchNext(FALSE, &gleichung)){
	  TermT sneu  = MO_AllTermErsetztNeu(s,s1); /* neue Kopie */
	  UTermT sneu1 = anGleicherStelle(sneu,s,s1);
	  /* sigma(u) == s1, sigma(v) == sneu1 */
          TermT tlneu = (s == tl) ? sneu : tl;
          TermT trneu = (s == tl) ? tr : sneu;
          EConsT e1 = CO_newEcons (s1,sneu1,NULL,Gt_C);
          EConsT e2 = CO_newEcons (sneu1,s1,NULL,Ge_C);
          if (CO_satisfiableUnder(e1,c)){
            /* Optimierungspotential: */
            /* CO_getAllSolutionsInBothDirections(s1,sneu1,c,&d1,&d2) */
	    expanded = TRUE;
            if (TEX_OUTPUT){
              ct3_einruecken();
              IO_DruckeFlex("\\CRewrite{%d}{%p}{%%\n", 
                            -TP_ElternNr(gleichung),
                            (s == tl) ? linkeSeite : rechteSeite, s, s1);
            }
            ct3_level++;
            *resp = PREFER_D1
                ? ct3_handleNode(tlneu,trneu,c,e1,NO_HINT) 
                  && ct3_handleNode(tl,tr,c,e2,NO_HINT)
                : ct3_handleNode(tl,tr,c,e2,NO_HINT) 
                  && ct3_handleNode(tlneu,trneu,c,e1,NO_HINT);
          }
          CO_deleteEcons(e1);
          CO_deleteEcons(e2);
	  TO_TermLoeschen(sneu);
	}
      }
      s1 = TO_Schwanz(s1);
    } while (!expanded && (s1 != s0));
  }
  CLOSETRACE (("ct3_CRewOneSide(%s): %s expanded -> %b\n", 
               (s == tl) ? "left" : "right", expanded ? "" : "not" *resp));
  return expanded;
}

static BOOLEAN ct3_handleNode (TermT s, TermT t, ContextT c, EConsT e, 
                              unsigned hint MAYBE_UNUSEDPARAM)
     /* falls e == NULL, dann ist c satisfiable! */
{
  BOOLEAN res = FALSE;
  unsigned int oldlevel = ct3_level;
  OPENTRACE (("ct3_handleNode: %t =?= %t %C %E %x\n", s, t, c, e, hint));
  ct3_einruecken();
  if (TEX_OUTPUT) 
    IO_DruckeFlex("\\Node{%t}{%t}{%E%C}{%%\n", s, t, e, c);
  ct3_level++;
  if ((ct3_level > DEPTH_LIMIT) && LIMIT_DEPTH){
    our_error("nesting to deep");
  }
  {
    if (DO_SJ(hint) && TO_TermeGleich(s,t)){
      if (TEX_OUTPUT) printf("%*s\\Equal%%\n", 2*ct3_level, "");
      RESTRACE(("ct3_handleNode: Terme sind direkt gleich: %t = %t\n",s,t));
      res = TRUE;
    }
    else if (DO_SJ(hint) && SS_TermpaarSubsummiertVonGM (s,t)){
      if (TEX_OUTPUT) printf("%*s\\Subsumed{%ld}%%\n", 2*ct3_level, "", 
                              -TP_ElternNr(MO_AngewendetesObjekt));
      RESTRACE(("ct3_handleNode: Terme werden subsumiert: %t = %t\n",s,t));
      res = TRUE;
    }
    else if (DO_SPLIT(hint) && (e != NULL)){ /* Erfuellbarkeit unklar, also erstmal splitten */
      DConsT d = CO_getAllSolutions(e,c);
      if (d == NULL){ 
        res = TRUE; /* OK, wir haben vorhin richtige Seite als erfuellbar getestet! */
        if (TEX_OUTPUT) printf("%*s\\Unsat%%\n", 2*ct3_level, "");
      }
      else {
        DConsT drun;
        if (0) ct3_print_DConsKette(d);
        if (TEX_OUTPUT) printf("%*s\\Split{%%\n", 2*ct3_level, "");
        ct3_level++;
        drun = d;
        do {
          res = ct3_handleNode(s,t,drun->c,NULL,NO_HINT);
          drun = drun->Nachf;
        } while (res && (drun != NULL));
      }
      CO_deleteDConsList(d);
    }
    else if (DO_INST(hint) && CO_somethingBound(c->bindings)){
      TermT s1 = CO_substitute(c->bindings,s);
      TermT t1 = CO_substitute(c->bindings,t);
      ContextT c1 = CO_solveEQPart(c);
      RESTRACE(("ct3_handleNode: Terme nach subst: %t = %t\n",s1,t1));
      if (TEX_OUTPUT) printf("%*s\\Inst{%%\n", 2*ct3_level, "");
      ct3_level++;
      res = ct3_handleNode(s1,t1,c1,NULL,NO_HINT);
      CO_deepDelete(c1);
      TO_TermeLoeschen(s1,t1);
    }
    else if (DO_REWR(hint)){
      TermT s1 = TO_Termkopie(s);
      TermT t1 = TO_Termkopie(t);
      if (! (   ct3_RewriteOneSide(s1,s1,t1,c,&res)
             || ct3_RewriteOneSide(t1,s1,t1,c,&res)
             || ct3_CRewOneSide(s1,s1,t1,c,&res)
             || ct3_CRewOneSide(t1,s1,t1,c,&res)   )){
        res = FALSE; /* Machen wir's nochmals explizit */
        if (TEX_OUTPUT) printf("%*s\\GiveUp%%\n", 2*ct3_level, "");
      }
      TO_TermeLoeschen(s1,t1);
    }
    else {
      res = FALSE; /* Machen wir's nochmals explizit */
      if (TEX_OUTPUT) printf("%*s\\GiveUp%%NO HINT\n", 2*ct3_level, "");
    }
  }
  while (ct3_level > oldlevel){
    ct3_level--;
    if (TEX_OUTPUT) printf("%*s%%\n", 2*ct3_level+1, "}");
  }
  CLOSETRACE (("ct3_handleNode: -> %b\n", res));
  return res;
}

/* -------------------------------------------------------------------------- */

BOOLEAN CO_GroundJoinable3(TermT s, TermT t)
{
  BOOLEAN res = FALSE;
  {
    ContextT c;
    c = CO_newContext(CO_allocBindings());
    counter++;
    if(TEX_OUTPUT) printf("\\ctree{%d}{%%\n", counter);
    ct3_level++;
    res = ct3_handleNode(s,t,c,NULL, do_rewr);
    if(TEX_OUTPUT) printf("}{%s}%%\n", res ? "\\Success" : "\\Fail");
    ct3_level--;
    CO_deepDelete(c);
  }
  return res;
}

/* ------------------------------------------------------------------------- */
/* -------- E O F ---------------------------------------------------------- */
/* ------------------------------------------------------------------------- */
