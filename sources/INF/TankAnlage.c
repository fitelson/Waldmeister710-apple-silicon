/* **************************************************************************
 * TankAnlage.c
 * 
 * 
 * Jean-Marie Gaillourdet, 11.09.2003
 *
 * ************************************************************************** */

#include "TankAnlage.h"

#include "Ausgaben.h"
#include "general.h"
#include "DSBaumOperationen.h"
#include "FehlerBehandlung.h"
#include "Grundzusammenfuehrung.h"
#include "Hauptkomponenten.h"
#include "NFBildung.h"
#include "Parameter.h"
#include "SpeicherVerwaltung.h"
#include "Subsumption.h"
#include "TermOperationen.h"
#include "Unifikation1.h"

static int ta_AnzahlDerTankstutzen = 0;
static TankstutzenT* ta_ts = NULL;


static void KPZusammengefuehrtDrucken (TermT ergebnis)
{
  if (IO_level3()) printf("... joined.\n");
  else if(IO_level4()) IO_DruckeFlex("... joined into %t.\n", ergebnis);
}

static void KPSubsummiertDrucken(void)
{
   if ((IO_level3()) || (IO_level4()))
      printf("... subsumed.\n");
}

static void KPPermSubsummiertDrucken (void)
{
   if ((IO_level3()) || (IO_level4()))
      printf("... perm-subsumed.\n");
}

static void DruckeMeldungKPgebildet(unsigned long Nummer, long VaterNr, long MutterNr, TermT links, TermT rechts)
{
  if (IO_level3())
    printf("critical pair %lu built with parents %ld and %ld.\n", Nummer, VaterNr, MutterNr);
  else if (IO_level4()){
    printf("critical pair %lu built with parents %ld and %ld:\n", Nummer, VaterNr, MutterNr);
    BO_TermpaarNormierenAlsKP(links, rechts);                                       /* !!! Seiteneffekt !!! */
    IO_DruckeFlex("       %t # %t\n",links, rechts);
  }
} 


static void DruckeMeldungZuKPgemacht(unsigned long Nummer, TermT links, TermT rechts)
{
  if (IO_level3())
    printf("... made ues %lu.\n",Nummer);
  else if (IO_level4())
    IO_DruckeFlex("... made ues %lu: %t # %t\n",Nummer, links, rechts);
}


void TA_Expandiere( Id id )
{
    AbstractTime memo;

    if (ID_i(id) != HK_getAbsTime_M()) {
      /*      IO_DruckeFlex("reexpanding %i\n",&id);*/
    }

    memo = HK_getAbsTime_M();

    HK_AbsTime = ID_i(id);
    U1_KPsBildenZuFaktum(RE_getActiveWithBirthday(ID_i(id)));
    HK_AbsTime = memo;
}

static BOOLEAN KPBehandelt(TermT lhs, TermT rhs)
{
#define IF_TERMZELLEN_JAGD 0
#if IF_TERMZELLEN_JAGD
    fprintf(stderr, "KPBehandelt: %d (%ld /%ld)\n", TO_Termlaenge(lhs)+TO_Termlaenge(rhs),
	    TO_FreispeicherManager.Anforderungen,SV_AnzNochBelegterElemente(TO_FreispeicherManager));
#endif

    NF_Normalform2(PA_GenR(), PA_GenE(), lhs, rhs);
    if (TO_TermeGleich(lhs, rhs)) {
	KPZusammengefuehrtDrucken (lhs);
	KPV_IncAnzKPdirektZusammengefuehrt();
    } else if (PA_GenP() && GZ_ACVerzichtbar(lhs, rhs) ) {
      KPPermSubsummiertDrucken ();
      KPV_IncAnzKPdirektPermSubsummiert();
    } else if (PA_GenS() && SS_TermpaarSubsummiertVonGM(lhs, rhs)) {
      KPSubsummiertDrucken ();
      KPV_IncAnzKPdirektSubsummiert();
    } else if (FALSE && GZ_Grundzusammenfuehrbar(lhs, rhs)) { /* nur zum Testen XXX */
      KPPermSubsummiertDrucken ();
      KPV_IncAnzKPdirektPermSubsummiert();
    } else {
      return FALSE;
    }

    return TRUE;
}


void TA_insertCP(TermT lhs, TermT rhs, AbstractTime i, AbstractTime j, Position xp)
{
    Id id = {i,j,xp};

    KPV_IncAnzKPGesamt();
    DruckeMeldungKPgebildet(GetKPV_AnzKPGesamt(), i, j, lhs, rhs);

    if (!KPBehandelt(lhs, rhs)) {

	int n;

	for (n=0; n<ta_AnzahlDerTankstutzen; n++) {
	    ta_ts[n]->faktenEinfuegen(ta_ts[n],lhs,rhs,id);
	};
        KPV_IncAnzKPInserted();
    }

    TO_TermeLoeschen(lhs, rhs);
}


void TA_IROpferMelden( RegelOderGleichungsT opfer )
{
  TermT l, r;

  l = TO_Termkopie(TP_LinkeSeite(opfer));
  r = TO_Termkopie(TP_RechteSeite(opfer));

  KPV_IncAnzKPGesamt(); 
  KPV_IncAnzKPIROpfer();
  DruckeMeldungZuKPgemacht(GetKPV_AnzKPGesamt(), l, r);
  if (!KPBehandelt(l, r)) {
      int n;
      IdMitTermenS tid = {l,r,{TP_getGeburtsTag_M(opfer),HK_AbsTime,0},
			  !RE_IstUngleichung(opfer)};
      
      for (n=0; n<ta_AnzahlDerTankstutzen; n++) {
	  ta_ts[n]->iROpferEinfuegen(ta_ts[n],&tid);
      }

      KPV_IncAnzKPInserted();
  }
  TO_TermeLoeschen(l,r);
}

void TA_TankstutzenAnmelden( TankstutzenT t )
{
    ta_AnzahlDerTankstutzen += 1;
    ta_ts = our_realloc(ta_ts,sizeof(TankstutzenT)*ta_AnzahlDerTankstutzen,"Tankstutzen");
    ta_ts[ta_AnzahlDerTankstutzen-1] = t;
}

void TA_TankstutzenAbmelden( TankstutzenT t )
{
    int i,j;
    
    for (i=0; i < ta_AnzahlDerTankstutzen; i++) {
	if (t == ta_ts[i]) {
	    for (j = i+1; j < ta_AnzahlDerTankstutzen; j++) {
		ta_ts[j-1] = ta_ts[j];
	    }
	}
    };
    ta_AnzahlDerTankstutzen -= 1;
    ta_ts = our_realloc(ta_ts,sizeof(TankstutzenT)*ta_AnzahlDerTankstutzen,"Tankstutzen");
}

void TA_Aufrauemen(void) 
{
    int i;
    
    for (i=0; i < ta_AnzahlDerTankstutzen; i++) {
	ta_ts[i]->freigeben(ta_ts[i]);
    };
    our_dealloc(ta_ts);
    ta_ts = NULL;
    ta_AnzahlDerTankstutzen = 0;
}

