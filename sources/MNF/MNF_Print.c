/**********************************************************************
* Filename : MNF_Print.c
*
* Kurzbeschreibung : MultipleNormalFormen: Ausgabe der Mengen/Elemente
*
* Autoren : Andreas Jaeger
*
* 18.11.96: Erstellung
*
*
*
* Benoetigt folgende Module:
*  - Ausgaben: Ausgeben von Termen
*  - Stack:
*
* Benutzte Globale Variablen aus anderen Modulen:
*   zur Zeit keine
*
**********************************************************************/

#include "antiwarnings.h"
#include "general.h"
#include "BeweisObjekte.h"
#include "compiler-extra.h"
#include "MNF_Print.h"
#include <limits.h>
#include "Hash.h"
#include "MNF_intern.h"
#include "MNF.h"
#include "NFBildung.h"
#include "Parameter.h"
#include "Ausgaben.h"
#include "Stack.h"


static MNF_ColTermPtrT
MNF_Get_ID (MNF_MengeT *menge, MNF_IdT id) {
   MNF_ColTermPtrT       colTermPtr = NULL;
   HashEntryPtrT         TraversePtr;

   HA_InitTraverseN(&menge->HashTabelle, &TraversePtr);
   while (!HA_EndTraverseN(&TraversePtr)) {
      HA_NextTraverseN(&TraversePtr, (void **)&colTermPtr);
      if (colTermPtr->bob_info.mnfID == id) {
         return colTermPtr;
      }
   }
   return NULL;
}

static void BuildStack (MNF_MengeT *menge, StackPtrT stack,
                        MNF_ColTermPtrT lastElem)
{
   MNF_ColTermPtrT       acctTermPtr;
   MNF_IdT     parentID;

   acctTermPtr = lastElem;
   parentID = acctTermPtr->bob_info.parentID;
   ST_New (stack);
   ST_Push (stack, acctTermPtr);
   
   while (parentID != 0) {
      acctTermPtr = MNF_Get_ID (menge, parentID);
      ST_Push (stack, acctTermPtr);
      parentID = acctTermPtr->bob_info.parentID;
   }
}

static void Output_OneStep (SeitenT Seite,
                            MNF_ColTermPtrT colTerm, TermT oldTerm,
                            TermT TermAndereSeite)
{
   TermT      copyTerm;
   UTermT     Index;
   UTermT     redPos;
   
   if (oldTerm != NULL) {
      /* Wurde Term mit NF_Bildung reduziert?-> Spezialbehandlung */
      if (colTerm->bob_info.RENummer == 0) {
        if (colTerm->bob_info.re != NULL){
          our_error("BL murks 1");
        }
         copyTerm = TO_Termkopie (oldTerm);      
         if (Seite == linkeSeite){
           BOB_PrepareNF(copyTerm, TermAndereSeite, colTerm->bob_info.time);
         }
         else {
           BOB_PrepareNF(TermAndereSeite, copyTerm, colTerm->bob_info.time);
         }
         (void)NF_NormalformRE (copyTerm);
         BOB_ClearNF();
         TO_TermLoeschen (copyTerm);
      } else {
        if (colTerm->bob_info.re->Nummer != colTerm->bob_info.RENummer){
          our_error("BL murks 2");
        }
         /* bob_info.redPosition ist eine Stelle in oldTerm, wir
            suchen aber die analoge Stelle in colTerm->term fuer die
            Beweisausgabe */
         redPos = colTerm->term;
         TO_doStellen(oldTerm, Index, 
                      {
                         if (Index == colTerm->bob_info.redPosition)
                            break;
                         else
                            redPos = TO_Schwanz (redPos);
                      });
         BOB_RedStep(colTerm->term, TermAndereSeite, colTerm->bob_info.re,Seite,redPos,colTerm->bob_info.RESeite);
      }
   }
   else {
     our_error("BL -- do not understand");
   }
}

static void SchrittVorwaerts(MNF_ColTermPtrT *termp, 
                             MNF_IdT *idp, StackT *stackp)
{
   if (ST_Empty (stackp)) {
      *termp = NULL;
      *idp = LONG_MAX;
   } else {
      *termp = ST_Pop (stackp);
      *idp  = (*termp)->bob_info.mnfID;
   }
}

