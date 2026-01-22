/**********************************************************************
* Filename : ClasHeuristics.
*
* Kurzbeschreibung : Heuristiken zur Classifikation
*
* Autoren : Andreas Jaeger
*
* 10.03.98: Erstellung
*
*
*
**********************************************************************/

#include <stdlib.h>
#include <string.h>
#include "compiler-extra.h"

#include "general.h"
#include "Ausgaben.h"
#include "ClasFunctions.h"
#include "ClasHeuristics.h"
#include "Ordnungen.h"
#include "Unifikation1.h"

/* Classifikationsroutinen haben alle folgende Aufrufsyntax:
   wtPtr: WeightEntryT
   lhs: TermT
   rhs: TermT
   actualParent: RegelOderGleichungsT
   otherParent: RegelOderGleichungsT
   actualPosition: UTermT : Position in actualParent?
Wo?-> CG/CP
   type ist ClassifyProcT
(WeightEntryT *wtPtr, TermT lhs, TermT rhs, RegelOderGleichungsT actualParent, RegelOderGleichungsT otherParent, UTermT actualPosition);
*/


void ch_Tid2LhsRhs(IdMitTermenT tid, UTermT* lhs, UTermT *rhs)
{
  if (tid->isCP){
    *lhs = tid->lhs;
    *rhs = tid->rhs;
  }
  else {
    Z_UngleichungEntmanteln(lhs,rhs,tid->lhs,tid->rhs);
  }
}

WeightT CH_AddWeight (IdMitTermenT tid)
{
  UTermT lhs, rhs;
  ch_Tid2LhsRhs(tid,&lhs,&rhs);
  return CF_Phi (lhs, tid->isCP) + CF_Phi (rhs, tid->isCP); 
}


WeightT CH_OrdWeight (IdMitTermenT tid)
{
  UTermT lhs, rhs;
  ch_Tid2LhsRhs(tid,&lhs,&rhs);
  return CF_Phi_KBO (lhs) + CF_Phi_KBO (rhs);
}

WeightT
CH_GtWeight (IdMitTermenT tid)
{
  WeightT res;
  UTermT lhs, rhs;
  ch_Tid2LhsRhs(tid,&lhs,&rhs);

#if 0
  {
    WeightT phi1 = CF_Phi (lhs, tid->isCP),
            phi2 = CF_Phi (rhs, tid->isCP);

    if ((phi1 <= 5) && (phi2 <= 5)){ /* Bevorzugen der 3er Permuatationen */
      return min (phi1,phi2);
    }
  }
#endif
  
   switch (ORD_VergleicheTerme(lhs, rhs))
     {
     case Groesser:
       res = CF_Phi (lhs, tid->isCP);
       break;
     case Kleiner:
       res = CF_Phi (rhs, tid->isCP);
       break;
     default:
       res = CF_Phi (lhs, tid->isCP) + CF_Phi (rhs, tid->isCP);
       break;
     }
   return res;
}


WeightT
CH_MixWeight (IdMitTermenT tid)
{
  WeightT w_lhs, w_rhs;
  WeightT res;
  UTermT lhs, rhs;
  ch_Tid2LhsRhs(tid,&lhs,&rhs);

  w_lhs = CF_Phi (lhs, tid->isCP);
  w_rhs = CF_Phi (rhs, tid->isCP);
  
  switch (ORD_VergleicheTerme(lhs, rhs))
    {
    case Groesser:
      res = w_lhs;
      break;
    case Kleiner:
      res = w_rhs;
      break;
    default:
      res = w_lhs + w_rhs;
      break;
    }
  return (w_lhs + w_rhs) * res + res + (w_lhs + w_rhs);
}

WeightT
CH_MixWeight2 (IdMitTermenT tid)
{
  WeightT w_lhs, w_rhs;
  WeightT res;
  UTermT lhs, rhs;
  ch_Tid2LhsRhs(tid,&lhs,&rhs);

  w_lhs = CF_Phi (lhs, tid->isCP);
  w_rhs = CF_Phi (rhs, tid->isCP);
  
  switch (ORD_VergleicheTerme(lhs, rhs))
    {
    case Groesser:
      res = w_lhs;
      break;
    case Kleiner:
      res = w_rhs;
      break;
    default:
      res = w_lhs + w_rhs;
      break;
    }
  return res * 10 + (w_lhs + w_rhs);
}


WeightT
CH_MaxWeight (IdMitTermenT tid)
{
  WeightT w_lhs, w_rhs;
  UTermT lhs, rhs;
  ch_Tid2LhsRhs(tid,&lhs,&rhs);
  
  w_lhs = CF_Phi (lhs, tid->isCP);
  w_rhs = CF_Phi (rhs, tid->isCP);

  return max (w_lhs, w_rhs);
}


/* prod(plus(length), plus(unif)) */
WeightT
CH_Unifikationsmass (IdMitTermenT tid)
{
  WeightT w_lhs, w_rhs;
  UTermT lhs, rhs;
  ch_Tid2LhsRhs(tid,&lhs,&rhs);

  w_lhs = CF_Phi (lhs, tid->isCP);
  w_rhs = CF_Phi (rhs, tid->isCP);

  return (w_lhs + w_rhs) * U1_Unifikationsmass(lhs, rhs);
}


WeightT
CH_Fifo (IdMitTermenT tid MAYBE_UNUSEDPARAM)
{
    static WeightT fifo_zaehler = 0;
    return fifo_zaehler++;  
}


WeightT
CH_Golem (IdMitTermenT tid)
{
  UTermT lhs, rhs;
  ch_Tid2LhsRhs(tid,&lhs,&rhs);
  return CF_GolemGewicht (lhs, tid->isCP) + CF_GolemGewicht (rhs, tid->isCP);
}

WeightT CH_PolyWeight (IdMitTermenT tid)
{
  WeightT res, lphi, rphi;
  UTermT lhs, rhs;
  ch_Tid2LhsRhs(tid,&lhs,&rhs);
  
  lphi = CF_Phi (lhs, tid->isCP);
  rphi = CF_Phi (rhs, tid->isCP);

  if (1||(min(lphi,rphi) < 30)){
    switch (ORD_VergleicheTerme(lhs, rhs)){
    case Groesser:
      res = lphi;
      break;
    case Kleiner:
      res = rphi;
      break;
    default:
      res = lphi + rphi;
      break;
    }
  }
  else {
    res = min (lphi, rphi);
  }
  res *= CF_PolyZuGross(lhs,rhs) ? 10 : 1;
  return res;
}

WeightT CH_PolyWeight2 (IdMitTermenT tid)
{
  WeightT res, lphi, rphi;
  UTermT lhs, rhs;
  ch_Tid2LhsRhs(tid,&lhs,&rhs);
  
  lphi = CF_Phi (lhs, tid->isCP);
  rphi = CF_Phi (rhs, tid->isCP);

  if (1||(min(lphi,rphi) < 30)){
    switch (ORD_VergleicheTerme(lhs, rhs)){
    case Groesser:
      res = lphi;
      break;
    case Kleiner:
      res = rphi;
      break;
    default:
      res = lphi + rphi;
      break;
    }
  }
  else {
    res = min (lphi, rphi);
  }
  if (CF_PolyZuGross2(lhs,rhs)){
    res *= 5;
  }
  else if (CF_PolyZuGross(lhs,rhs)){
    res *= 3;
  } 
  else {
    res *= 2;
  }
  return res;
}

