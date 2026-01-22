/**********************************************************************
* Filename : NormaleZiele.c
*
* Kurzbeschreibung : Normale Zielverwaltung
*
* Autoren : Andreas Jaeger
*
* 10.10.96: Erstellung aus Zielverwaltung.h heraus
* 22.02.97: Mehrfachaufrufclean. AJ.
*
* Die Funktionen in diesem Modul werden nie direkt aufgerufen, sondern
* nur von der Zielverwaltung!
*
**********************************************************************/

#include <stdlib.h>
#include <string.h>

#include "compiler-extra.h"
#include "antiwarnings.h"
#include "general.h"

#include "Ausgaben.h"
#include "Hauptkomponenten.h"
#include "MatchOperationen.h"
#include "NFBildung.h"
#include "NormaleZiele.h"
#include "Parameter.h"
#include "RUndEVerwaltung.h"
#include "SymbolOperationen.h"
#include "TermOperationen.h"
#include "Termpaare.h"
#include "Zielverwaltung.h"
#include "Zielverwaltung_private.h"
#include "ZielAusgaben.h"

/*********************/
/* Typdeklarationen  */
/*********************/

/*
  reduziert: wird gesetzt, wenn links oder rechts reduziert wurde.
  Falls im Wiederholenmodus gearbeitet wird, kann somit festgestellt
  werden, ob links/rechts die originalen Ziele sind, oder reduzierte
  Elemente.
*/
typedef struct privateNormaleZieleTS {
   TermT               links;
   TermT               rechts;
   BOOLEAN             reduziert;
   unsigned            len;
   unsigned            num;
   AbstractTime       *times;
} privateNormaleZieleT, *privateNormaleZielePtrT;


void NZ_InitZielBeobachtungsProz (ZieleT  *ziel) {

  ziel->beobachtProz (ziel->privat.NZ->links, ziel->privat.NZ->rechts);
}


void NZ_ZielEinfuegen (ZieleT * ziel, TermT links, TermT rechts) 
{
   ziel->privat.NZ = our_alloc (sizeof(privateNormaleZieleT));
   ziel->privat.NZ->links = TO_Termkopie (links);
   ziel->privat.NZ->rechts = TO_Termkopie (rechts);
   ziel->privat.NZ->reduziert = FALSE;
   ziel->privat.NZ->num = 0;
   ziel->privat.NZ->len = 32;
   ziel->privat.NZ->times = our_alloc(sizeof(AbstractTime) * ziel->privat.NZ->len);
}


void NZ_LoeschePrivateData (ZieleT *ziel) 
{
   TO_TermLoeschen (ziel->privat.NZ->links);
   TO_TermLoeschen (ziel->privat.NZ->rechts);
   our_dealloc (ziel->privat.NZ->times);
   our_dealloc (ziel->privat.NZ);
}

static void InitElemente (ZieleT *ziel)
{
   if (ziel->privat.NZ->reduziert && ziel->modul == Z_Repeat) {
      TO_TermLoeschen (ziel->privat.NZ->links);
      TO_TermLoeschen (ziel->privat.NZ->rechts);
      ziel->privat.NZ->links = TO_Termkopie (ziel->orginalLinks);
      ziel->privat.NZ->rechts = TO_Termkopie (ziel->orginalRechts);
      ziel->privat.NZ->reduziert = FALSE;
   }
}

void NZ_AusgabeZiel (ZieleT * ziel, Z_AusgabeArtT Z_AusgabeArt) 
{
  if (ziel->modul == Z_Paramodulation) {
    switch (Z_AusgabeArt) {
    case Z_AusgabeAlles:
    case Z_AusgabeInitial:
      IO_DruckeFlex ("%t ?= %t\n",
                     ziel->privat.NZ->links, ziel->privat.NZ->rechts);
      IO_DruckeFlex ("        using narrowing to prove %t ?= %t\n", 
                     ziel->orginalLinks, ziel->orginalRechts);
      break;
    case Z_AusgabeKurz:
      IO_DruckeFlex ("true ?= false, current %t ?= %t\n", 
                     ziel->privat.NZ->links, ziel->privat.NZ->rechts);
      IO_DruckeFlex ("        using narrowing to prove %t ?= %t\n", 
                     ziel->orginalLinks, ziel->orginalRechts);
      break;
    case Z_AusgabeSpez:
      IO_DruckeFlex("\t%t = %t\n", ziel->orginalLinks, ziel->orginalRechts);
      break;
    }
  } else {
    switch (Z_AusgabeArt) {
    case Z_AusgabeAlles:
    case Z_AusgabeInitial:
      IO_DruckeFlex(" current  %t ?= %t\n", 
                    ziel->privat.NZ->links, ziel->privat.NZ->rechts);
      break;
    case Z_AusgabeKurz:
      IO_DruckeFlex("%t = %t", 
                    ziel->privat.NZ->links, ziel->privat.NZ->rechts);
      break;
    case Z_AusgabeSpez:
      IO_DruckeFlex("\t%t = %t\n", ziel->orginalLinks, ziel->orginalRechts);
      break;
    }
  }
}

   
void NZ_ZielBeobachtung (ZieleT *ziel, Z_BeobachtungsDurchlaufProcT AP)  
{
   AP (ziel->privat.NZ->links, ziel->privat.NZ->rechts, ziel->nummer);
}