/*
  Ausgabe des Zielpaares mit allen Vorfahren fuer Beweisobjekt
  */
static void mnf_OutputAllSteps (MNF_MengeT *menge, unsigned long nummer) {
   MNF_ColTermPtrT       leftPtr, rightPtr;
   MNF_IdT     acctLeftID, acctRightID;
   StackT                stackLeft, stackRight;
   TermT                 leftTerm, rightTerm;
   
   if (!menge->joinable) {
      return;
   }

   if (MNF_tcolour_red(*menge->joinedCTerm1)) {
      rightPtr = menge->joinedCTerm1;
      leftPtr = menge->joinedCTerm2;
      /* Wenn der linke Pointer NULL ist, dann muessen wir das
         initiale Element nehmen */
      if (leftPtr == NULL)
         leftPtr = menge->initialCTermLeft;
   }
   else {
      rightPtr = menge->joinedCTerm2;
      leftPtr = menge->joinedCTerm1;
      /* Wenn der rechte Pointer NULL ist, dann muessen wir das
         initiale Element nehmen */
      if (rightPtr == NULL)
         rightPtr = menge->initialCTermRight;
   }

   BuildStack (menge, &stackLeft, leftPtr);
   BuildStack (menge, &stackRight, rightPtr);

   /* mal erst die Anfangsziele nehmen */
   leftPtr  = ST_Pop (&stackLeft);
   rightPtr = ST_Pop (&stackRight);

   /* leftTerm/rightTerm speichern immer den Vorgaenger, dieser wird
      vom Beweisobjekt benoetigt.
      */

   leftTerm = leftPtr->term;
   rightTerm = rightPtr->term;

   BOB_UrspruenglichesZiel(leftTerm, rightTerm, nummer);

   /* nach left, right die jeweils erste Ableitung,
      sofern es eine gibt
    */
   SchrittVorwaerts(&leftPtr, &acctLeftID, &stackLeft);
   SchrittVorwaerts(&rightPtr, &acctRightID, &stackRight);

   while (leftPtr || rightPtr) {
      if (leftPtr && acctLeftID <= acctRightID) {
         /* Element von links ausgeben */
         Output_OneStep (linkeSeite, leftPtr, leftTerm,
                         rightTerm);
         leftTerm = leftPtr->term;
         SchrittVorwaerts(&leftPtr, &acctLeftID, &stackLeft);
      } 
      else if (rightPtr && acctRightID <= acctLeftID) {
         /* Element von rechts ausgeben */
         Output_OneStep (rechteSeite,rightPtr, rightTerm,
                         leftTerm);
         rightTerm = rightPtr->term;
         SchrittVorwaerts(&rightPtr, &acctRightID, &stackRight);
      }
      else {
        our_error("should not occur!");
      }
   }
  
   BOB_ZielBewiesen(leftTerm, rightTerm);
   ST_Delete (&stackLeft);
   ST_Delete (&stackRight);
}

void MNF_PrintJoinedBeweis (MNF_MengeT *menge, unsigned long nummer) 
{
  if (PA_doPCL()){
    if (!PA_PCLVerbose()){/* es muessen noch die noetigen Aktiva bestimmt werden */
      BOB_SetScanMode();
      mnf_OutputAllSteps(menge,nummer);
      BOB_BeweiseFuerAktiva();
    }
    BOB_SetPrintMode();
    BOB_BeweiseFuerAktiva();
    mnf_OutputAllSteps(menge,nummer);
  }
}

