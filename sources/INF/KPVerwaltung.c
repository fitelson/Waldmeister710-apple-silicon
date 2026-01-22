/* **************************************************************************
 * KPVerwaltung.c  Version 8: die KISS (Keep It Simple Stupid) Version
 * 
 * die Fakten werden nach dem Prinzip des NextWMLoops verwaltet 
 * Für CP und CG je ein Cache für Membereinträge und ein Heap für Axiome,
 * A-Sets und Interreduktionsopfer.
 * Fifo wird zunächst vernachlässigt und evtl. später nachgezogen.
 *
 * Modul zur Verwaltung der passiven Fakten.
 *
 * Bernd Loechner, 21.04.2001
 * auf KISS angepasst Hendrik Spies 22.05.2003
 *
 * ************************************************************************** */
 

#include "KPVerwaltung.h"



#include "Ausgaben.h"
#include "Axiome.h"
#include "AxiomZapfhahn.h"
#include "Communication.h"
#include "DSBaumOperationen.h"
#include "Fass.h"
#include "FIFO.h"
#include "general.h"
#include "Grundzusammenfuehrung.h"
#include "Hauptkomponenten.h"
#include "Id.h"
#include "KissKDHeap.h"
#include "NewClassification.h"
#include "NFBildung.h"
#include "Parameter.h"
#include "Position.h"
#include "RUndEVerwaltung.h"
#include "Statistiken.h"
#include "Subsumption.h"
#include "TermOperationen.h"
#include "TankAnlage.h"
#include "Unifikation1.h"
#include "Zapfhahn.h"
#include "ZapfAnlage.h"
#include "Zielverwaltung.h"

static struct ZapfhahnS axzcg,axzcp;

/* Zum Melden von Interreduzierten Gleichungen, auch subsumierte werden gemeldet, die
   keine IROpfer erzeugen*/
void KPV_KillParent(RegelOderGleichungsT re)
{
  re->lebtNoch = FALSE;
  if (TP_Gegenrichtung(re) != NULL)
    TP_Gegenrichtung(re)->lebtNoch = FALSE;
}


static void KPinitialDrucken(TermT lhs, TermT rhs)
{
  if(!COM_PrintIt()) return;
   if (IO_level3())
      printf("initial equation %Lu turned into ues %Lu.\n", GetKPV_AnzKPAusSpezifikation(), GetKPV_AnzKPGesamt());
   else if (IO_level4()){
      printf("initial equation %Lu turned into ues %Lu.\n", GetKPV_AnzKPAusSpezifikation(), GetKPV_AnzKPGesamt());
      IO_DruckeFlex("       %t # %t\n", lhs, rhs);
   }
   else if (IO_level2()){
      if (!IO_GUeberschrift){
	 printf("\nInitial equations:\n" );
	 printf("------------------\n\n" );
	 IO_GUeberschrift = TRUE;
      }
      printf("(%4Ld)  ", GetKPV_AnzKPAusSpezifikation());
      IO_DruckeFlex("%t = %t\n", lhs, rhs);
   }
}

/* ---------------------------------------------------------------------------*/
/* ------- Insert ------------------------------------------------------------*/
/* ---------------------------------------------------------------------------*/

void KPV_Axiom(TermT lhs, TermT rhs)
{
    KPV_IncAnzKPGesamt(); 
    KPV_IncAnzKPAusSpezifikation();
    KPinitialDrucken(lhs,rhs);
    AX_AddAxiom(lhs,rhs);
}

/*KPV_insertASet bekommt den Index eines neu aufgenommenen Faktums uebergeben
  und fuegt einen entsprechenden Eintrag in den CP und den CG Heap ein. Das
  Expandieren des Eintrages kommt speater in select()*/
void KPV_insertASet(AbstractTime i) 
{
    Id id;
    ID_i(id) = i;    
    ID_j(id) = 0;
    ID_xp(id) = 0;    
    TA_Expandiere(id);
}


