#include "ConfluenceTrees.h"
#include "ConfluenceTrees_intern.h"
#include "FehlerBehandlung.h"
#include "Ordnungen.h"
#include "compiler-extra.h"

#define ZM_TIME_MEASURE   0 /* 0, 1 */
#include "ZeitMessung.h"


unsigned int length_limit = 0;
unsigned int depth_limit  = 0;
BOOLEAN do_witnesses      = FALSE;
TermT witness_l           = NULL;
TermT witness_r           = NULL;
BOOLEAN CO_texOutput      = FALSE;

BOOLEAN CO_GroundJoinable(int n, TermT s, TermT t)
{
  if (ORD_OrdIstcLPO()){
    BOOLEAN res = FALSE;
    ZM_START(2);
    {
      switch (n) {
      case 0:  
        res = FALSE; 
        break;
      case 1:  
        res = CO_GroundJoinable1(s,t); 
        break;
      case 2:  
        res = CO_GroundJoinable2(s,t); 
        break;
      case 3:  
        res = CO_GroundJoinable3(s,t); 
        break;
      case 4:  
        res = CO_GroundJoinable4(s,t); 
        break;
      case 5:  
        res = CO_GroundJoinable5(s,t); 
        break;
      case 6:  
        res = CO_GroundJoinable6(s,t); 
        break;
      case 7:  
        res = CO_GroundJoinable7(s,t); 
        break;
      case 8:  
        res = CO_GroundJoinable8(s,t); 
        break;
      case 4567:  
        res = CO_GroundJoinable4(s,t); 
        res = CO_GroundJoinable5(s,t); 
        res = CO_GroundJoinable6(s,t); 
        res = CO_GroundJoinable7(s,t); 
        break;
      case 78:  
        res = CO_GroundJoinable7(s,t); 
        if (CO_GroundJoinable8(s,t) != res){
#if 0
          IO_DruckeFlex("## %b %b\n", res, !res);
          res = TRUE;
#else
          CO_texOutput = TRUE;
          res = CO_GroundJoinable7(s,t); 
          res = CO_GroundJoinable8(s,t); 
          CO_texOutput = FALSE;
          res = TRUE;
#endif
        }
        break;
      case 10:  
        length_limit = 5;
        res = CO_GroundJoinable1(s,t); 
        length_limit = 0;
        break;
      case 11:  
        length_limit = 7;
        res = CO_GroundJoinable1(s,t); 
        length_limit = 0;
        break;
      case 12:  
        length_limit = 9;
        res = CO_GroundJoinable1(s,t); 
        length_limit = 0;
        break;
      case 20:  
        depth_limit = 4;
        res = CO_GroundJoinable2(s,t); 
        depth_limit = 0;
        break;
      case 21:  
        depth_limit = 6;
        res = CO_GroundJoinable2(s,t); 
        depth_limit = 0;
        break;
      case 22:  
        depth_limit = 8;
        res = CO_GroundJoinable2(s,t); 
        depth_limit = 0;
        break;
      case 30:  
        res = CO_GroundJoinable30(s,t); 
        break;
      case 31:  
        res = CO_GroundJoinable31(s,t); 
        break;
      case 32:  
        res = CO_GroundJoinable32(s,t); 
        break;
      case 40:  
        res = CO_GroundJoinable40(s,t); 
        break;
      case 41:  
        res = CO_GroundJoinable41(s,t); 
        break;
      case 42:  
        res = CO_GroundJoinable42(s,t); 
        break;
      case 50:  
        res = CO_GroundJoinable50(s,t); 
        break;
      case 51:  
        res = CO_GroundJoinable51(s,t); 
        break;
      case 52:  
        res = CO_GroundJoinable52(s,t); 
        break;
      case 60:  
        res = CO_GroundJoinable60(s,t); 
        break;
      case 61:  
        res = CO_GroundJoinable61(s,t); 
        break;
      case 62:  
        res = CO_GroundJoinable62(s,t); 
        break;
      case 70:  
        res = CO_GroundJoinable70(s,t); 
        break;
      case 71:  
        res = CO_GroundJoinable71(s,t); 
        break;
      case 72:  
        res = CO_GroundJoinable72(s,t); 
        break;
      case 80:  
        res = CO_GroundJoinable80(s,t); 
        break;
      default: our_error("unsupported version!"); break;
      }
    }
    ZM_STOP(2,res);
    return res;
  }
  else {
    return FALSE;
  }
}

BOOLEAN CO_GroundJoinableWithWitnesses(int n MAYBE_UNUSEDPARAM, TermT s MAYBE_UNUSEDPARAM, TermT t MAYBE_UNUSEDPARAM, 
                                       TermT *s0 MAYBE_UNUSEDPARAM, TermT *t0 MAYBE_UNUSEDPARAM)
{
  /*  our_error("not yet implemented: CO_GroundJoinableWithWitnesses");*/
  BOOLEAN res = CO_GroundJoinable(n,s,t);
  *s0 = NULL;
  *t0 = NULL;
  return res;
}