/* Ausgeben aller neuen Elemente */
void MNF_PrintMengeNew (MNF_MengeT *menge, const char *leftStr, 
                        const char *rightStr,
                        const char *title, TermT t1, TermT t2)
{
   MNF_ColTermPtrT       colTermPtr = NULL;
   BOOLEAN               first = TRUE;
   unsigned int          countPrintedR = 0;
   unsigned int          countPrintedG = 0;
   int                   AusgabeLevel = IO_AusgabeLevel();
   const unsigned int    maxPrintElems = 20;
   HashEntryPtrT         TraversePtr;

   if ((AusgabeLevel < 1) || (menge->joinable))
      return;
   HA_InitTraverseN(&menge->HashTabelle, &TraversePtr);
   while (!HA_EndTraverseN (&TraversePtr)) {
      HA_NextTraverseN (&TraversePtr, (void **)&colTermPtr);
      if (MNF_tnew (*colTermPtr)) {
         if (first) {
            printf(title);
            if (AusgabeLevel > 1)
               IO_DruckeFlex(": %t ?= %t\n", t1, t2);
            else
               printf("\n");
            first = FALSE;
         }
         MNF_setold (*colTermPtr);
         if (AusgabeLevel > 2) {
            if (MNF_tcolour_green (*colTermPtr) && 
                (countPrintedG < maxPrintElems)) {
               countPrintedG++;
               printf(leftStr);
               IO_DruckeFlex("%t\n", colTermPtr->term);
            }
            else if (MNF_tcolour_red (*colTermPtr) && 
                     (countPrintedR < maxPrintElems)) {
               countPrintedR++;
               printf(rightStr);
               IO_DruckeFlex("%t\n", colTermPtr->term);
            }
         }
      }
   }
   /* Wenn modifiziert wurde, wird noch die Summe der Elemente ausgegeben */
   if (!first) {
      printf("no. of elems in goal set: %u\n", menge->size);
   }
}


/* Ausgeben aller Elemente */
void MNF_PrintMengeComplete (MNF_MengeT *menge, 
                             const char *leftStr, const char *rightStr)
{
   MNF_ColTermPtrT       colTermPtr = NULL;
   HashEntryPtrT         TraversePtr;

   MNF_PrintMengeStatLong (menge);
   if (menge->size == 0)
      return;

   /* erst die linke Seite */
   HA_InitTraverseN (&menge->HashTabelle, &TraversePtr);
   while (!HA_EndTraverseN (&TraversePtr)) {
      HA_NextTraverseN (&TraversePtr, (void **)&colTermPtr);
      if (MNF_tcolour_green (*colTermPtr)) {
         printf(leftStr);
         IO_DruckeFlex("%t\n", colTermPtr->term);
      }
   }
   HA_InitTraverseN (&menge->HashTabelle, &TraversePtr);
   while (!HA_EndTraverseN (&TraversePtr)) {
      HA_NextTraverseN (&TraversePtr, (void **)&colTermPtr);
      if (MNF_tcolour_red (*colTermPtr)) {
         printf(rightStr);
         IO_DruckeFlex("%t\n", colTermPtr->term);
      }
   }
}

/* Ausgabe einer kurzen Statistik zur Nachfolgermenge */
void MNF_PrintMengeStatShort (MNF_MengeT *menge) 
{
   printf("\tno. of elems in set: %u\n", menge->size);
}


/* Ausgabe einer langen Statistik zur Nachfolgermenge */
void MNF_PrintMengeStatLong (MNF_MengeT *menge) 
{
   printf("\tno. of elems in set: %u\n", menge->size);
   printf("\tuse analyse: %s\n", menge->useAnalyse ? "yes" : "no");
   printf("\ttry all elements: %s\n", 
          menge->MNF_AddTryAllElements ? "yes" : "no");
   printf("\thash size: %d\n", menge->HashTabelle.size);

}


/*
  Ausgabe eines CTerms mit allen Informationen
  */
void MNF_PrintColTerm (MNF_ColTermT *colTermPtr) 
{

   if (colTermPtr->bob_info.parentID == 0)
      printf("Term no. %lu (initial", colTermPtr->bob_info.mnfID);
   else {
      printf("Term no %lu (from: %4lu with %s %5ld order: ",
              colTermPtr->bob_info.mnfID,
              colTermPtr->bob_info.parentID,
              colTermPtr->bob_info.RENummer > 0 ? "rule" : "equation",
              colTermPtr->bob_info.RENummer);
      switch (colTermPtr->flags.vgl) {
      case Kleiner:
         printf(" less");
         break;
      case Groesser:
         printf(" greater");
         break;
      case Gleich:
         printf(" equal");
         break;
      case Unvergleichbar:
         printf(" uncomparable");
         break;
      default:
         our_error ("Unbekannter Vergleichswert");
      }
   }         
   IO_DruckeFlex("): %t\n", colTermPtr->term);
   if (colTermPtr->bob_info.redPosition != NULL)
      IO_DruckeFlex("Position: %t\n", colTermPtr->bob_info.redPosition);
#if MNF_TEST
   printf("  in=%4lu, out=%4lu, outUp=%4lu, status=%4lu, "
          "GlS=%3lu, GesS=%3lu\n",
          colTermPtr->in, colTermPtr->out, colTermPtr->outUp, 
          colTermPtr->status,
           colTermPtr->anzGlSchritte, colTermPtr->anzGesSchritte);
#endif   
}