/* Diese Funktion soll folgendes tun: ein Interreduktionsopfer soll in P
 * aufgenommen werden. Interreduktionsopfer landen dabei immer im jeweiligen
 * Heap (CG für Ziele, CP normal).  bei "normalen" Interreduktionsopfern wird
 * nur das Gewicht berechnet, um den Waisenmord schaltbar zu halten werden
 * Waisen nicht überschrieben
 */
void KPV_IROpferBehandeln (RegelOderGleichungsT opfer) 
{
    TA_IROpferMelden(opfer);
}


/** Liefert TRUE zurueck, wenn der entsprechende Eintrag von dem uebergebenen
 *  Heap bereits zurueckgegeben worden ist. Ansonsten FALSE.
 */
BOOLEAN KPV_alreadySelected(AbstractTime i, AbstractTime j, Position xp, 
			    BOOLEAN isCP, TermT linkeSeite, TermT rechteSeite) 
{
    IdMitTermenS tid = {linkeSeite,rechteSeite,{i,j,xp},isCP};    
	    
    return ZPA_BereitsSelektiert(&tid);    
}

void SUE_selectedCPRE(SelectRecT *selRec)
{
    KPV_IncAktivierterFakten();
    if (Z_IstGemantelteUngleichung(selRec->lhs, selRec->rhs)) {
      KPV_IncAktivierterG();
      if (selRec->wasFIFO)
        KPV_IncAktivierterGFifo();
      else 
        KPV_IncAktivierterGHeu();
    } else {
      KPV_IncAktivierterRE();
      if (selRec->wasFIFO)
        KPV_IncAktivierterREFifo();
      else
        KPV_IncAktivierterREHeu();
    }
    KPV_IncAnzKPZuRE();
}


BOOLEAN KPV_Select(SelectRecT *selRec)
{
  while (!HK_TestStrategieWechsel() && ZPA_Selektiere(selRec)){
    if (IO_level4()){
      IO_DruckeFlex("Select            %w: %t = %t\n", &(selRec->wid), selRec->lhs, selRec->rhs);
    }
    NF_Normalform2(PA_SelR(), PA_SelE(), selRec->lhs, selRec->rhs);
    if (IO_level4()){
      IO_DruckeFlex("Select Normalized %w: %t = %t\n", &(selRec->wid), selRec->lhs, selRec->rhs);
    }

    selRec->result = HK_VariantentestUndVergleich(selRec->lhs, selRec->rhs);
    if (selRec->result == Gleich){
      KPV_IncAnzKPspaeterZusammengefuehrt();
      if (IO_level4()){
        IO_DruckeFlex("joined            %w: %t = %t\n", &(selRec->wid), selRec->lhs, selRec->rhs);
      }
    } else if ((selRec->result == Unvergleichbar) && PA_SelS() 
	       && SS_TermpaarSubsummiertVonGM(selRec->lhs, selRec->rhs)) {
      KPV_IncAnzKPspaeterSubsummiert();
      if (IO_level4()){
        IO_DruckeFlex("subsumed          %w: %t = %t\n", &(selRec->wid), selRec->lhs, selRec->rhs);
      }
    } else if (PA_SelP() && GZ_ACVerzichtbar(selRec->lhs, selRec->rhs)) {
      KPV_IncAnzKPspaeterPermSubsummiert();
      if (IO_level4()){
        IO_DruckeFlex("perm subsumed     %w: %t = %t\n", &(selRec->wid), selRec->lhs, selRec->rhs);
      }
    } else if (!GZ_ZSFB_BEHALTEN && GZ_doGZFB() 
	       && GZ_Grundzusammenfuehrbar(selRec->lhs,selRec->rhs)) {
      if (1||IO_level4()){
        IO_DruckeFlex("gj deleted        %w: %t = %t\n", &(selRec->wid), selRec->lhs, selRec->rhs);
      }
      KPV_IncAnzKPspaeterPermSubsummiert(); /* XXX */
    } else {
      ZPA_Weiterdrehen();
      return TRUE;
    }
    TO_TermeLoeschen(selRec->lhs, selRec->rhs);
  }
  return FALSE;
}


/* ---------------------------------------------------------------------------*/
/* ------- RECONSTRUCTION ----------------------------------------------------*/
/* ---------------------------------------------------------------------------*/