BOOLEAN CO_HandleGoal(TermT s MAYBE_UNUSEDPARAM, TermT t MAYBE_UNUSEDPARAM)
{
  our_error("no longer use for CO_HandleGoal");
  return FALSE;
}

#if 0

BOOLEAN CO_GroundJoinableWithWitnesses(int n, TermT s, TermT t, 
                                       TermT *s0, TermT *t0)
{
  BOOLEAN res;
  do_witnesses = TRUE;
  if(witness_l != NULL){
    printf("PUUUUUUUUUUH!\n"); /* Speicherleck, wenn ueber E backtracking */
  }
  /*witness_l = NULL; witness_r = NULL;*/
  res = CO_GroundJoinable(n,s,t);
  if(res){
    *s0 = NULL;
    *t0 = NULL;
  }
  else {
    *s0 = witness_l;
    *t0 = witness_r;
    witness_l = NULL;
    witness_r = NULL;
  }
  do_witnesses = FALSE;
  return res;
}




/* -------------------------------------------------------------------------- */
/* - Stuff for the TERMINATION MODE ----------------------------------------- */
/* -------------------------------------------------------------------------- */


static BOOLEAN HandleSingleGoal(TermT s, TermT t)
{
  IO_DruckeFlex("checking %t = %t...\n\n", s,t);
  {
    BOOLEAN res;
    ZM_START(2);
    {
      ContextT c;
      level = 0;
      c = CO_newContext(CO_allocBindings());
      tree_mode = Weitersuchen;
      res = handleNode(s,t,c);
      tree_mode = Abbrechen;
      CO_deepDelete(c);
    }
    ZM_STOP(2,res);
    if (res){
      printf("\n...could be joined in all instances!\n\n");
      return TRUE;
    }
    else {
      printf("\n...could not be joined in all instances!\n\n");
      return FALSE;
    }
  }
}

BOOLEAN CO_HandleGoal(TermT s, TermT t)
{
  if (!ORD_OrdIstcLPO()){
    printf("Have to use LPO\n");
    return FALSE;
  }
  IO_DruckeFlex("%t = %t\n", s,t);
  if (!TO_TermIstVar(t) &&
      (strcmp(IO_FktSymbName(TO_TopSymbol(t)),"eqn") == 0)){
    return HandleConstrainedGoal(t,s);
  }
  else if (!TO_TermIstVar(s) &&
           (strcmp(IO_FktSymbName(TO_TopSymbol(s)),"eqn") == 0)){  
    return HandleConstrainedGoal(s,t);
  }
  else {
    return HandleSingleGoal(s,t);
  }
}


static BOOLEAN HandleConstrainedGoal(TermT s, TermT t)
{
  BOOLEAN res = TRUE;
  UTermT l = TO_ErsterTeilterm(s), r = TO_ZweiterTeilterm(s);
  while (!TO_TermIstVar(t) &&
         (strcmp(IO_FktSymbName(TO_TopSymbol(t)),"or") == 0)){
    res = HandleOneConstraint(l,r,TO_ErsterTeilterm(t)) && res;
    t = TO_ZweiterTeilterm(t);
  }
  res = HandleOneConstraint(l,r,t) && res;
  return res;
}


static BOOLEAN HandleOneConstraint(TermT l, TermT r, TermT ct)
{
  EConsT e = CO_constructECons(ct);
  ContextT c = mkContextFromECons(e);
  ContextT c1 = CO_solveEQPart(c);
  ContextT cold = CO_getCurrentContext();
  TermT s = CO_substitute(c->bindings,l);
  TermT t = CO_substitute(c->bindings,r);
  BOOLEAN res;
  IO_DruckeFlex(  "consider     %t = %t under %C\n", l,r,c);
  if(CO_somethingBound(c->bindings)){
    IO_DruckeFlex("after subst: %t = %t under %C\n", s,t,c1);
  }
  CO_setCurrentContext(c1);
  NF_NormalformRE(s);
  NF_NormalformRE(t);
  IO_DruckeFlex(  "after norm.: %t = %t\n", s,t);
  if (TO_TermeGleich(s,t)){
    IO_DruckeFlex("terms are equal\n\n");
    res = TRUE;
  }
  else {
    IO_DruckeFlex("terms are not equal\n\n");
    res = FALSE;
  }
  CO_setCurrentContext(cold);
  CO_flatDelete(c1);
  CO_flatDelete(c);
  TO_TermeLoeschen(s,t);
  CO_deleteEConsList(e,NULL);
  return res;
}


static ContextT mkContextFromECons(EConsT e)
{
  ContextT c = CO_newContext(CO_allocBindings());
  while (e != NULL){
    if (e->tp == Eq_C){
      if (TO_TermIstVar(e->s)){
        c->bindings[TO_TopSymbol(e->s)] = e->t;
      }
      else {
        our_error("bei ceq(x,t) muss links eine Variable stehen");
      }
    }
    else {
      c->econses = CO_newEcons(e->s,e->t,c->econses,e->tp);
    }
    e = e->Nachf;
  }
  return c;
}

#endif
/* ------------------------------------------------------------------------- */
/* -------- E O F ---------------------------------------------------------- */
/* ------------------------------------------------------------------------- */