/*
  Ausgabe der kompletten Nachfolgermenge mit allen Informationen,
  wird beim Debugging benoetigt.
  */
void MNF_PrintMenge (MNF_MengeT *menge) {
   MNF_ColTermPtrT       colTermPtr = NULL;
   HashEntryPtrT         TraversePtr;

   printf("\nComplete MNF-Set\n");
   HA_InitTraverseN (&menge->HashTabelle, &TraversePtr);
   while (!HA_EndTraverseN (&TraversePtr)) {
      HA_NextTraverseN (&TraversePtr, (void **)&colTermPtr);
      MNF_PrintColTerm (colTermPtr);
   }   
}


/*
  Ausgabe des Zielpaares mit allen Vorfahren
  */
static void MNF_PrintJoinedInternal (MNF_MengeT *menge)
{
   MNF_ColTermPtrT       acctTermPtr;
   MNF_IdT     parentID;
   
   if (!menge->joinable) {
      printf("Elements are not joinable -> no Output from "
             "MNF_PrintJoinedInternal\n");
      return;
   }

   printf("\nJoined/Subsumed term:\n");
   acctTermPtr = menge->joinedCTerm1;
   parentID = acctTermPtr->bob_info.parentID;
   MNF_PrintColTerm (acctTermPtr);

   while (parentID != 0) {
      acctTermPtr = MNF_Get_ID (menge, parentID);
      MNF_PrintColTerm (acctTermPtr);
      parentID = acctTermPtr->bob_info.parentID;
   }
   
   acctTermPtr = menge->joinedCTerm2;
   if (acctTermPtr == NULL) {
      printf("Joined with other side of unmodified goal.\n");
      return;
   }
   printf("\nJoined/Subsumed term - other side:\n");

   parentID = acctTermPtr->bob_info.parentID;
   MNF_PrintColTerm (acctTermPtr);

   
   while (parentID != 0) {
      acctTermPtr = MNF_Get_ID (menge, parentID);
      MNF_PrintColTerm (acctTermPtr);
      parentID = acctTermPtr->bob_info.parentID;
   }
}

void MNF_PrintJoinedComplete (MNF_MengeT *menge) {

   MNF_PrintJoinedInternal (menge);
}


void MNF_PrintJoinedShort (MNF_MengeT *menge) {
   MNF_ColTermPtrT       leftPtr, rightPtr;

   if (!menge->joinable) {
     printf("Elements are not joinable -> no Output from "
            "MNF_PrintJoinedShort\n");
      return;
   }

   /* joinedCTerm1 existiert immer, joinedCTerm2 nicht unbedingt */
   if (menge->joinedCTerm2 == NULL) {
      if (MNF_tcolour_red (*menge->joinedCTerm1)) {
         rightPtr = menge->joinedCTerm1;
         leftPtr = menge->initialCTermLeft;
      } else {
         leftPtr = menge->joinedCTerm1;
         rightPtr = menge->initialCTermRight;
      }
   } else {
      if (MNF_tcolour_red (*menge->joinedCTerm1)) {
         rightPtr = menge->joinedCTerm1;
         leftPtr = menge->joinedCTerm2;
      } else {
         rightPtr = menge->joinedCTerm2;
         leftPtr = menge->joinedCTerm1;
      }
   }
      
   printf("\nJoined/Subsumed term pair:\n");
   IO_DruckeFlex(" %t = %t\n", leftPtr->term, rightPtr->term);
}