/** Rekonstruiert die in x enthaltenen Informationen und liefert sie 
 *  in Form eines Select Records zurück, falls x ein Interreduktionsopfer
 *  enthält
 */
static void kpv_reconstructIR(WId* x, SelectRecT *selRec, BOOLEAN normalize) {
  AbstractTime memo;
  TermT reproLhs, reproRhs;
  RegelOderGleichungsT re = RE_getActiveWithBirthday(WID_i(*x));

  if (TP_getTodesTag_M(re) != WID_j(*x)) our_error("KPV: reconstructIR: Todestag != j");
  
  memo = HK_getAbsTime_M();
  HK_AbsTime = TP_getTodesTag_M(re);    
  { /* Zeitreise */
      reproLhs = TO_Termkopie(TP_LinkeSeite(re));
      reproRhs = TO_Termkopie(TP_RechteSeite(re));
      if (normalize){
        NF_Normalform2(PA_GenR(), PA_GenE(), reproLhs, reproRhs);
      }
      BO_TermpaarNormierenAlsKP(reproLhs, reproRhs);
  }
  HK_AbsTime = memo;    

  selRec->tp = NULL;
  selRec->actualParent = re;
  selRec->otherParent = NULL;
  selRec->lhs = reproLhs;
  selRec->rhs = reproRhs;
  selRec->wid = *x;
}

static void kpv_reconstructACOverlap(WId* x, SelectRecT *selRec,
                                     BOOLEAN normalize)
{
  RegelOderGleichungsT vater = RE_getActiveWithBirthday(WID_i(*x));
  BOOLEAN rechts = POS_i_right(WID_xp(*x));
  if (rechts && RE_IstGleichung(vater)){
    rechts = FALSE;
    vater = TP_Gegenrichtung(vater);
  }

  U1_CC_KPRekonstruieren(vater, POS_pos(WID_xp(*x)), 
                         rechts,&(selRec->lhs), &(selRec->rhs),
                         &(selRec->otherParent));
  
  selRec->actualParent = vater;
  selRec->tp = NULL;
  selRec->wid = *x;
	selRec->result = 0;
	selRec->wasFIFO = FALSE;
	selRec->istGZFB = FALSE;
  
  if (normalize){
    AbstractTime memo = HK_getAbsTime_M();
    HK_AbsTime = WID_i(*x);
    NF_Normalform2(PA_GenR(), PA_GenE(), selRec->lhs, selRec->rhs);
    HK_AbsTime = memo;
  }
}

/** Rekonstruiert die in x enthaltenen Informationen und liefert sie 
 *  in Form eines Select Records zurück, falls x ein Kritisches Paar
 *  enthält
 */
static void kpv_reconstructCP(WId* x, SelectRecT *selRec, BOOLEAN normalize)
{
  AbstractTime memo;
  TermT reproLhs, reproRhs;

  selRec->actualParent = RE_getActiveWithBirthday(WID_i(*x));
  selRec->otherParent  = RE_getActiveWithBirthday(WID_j(*x));

  if (selRec->actualParent->geburtsTag < selRec->otherParent->geburtsTag) {
      our_error( "############## ActualParent ist nicht A[i]\n");
  }

  
  if (POS_i_right(WID_xp(*x)) && TP_IstMonogleichung(selRec->actualParent)) {
    our_error("kpv_reconstructCP: Unerlaubte Rechtsueberlappung");
  }
  if (POS_j_right(WID_xp(*x)) && TP_IstMonogleichung(selRec->otherParent)) {
    our_error("kpv_reconstructCP: Unerlaubte Rechtsueberlappung");
  }
 
  if (POS_i_right(WID_xp(*x)) && !TP_IstMonogleichung(selRec->actualParent)) {
    selRec->actualParent = TP_Gegenrichtung(selRec->actualParent);
  }
  if (POS_j_right(WID_xp(*x)) && !TP_IstMonogleichung(selRec->otherParent)) {
    selRec->otherParent = TP_Gegenrichtung(selRec->otherParent);
  }
  
  memo = HK_getAbsTime_M();
  HK_AbsTime = WID_i(*x); 
  {
      if (POS_into_i(WID_xp(*x))) {
	  U1_KPRekonstruieren(selRec->actualParent, POS_pos(WID_xp(*x)), 
			      selRec->otherParent, &reproLhs, &reproRhs);
      } else {
	  U1_KPRekonstruieren(selRec->otherParent, POS_pos(WID_xp(*x)) , 
			      selRec->actualParent, &reproLhs, &reproRhs);
      }
      if (normalize){
        NF_Normalform2(PA_GenR(), PA_GenE(), reproLhs, reproRhs);
      }
      BO_TermpaarNormierenAlsKP(reproLhs, reproRhs);
  }
  HK_AbsTime = memo;
  selRec->tp = NULL;
  selRec->lhs = reproLhs;
  selRec->rhs = reproRhs;
  selRec->wid = *x;
}

