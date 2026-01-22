/* ------------------------------------------------------------------------- */
/*                                                                           */
/* Interreduktion.c                                                          */
/*                                                                           */
/* Interreduktion der Regel- und Gleichungsmenge                             */
/*                                                                           */
/* --------------------------------------------------------------------------*/

#include "Interreduktion.h"

#include "Ausgaben.h"
#include "compiler-extra.h"
#include "Dispatcher.h"
#include "DSBaumOperationen.h"
#include "Grundzusammenfuehrung.h"
#include "Hauptkomponenten.h"
#include "KPVerwaltung.h"
#include "MatchOperationen.h"
#include "NFBildung.h"
#include "RUndEVerwaltung.h"
#include "Statistiken.h"
#include "Subsumption.h"
#include "TermOperationen.h"
#include "Termpaare.h"

/* ----- Zwischenpuffer ---------------------------------------------------- */

typedef struct {
  RegelOderGleichungsT *xs;
  unsigned long size;
  unsigned long current;
} ZwischenpufferT;

static ZwischenpufferT ir_Zwischenpuffer = {NULL, 0, 0};

static void ir_inZwischenpufferAufnehmen(RegelOderGleichungsT x)
{
  if (ir_Zwischenpuffer.current == ir_Zwischenpuffer.size){
    ir_Zwischenpuffer.size += 100;
    ir_Zwischenpuffer.xs = 
      our_realloc(ir_Zwischenpuffer.xs, 
                  ir_Zwischenpuffer.size * sizeof(RegelOderGleichungsT), 
                  "extending ir_Zwischenpuffer");
  }
  ir_Zwischenpuffer.xs[ir_Zwischenpuffer.current] = x;
  ir_Zwischenpuffer.current++;
}

static void ir_ZwischenpufferAbraeumen(void)
{
  unsigned long index;
  unsigned long current = ir_Zwischenpuffer.current;
  for (index = 0; index < current; index++){
    KPV_IROpferBehandeln(ir_Zwischenpuffer.xs[index]);
  }
  ir_Zwischenpuffer.current = 0;
}

/* ----- Ausgaben ---------------------------------------------------------- */

static void ir_DruckeGeloescht(RegelOderGleichungsT re)
{
  if ((IO_level4()) || (IO_level3())|| (IO_level2())){
    IO_DruckeFlex("%s %d removed.\n", RE_Bezeichnung(re), RE_AbsNr(re));
  }
} 

static void ir_DruckeGrundzusammengefuehrt(RegelOderGleichungsT re)
{
  if ((IO_level4()) || (IO_level3())|| (IO_level2())){
    IO_DruckeFlex("%s %d ground joined.\n", RE_Bezeichnung(re), RE_AbsNr(re));
  }
} 

/* ----- Anwendungsprozeduren ---------------------------------------------- */

static void ir_OpferUmbringen(RegelOderGleichungsT opfer)
{
  RE_FaktumToeten(opfer);
  DIS_DelActive(opfer);
  ir_DruckeGeloescht(opfer);
#ifdef CCE_Protocol
  IO_CCElog(CCE_Delete, r->CCE_lhs, r->CCE_rhs);
  TO_TermeLoeschen(r->CCE_lhs, r->CCE_rhs);
  r->CCE_lhs = NULL; r->CCE_rhs = NULL;
#endif
}

static void ir_OpferGrundzusammenfuehren(RegelOderGleichungsT opfer)
{
  if (GZ_ZSFB_BEHALTEN){
    KPV_KillParent(opfer); /* XXXX */
    opfer->Grundzusammenfuehrbar = TRUE;
    opfer->DarfNichtUeberlappen = TRUE;
    if (RE_IstGleichung(opfer)){
      opfer->Gegenrichtung->Grundzusammenfuehrbar = TRUE;
      opfer->Gegenrichtung->DarfNichtUeberlappen = TRUE;
    }
  }
  else {
    RE_FaktumToeten(opfer);
  }
  DIS_GJActive(opfer);
  ir_DruckeGrundzusammengefuehrt(opfer);
}

/* ----- Durchlauchroutinen ------------------------------------------------ */

static void ir_Interreduktion(unsigned long firstIndex, 
                              unsigned long lastIndex,
                              RegelOderGleichungsT neuesObjekt)
