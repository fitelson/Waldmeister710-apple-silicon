/**********************************************************************
* Filename : ClasPatch
*
* Kurzbeschreibung : Classificationsheuristiken - Spielwiese
*                    
* Autoren : Andreas Jaeger,...
*
* 25.03.98: Erstellung
* 
*
*
**********************************************************************/


#include <stdlib.h>
#include <string.h>
#include "compiler-extra.h"
#include <limits.h>

#include "Ausgaben.h"
#include "ClasFunctions.h"
#include "ClasHeuristics.h"
#include "ClasPatch.h"
#include "Ordnungen.h"
#include "SymbolGewichte.h"
#include "SymbolOperationen.h"
#include "TermOperationen.h"
#include "Unifikation1.h"
#include "Unifikation2.h"
#include "general.h"
#include "DSBaumOperationen.h"

/* Zum Berechnen der transitiven Huelle. */ 
static void RoyWarshall (BOOLEAN *R, unsigned int n)
{
  unsigned int i,j,k;
  for (i=0; i<n; i++)
    for (j=0; j<n; j++)
      if (R[j*n+i])
	for (k=0; k<n; k++)
	  R[j*n+k] = R[j*n+k] || R[i*n+k];
}

static void RelationInit (BOOLEAN *R, unsigned int n)
{
  unsigned int i,j;
  for (i=0; i<n; i++)
    for (j=0; j<n; j++)
      R[i*n+j] = FALSE;
}

static unsigned int Diagonaleinsen (BOOLEAN *R, unsigned int n)
{
  unsigned int i,r=0;
  for (i=0; i<n; i++)
    if (R[i*n+i])
      r++;
  return r;
}
static void RelationSetzen(BOOLEAN *linksvon, unsigned int n, UTermT t);

static void ACTeiltermBehandlen(BOOLEAN *linksvon, unsigned int n, SymbolT f, UTermT t)
{
  SymbolT x = f;
  BOOLEAN letzteRunde = FALSE;
  for(;;) {
    UTermT t1 = !letzteRunde ? TO_ErsterTeilterm(t) : TO_ZweiterTeilterm(t);

    SymbolT y = TO_TopSymbol(t1);
    if (SO_SymbIstVar(y)){
      if (SO_SymbUngleich(x,f) && SO_SymbUngleich(x,y))
	linksvon[(SO_VarNummer(x)-1)*n+(SO_VarNummer(y)-1)] = TRUE;
      x = y;
    }
    else
      RelationSetzen(linksvon,n,t1);
    
    if (letzteRunde) return;

    if (SO_SymbGleich(f,TO_TopSymbol(TO_ZweiterTeilterm(t))))
      t = TO_ZweiterTeilterm(t);
    else
      letzteRunde = TRUE;
  } 
}
static void RelationSetzen(BOOLEAN *linksvon, unsigned int n, UTermT t)
{
  UTermT NULLt = TO_NULLTerm(t);
  do { 
    SymbolT x = TO_TopSymbol(t);
    if (SO_IstACSymbol(x)){
      ACTeiltermBehandlen(linksvon, n, x, t);
      t = TO_TermEnde(t);
    }
  }
  while (TO_PhysischUngleich(t = TO_Schwanz(t),NULLt));
}
static int ACKonsistenz (TermT l, TermT r)
{
  unsigned int n = CF_AnzahlVariablen(l,r);
  if (n<2)
    return 0;
  else {
    unsigned int res;
    BOOLEAN *linksvon = array_alloc(n*n,BOOLEAN);
    RelationInit(linksvon,n);
    RelationSetzen(linksvon,n,l);
    RelationSetzen(linksvon,n,r);
    RoyWarshall(linksvon,n);
    res = (Diagonaleinsen(linksvon,n)*99) / n;
    array_dealloc(linksvon);
    return res;
  }
}

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

static int CPvariante = 0;
static int CGvariante = 0;



void CPA_CPInit (int no)
{
  CPvariante = no;
  printf("  Spielwiese fuer CPs: Variante %d initialisiert.\n", no);
}


void CPA_CGInit (int no)
{
  CGvariante = no;
  printf("  Spielwiese fuer CGs: Variante %d initialisiert.\n", no);
}


WeightT CPA_CPPatch (IdMitTermenT tid)
{
  switch (CPvariante)
    {
      /* bitte die Nummern nicht aendern, sondern neue Vergeben */
    case 17:
      return CH_MixWeight(tid) * 100 + ACKonsistenz(tid->lhs,tid->rhs);
    case 18:
      return 3*CF_AnzahlVariablen(tid->lhs,tid->rhs)+CF_Phi(tid->lhs,tid->isCP)+CF_Phi(tid->rhs,tid->isCP);
    case 19:
      {
        WeightT res;
        WeightT phi1 = CF_Phi (tid->lhs, tid->isCP),
                phi2 = CF_Phi (tid->rhs, tid->isCP);
        switch (ORD_VergleicheTerme(tid->lhs, tid->rhs))
          {
#define GROSS 1.333333
#define KLEIN 1.0
          case Groesser:
            res =(WeightT)( GROSS*(double)phi1 + KLEIN*(double)phi2);
            break;
          case Kleiner:
            res = (WeightT)(KLEIN*(double)phi1 + GROSS*(double)phi2);
            break;
          default:
            res = (WeightT)(GROSS*(double)phi1 + GROSS*(double)phi2);
            break;
          }
        return res;
      }
    case 20:
      return CF_AnzahlVariablen(tid->lhs,tid->rhs) <= 4 ? 
      CH_GtWeight(tid) :
      3*CF_AnzahlVariablen(tid->lhs,tid->rhs) + CH_GtWeight(tid);
    case 21:
      return CH_GtWeight(tid)+
	     CF_Generation(RE_getActiveWithBirthday(ID_i(tid->id)),
			   RE_getActiveWithBirthday(ID_j(tid->id)));
    case 30:
      {
        WeightT res;
        switch (ORD_VergleicheTerme(tid->lhs, tid->rhs))
          {
          case Groesser:
            res = CF_Phi (tid->lhs, tid->isCP);
            if (SO_Stelligkeit(TO_TopSymbol(tid->lhs)) == 2){
              /*              IO_DruckeFlex("Patsch %t = %t\n", lhs, rhs);*/
              res = res  / 2;
            }
            break;
          case Kleiner:
            res = CF_Phi (tid->rhs, tid->isCP);
            if (SO_Stelligkeit(TO_TopSymbol(tid->rhs)) == 2){
              /*              IO_DruckeFlex("Patsch2 %t = %t\n", lhs, rhs);*/
              res = res / 2;
            }
            break;
          default:
            res = CF_Phi (tid->lhs, tid->isCP) + CF_Phi (tid->rhs, tid->isCP);
            break;
          }
        return res;
      }
    default:
      our_error ("Nicht implementiert!");
      break;
    }
  
  return 1;
}


WeightT CPA_CGPatch (IdMitTermenT tid)
{
  switch (CGvariante)
    {
      case 1: {
        BOOLEAN unifizierbar = U2_NeuUnifiziert(tid->lhs,tid->rhs);
        WeightT Resultat = unifizierbar ? 0 : CH_AddWeight(tid);
        return Resultat;
      }
        
    default:
      our_error ("Nicht implementiert!");
      break;
    }
  
  return 1;
}