static void kpv_reconstructAxiom( WId* wid, SelectRecT* selRec ) 
{
    if (WID_xp(*wid) <= AX_AnzahlAxiome()) {
	selRec->actualParent = NULL;
	selRec->otherParent = NULL;
	selRec->tp = NULL;
	selRec->result = 0;
	selRec->wasFIFO = FALSE;
	selRec->istGZFB = FALSE;
	selRec->wid = *wid;
	AX_AxiomKopieren(WID_xp(*wid),&(selRec->lhs),&(selRec->rhs));
    } else {
	our_error("kpv_reconstructAxiom: invalid axiom number\n");
    }
}

void KPV_reconstruct( WId* wid, SelectRecT* selRec, BOOLEAN normalize )
{
  if (WID_isAxiom(*wid)) {
    kpv_reconstructAxiom(wid,selRec);
  } else if (WID_isCP(*wid)) {
    kpv_reconstructCP(wid,selRec, normalize);
  } else if (WID_isACGOverlap(*wid)) {
    kpv_reconstructACOverlap(wid,selRec, normalize);
  } else if (WID_isIR(*wid)) {
    kpv_reconstructIR(wid,selRec, normalize);
  } else if (WID_isLemma(*wid)) {
    our_error("KPV_reconstruct: Lemma reconstruction not yet implemented\n");
  } else if (WID_isASet(*wid)) {
    our_error("KPV_reconstruct: ASet reconstruction not possible\n");
  } else if (WID_isPSet(*wid)) {
    our_error("KPV_reconstruct: PSet reconstruction not possible\n");
  } else {
    our_error("KPV_reconstruct: invalid Id given\n");
  }
}


/* --------------------------------------------------------------------------*/
/* ------------ CLEANING WOMAN ----------------------------------------------*/
/* --------------------------------------------------------------------------*/

void KPV_PassiveFaktenWegwerfen(void)
{ 
#if 0
  KKDH_Walkthrough(newCPHeap, abraeumFct);
  /*KKDH_Walkthrough(newCGHeap, abraeumFct);*/
#else
  SelectRecT selRec;
  while (ZPA_Selektiere(&selRec))    
    ;
#endif
}

void KPV_CLEANUP(void) {
    ZPA_Aufrauemen();
    TA_Aufrauemen();
    AX_Cleanup();
}

void KPV_SOFTCLEANUP(void)
{
    ZPA_Aufrauemen();
    TA_Aufrauemen();
    AX_SoftCleanup();
}

extern ClassifyProcT ClassifyProc;
extern ClassifyProcT CGClassifyProc;

static FassT kpv_CPFass,kpv_CGFass;

