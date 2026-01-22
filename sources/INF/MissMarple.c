#include "ACGrundreduzibilitaet.h"
#include "Grundzusammenfuehrung.h"
#include "SpeicherVerwaltung.h"
#include "SymbolOperationen.h"
#include "TermOperationen.h"
#include "MissMarple.h"

#ifndef CCE_Source
#include "Lemmata.h"
#include "Parameter.h"
#else
#define PA_C_Lemma() (1)
#define LE_LemmataAktivieren(x,y) /* nix tun */
#include "parse-util.h"
#endif

typedef struct {
  unsigned int gemeldet; /* bitset */
  unsigned int reagiert; /* bitset */
  RegelT       ARegel;
  GleichungsT  CGleichung;
  GleichungsT  C_Gleichung;
  RegelT       IRegel;
  RegelT       I_Regel;
} *mm_SymolInfoT;

static mm_SymolInfoT mm_SymbolInfo;

static void mm_Nullen(void)
{
  SymbolT i;
  SO_forSymboleAufwaerts(i, SO_KleinstesFktSymb, SO_GroesstesFktSymb){
    mm_SymbolInfo[i].gemeldet    = 0;
    mm_SymbolInfo[i].reagiert    = 0;
    mm_SymbolInfo[i].ARegel      = NULL;
    mm_SymbolInfo[i].CGleichung  = NULL;
    mm_SymbolInfo[i].C_Gleichung = NULL;
    mm_SymbolInfo[i].IRegel      = NULL;
    mm_SymbolInfo[i].I_Regel     = NULL;
  }
}

static void mm_Initialisieren(void)
{
  mm_SymbolInfo = our_alloc((SO_GroesstesFktSymb + 1) * sizeof(*mm_SymbolInfo));
  mm_Nullen();
}

static void mm_Amelden(SymbolT f,RegelT r)
{
  mm_SymbolInfo[f].gemeldet |= 1;
  mm_SymbolInfo[f].ARegel = r;
}

static void mm_Cmelden(SymbolT f, GleichungsT e)
{
  mm_SymbolInfo[f].gemeldet |= 2;
  mm_SymbolInfo[f].CGleichung = e;
}

static void mm_C_melden(SymbolT f, GleichungsT e) 
{
  mm_SymbolInfo[f].gemeldet |= 4;
  mm_SymbolInfo[f].C_Gleichung = e;
}

static void mm_Imelden(SymbolT f,RegelT r)
{
  mm_SymbolInfo[f].gemeldet |= 8; 
  mm_SymbolInfo[f].IRegel = r;
}

static void mm_I_melden(SymbolT f,RegelT r)
{
  mm_SymbolInfo[f].gemeldet |= 16;
  mm_SymbolInfo[f].I_Regel = r;
}

BOOLEAN MM_AC_aktiviert(SymbolT f)
{
  return SO_SymbIstFkt(f) && ((mm_SymbolInfo[f].gemeldet & 3) == 3);
}

BOOLEAN MM_ACC_aktiviert(SymbolT f)
{
  return SO_SymbIstFkt(f) && ((mm_SymbolInfo[f].gemeldet & 7) == 7);
}

BOOLEAN MM_ACCII_aktiviert(SymbolT f)
{
  return SO_SymbIstFkt(f) && ((mm_SymbolInfo[f].gemeldet & 31) == 31);
}

RegelT MM_ARegel(SymbolT f)
{
  if (SO_SymbIstVar(f)) return NULL;
  return mm_SymbolInfo[f].ARegel;
}

GleichungsT MM_CGleichung(SymbolT f)
{
  if (SO_SymbIstVar(f)) return NULL;
  return mm_SymbolInfo[f].CGleichung;
}

GleichungsT MM_C_Gleichung(SymbolT f)
{
  if (SO_SymbIstVar(f)) return NULL;
  return mm_SymbolInfo[f].C_Gleichung;
}

RegelT MM_IRegel(SymbolT f)
{
  if (SO_SymbIstVar(f)) return NULL;
  return mm_SymbolInfo[f].IRegel;
}

RegelT MM_I_Regel(SymbolT f)
{
  if (SO_SymbIstVar(f)) return NULL;
  return mm_SymbolInfo[f].I_Regel;
}

static void mm_Reagieren(SymbolT f)
{
  if (MM_ACC_aktiviert(f) && !(mm_SymbolInfo[f].reagiert & 1)){
    mm_SymbolInfo[f].reagiert |= 1;
    AC_lohntSich();
    GZ_ACC_vorhanden(f);
    if (PA_FullPermLemmata() > 0){
      LE_LemmataAktivieren(FullPermLemmata,f);
    }
    else if (PA_PermLemmata()){
      LE_LemmataAktivieren(PermLemmata,f);
    }
  }
  if (MM_ACCII_aktiviert(f) && !(mm_SymbolInfo[f].reagiert & 2)){
    mm_SymbolInfo[f].reagiert |= 2;
    GZ_ACCII_vorhanden(f);
    if (PA_IdempotenzLemmata()){
      LE_LemmataAktivieren(IdempotenzLemmata,f);
    }
  }
  if (MM_AC_aktiviert(f) && !(mm_SymbolInfo[f].reagiert & 4)){
    mm_SymbolInfo[f].reagiert |= 4;
    if (PA_C_Lemma()){
      LE_LemmataAktivieren(C_Lemma,f);
    }
  }
}

void MM_untersuchen(RegelOderGleichungsT re)
{
  TermT l = re->linkeSeite;
  TermT r = re->rechteSeite;
  SymbolT f = TO_TopSymbol(l);

  if (mm_SymbolInfo == NULL){
    mm_Initialisieren();
  }
  if (TO_IstAssoziativitaet(l,r)){
    mm_Amelden(f,re);
  }
  else if (TO_IstKommutativitaet(l,r)){
    mm_Cmelden(f,re);
  }
  else if (TO_IstKommutativitaet_(l,r)){
    mm_C_melden(f,re);
  }
  else if (TO_IstIdempotenz(l,r)){
    mm_Imelden(f,re);
  }
  else if (TO_IstIdempotenz_(l,r)){
    mm_I_melden(f,re);
  }
  else {
    return;
  }
  mm_Reagieren(f);
}

void MM_SoftCleanup(void)
{
  if (mm_SymbolInfo != NULL){
    mm_Nullen();
  }
}

void MM_Cleanup(void)
{
  if (mm_SymbolInfo != NULL){
    our_dealloc(mm_SymbolInfo);
    mm_SymbolInfo = NULL;
  }
}
