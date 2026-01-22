/**********************************************************************
* Filename : Clas_CP_Goal.c
*
* Kurzbeschreibung : CP-In-Goal und Goal-In-CP Classifikationen
*
* Autoren : Thomas Hillenbrand, Andreas Jaeger
*
* 10.03.98: Erstellung aus SH2.c heraus
*
*
*
**********************************************************************/

#include <stdlib.h>
#include "compiler-extra.h"

#include "general.h"
#include "Ausgaben.h"
#include "Clas_CP_Goal.h"
#include "DSBaumOperationen.h"
#include "MatchOperationen.h"
#include "SymbolGewichte.h"
#include "SymbolOperationen.h"
#include "TermOperationen.h"
#include "Zielverwaltung.h"

typedef enum {Nullmatch, Einfachmatch, Doppelmatch} LevelT;

typedef struct {
  LevelT Level;
  signed int Masse;
  BOOLEAN schonErgebnisse;
} ErgebnisT;

static TermT KPLinks,
             KPRechts;
static WeightT SingleMatchFactor, NoMatchFactor, MinStruct;


/* Die Teilminimierung erfolgt lexikographisch ueber Level und Masse.
   Level wird dabei umgekehrt minimiert ("je hoeher, desto minimaler").
   Die Gesamtminimierung erfolgt nur ueber Masse. */

static ErgebnisT GesamtErgebnis,
                 TeilErgebnis;

static long int phi(TermT term)
{
  SymbolT symb;
  long int weight = 0;
  TO_doSymbole(term, symb,{
    weight  += SG_SymbolGewichtCP(symb);
  });
  return weight;
}

static void GesamtminimierungBeginnen(void)
{
  GesamtErgebnis.schonErgebnisse = FALSE;
}

static void TeilminimierungBeginnen(void)
{
  TeilErgebnis.schonErgebnisse = FALSE;
}

static void TeilMinimieren(LevelT Level, signed int Masse)
{
  if (TeilErgebnis.schonErgebnisse) {
    if (TeilErgebnis.Level<Level) {
      TeilErgebnis.Level = Level;
      TeilErgebnis.Masse = Masse;
    }
    else if (TeilErgebnis.Level==Level && TeilErgebnis.Masse>Masse) 
      TeilErgebnis.Masse = Masse;
  }
  else {
    TeilErgebnis.schonErgebnisse = TRUE;
    TeilErgebnis.Level = Level;
    TeilErgebnis.Masse = Masse;
  }
}

static void GesamtMinimieren(void)
{
  if (GesamtErgebnis.schonErgebnisse) {
    if (GesamtErgebnis.Masse>TeilErgebnis.Masse)
      GesamtErgebnis.Masse = TeilErgebnis.Masse;
  }
  else {
    GesamtErgebnis.schonErgebnisse = TRUE;
    GesamtErgebnis.Masse = TeilErgebnis.Masse;
  }
}


static BOOLEAN NochKeinDoppelmatch(void)
{
  return !TeilErgebnis.schonErgebnisse || TeilErgebnis.Level<Doppelmatch;
}

#define Matching_KP(/*Bezeichner*/ Index, /*{Links; Rechts}*/ SenkeSeite, /*{"_Sigma"; "_"}*/ Sigma)                \
  MO ## Sigma ## TermIstGeneralisierung(KP ## SenkeSeite, Index)                /* KP ist Senke -> Ziel gibt Index */