void KPV_InitAposteriori (void)
{
    ZPA_Initialisieren();

    AXZ_mkZapfhahn(&axzcp, TRUE, PA_WeightAxiom() ? ClassifyProc : CH_Fifo );
    AXZ_mkZapfhahn(&axzcg, FALSE, PA_WeightAxiom() ? CGClassifyProc : CH_Fifo );

    ZPA_ZapfhahnAnmelden(&axzcp,TRUE,-1,-1);
    ZPA_ZapfhahnAnmelden(&axzcg,TRUE,-1,-1);

    kpv_CPFass = FA_mkFass(CP,ClassifyProc);
    kpv_CGFass = FA_mkFass(CG,CGClassifyProc);

    ZPA_ZapfhahnAnmelden(FIFO_mkTacho(TRUE),FALSE,PA_thresholdCP(),0);
    ZPA_ZapfhahnAnmelden(FA_mkZapfhahn(kpv_CPFass),
			 FALSE,PA_moduloCP()-PA_thresholdCP(),0);

    ZPA_ZapfhahnAnmelden(FIFO_mkTacho(FALSE),FALSE,PA_thresholdCG(),1);
    ZPA_ZapfhahnAnmelden(FA_mkZapfhahn(kpv_CGFass),
			 FALSE,PA_moduloCG()-PA_thresholdCG(),1);

    TA_TankstutzenAnmelden(FA_mkTankstutzen(kpv_CPFass));
    TA_TankstutzenAnmelden(FA_mkTankstutzen(kpv_CGFass));
}

/* --------------------------------------------------------------------------*/
/* ------- Restekiste -------------------------------------------------------*/
/* --------------------------------------------------------------------------*/

unsigned long int KPH_LebendeKPsZaehlen(void)
{
  our_error("KPH_LebendeKPsZaehlen not yet implemented in KP Version ");
  return LONG_MAX;
}

void KPV_insertCP(TermT lhs, TermT rhs, AbstractTime i, AbstractTime j, Position xp)
{
    KPV_IncAnzCPs();

    if (PA_doACGSKCheck() && (i != 0)&&(j != 0)){
      WId wid = {0,{i,j,xp}};
      TermT reproLhs, reproRhs;
      TermT reproLhs2, reproRhs2;
      RegelOderGleichungsT actualParent, otherParent;

      actualParent = RE_getActiveWithBirthday(WID_i(wid));
      otherParent  = RE_getActiveWithBirthday(WID_j(wid));

      if ((TP_IstACGrundreduzibel(actualParent) || TP_IstACGrundreduzibel(otherParent)) || 
          (PA_doSK() && (actualParent->StrengKonsistent|| otherParent->StrengKonsistent))) {

        SelectRecT selRec;
        KPV_reconstruct(&wid, &selRec, FALSE);
        reproLhs = selRec.lhs;
        reproRhs = selRec.rhs;

        reproLhs2 = TO_Termkopie(reproLhs);
        reproRhs2 = TO_Termkopie(reproRhs);
        BO_TermpaarNormierenAlsKP(reproRhs, reproLhs);
        BO_TermpaarNormierenAlsKP(reproLhs2, reproRhs2);
        BO_TermpaarNormierenAlsKP(rhs, lhs);
        if ((!TO_TermeGleich(lhs, reproLhs) || !TO_TermeGleich(rhs,reproRhs)) &&
            (!TO_TermeGleich(rhs, reproLhs2) || !TO_TermeGleich(lhs,reproRhs2))){
          IO_DruckeFlex("orig: %t = %t\n", lhs, rhs);
          IO_DruckeFlex("repr: %t = %t\n\n", reproLhs, reproRhs);
          BO_TermpaarNormierenAlsKP(reproLhs, reproRhs);
          BO_TermpaarNormierenAlsKP(lhs, rhs);
          IO_DruckeFlex("orig: %t = %t\n", lhs, rhs);
          IO_DruckeFlex("repr: %t = %t\n", reproLhs, reproRhs);
          IO_DruckeFlex("Wid = %w\n", &wid);
          /*          IO_DruckeFlex("%w  given: %t = %t, reconstr %t = %t\n", wid, lhs, rhs, reproLhs, reproRhs);*/
          our_error("peng");
        }
        TO_TermeLoeschen(reproLhs,reproRhs);
        TO_TermeLoeschen(reproLhs2,reproRhs2);
      }
    }

/*     IO_DruckeFlex("### <%d,%d,%z> %t = %t\n",i,j,xp,lhs,rhs); */
    TA_insertCP(lhs,rhs,i,j,xp);
}

void KPV_CGHeuristikUmstellen(ClassifyProcT h)
{
  FA_changeClass(kpv_CGFass,h);
}
