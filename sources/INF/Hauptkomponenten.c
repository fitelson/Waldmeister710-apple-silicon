/* -------------------------------------------------------------------------------------------------------------*/
/* -                                                                                                            */
/* - Hauptkomponenten.c                                                                                         */
/* - *******************                                                                                        */
/* -                                                                                                            */
/* - Enthaelt die Operationen, die zum Programm-Start mit den in der KP-Menge enthaltenen Gleichungen der Spezi-*/
/* - fikation ausgefuehrt werden (I) sowie eine Prozedur, die alle KP's der Reihe nach behandelt und sie zu-    */
/* - sammenzufuehren sucht (II).                                                                                */
/* - 29.06.94 Arnim. AlsRegel/GleichungAufnehmen fertiggemacht.                                                 */
/* - 10.94/1.11.94 Arnim. Prozeduren fuer die failing completion eingebaut.                                     */
/* -------------------------------------------------------------------------------------------------------------*/
#include "antiwarnings.h"
#include "compiler-extra.h"
#include "general.h"

#include <string.h>
#include <math.h>
#include <sys/types.h>
#include <unistd.h>

#include "Ausgaben.h"
#include "Communication.h"
#include "Dispatcher.h"
#include "DCommunicator.h"
#include "DSBaumOperationen.h"
#include "Hauptkomponenten.h"
#include "Interreduktion.h"
#include "Konsulat.h"
#include "KPVerwaltung.h"
#include "Lemmata.h"
#include "LispLists.h"
#include "MissMarple.h"
#include "NFBildung.h"
#include "Ordnungen.h"
#include "Parameter.h"
#include "PhilMarlow.h"
#include "RUndEVerwaltung.h"
#include "Signale.h"
#include "SpezNormierung.h"
#include "Stringterms.h"
#include "Statistiken.h"
#include "Subsumption.h"
#include "TermOperationen.h"
#include "Termpaare.h"
#include "Unifikation1.h"
#include "WaldmeisterII.h"
#include "general.h"
#include "Zielverwaltung.h"

/* -------------------------------------------------------------------------------*/
/* - static Variablen ------------------------------------------------------------*/
/* -------------------------------------------------------------------------------*/

BOOLEAN HK_fair;

unsigned long HK_SelectZaehler = 0;

AbstractTime HK_AbsTime = 0;
AbstractTime HK_TurnTime = 0;

AbstractTime hk_SleepTime;

static StrategieT *hk_Strategie;
static BOOLEAN hk_Wechsle;


static void hk_NeuesFaktumDrucken (RegelOderGleichungsT faktum)
{
  BOOLEAN IstRegel = RE_IstRegel(faktum);
  if (IO_level2() || IO_level4()){
#if COMP_POWER
    printf("%snew %s%s%s%s%7ld ", COM_Prefix,
            RE_Bezeichnung(faktum),": "," ",IstRegel ? "    " : "", 
            RE_AbsNr(faktum));
#else
    printf("%snew %s%s%s%s%7ld ", COM_Prefix,
            RE_Bezeichnung(faktum),
            faktum->ACGrundreduzibel ? "A:" : faktum->Grundzusammenfuehrbar ? "G:" : ": ",
            faktum->StrengKonsistent ? "S" : " ",
            IstRegel ? "    " : "", 
            RE_AbsNr(faktum));
#endif
    IO_DruckeFaktum(TP_LinkeSeite(faktum), IstRegel, TP_RechteSeite(faktum));
  }
  else if (IO_level3()){
    printf("...and added as %s with number %ld.\n\n",
            TP_IstMonogleichung(faktum) ? "mono equation" : RE_Bezeichnung(faktum),
            RE_AbsNr(faktum));
  }
  else if (IO_level4()){
    printf("...and added as %s with number %ld.\n\n",
            TP_IstMonogleichung(faktum) ? "mono equation" : RE_Bezeichnung(faktum),
            RE_AbsNr(faktum));
    IO_DruckeFaktum(TP_LinkeSeite(faktum), IstRegel, TP_RechteSeite(faktum));
  }
#ifdef CCE_Protocol
  IO_CCElog(CCE_Add, faktum->linkeSeite, faktum->rechteSeite);
#endif
}


/* -------------------------------------------------------------------------------*/
/* --------------- Ziel-Behandlung -----------------------------------------------*/
/* -------------------------------------------------------------------------------*/

void HK_GleichungenUndZieleHolen(void)
{
   ListenT Index;

   for (Index=PM_Gleichungen; Index; Cdr(&Index)) {
     ListenT Eintrag = car(Index);
     KPV_Axiom(TO_Termkopie(car(Eintrag)), TO_Termkopie(cdr(Eintrag)));
   }
   Z_ZieleAusSpezifikationHolen();
}



