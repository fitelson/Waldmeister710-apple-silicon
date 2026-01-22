/**********************************************************************
* Filename : MNF_Ziele.h
*
* Kurzbeschreibung : Zielbehandlung mit MultipleNormalFormen
*
* Autoren : Andreas Jaeger
*
* 09.10.96: Erstellung, Teilweise aus Zielverwaltung "geklaut"
* 22.02.97: Mehrfachaufrufclean. AJ.
*
*
* Benoetigt folgende Module:
*  - MNF: Verwalten der Ziele
*
**********************************************************************/

#include "compiler-extra.h"
#include "general.h"
#include "Ausgaben.h"
#include "Konsulat.h"
#include "MatchOperationen.h"
#include "MNF.h"
#include "MNF_Print.h"
#include "MNF_Ziele.h"
#include "Parameter.h"
#include "Statistiken.h"
#include "SpeicherVerwaltung.h"
#include "TermOperationen.h"
#include "Termpaare.h"
#include "Zielverwaltung.h"
#include "Zielverwaltung_private.h"
#include "ZielAusgaben.h"

/* Typen     */
/*===========*/

typedef struct privateMNFZieleTS 
{
   struct MNF_MengeTS     *nfMenge;
} privateMNFZieleT, *privateMNFZielePtrT;


static void MNFZ_PrintJoined (ZielePtrT goal) 
{
   TermT  joinedTerm;
   
   if (IO_AusgabeLevel() >= 2) {
     printf("joined goal:      %8u  ", goal->nummer);
     joinedTerm = MNF_GetJoinedTerm (goal->privat.MNF->nfMenge);
     IO_DruckeFlex("%t ?= %t to %t\n", goal->orginalLinks, goal->orginalRechts,
                   joinedTerm);
   }
   else
     printf("joined goal:      %8u\n", goal->nummer);
   ZA_ZielZusammengefuehrtDrucken();
   if (IO_AusgabeLevel() > 2 && !COMP_POWER)
     MNF_PrintJoinedComplete (goal->privat.MNF->nfMenge);
   
   MNF_PrintJoinedBeweis (goal->privat.MNF->nfMenge, goal->nummer);
}


/* ------------------------------------------------------------------------- */
/*                                                                           */
/* ------------------------------------------------------------------------- */

static void MNFZ_MNFStat (ZieleT *ziel) 
{
   unsigned int size;
   
   size = MNF_GetSize (ziel->privat.MNF->nfMenge);
   if (size >  GetMNFZ_MaxCountMNFSet())
      SetMNFZ_MaxCountMNFSet(size);
   if (ziel->status != Z_Open)
         MNFZ_PrintJoined (ziel);
}

void MNFZ_InitZielBeobachtungsProz (ZielePtrT zielPtr) {

  zielPtr->beobachtProz (zielPtr->orginalLinks, zielPtr->orginalRechts);
}

void MNFZ_ZielEinfuegen (ZieleT * ziel) 
{
   ziel->privat.MNF = (privateMNFZieleT *)our_alloc (sizeof(privateMNFZieleT));
   ziel->privat.MNF->nfMenge = MNF_InitNMenge (ziel->orginalLinks,
                                               ziel->orginalRechts);
}


void MNFZ_LoeschePrivateData (ZieleT *ziel) 
{
   MNF_DeleteNMenge ( ziel->privat.MNF->nfMenge);
   our_dealloc (ziel->privat.MNF);
}


void MNFZ_AusgabeZiel (ZieleT * ziel,Z_AusgabeArtT Z_AusgabeArt) 
{
   TermT left, right;
   
   switch (Z_AusgabeArt) {
   case Z_AusgabeAlles:
      if (MNF_JoinedNMenge (ziel->privat.MNF->nfMenge)) {
         MNF_GetJoinedTermpair (ziel->privat.MNF->nfMenge, &left, &right);
         IO_DruckeFlex(" current  %t = %t\n", left, right);
         MNF_PrintJoinedComplete (ziel->privat.MNF->nfMenge);
      }
#if 0
        MNF_PrintMengeComplete (ziel->privat.MNF->nfMenge, "lhs: ", "rhs: ");
#endif
      break;
   case Z_AusgabeInitial:
      if (MNF_JoinedNMenge (ziel->privat.MNF->nfMenge)) {
         MNF_GetJoinedTermpair (ziel->privat.MNF->nfMenge, &left, &right);
         IO_DruckeFlex(" current  %t = %t\n", left, right);
      }
      break;
   case Z_AusgabeKurz:
      if (MNF_JoinedNMenge (ziel->privat.MNF->nfMenge)) {
         MNF_GetJoinedTermpair (ziel->privat.MNF->nfMenge, &left, &right);
         IO_DruckeFlex("%t = %t", left, right);
      }
      break;
   case Z_AusgabeSpez:
      IO_DruckeFlex("\t%t = %t\n", ziel->orginalLinks, ziel->orginalRechts);
      break;
   }
}

   
void MNFZ_ZielBeobachtung (ZieleT *ziel, Z_BeobachtungsDurchlaufProcT AP) 
{
   AP (ziel->orginalLinks, ziel->orginalRechts, ziel->nummer);
}


static BOOLEAN MNF_ZielErledigt (ZieleT *ziel, BOOLEAN expand)
{
   BOOLEAN joined;
   
   joined = MNF_JoinedNMenge (ziel->privat.MNF->nfMenge);
   if (!joined && expand) {
      MNF_ExpandNMenge (ziel->privat.MNF->nfMenge);
      joined = MNF_JoinedNMenge (ziel->privat.MNF->nfMenge);
   }
   if (joined)
      ziel->status = Z_Joined;

   MNFZ_MNFStat (ziel);
   
   return ziel->status != Z_Open;
}

BOOLEAN MNFZ_TesteZielErledigt (ZieleT *ziel) 
{
   return (MNF_ZielErledigt (ziel, FALSE));
}

static BOOLEAN ZielErledigtMitNeuerRegel (ZieleT *ziel, RegelT Regel) 
{
   BOOLEAN Resultat;
   MNF_AddRegelNM (ziel->privat.MNF->nfMenge, Regel);
   Resultat = MNF_ZielErledigt (ziel, TRUE);
   if (Resultat && PA_KernAn())
     KO_KonsultationErforderlich = TRUE;
   return Resultat;
}

static BOOLEAN ZielErledigtMitNeuerGleichung (ZieleT *ziel, 
                                              GleichungsT Gleichung) 
{
   MNF_AddGleichungNM (ziel->privat.MNF->nfMenge, Gleichung);
   return MNF_ZielErledigt (ziel, TRUE);
}

BOOLEAN MNFZ_ZielErledigtMitNeuemFaktum (ZieleT *ziel, 
                                         RegelOderGleichungsT Faktum)
{
  if (RE_IstRegel(Faktum)){
    return ZielErledigtMitNeuerRegel(ziel, Faktum);
  }
  else {
    return ZielErledigtMitNeuerGleichung(ziel, Faktum);
  }
}


BOOLEAN MNFZ_ZielFinalerDurchlauf (ZieleT * ziel) 
{
   return MNF_ZielErledigt (ziel, TRUE);
}



void MNFZ_InitAposteriori(void)
{
  MNF_InitAposteriori ();
}

