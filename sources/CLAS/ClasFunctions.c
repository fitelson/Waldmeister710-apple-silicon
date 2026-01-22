/**********************************************************************
* Filename : ClasFunctions
*
* Kurzbeschreibung : Funktionen zur Classifikation, LowLevel Routinen,
*                    wird von GLAM und ClasHeuristics benutzt
*
* Autoren : Andreas Jaeger
*
* 02.03.98: Erstellung
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
#include "DSBaumOperationen.h"
#include "Interreduktion.h"
#include "SymbolOperationen.h"
#include "SymbolGewichte.h"
#include "Ordnungen.h"

WeightT CF_Phi (TermT term, BOOLEAN isCP)
{
  SymbolT symb;
  WeightT weight = 0;
  SG_GewichtstafelT Tafel = isCP ? Sg_CP_Tafel : Sg_CG_Tafel;
  TO_doSymbole(term, symb, {
    weight  += SG_SymbolGewicht(Tafel,symb);
  });
  return weight;
}


WeightT CF_Phi_KBO (TermT term)
{
  SymbolT symb;
  WeightT weight = 0;
  TO_doSymbole(term, symb, {
    weight  += SG_SymbolGewichtKBO (symb);
  });
  return weight;
}


/* gewichteter Term, Skoleminhalte auf 0 */
WeightT CF_GolemGewicht (TermT Term, BOOLEAN isCP)
{
  SG_GewichtstafelT Tafel = isCP ? Sg_CP_Tafel : Sg_CG_Tafel;
  WeightT gew = 0;
  UTermT nullTerm = TO_NULLTerm (Term);
  UTermT index = Term;

  do {
    gew += SG_SymbolGewicht(Tafel,TO_TopSymbol(index));
    if (TO_IstSkolemTerm (index)) {
      index = TO_NaechsterTeilterm (index);
    } else {
      index = TO_Schwanz (index);
    }
  } while (TO_PhysischUngleich (index, nullTerm));
  return gew;
}


/* ist Termpaar richtbar und kann RE reduzieren */
BOOLEAN CF_KillerPraedikatR (TermT lhs, TermT rhs,
			     RegelOderGleichungsT actualParent MAYBE_UNUSEDPARAM,
			     RegelOderGleichungsT otherParent MAYBE_UNUSEDPARAM)
{
  switch (ORD_VergleicheTerme(lhs, rhs))
    {
    case Kleiner:
      if (!IR_TPRReduziertRE(rhs, lhs))
	return FALSE;
    break;
    case Groesser:
      if (!IR_TPRReduziertRE(lhs, rhs))
	return FALSE;
      break;
    default:
      return FALSE;
      break;
    }
  printf("Killer-KP!\n");
  return TRUE;
}


/* ist Termpaar unrichtbar und kann RE reduzieren */
BOOLEAN CF_KillerPraedikatE (TermT lhs, TermT rhs,
			     RegelOderGleichungsT actualParent MAYBE_UNUSEDPARAM,
			     RegelOderGleichungsT otherParent MAYBE_UNUSEDPARAM)
{
  if ((ORD_VergleicheTerme(lhs, rhs) == Unvergleichbar) &&
      (IR_TPEReduziertRE(lhs, rhs)))
    {
      printf("Killer-KP!\n");
      return TRUE;
    }
  return FALSE;
}


/* kann Termpaar RE reduzieren */
BOOLEAN CF_KillerPraedikatRE (TermT lhs, TermT rhs,
			      RegelOderGleichungsT actualParent MAYBE_UNUSEDPARAM,
			      RegelOderGleichungsT otherParent MAYBE_UNUSEDPARAM)
{
  switch (ORD_VergleicheTerme(lhs, rhs))
    {
    case Kleiner:
      if (!IR_TPRReduziertRE(rhs, lhs))
	return FALSE;
      break;
    case Groesser:
      if (!IR_TPRReduziertRE(lhs, rhs))
	return FALSE;
      break;
    case Unvergleichbar:
      if (!IR_TPEReduziertRE(lhs, rhs))
	return FALSE;
    default:
      return FALSE;
      break;
    }
  printf("Killer-KP!\n");
  return TRUE;
}