BOOLEAN HK_ZieleAufZusammenfuehrbarkeitTesten(void)
/* Alle Ziele aus der Zielmenge bzgl. des aktuellen Regel- und
   Gleichungs-Systems abschliessend auf NF bringen. */
{
   if (Z_ZielmengeLeer())
     return TRUE;
   IO_DruckeUeberschrift("Treatment of Goals");
   return Z_FinalerZielDurchlauf();
}


/* -------------------------------------------------------------------------------*/
/* --------------- Vervollstaendigung --------------------------------------------*/
/* -------------------------------------------------------------------------------*/

VergleichsT HK_VariantentestUndVergleich(TermT links, TermT rechts)
{
  BOOLEAN Wegwerfen = (PA_GolemAn() && (Z_IstSkolemvarianteZuUngleichung(links,rechts) || 
                                        Z_IstGrundungleichung(links,rechts)));
  VergleichsT Resultat = Wegwerfen ? Gleich : ORD_VergleicheTerme(links,rechts);
  HK_fair = HK_fair && (!Wegwerfen || TO_TermeGleich(links,rechts));
  return Resultat;
}

static void hk_calcSleepTime(void)
{
  double x = (double) COM_GlobalTime() / (double) HK_TurnTime;
  double percent =  1.0/(1.0+(1.0/0.05 - 1)*exp(-2.5*x));
  if (!COM_Vadder) percent = 1.0 - percent;
  if (percent < 0.005) percent = 0.005;
  if (percent > 0.995) percent = 0.995;
  hk_SleepTime = hk_SleepTime + (int) (200.0*percent+0.5) + 1;
  if(!COMP_COMP)printf("%snext hk_SleepTime: %5ld percent = %5.3f #################\n", COM_Prefix, hk_SleepTime, percent);
}


BOOLEAN HK_TestStrategieWechsel (void)
{
  HK_SelectZaehler++;
  if (COM_forked){
    if (HK_SelectZaehler >= hk_SleepTime){
      COM_wakeOther();
      COM_sleep();
      hk_calcSleepTime();
      if (COM_getState() != COM_state_unset) 
        hk_Wechsle = TRUE;
    }
  }
  else {
    hk_Wechsle = (hk_Strategie != NULL) && 
                 (HK_SelectZaehler == hk_Strategie->Schrittzahl);
  }
  return hk_Wechsle;
}

#if IF_TERMZELLEN_JAGD
extern int PA_HACK;
#endif
HK_ErgebnisT HK_Vervollstaendigung (StrategieT *strategie)
{
  RegelOderGleichungsT zuletztEingefuegt;
  BOOLEAN Proofmodus;

  SelectRecT selRec;

  HK_fair = TRUE;
  Proofmodus = (PA_ExtModus() == proof);

  PA_PrintParas();

  if (WM_ErsterAnlauf() && COM_PrintIt()) {
    IO_DruckeUeberschrift(Proofmodus ? "COMPLETION - PROOF" : "COMPLETION");
  }

  hk_Strategie = strategie;
  HK_SelectZaehler = 0;
  hk_Wechsle = FALSE;

  while (LE_getLemma(&selRec) || KPV_Select(&selRec)){
    /*    if (HK_AbsTime == 1007) return HK_VervollAbgebrochen;*/
#if IF_TERMZELLEN_JAGD
    if (HK_AbsTime == PA_HACK) {
      fprintf(stderr,"ABBRUCH\n");
      TO_TermeLoeschen(selRec.lhs, selRec.rhs);
      HK_fair = FALSE;
      return HK_Vervollstaendigt;
    }
#endif
    if ((selRec.result == Unvergleichbar) && PA_FailingCompletion()){
      IO_DruckeFlex("Completion aborted since critical pair can't be oriented:\n %t <<##>> %t\n",
                    selRec.lhs, selRec.rhs);
      TO_TermeLoeschen(selRec.lhs, selRec.rhs);
      return HK_VervollAbgebrochen;
    }
    SUE_selectedCPRE(&selRec);
    HK_AbsTime++;
    zuletztEingefuegt = RE_ErzeugtesFaktum(selRec);
    hk_NeuesFaktumDrucken(zuletztEingefuegt);
    DIS_AddActive(zuletztEingefuegt);
    RE_FaktumEinfuegen(zuletztEingefuegt);
    BOB_BeweisFuerAktivum(zuletztEingefuegt);
    MM_untersuchen(zuletztEingefuegt);
    if (Proofmodus && Z_ZieleErledigtMitNeuemFaktum(zuletztEingefuegt)){  
      if (COM_getState() != COM_state_unset) return HK_Ueberholt;
      return HK_ZielBewiesen;
      break; /* Falls Ziele erledigt, keine KP-Bildung mehr! */
    }
    IR_NeueInterreduktion(zuletztEingefuegt); /* evtl. vorziehen, da RedRel nicht interred bei Zielbeh. */
    DIS_Flush();
    if(0)FP_ACKPsBildenZuGleichung(zuletztEingefuegt->linkeSeite, zuletztEingefuegt->rechteSeite);
    KPV_insertASet(TP_getGeburtsTag_M(zuletztEingefuegt));
    if ((PA_StatistikIntervall() > 0) &&
        (HK_AbsTime % PA_StatistikIntervall() == 0)) ST_SignalUsrStatistik("###");
    KO_Konsultieren();
  }
  
  if (COM_getState() != COM_state_unset) return HK_Ueberholt;
  if (hk_Wechsle) return HK_StrategieWechseln;
  return HK_Vervollstaendigt;
}