static void nz_ZielBeweis (ZieleT *ziel)  
{
  if (ziel->modul == Z_Paramodulation) {
    TermT Flunder1 = TO_(SO_TRUE);
    TermT Flunder2 = TO_(SO_FALSE);
    BOB_BeweisFuerNormalesZiel(Flunder1, Flunder2, ziel->nummer,
                               1, &(ziel->zusammenfuehrungszeitpunkt));
    TO_TermeLoeschen (Flunder1,Flunder2);
  }
  else if (ziel->modul == Z_Repeat) {
    BOB_BeweisFuerNormalesZiel (ziel->orginalLinks, ziel->orginalRechts, ziel->nummer, 
                                1, &(ziel->zusammenfuehrungszeitpunkt));
  }
  else {
    BOB_BeweisFuerNormalesZiel (ziel->orginalLinks, ziel->orginalRechts, ziel->nummer, 
                                ziel->privat.NZ->num, ziel->privat.NZ->times);
  }
}

static void nz_ZielBeweisen (ZieleT *ziel)  
{
  if (PA_doPCL()){
    if (!PA_PCLVerbose()){/* es muessen noch die noetigen Aktiva bestimmt werden */
      BOB_SetScanMode();
      nz_ZielBeweis(ziel);
      BOB_BeweiseFuerAktiva();
    }
    BOB_SetPrintMode();
    BOB_BeweiseFuerAktiva();
    nz_ZielBeweis(ziel);
  }
}

BOOLEAN NZ_TesteZielErledigt (ZieleT *ziel) 
{
   if (TO_TermeGleich(ziel->privat.NZ->links, ziel->privat.NZ->rechts)){
      if (IO_AusgabeLevel() >= 2) {
         printf("joined goal:    %8u  ", ziel->nummer);
         IO_DruckeFlex("%t ?= %t to %t\n", 
                       ziel->orginalLinks, ziel->orginalRechts,
                       ziel->privat.NZ->rechts);
      }
      else
         printf("joined goal:    %8u\n", ziel->nummer);
      ZA_ZielZusammengefuehrtDrucken(); /* ... und somit zusammengefuehrt.*/
      ziel->status = Z_Joined;
      ziel->zusammenfuehrungszeitpunkt = HK_getAbsTime_M();
      nz_ZielBeweisen(ziel);
      return TRUE;
   }
   return FALSE;
}


BOOLEAN NZ_ZielErledigtMitNeuemFaktum (ZieleT *ziel, 
                                       RegelOderGleichungsT Faktum)
{
   BOOLEAN reduziert;

   InitElemente (ziel);
   if (ziel->modul == Z_Repeat) {
     reduziert = NF_NormalformRE2(ziel->privat.NZ->links, ziel->privat.NZ->rechts);
   } else {
     reduziert = NF_NormalformstRE(Faktum, ziel->privat.NZ->links);
     reduziert |= NF_NormalformstRE(Faktum, ziel->privat.NZ->rechts);
     if (reduziert){
       if (ziel->privat.NZ->num == ziel->privat.NZ->len){
         ziel->privat.NZ->len += 32;
         ziel->privat.NZ->times = our_realloc(ziel->privat.NZ->times,
                                              sizeof(AbstractTime) * ziel->privat.NZ->len, 
                                              "times-array for goals");
       }
       ziel->privat.NZ->times[ziel->privat.NZ->num] = HK_getAbsTime_M();
       ziel->privat.NZ->num++;
     }
   }
   if (reduziert){
      ziel->privat.NZ->reduziert = TRUE;
      if (ziel->beobachtProz)
         ziel->beobachtProz (ziel->privat.NZ->links, ziel->privat.NZ->rechts);
      if (ziel->modul != Z_Repeat)
	ZA_ZielReduziertDrucken (ziel->nummer, ziel->privat.NZ->links, 
                                 ziel->privat.NZ->rechts);
      /* Ziel .. konnte reduziert werden: l =? r */
      if (NZ_TesteZielErledigt (ziel))
         return TRUE;
   }
   return FALSE;
}


BOOLEAN NZ_ZielFinalerDurchlauf (ZieleT * ziel) 
{
   BOOLEAN reduziert;

   InitElemente (ziel);
   reduziert = NF_NormalformRE2(ziel->privat.NZ->links, ziel->privat.NZ->rechts);
   if (reduziert){
      ziel->privat.NZ->reduziert = TRUE;
      if (ziel->beobachtProz)
         ziel->beobachtProz (ziel->privat.NZ->links, ziel->privat.NZ->rechts);
      if (ziel->modul != Z_Repeat)
	 ZA_ZielReduziertDrucken (ziel->nummer, ziel->privat.NZ->links, 
                                  ziel->privat.NZ->rechts);
      /* Ziel .. konnte reduziert werden: l =? r */
      if (TO_TermeGleich(ziel->privat.NZ->links, ziel->privat.NZ->rechts)){
         ZA_ZielZusammengefuehrtDrucken(); /* ... und somit zusammengefuehrt.*/
         ziel->status = Z_Joined;
         return TRUE;
      }
   }

   return FALSE;
}