#define Matching_Ziel(/*Bezeichner*/ Index, /*{Links; Rechts}*/ SenkeSeite, /*{"_Sigma"; "_"}*/ Sigma)              \
  (phi(Index)>=MinStruct &&                                                                                         \
    MO ## Sigma ## TermIstGeneralisierung(Index, Ziel ## SenkeSeite))          /* Ziel ist Senke -> KP gibt Index */
  /* Mit Ziel als Senke ist KP Quelle, d. h. Index durchlaeuft KP, also Experte GoalInCP.
     Dafuer Matchpraedikat erweitert um Bedingung AddWeight(Index)>=MinStruct */


#define Matching(/*Bezeichner*/ Index, /*{"KP"; "Ziel"}*/ Senke, /*{Links; Rechts}*/ SenkeSeite)                    \
  Matching_ ## Senke(Index, SenkeSeite, _)

#define SigmaMatching(/*Bezeichner*/ Index, /*{"KP"; "Ziel"}*/ Senke, /*{Links; Rechts}*/ SenkeSeite)               \
  Matching_ ## Senke(Index, SenkeSeite, _Sigma)            
            
            
#define RumpfXinYDurchlauffunktion(/*{"KP"; "Ziel"}*/ Quelle, /*{"KP"; "Ziel"}*/ Senke)                             \
{                                                                                                                   \
  UTermT IndexLinks = Quelle ## Links;                                                                              \
  signed long int GewichtLinks = phi(Quelle ## Links),                                                              \
                  GewichtRechts = phi(Quelle ## Rechts);                                                            \
  TeilminimierungBeginnen();                                                                                        \
  do {                                                                                                              \
    if (Matching(IndexLinks,Senke,Links)) {                                                                         \
      UTermT IndexRechts = Quelle ## Rechts;                                                                        \
      signed int TeilgewichtLinks = GewichtLinks-phi(IndexLinks);                                                   \
      TeilMinimieren(Einfachmatch,(TeilgewichtLinks+GewichtRechts)*SingleMatchFactor);                              \
      do {                                                                                                          \
        if (SigmaMatching(IndexRechts,Senke,Rechts))                                                                \
          TeilMinimieren(Doppelmatch,TeilgewichtLinks+GewichtRechts-phi(IndexRechts));                              \
      } while ((IndexRechts = TO_Schwanz(IndexRechts)));                                                            \
    }                                                                                                               \
    if (Matching(IndexLinks,Senke,Rechts)) {                                                                        \
      UTermT IndexRechts = Quelle ## Rechts;                                                                        \
      signed int TeilgewichtLinks = GewichtLinks-phi(IndexLinks);                                                   \
      TeilMinimieren(Einfachmatch,(TeilgewichtLinks+GewichtRechts)*SingleMatchFactor);                              \
      do {                                                                                                          \
        if (SigmaMatching(IndexRechts,Senke,Links))                                                                 \
          TeilMinimieren(Doppelmatch,TeilgewichtLinks+GewichtRechts-phi(IndexRechts));                              \
      } while ((IndexRechts = TO_Schwanz(IndexRechts)));                                                            \
    }                                                                                                               \
  } while ((IndexLinks = TO_Schwanz(IndexLinks)));                                                                  \
  if (NochKeinDoppelmatch()) {                                                                                      \
    UTermT IndexRechts = Quelle ## Rechts;                                                                          \
    do {                                                                                                            \
      if (Matching(IndexRechts,Senke,Links) || Matching(IndexRechts,Senke,Rechts))                                  \
        TeilMinimieren(Einfachmatch,(GewichtRechts-phi(IndexRechts)+GewichtLinks)*SingleMatchFactor);               \
    } while ((IndexRechts = TO_Schwanz(IndexRechts)));                                                              \
  }                                                                                                                 \
  TeilMinimieren(Nullmatch,(phi(KPLinks)+phi(KPRechts))*NoMatchFactor);                                             \
  GesamtMinimieren();                                                                                               \
}


static void CPinGoalDurchlauffunktion(TermT ZielLinks, TermT ZielRechts, unsigned long Zielnummer MAYBE_UNUSEDPARAM)
  RumpfXinYDurchlauffunktion(Ziel, KP)


static void GoalinCPDurchlauffunktion(TermT ZielLinks, TermT ZielRechts, unsigned long Zielnummer MAYBE_UNUSEDPARAM)
  RumpfXinYDurchlauffunktion(KP, Ziel)

/* Exportierte Funktionen */
     
#define DefiniereXinY(/*{CPinGoal, GoalinCP}*/ XinY)                                                              \
  static WeightT XinY ## intern (TermT KPLinks_, TermT KPRechts_)                                          \
  {                                                                                                               \
    BO_TermpaarNormierenAlsKP(KPLinks = KPLinks_, KPRechts = KPRechts_);                                          \
    GesamtminimierungBeginnen();                                                                                  \
    Z_ZielmengenBeobachtungsDurchlauf(XinY ## Durchlauffunktion);                                                 \
    return GesamtErgebnis.Masse;                                                                                  \
  }

DefiniereXinY(CPinGoal)

DefiniereXinY(GoalinCP)

WeightT CPinGoal (IdMitTermenT tid)
{
   return CPinGoalintern (tid->lhs, tid->rhs);
}

WeightT GoalinCP (IdMitTermenT tid)
{
   return GoalinCPintern (tid->lhs, tid->rhs);
}


void
CIGICInit(WeightT  cigic_single_match, WeightT cigic_no_match, WeightT cigic_min_struct)
{
  SingleMatchFactor = cigic_single_match;
  NoMatchFactor = cigic_no_match;
  MinStruct = cigic_min_struct;

  if (!SingleMatchFactor)
     SingleMatchFactor = 5;
  if (!NoMatchFactor)
     NoMatchFactor = 50;
  if (!MinStruct)
     MinStruct = 4;
#if !COMP_POWER
  printf("  Goal-CP/CP-Goal (sm=%ld, nm=%ld, ms=%ld)\n",
	  SingleMatchFactor, NoMatchFactor, MinStruct);
#endif
}
     