/* ---------------------------------------------------------------------------- */
/* ---------------------------------------------------------------------------- */
/* ---------------------------------------------------------------------------- */
/* ---------------------------------------------------------------------------- */
/* ---------------------------------------------------------------------------- */

BOOLEAN HK_modulo = FALSE;
int     HK_noOfSlaves;
int     HK_myId;

void  KPV_calcSlaveASet(void){}
void  KPV_readWeightlimit(void){}


static void hk_addActive(void)
{
  int flags;
  int generation;
  PTermpaarT tp;
  TermT lhs, rhs;
  RegelOderGleichungsT zuletztEingefuegt;

  flags = DCM_RecvInt();
  generation = DCM_RecvInt();
  tp = DCM_RecvBytes();
  STT_TermpaarAuspacken(tp,&lhs,&rhs);
  our_dealloc(tp);
  zuletztEingefuegt = RE_ErzeugeFaktumAusFertigenTeilen(flags, generation, lhs, rhs);
  if (SLAVE_VERBOSE){
    hk_NeuesFaktumDrucken(zuletztEingefuegt);
  }
  RE_FaktumEinfuegen(zuletztEingefuegt);
}

static void hk_delActives(void)
{
  unsigned long ul;
  RegelOderGleichungsT opfer;
  for (ul = DCM_RecvULong(); ul != 0; ul = DCM_RecvULong()){
    opfer = RE_getActiveWithBirthday(ul);
    RE_FaktumToeten(opfer);
    if (SLAVE_VERBOSE){
      printf("%s %ld removed.\n", RE_Bezeichnung(opfer), RE_AbsNr(opfer));
    }
    opfer->lebtNoch = FALSE;
  }
}

static void hk_gjActives(void)
{
  unsigned long ul;
  RegelOderGleichungsT opfer;
  for (ul = DCM_RecvULong(); ul != 0; ul = DCM_RecvULong()){
    opfer = RE_getActiveWithBirthday(ul);
    opfer->Grundzusammenfuehrbar = TRUE;
    opfer->DarfNichtUeberlappen = TRUE;
    if (TP_Gegenrichtung(opfer)!= NULL){
      opfer->Gegenrichtung->Grundzusammenfuehrbar = TRUE;
      opfer->Gegenrichtung->DarfNichtUeberlappen = TRUE;
    }
    if (SLAVE_VERBOSE){
      if (RE_IstRegel(opfer)){
        IO_DruckeFlex("Regel %d grundzusammenfuehrbar\n",TP_ElternNr(opfer));
      }
      else {
        IO_DruckeFlex("Gleichung %d grundzusammenfuehrbar\n",-TP_ElternNr(opfer));
      }
    }
  }
}

static void hk_AddMsg(void)
{
  HK_AbsTime++;
  hk_addActive();
  hk_delActives();
  hk_gjActives();
  KPV_readWeightlimit();
  if (HK_AbsTime % 1000 == 0){
    ST_SignalUsrStatistik("###");
  }
}

static void hk_WiWMsg(void)
{
  HK_noOfSlaves = DCM_RecvInt();
  HK_myId       = DCM_RecvInt();
  printf("There are %d slaves, I am no. %d\n", HK_noOfSlaves, HK_myId);
}

HK_ErgebnisT HK_SlaveLoop(void)
{
  BOOLEAN Ende = FALSE;
  DCM_SlaveStart();
  while (!Ende){
    switch (DCM_NextMsg()){
    case DCM_EndMsg: 
      Ende = TRUE;
      break;
    case DCM_AddMsg: 
      hk_AddMsg();
      if (HK_modulo){
        KPV_calcSlaveASet();
      }
      break;
    case DCM_ExpMsg: 
      KPV_calcSlaveASet();
      break;
    case DCM_WiWMsg: 
      hk_WiWMsg();
      break;
    case DCM_RetMsg: 
    default:
      our_error("HK_SlaveLoop: unexpected message");
    }
  }
  return HK_Beendet;
}