BOOLEAN CF_EChildPraedikat(TermT lhs MAYBE_UNUSEDPARAM, TermT rhs MAYBE_UNUSEDPARAM, 
		   RegelOderGleichungsT actualParent, RegelOderGleichungsT otherParent)
{
  return (RE_IstGleichung(actualParent) || RE_IstGleichung(otherParent));
}



static SymbolT Mal,
               Plus;

static unsigned int Summengrad(TermT t)
{
  return TO_Symbollaenge(Plus,t)+1; /* fuer t=0 eigentlich zu viel */
}

static unsigned int Monomgrad(TermT t)
{
  SymbolT f = TO_TopSymbol(t);
  if (SO_SymbIstVar(f))
    return 1;
  else if (SO_SymbGleich(f,Mal)) {
    UTermT NULLt = TO_NULLTerm(t);
    unsigned int Ergebnis = 2;
    t = TO_Schwanz(t);
    do {
      SymbolT g = TO_TopSymbol(t);
      if (SO_SymbGleich(g,Mal))
        Ergebnis++;
      else if (SO_SymbIstFkt(g))
        return 1;                          /* kann bei gewaehlter Ordnung ueberhaupt nur in Anfangsphase auftreten */
    } while (TO_PhysischUngleich(t = TO_Schwanz(t),NULLt));
    return Ergebnis;
  }
  else {
    UTermT NULLt = TO_NULLTerm(t);
    unsigned int Ergebnis = 1;
    t = TO_ErsterTeilterm(t);
    while (TO_PhysischUngleich(t,NULLt)) {
      Maximieren(&Ergebnis,Monomgrad(t));
      t = TO_NaechsterTeilterm(t);
    }
    return Ergebnis;
  }
}

int CF_AnzahlVariablen(TermT s, TermT t)
{
  SymbolT sym,max = 0;
  BO_TermpaarNormierenAlsKP(s,t);
  TO_doSymbole(s,sym,{if (sym < max) max = sym;});
  TO_doSymbole(t,sym,{if (sym < max) max = sym;});
  return -max;
}

BOOLEAN CF_PolyZuGross(TermT s, TermT t)
{
  unsigned int S = max(Summengrad(s),Summengrad(t)),
               M = max(Monomgrad(s),Monomgrad(t)),
               V = CF_AnzahlVariablen(s,t);
  BOOLEAN Erlaubt = (S<=3 ? M<=4 : S<=5 && M<=3) && V<=4;
  return !Erlaubt;
}

BOOLEAN CF_PolyZuGross2(TermT s, TermT t)
{
  unsigned int S = max(Summengrad(s),Summengrad(t)),
               M = max(Monomgrad(s),Monomgrad(t)),
               V = CF_AnzahlVariablen(s,t);
  BOOLEAN Erlaubt = (S<=4 ? M<=5 : S<=6 && M<=4) && V<=6;
  return !Erlaubt;
}


void CF_PolyWeightInitialisieren(void)
{
  SymbolT f;
  Mal = Plus = SO_ErstesVarSymb;
  SO_forFktSymbole(f) {
    SymbolT g = SO_DistribuiertUeber(f);
    if (SO_SymbUngleich(g,SO_ErstesVarSymb)) {
      Mal = f;
      Plus = g;
      return;
    }
  }
  our_error("Polyweight ohne Vorliegen von Distributivgesetzen aufgerufen!");
}

unsigned int CF_Generation(RegelOderGleichungsT fadder, RegelOderGleichungsT mudder)
{
  unsigned int genv = 0;
  unsigned int genm = 0;
  unsigned int gen = 0;
  if (fadder != NULL)
    genv = TP_GetGeneration(fadder);
  if (mudder != NULL)
    genm = TP_GetGeneration(mudder);
  return (gen+1)+(genv+1)*(genm+1)*(1+min(genv,genm));
}