/* Eigentliche Interreduktion */
{
  BOOLEAN neuesObjektIstGleichung = RE_IstGleichung(neuesObjekt);
  unsigned long index;

  /* exclude lastIndex because of RE_getActive(lastIndex) == neuesObjekt */
  for (index = firstIndex; index < lastIndex; index++){  
    RegelOderGleichungsT opfer = RE_getActive (index);
    if (TP_lebtAktuell_M(opfer)){/* potentielles IR-Opfer */
      if (neuesObjektIstGleichung 
          && RE_IstGleichung(opfer)
          && SS_TermpaarSubsummiertTermpaar(TP_LinkeSeite(neuesObjekt), 
                                            TP_RechteSeite(neuesObjekt),
					    TP_LinkeSeite(opfer), 
                                            TP_RechteSeite(opfer))){
	ir_OpferUmbringen(opfer);
	IncAnzErfSubsumptionenAlt();
	/* nicht mehr fuer P vormerken! */
      }
      else if (NF_ObjektAnwendbar(neuesObjekt, TP_LinkeSeite(opfer), opfer->laengeLinks) ||
	       NF_ObjektAnwendbar(neuesObjekt, TP_RechteSeite(opfer), opfer->laengeRechts)){
	ir_OpferUmbringen(opfer);
	/* Inc irgendwas */
	ir_inZwischenpufferAufnehmen(opfer);
      }
    }
  }
}

static void ir_GZFBRueckwaerts(unsigned long firstIndex, 
                               unsigned long lastIndex,
                               RegelOderGleichungsT neuesObjekt)
{
  unsigned long index;
  if(!GZ_doGZFB())  return;
  if(!GZ_ZSFB_BEHALTEN) return;

  /* exclude lastIndex because of RE_getActive(lastIndex) == neuesObjekt */   
  for (index = firstIndex; index < lastIndex; index++){  
    RegelOderGleichungsT opfer = RE_getActive (index);
    if (TP_lebtAktuell_M(opfer) &&
        (!GZ_ZSFB_BEHALTEN || !opfer->Grundzusammenfuehrbar) && 
        GZ_GrundzusammenfuehrbarRueckwaerts(opfer,neuesObjekt)){
      ir_OpferGrundzusammenfuehren(opfer);
    }
  }
}

/* ------------------------------------------------------------------------- */
/* ----- Schnittstelle nach aussen ----------------------------------------- */
/* ------------------------------------------------------------------------- */

/* Wichtig: Der Zwischenpuffer wird benoetigt, um eine genau
 * definierte Rewrite-Relation beim Einfuegen in P zu haben.
 */
void IR_NeueInterreduktion(RegelOderGleichungsT neuesObjekt)
{
  unsigned long firstIndex = RE_ActiveFirstIndex();
  unsigned long lastIndex  = RE_ActiveLastIndex();

  if (RE_getActive(lastIndex) != neuesObjekt){
    our_error("violation of internal invariant (IR_NeueInterreduktion)");
  }

  ir_Interreduktion(firstIndex, lastIndex, neuesObjekt);
  ir_ZwischenpufferAbraeumen();
  ir_GZFBRueckwaerts(firstIndex, lastIndex, neuesObjekt);
}

/* ------------------------------------------------------------------------- */
/* ----- Test auf Killer-Eigenschaft --------------------------------------- */
/* ------------------------------------------------------------------------- */

static BOOLEAN ir_TermpaarAnwendbar (TermT lhs, TermT rhs)
{
  RegelOderGleichungsT Index;

  RE_forRegelnRobust(Index, {
    if (NF_TermpaarAnwendbar(lhs, rhs, TP_LinkeSeite(Index)))
      return TRUE;
  });
  RE_forGleichungenRobust(Index, {
    if (NF_TermpaarAnwendbar(lhs, rhs, TP_LinkeSeite(Index))) {
      /* reicht, da jede Seite indiziert ist */
      return TRUE;
    }
  });
  return FALSE;
}

BOOLEAN IR_TPRReduziertRE(TermT Links, TermT Rechts)
{
  unsigned int dummy;
  /* wg. Anpassung der Variablenbaenke wird BO_TermpaarNormieren und nicht
     BO_TermpaarNormierenAlsKP verwendet. */
  BO_TermpaarNormieren (Links, Rechts, &dummy);
  return ir_TermpaarAnwendbar (Links, Rechts);
}

BOOLEAN IR_TPEReduziertRE(TermT Links, TermT Rechts)
{
  unsigned int dummy;
  /* wg. Anpassung der Variablenbaenke wird BO_TermpaarNormieren und nicht
     BO_TermpaarNormierenAlsKP verwendet. */
  BO_TermpaarNormieren (Links, Rechts, &dummy);
  if (ir_TermpaarAnwendbar (Links, Rechts))
    return TRUE;
  BO_TermpaarNormieren (Rechts, Links, &dummy);
  return ir_TermpaarAnwendbar (Rechts, Links);
}

/* ------------------------------------------------------------------------- */
/* ----- EOF --------------------------------------------------------------- */
/* ------------------------------------------------------------------------- */
